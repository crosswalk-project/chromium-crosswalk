// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/memory/shared_memory.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/win/windows_version.h"
#include "content/child/request_extra_data.h"
#include "content/child/service_worker/service_worker_network_provider.h"
#include "content/common/frame_messages.h"
#include "content/common/ssl_status_serialization.h"
#include "content/common/view_messages.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/web_ui_controller_factory.h"
#include "content/public/common/bindings_policy.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/page_zoom.h"
#include "content/public/common/url_constants.h"
#include "content/public/common/url_utils.h"
#include "content/public/renderer/content_renderer_client.h"
#include "content/public/renderer/document_state.h"
#include "content/public/renderer/navigation_state.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/frame_load_waiter.h"
#include "content/public/test/render_view_test.h"
#include "content/public/test/test_utils.h"
#include "content/renderer/accessibility/renderer_accessibility.h"
#include "content/renderer/devtools/devtools_agent.h"
#include "content/renderer/history_controller.h"
#include "content/renderer/history_serialization.h"
#include "content/renderer/navigation_state_impl.h"
#include "content/renderer/render_process.h"
#include "content/renderer/render_view_impl.h"
#include "content/shell/browser/shell.h"
#include "content/shell/browser/shell_browser_context.h"
#include "content/test/mock_keyboard.h"
#include "net/base/net_errors.h"
#include "net/cert/cert_status_flags.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/WebData.h"
#include "third_party/WebKit/public/platform/WebHTTPBody.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/platform/WebURLResponse.h"
#include "third_party/WebKit/public/web/WebDataSource.h"
#include "third_party/WebKit/public/web/WebDeviceEmulationParams.h"
#include "third_party/WebKit/public/web/WebHistoryCommitType.h"
#include "third_party/WebKit/public/web/WebHistoryItem.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebPerformance.h"
#include "third_party/WebKit/public/web/WebRuntimeFeatures.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "third_party/WebKit/public/web/WebWindowFeatures.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gfx/range/range.h"

#if defined(USE_AURA) && defined(USE_X11)
#include <X11/Xlib.h>
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"
#include "ui/events/test/events_test_utils.h"
#include "ui/events/test/events_test_utils_x11.h"
#endif

#if defined(USE_OZONE)
#include "ui/events/keycodes/keyboard_code_conversion.h"
#endif

using blink::WebFrame;
using blink::WebInputEvent;
using blink::WebLocalFrame;
using blink::WebMouseEvent;
using blink::WebRuntimeFeatures;
using blink::WebString;
using blink::WebTextDirection;
using blink::WebURLError;

namespace content {

namespace {

static const int kProxyRoutingId = 13;

#if (defined(USE_AURA) && defined(USE_X11)) || defined(USE_OZONE)
// Converts MockKeyboard::Modifiers to ui::EventFlags.
int ConvertMockKeyboardModifier(MockKeyboard::Modifiers modifiers) {
  static struct ModifierMap {
    MockKeyboard::Modifiers src;
    int dst;
  } kModifierMap[] = {
    { MockKeyboard::LEFT_SHIFT, ui::EF_SHIFT_DOWN },
    { MockKeyboard::RIGHT_SHIFT, ui::EF_SHIFT_DOWN },
    { MockKeyboard::LEFT_CONTROL, ui::EF_CONTROL_DOWN },
    { MockKeyboard::RIGHT_CONTROL, ui::EF_CONTROL_DOWN },
    { MockKeyboard::LEFT_ALT,  ui::EF_ALT_DOWN },
    { MockKeyboard::RIGHT_ALT, ui::EF_ALT_DOWN },
  };
  int flags = 0;
  for (size_t i = 0; i < arraysize(kModifierMap); ++i) {
    if (kModifierMap[i].src & modifiers) {
      flags |= kModifierMap[i].dst;
    }
  }
  return flags;
}
#endif

class WebUITestWebUIControllerFactory : public WebUIControllerFactory {
 public:
  WebUIController* CreateWebUIControllerForURL(WebUI* web_ui,
                                               const GURL& url) const override {
    return NULL;
  }
  WebUI::TypeID GetWebUIType(BrowserContext* browser_context,
                             const GURL& url) const override {
    return WebUI::kNoWebUI;
  }
  bool UseWebUIForURL(BrowserContext* browser_context,
                      const GURL& url) const override {
    return HasWebUIScheme(url);
  }
  bool UseWebUIBindingsForURL(BrowserContext* browser_context,
                              const GURL& url) const override {
    return HasWebUIScheme(url);
  }
};

}  // namespace

class RenderViewImplTest : public RenderViewTest {
 public:
  RenderViewImplTest() {
    // Attach a pseudo keyboard device to this object.
    mock_keyboard_.reset(new MockKeyboard());
  }

  ~RenderViewImplTest() override {}

  void SetUp() override {
    RenderViewTest::SetUp();
    // Enable Blink's experimental and test only features so that test code
    // does not have to bother enabling each feature.
    WebRuntimeFeatures::enableExperimentalFeatures(true);
    WebRuntimeFeatures::enableTestOnlyFeatures(true);
  }

  RenderViewImpl* view() {
    return static_cast<RenderViewImpl*>(view_);
  }

  int view_page_id() {
    return view()->page_id_;
  }

  RenderFrameImpl* frame() {
    return static_cast<RenderFrameImpl*>(view()->GetMainRenderFrame());
  }

  // Sends IPC messages that emulates a key-press event.
  int SendKeyEvent(MockKeyboard::Layout layout,
                   int key_code,
                   MockKeyboard::Modifiers modifiers,
                   base::string16* output) {
#if defined(OS_WIN)
    // Retrieve the Unicode character for the given tuple (keyboard-layout,
    // key-code, and modifiers).
    // Exit when a keyboard-layout driver cannot assign a Unicode character to
    // the tuple to prevent sending an invalid key code to the RenderView
    // object.
    CHECK(mock_keyboard_.get());
    CHECK(output);
    int length = mock_keyboard_->GetCharacters(layout, key_code, modifiers,
                                               output);
    if (length != 1)
      return -1;

    // Create IPC messages from Windows messages and send them to our
    // back-end.
    // A keyboard event of Windows consists of three Windows messages:
    // WM_KEYDOWN, WM_CHAR, and WM_KEYUP.
    // WM_KEYDOWN and WM_KEYUP sends virtual-key codes. On the other hand,
    // WM_CHAR sends a composed Unicode character.
    MSG msg1 = { NULL, WM_KEYDOWN, key_code, 0 };
    ui::KeyEvent evt1(msg1);
    NativeWebKeyboardEvent keydown_event(evt1);
    SendNativeKeyEvent(keydown_event);

    MSG msg2 = { NULL, WM_CHAR, (*output)[0], 0 };
    ui::KeyEvent evt2(msg2);
    NativeWebKeyboardEvent char_event(evt2);
    SendNativeKeyEvent(char_event);

    MSG msg3 = { NULL, WM_KEYUP, key_code, 0 };
    ui::KeyEvent evt3(msg3);
    NativeWebKeyboardEvent keyup_event(evt3);
    SendNativeKeyEvent(keyup_event);

    return length;
#elif defined(USE_AURA) && defined(USE_X11)
    // We ignore |layout|, which means we are only testing the layout of the
    // current locale. TODO(mazda): fix this to respect |layout|.
    CHECK(output);
    const int flags = ConvertMockKeyboardModifier(modifiers);

    ui::ScopedXI2Event xevent;
    xevent.InitKeyEvent(ui::ET_KEY_PRESSED,
                        static_cast<ui::KeyboardCode>(key_code),
                        flags);
    ui::KeyEvent event1(xevent);
    NativeWebKeyboardEvent keydown_event(event1);
    SendNativeKeyEvent(keydown_event);

    // X11 doesn't actually have native character events, but give the test
    // what it wants.
    xevent.InitKeyEvent(ui::ET_KEY_PRESSED,
                        static_cast<ui::KeyboardCode>(key_code),
                        flags);
    ui::KeyEvent event2(xevent);
    event2.set_character(GetCharacterFromKeyCode(event2.key_code(),
                                                 event2.flags()));
    ui::KeyEventTestApi test_event2(&event2);
    test_event2.set_is_char(true);
    NativeWebKeyboardEvent char_event(event2);
    SendNativeKeyEvent(char_event);

    xevent.InitKeyEvent(ui::ET_KEY_RELEASED,
                        static_cast<ui::KeyboardCode>(key_code),
                        flags);
    ui::KeyEvent event3(xevent);
    NativeWebKeyboardEvent keyup_event(event3);
    SendNativeKeyEvent(keyup_event);

    long c = GetCharacterFromKeyCode(static_cast<ui::KeyboardCode>(key_code),
                                     flags);
    output->assign(1, static_cast<base::char16>(c));
    return 1;
#elif defined(USE_OZONE)
    const int flags = ConvertMockKeyboardModifier(modifiers);

    ui::KeyEvent keydown_event(ui::ET_KEY_PRESSED,
                               static_cast<ui::KeyboardCode>(key_code),
                               flags);
    NativeWebKeyboardEvent keydown_web_event(keydown_event);
    SendNativeKeyEvent(keydown_web_event);

    ui::KeyEvent char_event(keydown_event.GetCharacter(),
                            static_cast<ui::KeyboardCode>(key_code),
                            flags);
    NativeWebKeyboardEvent char_web_event(char_event);
    SendNativeKeyEvent(char_web_event);

    ui::KeyEvent keyup_event(ui::ET_KEY_RELEASED,
                             static_cast<ui::KeyboardCode>(key_code),
                             flags);
    NativeWebKeyboardEvent keyup_web_event(keyup_event);
    SendNativeKeyEvent(keyup_web_event);

    long c = GetCharacterFromKeyCode(static_cast<ui::KeyboardCode>(key_code),
                                     flags);
    output->assign(1, static_cast<base::char16>(c));
    return 1;
#else
    NOTIMPLEMENTED();
    return L'\0';
#endif
  }

  void EnablePreferredSizeMode() {
    view()->OnEnablePreferredSizeChangedMode();
  }

  const gfx::Size& GetPreferredSize() {
    view()->CheckPreferredSize();
    return view()->preferred_size_;
  }

  void SetZoomLevel(double level) {
    view()->OnSetZoomLevelForView(false, level);
  }

  void NavigateMainFrame(const CommonNavigationParams& common_params,
                         const StartNavigationParams& start_params,
                         const RequestNavigationParams& request_params) {
    NavigateFrame(frame(), common_params, start_params, request_params);
  }

  void NavigateFrame(RenderFrameImpl* frame,
                     const CommonNavigationParams& common_params,
                     const StartNavigationParams& start_params,
                     const RequestNavigationParams& request_params) {
    // PlzNavigate
    if (base::CommandLine::ForCurrentProcess()->HasSwitch(
            switches::kEnableBrowserSideNavigation)) {
      frame->OnCommitNavigation(ResourceResponseHead(), common_params.url,
                                common_params, request_params);
      return;
    }
    frame->OnNavigate(common_params, start_params, request_params);
  }

  void SwapOut(RenderFrameImpl* frame,
               int proxy_routing_id,
               bool is_loading,
               const FrameReplicationState& replicated_frame_state) {
    frame->OnSwapOut(proxy_routing_id, is_loading, replicated_frame_state);
  }

  void SetEditableSelectionOffsets(int start, int end) {
    frame()->OnSetEditableSelectionOffsets(start, end);
  }

  void ExtendSelectionAndDelete(int before, int after) {
    frame()->OnExtendSelectionAndDelete(before, after);
  }

  void Unselect() { frame()->OnUnselect(); }

  void SetAccessibilityMode(AccessibilityMode new_mode) {
    frame()->OnSetAccessibilityMode(new_mode);
  }

  void SetCompositionFromExistingText(
      int start,
      int end,
      const std::vector<blink::WebCompositionUnderline>& underlines) {
    frame()->OnSetCompositionFromExistingText(start, end, underlines);
  }

 private:
  scoped_ptr<MockKeyboard> mock_keyboard_;
};

class DevToolsAgentTest : public RenderViewImplTest {
 public:
  void Attach() {
    std::string host_id = "host_id";
    agent()->OnAttach(host_id);
  }

  void Detach() {
    agent()->OnDetach();
  }

  bool IsPaused() {
    return agent()->paused_;
  }

  void DispatchDevToolsMessage(const std::string& message) {
    agent()->OnDispatchOnInspectorBackend(message);
  }

  void CloseWhilePaused() {
    EXPECT_TRUE(IsPaused());
    view()->NotifyOnClose();
  }

 private:
  DevToolsAgent* agent() {
    return frame()->devtools_agent();
  }
};

// Ensure that the main RenderFrame is deleted and cleared from the RenderView
// after closing it.
TEST_F(RenderViewImplTest, RenderFrameClearedAfterClose) {
  // Create a new main frame RenderFrame so that we don't interfere with the
  // shutdown of frame() in RenderViewTest.TearDown.
  blink::WebURLRequest popup_request(GURL("http://foo.com"));
  blink::WebView* new_web_view = view()->createView(
      GetMainFrame(), popup_request, blink::WebWindowFeatures(), "foo",
      blink::WebNavigationPolicyNewForegroundTab, false);
  RenderViewImpl* new_view = RenderViewImpl::FromWebView(new_web_view);

  // Close the view, causing the main RenderFrame to be detached and deleted.
  new_view->Close();
  EXPECT_FALSE(new_view->GetMainRenderFrame());

  // Clean up after the new view so we don't leak it.
  new_view->Release();
}

TEST_F(RenderViewImplTest, SaveImageFromDataURL) {
  const IPC::Message* msg1 = render_thread_->sink().GetFirstMessageMatching(
      ViewHostMsg_SaveImageFromDataURL::ID);
  EXPECT_FALSE(msg1);
  render_thread_->sink().ClearMessages();

  const std::string image_data_url =
      "data:image/gif;base64,R0lGODlhAQABAIAAAAUEBAAAACwAAAAAAQABAAACAkQBADs=";

  view()->saveImageFromDataURL(WebString::fromUTF8(image_data_url));
  ProcessPendingMessages();
  const IPC::Message* msg2 = render_thread_->sink().GetFirstMessageMatching(
      ViewHostMsg_SaveImageFromDataURL::ID);
  EXPECT_TRUE(msg2);

  ViewHostMsg_SaveImageFromDataURL::Param param1;
  ViewHostMsg_SaveImageFromDataURL::Read(msg2, &param1);
  EXPECT_EQ(get<1>(param1).length(), image_data_url.length());
  EXPECT_EQ(get<1>(param1), image_data_url);

  ProcessPendingMessages();
  render_thread_->sink().ClearMessages();

  const std::string large_data_url(1024 * 1024 * 10 - 1, 'd');

  view()->saveImageFromDataURL(WebString::fromUTF8(large_data_url));
  ProcessPendingMessages();
  const IPC::Message* msg3 = render_thread_->sink().GetFirstMessageMatching(
      ViewHostMsg_SaveImageFromDataURL::ID);
  EXPECT_TRUE(msg3);

  ViewHostMsg_SaveImageFromDataURL::Param param2;
  ViewHostMsg_SaveImageFromDataURL::Read(msg3, &param2);
  EXPECT_EQ(get<1>(param2).length(), large_data_url.length());
  EXPECT_EQ(get<1>(param2), large_data_url);

  ProcessPendingMessages();
  render_thread_->sink().ClearMessages();

  const std::string exceeded_data_url(1024 * 1024 * 10 + 1, 'd');

  view()->saveImageFromDataURL(WebString::fromUTF8(exceeded_data_url));
  ProcessPendingMessages();
  const IPC::Message* msg4 = render_thread_->sink().GetFirstMessageMatching(
      ViewHostMsg_SaveImageFromDataURL::ID);
  EXPECT_FALSE(msg4);
}

// Test that we get form state change notifications when input fields change.
TEST_F(RenderViewImplTest, DISABLED_OnNavStateChanged) {
  // Don't want any delay for form state sync changes. This will still post a
  // message so updates will get coalesced, but as soon as we spin the message
  // loop, it will generate an update.
  view()->set_send_content_state_immediately(true);

  LoadHTML("<input type=\"text\" id=\"elt_text\"></input>");

  // We should NOT have gotten a form state change notification yet.
  EXPECT_FALSE(render_thread_->sink().GetFirstMessageMatching(
      ViewHostMsg_UpdateState::ID));
  render_thread_->sink().ClearMessages();

  // Change the value of the input. We should have gotten an update state
  // notification. We need to spin the message loop to catch this update.
  ExecuteJavaScript("document.getElementById('elt_text').value = 'foo';");
  ProcessPendingMessages();
  EXPECT_TRUE(render_thread_->sink().GetUniqueMessageMatching(
      ViewHostMsg_UpdateState::ID));
}

TEST_F(RenderViewImplTest, OnNavigationHttpPost) {
  // An http url will trigger a resource load so cannot be used here.
  CommonNavigationParams common_params;
  StartNavigationParams start_params;
  RequestNavigationParams request_params;
  common_params.url = GURL("data:text/html,<div>Page</div>");
  common_params.navigation_type = FrameMsg_Navigate_Type::NORMAL;
  common_params.transition = ui::PAGE_TRANSITION_TYPED;
  request_params.page_id = -1;

  // Set up post data.
  const unsigned char* raw_data = reinterpret_cast<const unsigned char*>(
      "post \0\ndata");
  const unsigned int length = 11;
  const std::vector<unsigned char> post_data(raw_data, raw_data + length);
  start_params.is_post = true;
  start_params.browser_initiated_post_data = post_data;

  NavigateMainFrame(common_params, start_params, request_params);
  ProcessPendingMessages();

  const IPC::Message* frame_navigate_msg =
      render_thread_->sink().GetUniqueMessageMatching(
          FrameHostMsg_DidCommitProvisionalLoad::ID);
  EXPECT_TRUE(frame_navigate_msg);

  FrameHostMsg_DidCommitProvisionalLoad::Param host_nav_params;
  FrameHostMsg_DidCommitProvisionalLoad::Read(frame_navigate_msg,
                                              &host_nav_params);
  EXPECT_TRUE(get<0>(host_nav_params).is_post);

  // Check post data sent to browser matches
  EXPECT_TRUE(get<0>(host_nav_params).page_state.IsValid());
  scoped_ptr<HistoryEntry> entry =
      PageStateToHistoryEntry(get<0>(host_nav_params).page_state);
  blink::WebHTTPBody body = entry->root().httpBody();
  blink::WebHTTPBody::Element element;
  bool successful = body.elementAt(0, element);
  EXPECT_TRUE(successful);
  EXPECT_EQ(blink::WebHTTPBody::Element::TypeData, element.type);
  EXPECT_EQ(length, element.data.size());
  EXPECT_EQ(0, memcmp(raw_data, element.data.data(), length));
}

TEST_F(RenderViewImplTest, DecideNavigationPolicy) {
  WebUITestWebUIControllerFactory factory;
  WebUIControllerFactory::RegisterFactory(&factory);

  DocumentState state;
  state.set_navigation_state(NavigationStateImpl::CreateContentInitiated());

  // Navigations to normal HTTP URLs can be handled locally.
  blink::WebURLRequest request(GURL("http://foo.com"));
  blink::WebFrameClient::NavigationPolicyInfo policy_info(request);
  policy_info.frame = GetMainFrame();
  policy_info.extraData = &state;
  policy_info.navigationType = blink::WebNavigationTypeLinkClicked;
  policy_info.defaultPolicy = blink::WebNavigationPolicyCurrentTab;
  blink::WebNavigationPolicy policy = frame()->decidePolicyForNavigation(
          policy_info);
  EXPECT_EQ(blink::WebNavigationPolicyCurrentTab, policy);

  // Verify that form posts to WebUI URLs will be sent to the browser process.
  blink::WebURLRequest form_request(GURL("chrome://foo"));
  blink::WebFrameClient::NavigationPolicyInfo form_policy_info(form_request);
  form_policy_info.frame = GetMainFrame();
  form_policy_info.extraData = &state;
  form_policy_info.navigationType = blink::WebNavigationTypeFormSubmitted;
  form_policy_info.defaultPolicy = blink::WebNavigationPolicyCurrentTab;
  form_request.setHTTPMethod("POST");
  policy = frame()->decidePolicyForNavigation(form_policy_info);
  EXPECT_EQ(blink::WebNavigationPolicyIgnore, policy);

  // Verify that popup links to WebUI URLs also are sent to browser.
  blink::WebURLRequest popup_request(GURL("chrome://foo"));
  blink::WebFrameClient::NavigationPolicyInfo popup_policy_info(popup_request);
  popup_policy_info.frame = GetMainFrame();
  popup_policy_info.extraData = &state;
  popup_policy_info.navigationType = blink::WebNavigationTypeLinkClicked;
  popup_policy_info.defaultPolicy = blink::WebNavigationPolicyNewForegroundTab;
  policy = frame()->decidePolicyForNavigation(popup_policy_info);
  EXPECT_EQ(blink::WebNavigationPolicyIgnore, policy);
}

TEST_F(RenderViewImplTest, DecideNavigationPolicyHandlesAllTopLevel) {
  DocumentState state;
  state.set_navigation_state(NavigationStateImpl::CreateContentInitiated());

  RendererPreferences prefs = view()->renderer_preferences();
  prefs.browser_handles_all_top_level_requests = true;
  view()->OnSetRendererPrefs(prefs);

  const blink::WebNavigationType kNavTypes[] = {
    blink::WebNavigationTypeLinkClicked,
    blink::WebNavigationTypeFormSubmitted,
    blink::WebNavigationTypeBackForward,
    blink::WebNavigationTypeReload,
    blink::WebNavigationTypeFormResubmitted,
    blink::WebNavigationTypeOther,
  };

  blink::WebURLRequest request(GURL("http://foo.com"));
  blink::WebFrameClient::NavigationPolicyInfo policy_info(request);
  policy_info.frame = GetMainFrame();
  policy_info.extraData = &state;
  policy_info.defaultPolicy = blink::WebNavigationPolicyCurrentTab;

  for (size_t i = 0; i < arraysize(kNavTypes); ++i) {
    policy_info.navigationType = kNavTypes[i];

    blink::WebNavigationPolicy policy = frame()->decidePolicyForNavigation(
        policy_info);
    EXPECT_EQ(blink::WebNavigationPolicyIgnore, policy);
  }
}

TEST_F(RenderViewImplTest, DecideNavigationPolicyForWebUI) {
  // Enable bindings to simulate a WebUI view.
  view()->OnAllowBindings(BINDINGS_POLICY_WEB_UI);

  DocumentState state;
  state.set_navigation_state(NavigationStateImpl::CreateContentInitiated());

  // Navigations to normal HTTP URLs will be sent to browser process.
  blink::WebURLRequest request(GURL("http://foo.com"));
  blink::WebFrameClient::NavigationPolicyInfo policy_info(request);
  policy_info.frame = GetMainFrame();
  policy_info.extraData = &state;
  policy_info.navigationType = blink::WebNavigationTypeLinkClicked;
  policy_info.defaultPolicy = blink::WebNavigationPolicyCurrentTab;

  blink::WebNavigationPolicy policy = frame()->decidePolicyForNavigation(
      policy_info);
  EXPECT_EQ(blink::WebNavigationPolicyIgnore, policy);

  // Navigations to WebUI URLs will also be sent to browser process.
  blink::WebURLRequest webui_request(GURL("chrome://foo"));
  blink::WebFrameClient::NavigationPolicyInfo webui_policy_info(webui_request);
  webui_policy_info.frame = GetMainFrame();
  webui_policy_info.extraData = &state;
  webui_policy_info.navigationType = blink::WebNavigationTypeLinkClicked;
  webui_policy_info.defaultPolicy = blink::WebNavigationPolicyCurrentTab;
  policy = frame()->decidePolicyForNavigation(webui_policy_info);
  EXPECT_EQ(blink::WebNavigationPolicyIgnore, policy);

  // Verify that form posts to data URLs will be sent to the browser process.
  blink::WebURLRequest data_request(GURL("data:text/html,foo"));
  blink::WebFrameClient::NavigationPolicyInfo data_policy_info(data_request);
  data_policy_info.frame = GetMainFrame();
  data_policy_info.extraData = &state;
  data_policy_info.navigationType = blink::WebNavigationTypeFormSubmitted;
  data_policy_info.defaultPolicy = blink::WebNavigationPolicyCurrentTab;
  data_request.setHTTPMethod("POST");
  policy = frame()->decidePolicyForNavigation(data_policy_info);
  EXPECT_EQ(blink::WebNavigationPolicyIgnore, policy);

  // Verify that a popup that creates a view first and then navigates to a
  // normal HTTP URL will be sent to the browser process, even though the
  // new view does not have any enabled_bindings_.
  blink::WebURLRequest popup_request(GURL("http://foo.com"));
  blink::WebView* new_web_view = view()->createView(
      GetMainFrame(), popup_request, blink::WebWindowFeatures(), "foo",
      blink::WebNavigationPolicyNewForegroundTab, false);
  RenderViewImpl* new_view = RenderViewImpl::FromWebView(new_web_view);
  blink::WebFrameClient::NavigationPolicyInfo popup_policy_info(popup_request);
  popup_policy_info.frame = new_web_view->mainFrame()->toWebLocalFrame();
  popup_policy_info.extraData = &state;
  popup_policy_info.navigationType = blink::WebNavigationTypeLinkClicked;
  popup_policy_info.defaultPolicy = blink::WebNavigationPolicyNewForegroundTab;
  policy = static_cast<RenderFrameImpl*>(new_view->GetMainRenderFrame())->
      decidePolicyForNavigation(popup_policy_info);
  EXPECT_EQ(blink::WebNavigationPolicyIgnore, policy);

  // Clean up after the new view so we don't leak it.
  new_view->Close();
  new_view->Release();
}

// Ensure the RenderViewImpl sends an ACK to a SwapOut request, even if it is
// already swapped out.  http://crbug.com/93427.
TEST_F(RenderViewImplTest, SendSwapOutACK) {
  LoadHTML("<div>Page A</div>");
  int initial_page_id = view_page_id();

  // Increment the ref count so that we don't exit when swapping out.
  RenderProcess::current()->AddRefProcess();

  // Respond to a swap out request.
  SwapOut(frame(), kProxyRoutingId, true, content::FrameReplicationState());

  // Ensure the swap out commits synchronously.
  EXPECT_NE(initial_page_id, view_page_id());

  // Check for a valid OnSwapOutACK.
  const IPC::Message* msg = render_thread_->sink().GetUniqueMessageMatching(
      FrameHostMsg_SwapOut_ACK::ID);
  ASSERT_TRUE(msg);

  // It is possible to get another swap out request.  Ensure that we send
  // an ACK, even if we don't have to do anything else.
  render_thread_->sink().ClearMessages();
  SwapOut(frame(), kProxyRoutingId, false, content::FrameReplicationState());
  const IPC::Message* msg2 = render_thread_->sink().GetUniqueMessageMatching(
      FrameHostMsg_SwapOut_ACK::ID);
  ASSERT_TRUE(msg2);

  // If we navigate back to this RenderView, ensure we don't send a state
  // update for the swapped out URL.  (http://crbug.com/72235)
  CommonNavigationParams common_params;
  RequestNavigationParams request_params;
  common_params.url = GURL("data:text/html,<div>Page B</div>");
  common_params.navigation_type = FrameMsg_Navigate_Type::NORMAL;
  common_params.transition = ui::PAGE_TRANSITION_TYPED;
  request_params.current_history_list_length = 1;
  request_params.current_history_list_offset = 0;
  request_params.pending_history_list_offset = 1;
  request_params.page_id = -1;
  NavigateMainFrame(common_params, StartNavigationParams(), request_params);
  ProcessPendingMessages();
  const IPC::Message* msg3 = render_thread_->sink().GetUniqueMessageMatching(
      ViewHostMsg_UpdateState::ID);
  EXPECT_FALSE(msg3);
}

// Ensure the RenderViewImpl reloads the previous page if a reload request
// arrives while it is showing swappedout://.  http://crbug.com/143155.
TEST_F(RenderViewImplTest, ReloadWhileSwappedOut) {
  // Load page A.
  LoadHTML("<div>Page A</div>");

  // Load page B, which will trigger an UpdateState message for page A.
  LoadHTML("<div>Page B</div>");

  // Check for a valid UpdateState message for page A.
  ProcessPendingMessages();
  const IPC::Message* msg_A = render_thread_->sink().GetUniqueMessageMatching(
      ViewHostMsg_UpdateState::ID);
  ASSERT_TRUE(msg_A);
  ViewHostMsg_UpdateState::Param params;
  ViewHostMsg_UpdateState::Read(msg_A, &params);
  int page_id_A = get<0>(params);
  PageState state_A = get<1>(params);
  EXPECT_EQ(1, page_id_A);
  render_thread_->sink().ClearMessages();

  // Back to page A (page_id 1) and commit.
  CommonNavigationParams common_params_A;
  RequestNavigationParams request_params_A;
  common_params_A.navigation_type = FrameMsg_Navigate_Type::NORMAL;
  common_params_A.transition = ui::PAGE_TRANSITION_FORWARD_BACK;
  request_params_A.current_history_list_length = 2;
  request_params_A.current_history_list_offset = 1;
  request_params_A.pending_history_list_offset = 0;
  request_params_A.page_id = 1;
  request_params_A.nav_entry_id = 1;
  request_params_A.page_state = state_A;
  NavigateMainFrame(common_params_A, StartNavigationParams(), request_params_A);
  EXPECT_EQ(1, view()->historyBackListCount());
  EXPECT_EQ(2, view()->historyBackListCount() +
      view()->historyForwardListCount() + 1);
  ProcessPendingMessages();

  // Respond to a swap out request.
  SwapOut(frame(), kProxyRoutingId, true, content::FrameReplicationState());

  // Check for a OnSwapOutACK.
  const IPC::Message* msg = render_thread_->sink().GetUniqueMessageMatching(
      FrameHostMsg_SwapOut_ACK::ID);
  ASSERT_TRUE(msg);
  render_thread_->sink().ClearMessages();

  // It is possible to get a reload request at this point, containing the
  // params.page_state of the initial page (e.g., if the new page fails the
  // provisional load in the renderer process, after we unload the old page).
  // Ensure the old page gets reloaded, not swappedout://.
  CommonNavigationParams common_params;
  RequestNavigationParams request_params;
  common_params.url = GURL("data:text/html,<div>Page A</div>");
  common_params.navigation_type = FrameMsg_Navigate_Type::RELOAD;
  common_params.transition = ui::PAGE_TRANSITION_RELOAD;
  request_params.current_history_list_length = 2;
  request_params.current_history_list_offset = 0;
  request_params.pending_history_list_offset = 0;
  request_params.page_id = 1;
  request_params.nav_entry_id = 1;
  request_params.page_state = state_A;
  NavigateMainFrame(common_params, StartNavigationParams(), request_params);
  ProcessPendingMessages();

  // Verify page A committed, not swappedout://.
  const IPC::Message* frame_navigate_msg =
      render_thread_->sink().GetUniqueMessageMatching(
          FrameHostMsg_DidCommitProvisionalLoad::ID);
  EXPECT_TRUE(frame_navigate_msg);

  // Read URL out of the parent trait of the params object.
  FrameHostMsg_DidCommitProvisionalLoad::Param commit_load_params;
  FrameHostMsg_DidCommitProvisionalLoad::Read(frame_navigate_msg,
                                              &commit_load_params);
  EXPECT_NE(GURL("swappedout://"), get<0>(commit_load_params).url);
}

// Verify that security origins are replicated properly to RenderFrameProxies
// when swapping out.
TEST_F(RenderViewImplTest, OriginReplicationForSwapOut) {
  // This test should only run with --site-per-process, since origin
  // replication only happens in that mode.
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kSitePerProcess))
    return;

  LoadHTML(
      "Hello <iframe src='data:text/html,frame 1'></iframe>"
      "<iframe src='data:text/html,frame 2'></iframe>");
  WebFrame* web_frame = frame()->GetWebFrame();
  RenderFrameImpl* child_frame = static_cast<RenderFrameImpl*>(
      RenderFrame::FromWebFrame(web_frame->firstChild()));

  // Swap the child frame out and pass a serialized origin to be set for
  // WebRemoteFrame.
  content::FrameReplicationState replication_state;
  replication_state.origin = url::Origin("http://foo.com");
  SwapOut(child_frame, kProxyRoutingId, true, replication_state);

  // The child frame should now be a WebRemoteFrame.
  EXPECT_TRUE(web_frame->firstChild()->isWebRemoteFrame());

  // Expect the origin to be updated properly.
  blink::WebSecurityOrigin origin = web_frame->firstChild()->securityOrigin();
  EXPECT_EQ(origin.toString(),
            WebString::fromUTF8(replication_state.origin.string()));

  // Now, swap out the second frame using a unique origin and verify that it is
  // replicated correctly.
  replication_state.origin = url::Origin();
  RenderFrameImpl* child_frame2 = static_cast<RenderFrameImpl*>(
      RenderFrame::FromWebFrame(web_frame->lastChild()));
  SwapOut(child_frame2, kProxyRoutingId + 1, true, replication_state);
  EXPECT_TRUE(web_frame->lastChild()->isWebRemoteFrame());
  EXPECT_TRUE(web_frame->lastChild()->securityOrigin().isUnique());
}

// Test that we get the correct UpdateState message when we go back twice
// quickly without committing.  Regression test for http://crbug.com/58082.
// Disabled: http://crbug.com/157357 .
TEST_F(RenderViewImplTest,  DISABLED_LastCommittedUpdateState) {
  // Load page A.
  LoadHTML("<div>Page A</div>");

  // Load page B, which will trigger an UpdateState message for page A.
  LoadHTML("<div>Page B</div>");

  // Check for a valid UpdateState message for page A.
  ProcessPendingMessages();
  const IPC::Message* msg_A = render_thread_->sink().GetUniqueMessageMatching(
      ViewHostMsg_UpdateState::ID);
  ASSERT_TRUE(msg_A);
  ViewHostMsg_UpdateState::Param param;
  ViewHostMsg_UpdateState::Read(msg_A, &param);
  int page_id_A = get<0>(param);
  PageState state_A = get<1>(param);
  EXPECT_EQ(1, page_id_A);
  render_thread_->sink().ClearMessages();

  // Load page C, which will trigger an UpdateState message for page B.
  LoadHTML("<div>Page C</div>");

  // Check for a valid UpdateState for page B.
  ProcessPendingMessages();
  const IPC::Message* msg_B = render_thread_->sink().GetUniqueMessageMatching(
      ViewHostMsg_UpdateState::ID);
  ASSERT_TRUE(msg_B);
  ViewHostMsg_UpdateState::Read(msg_B, &param);
  int page_id_B = get<0>(param);
  PageState state_B = get<1>(param);
  EXPECT_EQ(2, page_id_B);
  EXPECT_NE(state_A, state_B);
  render_thread_->sink().ClearMessages();

  // Load page D, which will trigger an UpdateState message for page C.
  LoadHTML("<div>Page D</div>");

  // Check for a valid UpdateState for page C.
  ProcessPendingMessages();
  const IPC::Message* msg_C = render_thread_->sink().GetUniqueMessageMatching(
      ViewHostMsg_UpdateState::ID);
  ASSERT_TRUE(msg_C);
  ViewHostMsg_UpdateState::Read(msg_C, &param);
  int page_id_C = get<0>(param);
  PageState state_C = get<1>(param);
  EXPECT_EQ(3, page_id_C);
  EXPECT_NE(state_B, state_C);
  render_thread_->sink().ClearMessages();

  // Go back to C and commit, preparing for our real test.
  CommonNavigationParams common_params_C;
  RequestNavigationParams request_params_C;
  common_params_C.navigation_type = FrameMsg_Navigate_Type::NORMAL;
  common_params_C.transition = ui::PAGE_TRANSITION_FORWARD_BACK;
  request_params_C.current_history_list_length = 4;
  request_params_C.current_history_list_offset = 3;
  request_params_C.pending_history_list_offset = 2;
  request_params_C.page_id = 3;
  request_params_C.page_state = state_C;
  NavigateMainFrame(common_params_C, StartNavigationParams(), request_params_C);
  ProcessPendingMessages();
  render_thread_->sink().ClearMessages();

  // Go back twice quickly, such that page B does not have a chance to commit.
  // This leads to two changes to the back/forward list but only one change to
  // the RenderView's page ID.

  // Back to page B (page_id 2), without committing.
  CommonNavigationParams common_params_B;
  RequestNavigationParams request_params_B;
  common_params_B.navigation_type = FrameMsg_Navigate_Type::NORMAL;
  common_params_B.transition = ui::PAGE_TRANSITION_FORWARD_BACK;
  request_params_B.current_history_list_length = 4;
  request_params_B.current_history_list_offset = 2;
  request_params_B.pending_history_list_offset = 1;
  request_params_B.page_id = 2;
  request_params_B.page_state = state_B;
  NavigateMainFrame(common_params_B, StartNavigationParams(), request_params_B);

  // Back to page A (page_id 1) and commit.
  CommonNavigationParams common_params;
  RequestNavigationParams request_params;
  common_params.navigation_type = FrameMsg_Navigate_Type::NORMAL;
  common_params.transition = ui::PAGE_TRANSITION_FORWARD_BACK;
  request_params.current_history_list_length = 4;
  request_params.current_history_list_offset = 2;
  request_params.pending_history_list_offset = 0;
  request_params.page_id = 1;
  request_params.page_state = state_A;
  NavigateMainFrame(common_params, StartNavigationParams(), request_params);
  ProcessPendingMessages();

  // Now ensure that the UpdateState message we receive is consistent
  // and represents page C in both page_id and state.
  const IPC::Message* msg = render_thread_->sink().GetUniqueMessageMatching(
      ViewHostMsg_UpdateState::ID);
  ASSERT_TRUE(msg);
  ViewHostMsg_UpdateState::Read(msg, &param);
  int page_id = get<0>(param);
  PageState state = get<1>(param);
  EXPECT_EQ(page_id_C, page_id);
  EXPECT_NE(state_A, state);
  EXPECT_NE(state_B, state);
  EXPECT_EQ(state_C, state);
}

// Test that our IME backend sends a notification message when the input focus
// changes.
TEST_F(RenderViewImplTest, OnImeTypeChanged) {
  // Enable our IME backend code.
  view()->OnSetInputMethodActive(true);

  // Load an HTML page consisting of two input fields.
  view()->set_send_content_state_immediately(true);
  LoadHTML("<html>"
           "<head>"
           "</head>"
           "<body>"
           "<input id=\"test1\" type=\"text\" value=\"some text\"></input>"
           "<input id=\"test2\" type=\"password\"></input>"
           "<input id=\"test3\" type=\"text\" inputmode=\"verbatim\"></input>"
           "<input id=\"test4\" type=\"text\" inputmode=\"latin\"></input>"
           "<input id=\"test5\" type=\"text\" inputmode=\"latin-name\"></input>"
           "<input id=\"test6\" type=\"text\" inputmode=\"latin-prose\">"
               "</input>"
           "<input id=\"test7\" type=\"text\" inputmode=\"full-width-latin\">"
               "</input>"
           "<input id=\"test8\" type=\"text\" inputmode=\"kana\"></input>"
           "<input id=\"test9\" type=\"text\" inputmode=\"katakana\"></input>"
           "<input id=\"test10\" type=\"text\" inputmode=\"numeric\"></input>"
           "<input id=\"test11\" type=\"text\" inputmode=\"tel\"></input>"
           "<input id=\"test12\" type=\"text\" inputmode=\"email\"></input>"
           "<input id=\"test13\" type=\"text\" inputmode=\"url\"></input>"
           "<input id=\"test14\" type=\"text\" inputmode=\"unknown\"></input>"
           "<input id=\"test15\" type=\"text\" inputmode=\"verbatim\"></input>"
           "</body>"
           "</html>");
  render_thread_->sink().ClearMessages();

  struct InputModeTestCase {
    const char* input_id;
    ui::TextInputMode expected_mode;
  };
  static const InputModeTestCase kInputModeTestCases[] = {
     {"test1", ui::TEXT_INPUT_MODE_DEFAULT},
     {"test3", ui::TEXT_INPUT_MODE_VERBATIM},
     {"test4", ui::TEXT_INPUT_MODE_LATIN},
     {"test5", ui::TEXT_INPUT_MODE_LATIN_NAME},
     {"test6", ui::TEXT_INPUT_MODE_LATIN_PROSE},
     {"test7", ui::TEXT_INPUT_MODE_FULL_WIDTH_LATIN},
     {"test8", ui::TEXT_INPUT_MODE_KANA},
     {"test9", ui::TEXT_INPUT_MODE_KATAKANA},
     {"test10", ui::TEXT_INPUT_MODE_NUMERIC},
     {"test11", ui::TEXT_INPUT_MODE_TEL},
     {"test12", ui::TEXT_INPUT_MODE_EMAIL},
     {"test13", ui::TEXT_INPUT_MODE_URL},
     {"test14", ui::TEXT_INPUT_MODE_DEFAULT},
     {"test15", ui::TEXT_INPUT_MODE_VERBATIM},
  };

  const int kRepeatCount = 10;
  for (int i = 0; i < kRepeatCount; i++) {
    // Move the input focus to the first <input> element, where we should
    // activate IMEs.
    ExecuteJavaScript("document.getElementById('test1').focus();");
    ProcessPendingMessages();
    render_thread_->sink().ClearMessages();

    // Update the IME status and verify if our IME backend sends an IPC message
    // to activate IMEs.
    view()->UpdateTextInputType();
    const IPC::Message* msg = render_thread_->sink().GetMessageAt(0);
    EXPECT_TRUE(msg != NULL);
    EXPECT_EQ(ViewHostMsg_TextInputTypeChanged::ID, msg->type());
    ViewHostMsg_TextInputTypeChanged::Param params;
    ViewHostMsg_TextInputTypeChanged::Read(msg, &params);
    ui::TextInputType type = get<0>(params);
    ui::TextInputMode input_mode = get<1>(params);
    bool can_compose_inline = get<2>(params);
    EXPECT_EQ(ui::TEXT_INPUT_TYPE_TEXT, type);
    EXPECT_EQ(true, can_compose_inline);

    // Move the input focus to the second <input> element, where we should
    // de-activate IMEs.
    ExecuteJavaScript("document.getElementById('test2').focus();");
    ProcessPendingMessages();
    render_thread_->sink().ClearMessages();

    // Update the IME status and verify if our IME backend sends an IPC message
    // to de-activate IMEs.
    view()->UpdateTextInputType();
    msg = render_thread_->sink().GetMessageAt(0);
    EXPECT_TRUE(msg != NULL);
    EXPECT_EQ(ViewHostMsg_TextInputTypeChanged::ID, msg->type());
    ViewHostMsg_TextInputTypeChanged::Read(msg, & params);
    type = get<0>(params);
    input_mode = get<1>(params);
    EXPECT_EQ(ui::TEXT_INPUT_TYPE_PASSWORD, type);

    for (size_t i = 0; i < arraysize(kInputModeTestCases); i++) {
      const InputModeTestCase* test_case = &kInputModeTestCases[i];
      std::string javascript =
          base::StringPrintf("document.getElementById('%s').focus();",
                             test_case->input_id);
      // Move the input focus to the target <input> element, where we should
      // activate IMEs.
      ExecuteJavaScriptAndReturnIntValue(base::ASCIIToUTF16(javascript), NULL);
      ProcessPendingMessages();
      render_thread_->sink().ClearMessages();

      // Update the IME status and verify if our IME backend sends an IPC
      // message to activate IMEs.
      view()->UpdateTextInputType();
      const IPC::Message* msg = render_thread_->sink().GetMessageAt(0);
      EXPECT_TRUE(msg != NULL);
      EXPECT_EQ(ViewHostMsg_TextInputTypeChanged::ID, msg->type());
      ViewHostMsg_TextInputTypeChanged::Read(msg, & params);
      type = get<0>(params);
      input_mode = get<1>(params);
      EXPECT_EQ(test_case->expected_mode, input_mode);
    }
  }
}

// Test that our IME backend can compose CJK words.
// Our IME front-end sends many platform-independent messages to the IME backend
// while it composes CJK words. This test sends the minimal messages captured
// on my local environment directly to the IME backend to verify if the backend
// can compose CJK words without any problems.
// This test uses an array of command sets because an IME composotion does not
// only depends on IME events, but also depends on window events, e.g. moving
// the window focus while composing a CJK text. To handle such complicated
// cases, this test should not only call IME-related functions in the
// RenderWidget class, but also call some RenderWidget members, e.g.
// ExecuteJavaScript(), RenderWidget::OnSetFocus(), etc.
TEST_F(RenderViewImplTest, ImeComposition) {
  enum ImeCommand {
    IME_INITIALIZE,
    IME_SETINPUTMODE,
    IME_SETFOCUS,
    IME_SETCOMPOSITION,
    IME_CONFIRMCOMPOSITION,
    IME_CANCELCOMPOSITION
  };
  struct ImeMessage {
    ImeCommand command;
    bool enable;
    int selection_start;
    int selection_end;
    const wchar_t* ime_string;
    const wchar_t* result;
  };
  static const ImeMessage kImeMessages[] = {
    // Scenario 1: input a Chinese word with Microsoft IME (on Vista).
    {IME_INITIALIZE, true, 0, 0, NULL, NULL},
    {IME_SETINPUTMODE, true, 0, 0, NULL, NULL},
    {IME_SETFOCUS, true, 0, 0, NULL, NULL},
    {IME_SETCOMPOSITION, false, 1, 1, L"n", L"n"},
    {IME_SETCOMPOSITION, false, 2, 2, L"ni", L"ni"},
    {IME_SETCOMPOSITION, false, 3, 3, L"nih", L"nih"},
    {IME_SETCOMPOSITION, false, 4, 4, L"niha", L"niha"},
    {IME_SETCOMPOSITION, false, 5, 5, L"nihao", L"nihao"},
    {IME_CONFIRMCOMPOSITION, false, -1, -1, L"\x4F60\x597D", L"\x4F60\x597D"},
    // Scenario 2: input a Japanese word with Microsoft IME (on Vista).
    {IME_INITIALIZE, true, 0, 0, NULL, NULL},
    {IME_SETINPUTMODE, true, 0, 0, NULL, NULL},
    {IME_SETFOCUS, true, 0, 0, NULL, NULL},
    {IME_SETCOMPOSITION, false, 0, 1, L"\xFF4B", L"\xFF4B"},
    {IME_SETCOMPOSITION, false, 0, 1, L"\x304B", L"\x304B"},
    {IME_SETCOMPOSITION, false, 0, 2, L"\x304B\xFF4E", L"\x304B\xFF4E"},
    {IME_SETCOMPOSITION, false, 0, 3, L"\x304B\x3093\xFF4A",
     L"\x304B\x3093\xFF4A"},
    {IME_SETCOMPOSITION, false, 0, 3, L"\x304B\x3093\x3058",
     L"\x304B\x3093\x3058"},
    {IME_SETCOMPOSITION, false, 0, 2, L"\x611F\x3058", L"\x611F\x3058"},
    {IME_SETCOMPOSITION, false, 0, 2, L"\x6F22\x5B57", L"\x6F22\x5B57"},
    {IME_CONFIRMCOMPOSITION, false, -1, -1, L"", L"\x6F22\x5B57"},
    {IME_CANCELCOMPOSITION, false, -1, -1, L"", L"\x6F22\x5B57"},
    // Scenario 3: input a Korean word with Microsot IME (on Vista).
    {IME_INITIALIZE, true, 0, 0, NULL, NULL},
    {IME_SETINPUTMODE, true, 0, 0, NULL, NULL},
    {IME_SETFOCUS, true, 0, 0, NULL, NULL},
    {IME_SETCOMPOSITION, false, 0, 1, L"\x3147", L"\x3147"},
    {IME_SETCOMPOSITION, false, 0, 1, L"\xC544", L"\xC544"},
    {IME_SETCOMPOSITION, false, 0, 1, L"\xC548", L"\xC548"},
    {IME_CONFIRMCOMPOSITION, false, -1, -1, L"", L"\xC548"},
    {IME_SETCOMPOSITION, false, 0, 1, L"\x3134", L"\xC548\x3134"},
    {IME_SETCOMPOSITION, false, 0, 1, L"\xB140", L"\xC548\xB140"},
    {IME_SETCOMPOSITION, false, 0, 1, L"\xB155", L"\xC548\xB155"},
    {IME_CANCELCOMPOSITION, false, -1, -1, L"", L"\xC548"},
    {IME_SETCOMPOSITION, false, 0, 1, L"\xB155", L"\xC548\xB155"},
    {IME_CONFIRMCOMPOSITION, false, -1, -1, L"", L"\xC548\xB155"},
  };

  for (size_t i = 0; i < arraysize(kImeMessages); i++) {
    const ImeMessage* ime_message = &kImeMessages[i];
    switch (ime_message->command) {
      case IME_INITIALIZE:
        // Load an HTML page consisting of a content-editable <div> element,
        // and move the input focus to the <div> element, where we can use
        // IMEs.
        view()->OnSetInputMethodActive(ime_message->enable);
        view()->set_send_content_state_immediately(true);
        LoadHTML("<html>"
                "<head>"
                "</head>"
                "<body>"
                "<div id=\"test1\" contenteditable=\"true\"></div>"
                "</body>"
                "</html>");
        ExecuteJavaScript("document.getElementById('test1').focus();");
        break;

      case IME_SETINPUTMODE:
        // Activate (or deactivate) our IME back-end.
        view()->OnSetInputMethodActive(ime_message->enable);
        break;

      case IME_SETFOCUS:
        // Update the window focus.
        view()->OnSetFocus(ime_message->enable);
        break;

      case IME_SETCOMPOSITION:
        view()->OnImeSetComposition(
            base::WideToUTF16(ime_message->ime_string),
            std::vector<blink::WebCompositionUnderline>(),
            ime_message->selection_start,
            ime_message->selection_end);
        break;

      case IME_CONFIRMCOMPOSITION:
        view()->OnImeConfirmComposition(
            base::WideToUTF16(ime_message->ime_string),
            gfx::Range::InvalidRange(),
            false);
        break;

      case IME_CANCELCOMPOSITION:
        view()->OnImeSetComposition(
            base::string16(),
            std::vector<blink::WebCompositionUnderline>(),
            0, 0);
        break;
    }

    // Update the status of our IME back-end.
    // TODO(hbono): we should verify messages to be sent from the back-end.
    view()->UpdateTextInputType();
    ProcessPendingMessages();
    render_thread_->sink().ClearMessages();

    if (ime_message->result) {
      // Retrieve the content of this page and compare it with the expected
      // result.
      const int kMaxOutputCharacters = 128;
      base::string16 output =
          GetMainFrame()->contentAsText(kMaxOutputCharacters);
      EXPECT_EQ(base::WideToUTF16(ime_message->result), output);
    }
  }
}

// Test that the RenderView::OnSetTextDirection() function can change the text
// direction of the selected input element.
TEST_F(RenderViewImplTest, OnSetTextDirection) {
  // Load an HTML page consisting of a <textarea> element and a <div> element.
  // This test changes the text direction of the <textarea> element, and
  // writes the values of its 'dir' attribute and its 'direction' property to
  // verify that the text direction is changed.
  view()->set_send_content_state_immediately(true);
  LoadHTML("<html>"
           "<head>"
           "</head>"
           "<body>"
           "<textarea id=\"test\"></textarea>"
           "<div id=\"result\" contenteditable=\"true\"></div>"
           "</body>"
           "</html>");
  render_thread_->sink().ClearMessages();

  static const struct {
    WebTextDirection direction;
    const wchar_t* expected_result;
  } kTextDirection[] = {
    { blink::WebTextDirectionRightToLeft, L"\x000A" L"rtl,rtl" },
    { blink::WebTextDirectionLeftToRight, L"\x000A" L"ltr,ltr" },
  };
  for (size_t i = 0; i < arraysize(kTextDirection); ++i) {
    // Set the text direction of the <textarea> element.
    ExecuteJavaScript("document.getElementById('test').focus();");
    view()->OnSetTextDirection(kTextDirection[i].direction);

    // Write the values of its DOM 'dir' attribute and its CSS 'direction'
    // property to the <div> element.
    ExecuteJavaScript("var result = document.getElementById('result');"
                      "var node = document.getElementById('test');"
                      "var style = getComputedStyle(node, null);"
                      "result.innerText ="
                      "    node.getAttribute('dir') + ',' +"
                      "    style.getPropertyValue('direction');");

    // Copy the document content to std::wstring and compare with the
    // expected result.
    const int kMaxOutputCharacters = 16;
    base::string16 output = GetMainFrame()->contentAsText(kMaxOutputCharacters);
    EXPECT_EQ(base::WideToUTF16(kTextDirection[i].expected_result), output);
  }
}

// see http://crbug.com/238750
#if defined(OS_WIN)
#define MAYBE_OnHandleKeyboardEvent DISABLED_OnHandleKeyboardEvent
#else
#define MAYBE_OnHandleKeyboardEvent OnHandleKeyboardEvent
#endif

// Test that we can receive correct DOM events when we send input events
// through the RenderWidget::OnHandleInputEvent() function.
TEST_F(RenderViewImplTest, MAYBE_OnHandleKeyboardEvent) {
#if !defined(OS_MACOSX)
  // Load an HTML page consisting of one <input> element and three
  // contentediable <div> elements.
  // The <input> element is used for sending keyboard events, and the <div>
  // elements are used for writing DOM events in the following format:
  //   "<keyCode>,<shiftKey>,<controlKey>,<altKey>".
  // TODO(hbono): <http://crbug.com/2215> Our WebKit port set |ev.metaKey| to
  // true when pressing an alt key, i.e. the |ev.metaKey| value is not
  // trustworthy. We will check the |ev.metaKey| value when this issue is fixed.
  view()->set_send_content_state_immediately(true);
  LoadHTML("<html>"
           "<head>"
           "<title></title>"
           "<script type='text/javascript' language='javascript'>"
           "function OnKeyEvent(ev) {"
           "  var result = document.getElementById(ev.type);"
           "  result.innerText ="
           "      (ev.which || ev.keyCode) + ',' +"
           "      ev.shiftKey + ',' +"
           "      ev.ctrlKey + ',' +"
           "      ev.altKey;"
           "  return true;"
           "}"
           "</script>"
           "</head>"
           "<body>"
           "<input id='test' type='text'"
           "    onkeydown='return OnKeyEvent(event);'"
           "    onkeypress='return OnKeyEvent(event);'"
           "    onkeyup='return OnKeyEvent(event);'>"
           "</input>"
           "<div id='keydown' contenteditable='true'>"
           "</div>"
           "<div id='keypress' contenteditable='true'>"
           "</div>"
           "<div id='keyup' contenteditable='true'>"
           "</div>"
           "</body>"
           "</html>");
  ExecuteJavaScript("document.getElementById('test').focus();");
  render_thread_->sink().ClearMessages();

  static const MockKeyboard::Layout kLayouts[] = {
#if defined(OS_WIN)
    // Since we ignore the mock keyboard layout on Linux and instead just use
    // the screen's keyboard layout, these trivially pass. They are commented
    // out to avoid the illusion that they work.
    MockKeyboard::LAYOUT_ARABIC,
    MockKeyboard::LAYOUT_CANADIAN_FRENCH,
    MockKeyboard::LAYOUT_FRENCH,
    MockKeyboard::LAYOUT_HEBREW,
    MockKeyboard::LAYOUT_RUSSIAN,
#endif
    MockKeyboard::LAYOUT_UNITED_STATES,
  };

  for (size_t i = 0; i < arraysize(kLayouts); ++i) {
    // For each key code, we send three keyboard events.
    //  * we press only the key;
    //  * we press the key and a left-shift key, and;
    //  * we press the key and a right-alt (AltGr) key.
    // For each modifiers, we need a string used for formatting its expected
    // result. (See the above comment for its format.)
    static const struct {
      MockKeyboard::Modifiers modifiers;
      const char* expected_result;
    } kModifierData[] = {
      {MockKeyboard::NONE,       "false,false,false"},
      {MockKeyboard::LEFT_SHIFT, "true,false,false"},
#if defined(OS_WIN)
      {MockKeyboard::RIGHT_ALT,  "false,false,true"},
#endif
    };

    MockKeyboard::Layout layout = kLayouts[i];
    for (size_t j = 0; j < arraysize(kModifierData); ++j) {
      // Virtual key codes used for this test.
      static const int kKeyCodes[] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
        'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
        'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
        'W', 'X', 'Y', 'Z',
        ui::VKEY_OEM_1,
        ui::VKEY_OEM_PLUS,
        ui::VKEY_OEM_COMMA,
        ui::VKEY_OEM_MINUS,
        ui::VKEY_OEM_PERIOD,
        ui::VKEY_OEM_2,
        ui::VKEY_OEM_3,
        ui::VKEY_OEM_4,
        ui::VKEY_OEM_5,
        ui::VKEY_OEM_6,
        ui::VKEY_OEM_7,
#if defined(OS_WIN)
        // Not sure how to handle this key on Linux.
        ui::VKEY_OEM_8,
#endif
      };

      MockKeyboard::Modifiers modifiers = kModifierData[j].modifiers;
      for (size_t k = 0; k < arraysize(kKeyCodes); ++k) {
        // Send a keyboard event to the RenderView object.
        // We should test a keyboard event only when the given keyboard-layout
        // driver is installed in a PC and the driver can assign a Unicode
        // charcter for the given tuple (key-code and modifiers).
        int key_code = kKeyCodes[k];
        base::string16 char_code;
        if (SendKeyEvent(layout, key_code, modifiers, &char_code) < 0)
          continue;

        // Create an expected result from the virtual-key code, the character
        // code, and the modifier-key status.
        // We format a string that emulates a DOM-event string produced hy
        // our JavaScript function. (See the above comment for the format.)
        static char expected_result[1024];
        expected_result[0] = 0;
        base::snprintf(&expected_result[0],
                       sizeof(expected_result),
                       "\n"       // texts in the <input> element
                       "%d,%s\n"  // texts in the first <div> element
                       "%d,%s\n"  // texts in the second <div> element
                       "%d,%s",   // texts in the third <div> element
                       key_code, kModifierData[j].expected_result,
                       static_cast<int>(char_code[0]),
                       kModifierData[j].expected_result,
                       key_code, kModifierData[j].expected_result);

        // Retrieve the text in the test page and compare it with the expected
        // text created from a virtual-key code, a character code, and the
        // modifier-key status.
        const int kMaxOutputCharacters = 1024;
        std::string output = base::UTF16ToUTF8(
            GetMainFrame()->contentAsText(kMaxOutputCharacters));
        EXPECT_EQ(expected_result, output);
      }
    }
  }
#else
  NOTIMPLEMENTED();
#endif
}

// Test that our EditorClientImpl class can insert characters when we send
// keyboard events through the RenderWidget::OnHandleInputEvent() function.
// This test is for preventing regressions caused only when we use non-US
// keyboards, such as Issue 10846.
// see http://crbug.com/244562
#if defined(OS_WIN)
#define MAYBE_InsertCharacters DISABLED_InsertCharacters
#else
#define MAYBE_InsertCharacters InsertCharacters
#endif
TEST_F(RenderViewImplTest, MAYBE_InsertCharacters) {
#if !defined(OS_MACOSX)
  static const struct {
    MockKeyboard::Layout layout;
    const wchar_t* expected_result;
  } kLayouts[] = {
#if 0
    // Disabled these keyboard layouts because buildbots do not have their
    // keyboard-layout drivers installed.
    {MockKeyboard::LAYOUT_ARABIC,
     L"\x0030\x0031\x0032\x0033\x0034\x0035\x0036\x0037"
     L"\x0038\x0039\x0634\x0624\x064a\x062b\x0628\x0644"
     L"\x0627\x0647\x062a\x0646\x0645\x0629\x0649\x062e"
     L"\x062d\x0636\x0642\x0633\x0641\x0639\x0631\x0635"
     L"\x0621\x063a\x0626\x0643\x003d\x0648\x002d\x0632"
     L"\x0638\x0630\x062c\x005c\x062f\x0637\x0028\x0021"
     L"\x0040\x0023\x0024\x0025\x005e\x0026\x002a\x0029"
     L"\x0650\x007d\x005d\x064f\x005b\x0623\x00f7\x0640"
     L"\x060c\x002f\x2019\x0622\x00d7\x061b\x064e\x064c"
     L"\x064d\x2018\x007b\x064b\x0652\x0625\x007e\x003a"
     L"\x002b\x002c\x005f\x002e\x061f\x0651\x003c\x007c"
     L"\x003e\x0022\x0030\x0031\x0032\x0033\x0034\x0035"
     L"\x0036\x0037\x0038\x0039\x0634\x0624\x064a\x062b"
     L"\x0628\x0644\x0627\x0647\x062a\x0646\x0645\x0629"
     L"\x0649\x062e\x062d\x0636\x0642\x0633\x0641\x0639"
     L"\x0631\x0635\x0621\x063a\x0626\x0643\x003d\x0648"
     L"\x002d\x0632\x0638\x0630\x062c\x005c\x062f\x0637"
    },
    {MockKeyboard::LAYOUT_HEBREW,
     L"\x0030\x0031\x0032\x0033\x0034\x0035\x0036\x0037"
     L"\x0038\x0039\x05e9\x05e0\x05d1\x05d2\x05e7\x05db"
     L"\x05e2\x05d9\x05df\x05d7\x05dc\x05da\x05e6\x05de"
     L"\x05dd\x05e4\x002f\x05e8\x05d3\x05d0\x05d5\x05d4"
     L"\x0027\x05e1\x05d8\x05d6\x05e3\x003d\x05ea\x002d"
     L"\x05e5\x002e\x003b\x005d\x005c\x005b\x002c\x0028"
     L"\x0021\x0040\x0023\x0024\x0025\x005e\x0026\x002a"
     L"\x0029\x0041\x0042\x0043\x0044\x0045\x0046\x0047"
     L"\x0048\x0049\x004a\x004b\x004c\x004d\x004e\x004f"
     L"\x0050\x0051\x0052\x0053\x0054\x0055\x0056\x0057"
     L"\x0058\x0059\x005a\x003a\x002b\x003e\x005f\x003c"
     L"\x003f\x007e\x007d\x007c\x007b\x0022\x0030\x0031"
     L"\x0032\x0033\x0034\x0035\x0036\x0037\x0038\x0039"
     L"\x05e9\x05e0\x05d1\x05d2\x05e7\x05db\x05e2\x05d9"
     L"\x05df\x05d7\x05dc\x05da\x05e6\x05de\x05dd\x05e4"
     L"\x002f\x05e8\x05d3\x05d0\x05d5\x05d4\x0027\x05e1"
     L"\x05d8\x05d6\x05e3\x003d\x05ea\x002d\x05e5\x002e"
     L"\x003b\x005d\x005c\x005b\x002c"
    },
#endif
#if defined(OS_WIN)
    // On Linux, the only way to test alternate keyboard layouts is to change
    // the keyboard layout of the whole screen. I'm worried about the side
    // effects this may have on the buildbots.
    {MockKeyboard::LAYOUT_CANADIAN_FRENCH,
     L"\x0030\x0031\x0032\x0033\x0034\x0035\x0036\x0037"
     L"\x0038\x0039\x0061\x0062\x0063\x0064\x0065\x0066"
     L"\x0067\x0068\x0069\x006a\x006b\x006c\x006d\x006e"
     L"\x006f\x0070\x0071\x0072\x0073\x0074\x0075\x0076"
     L"\x0077\x0078\x0079\x007a\x003b\x003d\x002c\x002d"
     L"\x002e\x00e9\x003c\x0029\x0021\x0022\x002f\x0024"
     L"\x0025\x003f\x0026\x002a\x0028\x0041\x0042\x0043"
     L"\x0044\x0045\x0046\x0047\x0048\x0049\x004a\x004b"
     L"\x004c\x004d\x004e\x004f\x0050\x0051\x0052\x0053"
     L"\x0054\x0055\x0056\x0057\x0058\x0059\x005a\x003a"
     L"\x002b\x0027\x005f\x002e\x00c9\x003e\x0030\x0031"
     L"\x0032\x0033\x0034\x0035\x0036\x0037\x0038\x0039"
     L"\x0061\x0062\x0063\x0064\x0065\x0066\x0067\x0068"
     L"\x0069\x006a\x006b\x006c\x006d\x006e\x006f\x0070"
     L"\x0071\x0072\x0073\x0074\x0075\x0076\x0077\x0078"
     L"\x0079\x007a\x003b\x003d\x002c\x002d\x002e\x00e9"
     L"\x003c"
    },
    {MockKeyboard::LAYOUT_FRENCH,
     L"\x00e0\x0026\x00e9\x0022\x0027\x0028\x002d\x00e8"
     L"\x005f\x00e7\x0061\x0062\x0063\x0064\x0065\x0066"
     L"\x0067\x0068\x0069\x006a\x006b\x006c\x006d\x006e"
     L"\x006f\x0070\x0071\x0072\x0073\x0074\x0075\x0076"
     L"\x0077\x0078\x0079\x007a\x0024\x003d\x002c\x003b"
     L"\x003a\x00f9\x0029\x002a\x0021\x0030\x0031\x0032"
     L"\x0033\x0034\x0035\x0036\x0037\x0038\x0039\x0041"
     L"\x0042\x0043\x0044\x0045\x0046\x0047\x0048\x0049"
     L"\x004a\x004b\x004c\x004d\x004e\x004f\x0050\x0051"
     L"\x0052\x0053\x0054\x0055\x0056\x0057\x0058\x0059"
     L"\x005a\x00a3\x002b\x003f\x002e\x002f\x0025\x00b0"
     L"\x00b5\x00e0\x0026\x00e9\x0022\x0027\x0028\x002d"
     L"\x00e8\x005f\x00e7\x0061\x0062\x0063\x0064\x0065"
     L"\x0066\x0067\x0068\x0069\x006a\x006b\x006c\x006d"
     L"\x006e\x006f\x0070\x0071\x0072\x0073\x0074\x0075"
     L"\x0076\x0077\x0078\x0079\x007a\x0024\x003d\x002c"
     L"\x003b\x003a\x00f9\x0029\x002a\x0021"
    },
    {MockKeyboard::LAYOUT_RUSSIAN,
     L"\x0030\x0031\x0032\x0033\x0034\x0035\x0036\x0037"
     L"\x0038\x0039\x0444\x0438\x0441\x0432\x0443\x0430"
     L"\x043f\x0440\x0448\x043e\x043b\x0434\x044c\x0442"
     L"\x0449\x0437\x0439\x043a\x044b\x0435\x0433\x043c"
     L"\x0446\x0447\x043d\x044f\x0436\x003d\x0431\x002d"
     L"\x044e\x002e\x0451\x0445\x005c\x044a\x044d\x0029"
     L"\x0021\x0022\x2116\x003b\x0025\x003a\x003f\x002a"
     L"\x0028\x0424\x0418\x0421\x0412\x0423\x0410\x041f"
     L"\x0420\x0428\x041e\x041b\x0414\x042c\x0422\x0429"
     L"\x0417\x0419\x041a\x042b\x0415\x0413\x041c\x0426"
     L"\x0427\x041d\x042f\x0416\x002b\x0411\x005f\x042e"
     L"\x002c\x0401\x0425\x002f\x042a\x042d\x0030\x0031"
     L"\x0032\x0033\x0034\x0035\x0036\x0037\x0038\x0039"
     L"\x0444\x0438\x0441\x0432\x0443\x0430\x043f\x0440"
     L"\x0448\x043e\x043b\x0434\x044c\x0442\x0449\x0437"
     L"\x0439\x043a\x044b\x0435\x0433\x043c\x0446\x0447"
     L"\x043d\x044f\x0436\x003d\x0431\x002d\x044e\x002e"
     L"\x0451\x0445\x005c\x044a\x044d"
    },
#endif  // defined(OS_WIN)
    {MockKeyboard::LAYOUT_UNITED_STATES,
     L"\x0030\x0031\x0032\x0033\x0034\x0035\x0036\x0037"
     L"\x0038\x0039\x0061\x0062\x0063\x0064\x0065\x0066"
     L"\x0067\x0068\x0069\x006a\x006b\x006c\x006d\x006e"
     L"\x006f\x0070\x0071\x0072\x0073\x0074\x0075\x0076"
     L"\x0077\x0078\x0079\x007a\x003b\x003d\x002c\x002d"
     L"\x002e\x002f\x0060\x005b\x005c\x005d\x0027\x0029"
     L"\x0021\x0040\x0023\x0024\x0025\x005e\x0026\x002a"
     L"\x0028\x0041\x0042\x0043\x0044\x0045\x0046\x0047"
     L"\x0048\x0049\x004a\x004b\x004c\x004d\x004e\x004f"
     L"\x0050\x0051\x0052\x0053\x0054\x0055\x0056\x0057"
     L"\x0058\x0059\x005a\x003a\x002b\x003c\x005f\x003e"
     L"\x003f\x007e\x007b\x007c\x007d\x0022"
#if defined(OS_WIN)
     // This is ifdefed out for Linux to correspond to the fact that we don't
     // test alt+keystroke for now.
     L"\x0030\x0031\x0032\x0033\x0034\x0035\x0036\x0037"
     L"\x0038\x0039\x0061\x0062\x0063\x0064\x0065\x0066"
     L"\x0067\x0068\x0069\x006a\x006b\x006c\x006d\x006e"
     L"\x006f\x0070\x0071\x0072\x0073\x0074\x0075\x0076"
     L"\x0077\x0078\x0079\x007a\x003b\x003d\x002c\x002d"
     L"\x002e\x002f\x0060\x005b\x005c\x005d\x0027"
#endif
    },
  };

  for (size_t i = 0; i < arraysize(kLayouts); ++i) {
    // Load an HTML page consisting of one <div> element.
    // This <div> element is used by the EditorClientImpl class to insert
    // characters received through the RenderWidget::OnHandleInputEvent()
    // function.
    view()->set_send_content_state_immediately(true);
    LoadHTML("<html>"
             "<head>"
             "<title></title>"
             "</head>"
             "<body>"
             "<div id='test' contenteditable='true'>"
             "</div>"
             "</body>"
             "</html>");
    ExecuteJavaScript("document.getElementById('test').focus();");
    render_thread_->sink().ClearMessages();

    // For each key code, we send three keyboard events.
    //  * Pressing only the key;
    //  * Pressing the key and a left-shift key, and;
    //  * Pressing the key and a right-alt (AltGr) key.
    static const MockKeyboard::Modifiers kModifiers[] = {
      MockKeyboard::NONE,
      MockKeyboard::LEFT_SHIFT,
#if defined(OS_WIN)
      MockKeyboard::RIGHT_ALT,
#endif
    };

    MockKeyboard::Layout layout = kLayouts[i].layout;
    for (size_t j = 0; j < arraysize(kModifiers); ++j) {
      // Virtual key codes used for this test.
      static const int kKeyCodes[] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
        'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
        'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
        'W', 'X', 'Y', 'Z',
        ui::VKEY_OEM_1,
        ui::VKEY_OEM_PLUS,
        ui::VKEY_OEM_COMMA,
        ui::VKEY_OEM_MINUS,
        ui::VKEY_OEM_PERIOD,
        ui::VKEY_OEM_2,
        ui::VKEY_OEM_3,
        ui::VKEY_OEM_4,
        ui::VKEY_OEM_5,
        ui::VKEY_OEM_6,
        ui::VKEY_OEM_7,
#if defined(OS_WIN)
        // Unclear how to handle this on Linux.
        ui::VKEY_OEM_8,
#endif
      };

      MockKeyboard::Modifiers modifiers = kModifiers[j];
      for (size_t k = 0; k < arraysize(kKeyCodes); ++k) {
        // Send a keyboard event to the RenderView object.
        // We should test a keyboard event only when the given keyboard-layout
        // driver is installed in a PC and the driver can assign a Unicode
        // charcter for the given tuple (layout, key-code, and modifiers).
        int key_code = kKeyCodes[k];
        base::string16 char_code;
        if (SendKeyEvent(layout, key_code, modifiers, &char_code) < 0)
          continue;
      }
    }

    // Retrieve the text in the test page and compare it with the expected
    // text created from a virtual-key code, a character code, and the
    // modifier-key status.
    const int kMaxOutputCharacters = 4096;
    base::string16 output = GetMainFrame()->contentAsText(kMaxOutputCharacters);
    EXPECT_EQ(base::WideToUTF16(kLayouts[i].expected_result), output);
  }
#else
  NOTIMPLEMENTED();
#endif
}

// Crashy, http://crbug.com/53247.
TEST_F(RenderViewImplTest, DISABLED_DidFailProvisionalLoadWithErrorForError) {
  GetMainFrame()->enableViewSourceMode(true);
  WebURLError error;
  error.domain = WebString::fromUTF8(net::kErrorDomain);
  error.reason = net::ERR_FILE_NOT_FOUND;
  error.unreachableURL = GURL("http://foo");
  WebLocalFrame* web_frame = GetMainFrame();

  // Start a load that will reach provisional state synchronously,
  // but won't complete synchronously.
  CommonNavigationParams common_params;
  common_params.navigation_type = FrameMsg_Navigate_Type::NORMAL;
  common_params.url = GURL("data:text/html,test data");
  NavigateMainFrame(common_params, StartNavigationParams(),
                    RequestNavigationParams());

  // An error occurred.
  view()->GetMainRenderFrame()->didFailProvisionalLoad(
      web_frame, error, blink::WebStandardCommit);
  // Frame should exit view-source mode.
  EXPECT_FALSE(web_frame->isViewSourceModeEnabled());
}

TEST_F(RenderViewImplTest, DidFailProvisionalLoadWithErrorForCancellation) {
  GetMainFrame()->enableViewSourceMode(true);
  WebURLError error;
  error.domain = WebString::fromUTF8(net::kErrorDomain);
  error.reason = net::ERR_ABORTED;
  error.unreachableURL = GURL("http://foo");
  WebLocalFrame* web_frame = GetMainFrame();

  // Start a load that will reach provisional state synchronously,
  // but won't complete synchronously.
  CommonNavigationParams common_params;
  common_params.navigation_type = FrameMsg_Navigate_Type::NORMAL;
  common_params.url = GURL("data:text/html,test data");
  NavigateMainFrame(common_params, StartNavigationParams(),
                    RequestNavigationParams());

  // A cancellation occurred.
  view()->GetMainRenderFrame()->didFailProvisionalLoad(
      web_frame, error, blink::WebStandardCommit);
  // Frame should stay in view-source mode.
  EXPECT_TRUE(web_frame->isViewSourceModeEnabled());
}

// Regression test for http://crbug.com/41562
TEST_F(RenderViewImplTest, UpdateTargetURLWithInvalidURL) {
  const GURL invalid_gurl("http://");
  view()->setMouseOverURL(blink::WebURL(invalid_gurl));
  EXPECT_EQ(invalid_gurl, view()->target_url_);
}

TEST_F(RenderViewImplTest, SetHistoryLengthAndOffset) {
  // No history to merge; one committed page.
  view()->OnSetHistoryOffsetAndLength(0, 1);
  EXPECT_EQ(1, view()->history_list_length_);
  EXPECT_EQ(0, view()->history_list_offset_);

  // History of length 1 to merge; one committed page.
  view()->OnSetHistoryOffsetAndLength(1, 2);
  EXPECT_EQ(2, view()->history_list_length_);
  EXPECT_EQ(1, view()->history_list_offset_);
}

TEST_F(RenderViewImplTest, ContextMenu) {
  LoadHTML("<div>Page A</div>");

  // Create a right click in the center of the iframe. (I'm hoping this will
  // make this a bit more robust in case of some other formatting or other bug.)
  WebMouseEvent mouse_event;
  mouse_event.type = WebInputEvent::MouseDown;
  mouse_event.button = WebMouseEvent::ButtonRight;
  mouse_event.x = 250;
  mouse_event.y = 250;
  mouse_event.globalX = 250;
  mouse_event.globalY = 250;

  SendWebMouseEvent(mouse_event);

  // Now simulate the corresponding up event which should display the menu
  mouse_event.type = WebInputEvent::MouseUp;
  SendWebMouseEvent(mouse_event);

  EXPECT_TRUE(render_thread_->sink().GetUniqueMessageMatching(
      FrameHostMsg_ContextMenu::ID));
}

TEST_F(RenderViewImplTest, TestBackForward) {
  LoadHTML("<div id=pagename>Page A</div>");
  PageState page_a_state =
      HistoryEntryToPageState(view()->history_controller()->GetCurrentEntry());
  int was_page_a = -1;
  base::string16 check_page_a =
      base::ASCIIToUTF16(
          "Number(document.getElementById('pagename').innerHTML == 'Page A')");
  EXPECT_TRUE(ExecuteJavaScriptAndReturnIntValue(check_page_a, &was_page_a));
  EXPECT_EQ(1, was_page_a);

  LoadHTML("<div id=pagename>Page B</div>");
  int was_page_b = -1;
  base::string16 check_page_b =
      base::ASCIIToUTF16(
          "Number(document.getElementById('pagename').innerHTML == 'Page B')");
  EXPECT_TRUE(ExecuteJavaScriptAndReturnIntValue(check_page_b, &was_page_b));
  EXPECT_EQ(1, was_page_b);

  PageState back_state =
      HistoryEntryToPageState(view()->history_controller()->GetCurrentEntry());

  LoadHTML("<div id=pagename>Page C</div>");
  int was_page_c = -1;
  base::string16 check_page_c =
      base::ASCIIToUTF16(
          "Number(document.getElementById('pagename').innerHTML == 'Page C')");
  EXPECT_TRUE(ExecuteJavaScriptAndReturnIntValue(check_page_c, &was_page_c));
  EXPECT_EQ(1, was_page_c);

  PageState forward_state =
      HistoryEntryToPageState(view()->history_controller()->GetCurrentEntry());
  GoBack(back_state);
  EXPECT_TRUE(ExecuteJavaScriptAndReturnIntValue(check_page_b, &was_page_b));
  EXPECT_EQ(1, was_page_b);

  PageState back_state2 =
      HistoryEntryToPageState(view()->history_controller()->GetCurrentEntry());

  GoForward(forward_state);
  EXPECT_TRUE(ExecuteJavaScriptAndReturnIntValue(check_page_c, &was_page_c));
  EXPECT_EQ(1, was_page_c);

  GoBack(back_state2);
  EXPECT_TRUE(ExecuteJavaScriptAndReturnIntValue(check_page_b, &was_page_b));
  EXPECT_EQ(1, was_page_b);

  forward_state =
      HistoryEntryToPageState(view()->history_controller()->GetCurrentEntry());
  GoBack(page_a_state);
  EXPECT_TRUE(ExecuteJavaScriptAndReturnIntValue(check_page_a, &was_page_a));
  EXPECT_EQ(1, was_page_a);

  GoForward(forward_state);
  EXPECT_TRUE(ExecuteJavaScriptAndReturnIntValue(check_page_b, &was_page_b));
  EXPECT_EQ(1, was_page_b);
}

#if defined(OS_MACOSX) || defined(USE_AURA)
TEST_F(RenderViewImplTest, GetCompositionCharacterBoundsTest) {

#if defined(OS_WIN)
  // http://crbug.com/304193
  if (base::win::GetVersion() < base::win::VERSION_VISTA)
    return;
#endif

  LoadHTML("<textarea id=\"test\"></textarea>");
  ExecuteJavaScript("document.getElementById('test').focus();");

  const base::string16 empty_string;
  const std::vector<blink::WebCompositionUnderline> empty_underline;
  std::vector<gfx::Rect> bounds;
  view()->OnSetFocus(true);
  view()->OnSetInputMethodActive(true);

  // ASCII composition
  const base::string16 ascii_composition = base::UTF8ToUTF16("aiueo");
  view()->OnImeSetComposition(ascii_composition, empty_underline, 0, 0);
  view()->GetCompositionCharacterBounds(&bounds);
  ASSERT_EQ(ascii_composition.size(), bounds.size());
  for (size_t i = 0; i < bounds.size(); ++i)
    EXPECT_LT(0, bounds[i].width());
  view()->OnImeConfirmComposition(
      empty_string, gfx::Range::InvalidRange(), false);

  // Non surrogate pair unicode character.
  const base::string16 unicode_composition = base::UTF8ToUTF16(
      "\xE3\x81\x82\xE3\x81\x84\xE3\x81\x86\xE3\x81\x88\xE3\x81\x8A");
  view()->OnImeSetComposition(unicode_composition, empty_underline, 0, 0);
  view()->GetCompositionCharacterBounds(&bounds);
  ASSERT_EQ(unicode_composition.size(), bounds.size());
  for (size_t i = 0; i < bounds.size(); ++i)
    EXPECT_LT(0, bounds[i].width());
  view()->OnImeConfirmComposition(
      empty_string, gfx::Range::InvalidRange(), false);

  // Surrogate pair character.
  const base::string16 surrogate_pair_char =
      base::UTF8ToUTF16("\xF0\xA0\xAE\x9F");
  view()->OnImeSetComposition(surrogate_pair_char,
                              empty_underline,
                              0,
                              0);
  view()->GetCompositionCharacterBounds(&bounds);
  ASSERT_EQ(surrogate_pair_char.size(), bounds.size());
  EXPECT_LT(0, bounds[0].width());
  EXPECT_EQ(0, bounds[1].width());
  view()->OnImeConfirmComposition(
      empty_string, gfx::Range::InvalidRange(), false);

  // Mixed string.
  const base::string16 surrogate_pair_mixed_composition =
      surrogate_pair_char + base::UTF8ToUTF16("\xE3\x81\x82") +
      surrogate_pair_char + base::UTF8ToUTF16("b") + surrogate_pair_char;
  const size_t utf16_length = 8UL;
  const bool is_surrogate_pair_empty_rect[8] = {
    false, true, false, false, true, false, false, true };
  view()->OnImeSetComposition(surrogate_pair_mixed_composition,
                              empty_underline,
                              0,
                              0);
  view()->GetCompositionCharacterBounds(&bounds);
  ASSERT_EQ(utf16_length, bounds.size());
  for (size_t i = 0; i < utf16_length; ++i) {
    if (is_surrogate_pair_empty_rect[i]) {
      EXPECT_EQ(0, bounds[i].width());
    } else {
      EXPECT_LT(0, bounds[i].width());
    }
  }
  view()->OnImeConfirmComposition(
      empty_string, gfx::Range::InvalidRange(), false);
}
#endif

TEST_F(RenderViewImplTest, ZoomLimit) {
  const double kMinZoomLevel = ZoomFactorToZoomLevel(kMinimumZoomFactor);
  const double kMaxZoomLevel = ZoomFactorToZoomLevel(kMaximumZoomFactor);

  // Verifies navigation to a URL with preset zoom level indeed sets the level.
  // Regression test for http://crbug.com/139559, where the level was not
  // properly set when it is out of the default zoom limits of WebView.
  CommonNavigationParams common_params;
  common_params.url = GURL("data:text/html,min_zoomlimit_test");
  view()->OnSetZoomLevelForLoadingURL(common_params.url, kMinZoomLevel);
  NavigateMainFrame(common_params, StartNavigationParams(),
                    RequestNavigationParams());
  ProcessPendingMessages();
  EXPECT_DOUBLE_EQ(kMinZoomLevel, view()->GetWebView()->zoomLevel());

  // It should work even when the zoom limit is temporarily changed in the page.
  view()->GetWebView()->zoomLimitsChanged(ZoomFactorToZoomLevel(1.0),
                                          ZoomFactorToZoomLevel(1.0));
  common_params.url = GURL("data:text/html,max_zoomlimit_test");
  view()->OnSetZoomLevelForLoadingURL(common_params.url, kMaxZoomLevel);
  NavigateMainFrame(common_params, StartNavigationParams(),
                    RequestNavigationParams());
  ProcessPendingMessages();
  EXPECT_DOUBLE_EQ(kMaxZoomLevel, view()->GetWebView()->zoomLevel());
}

TEST_F(RenderViewImplTest, SetEditableSelectionAndComposition) {
  // Load an HTML page consisting of an input field.
  LoadHTML("<html>"
           "<head>"
           "</head>"
           "<body>"
           "<input id=\"test1\" value=\"some test text hello\"></input>"
           "</body>"
           "</html>");
  ExecuteJavaScript("document.getElementById('test1').focus();");
  SetEditableSelectionOffsets(4, 8);
  const std::vector<blink::WebCompositionUnderline> empty_underline;
  SetCompositionFromExistingText(7, 10, empty_underline);
  blink::WebTextInputInfo info = view()->webview()->textInputInfo();
  EXPECT_EQ(4, info.selectionStart);
  EXPECT_EQ(8, info.selectionEnd);
  EXPECT_EQ(7, info.compositionStart);
  EXPECT_EQ(10, info.compositionEnd);
  Unselect();
  info = view()->webview()->textInputInfo();
  EXPECT_EQ(0, info.selectionStart);
  EXPECT_EQ(0, info.selectionEnd);
}


TEST_F(RenderViewImplTest, OnExtendSelectionAndDelete) {
  // Load an HTML page consisting of an input field.
  LoadHTML("<html>"
           "<head>"
           "</head>"
           "<body>"
           "<input id=\"test1\" value=\"abcdefghijklmnopqrstuvwxyz\"></input>"
           "</body>"
           "</html>");
  ExecuteJavaScript("document.getElementById('test1').focus();");
  SetEditableSelectionOffsets(10, 10);
  ExtendSelectionAndDelete(3, 4);
  blink::WebTextInputInfo info = view()->webview()->textInputInfo();
  EXPECT_EQ("abcdefgopqrstuvwxyz", info.value);
  EXPECT_EQ(7, info.selectionStart);
  EXPECT_EQ(7, info.selectionEnd);
  SetEditableSelectionOffsets(4, 8);
  ExtendSelectionAndDelete(2, 5);
  info = view()->webview()->textInputInfo();
  EXPECT_EQ("abuvwxyz", info.value);
  EXPECT_EQ(2, info.selectionStart);
  EXPECT_EQ(2, info.selectionEnd);
}

// Test that the navigating specific frames works correctly.
TEST_F(RenderViewImplTest, NavigateSubframe) {
  // Load page A.
  LoadHTML("hello <iframe srcdoc='fail' name='frame'></iframe>");

  // Navigate the frame only.
  CommonNavigationParams common_params;
  RequestNavigationParams request_params;
  common_params.url = GURL("data:text/html,world");
  common_params.navigation_type = FrameMsg_Navigate_Type::NORMAL;
  common_params.transition = ui::PAGE_TRANSITION_TYPED;
  request_params.current_history_list_length = 1;
  request_params.current_history_list_offset = 0;
  request_params.pending_history_list_offset = 1;
  request_params.page_id = -1;
  request_params.browser_navigation_start =
      base::TimeTicks::FromInternalValue(1);

  RenderFrameImpl* subframe = RenderFrameImpl::FromWebFrame(
      view()->webview()->findFrameByName("frame"));
  NavigateFrame(subframe, common_params, StartNavigationParams(),
                request_params);
  FrameLoadWaiter(subframe).Wait();

  // Copy the document content to std::wstring and compare with the
  // expected result.
  const int kMaxOutputCharacters = 256;
  std::string output = base::UTF16ToUTF8(
      GetMainFrame()->contentAsText(kMaxOutputCharacters));
  EXPECT_EQ(output, "hello \n\nworld");
}

// This test ensures that a RenderFrame object is created for the top level
// frame in the RenderView.
TEST_F(RenderViewImplTest, BasicRenderFrame) {
  EXPECT_TRUE(view()->main_render_frame_);
}

TEST_F(RenderViewImplTest, GetSSLStatusOfFrame) {
  LoadHTML("<!DOCTYPE html><html><body></body></html>");

  WebLocalFrame* frame = GetMainFrame();
  SSLStatus ssl_status = view()->GetSSLStatusOfFrame(frame);
  EXPECT_FALSE(net::IsCertStatusError(ssl_status.cert_status));

  const_cast<blink::WebURLResponse&>(frame->dataSource()->response()).
      setSecurityInfo(
          SerializeSecurityInfo(0, net::CERT_STATUS_ALL_ERRORS, 0, 0,
                                SignedCertificateTimestampIDStatusList()));
  ssl_status = view()->GetSSLStatusOfFrame(frame);
  EXPECT_TRUE(net::IsCertStatusError(ssl_status.cert_status));
}

TEST_F(RenderViewImplTest, MessageOrderInDidChangeSelection) {
  view()->OnSetInputMethodActive(true);
  view()->set_send_content_state_immediately(true);
  LoadHTML("<textarea id=\"test\"></textarea>");

  view()->handling_input_event_ = true;
  ExecuteJavaScript("document.getElementById('test').focus();");

  bool is_input_type_called = false;
  bool is_selection_called = false;
  size_t last_input_type = 0;
  size_t last_selection = 0;

  for (size_t i = 0; i < render_thread_->sink().message_count(); ++i) {
    const uint32 type = render_thread_->sink().GetMessageAt(i)->type();
    if (type == ViewHostMsg_TextInputTypeChanged::ID) {
      is_input_type_called = true;
      last_input_type = i;
    } else if (type == ViewHostMsg_SelectionChanged::ID) {
      is_selection_called = true;
      last_selection = i;
    }
  }

  EXPECT_TRUE(is_input_type_called);
  EXPECT_TRUE(is_selection_called);

  // InputTypeChange shold be called earlier than SelectionChanged.
  EXPECT_LT(last_input_type, last_selection);
}

class SuppressErrorPageTest : public RenderViewImplTest {
 public:
  ContentRendererClient* CreateContentRendererClient() override {
    return new TestContentRendererClient;
  }

  RenderViewImpl* view() {
    return static_cast<RenderViewImpl*>(view_);
  }

  RenderFrameImpl* frame() {
    return static_cast<RenderFrameImpl*>(view()->GetMainRenderFrame());
  }

 private:
  class TestContentRendererClient : public ContentRendererClient {
   public:
    bool ShouldSuppressErrorPage(RenderFrame* render_frame,
                                 const GURL& url) override {
      return url == GURL("http://example.com/suppress");
    }

    void GetNavigationErrorStrings(content::RenderView* render_view,
                                   blink::WebFrame* frame,
                                   const blink::WebURLRequest& failed_request,
                                   const blink::WebURLError& error,
                                   std::string* error_html,
                                   base::string16* error_description) override {
      if (error_html)
        *error_html = "A suffusion of yellow.";
    }
  };
};

#if defined(OS_ANDROID)
// Crashing on Android: http://crbug.com/311341
#define MAYBE_Suppresses DISABLED_Suppresses
#else
#define MAYBE_Suppresses Suppresses
#endif

TEST_F(SuppressErrorPageTest, MAYBE_Suppresses) {
  WebURLError error;
  error.domain = WebString::fromUTF8(net::kErrorDomain);
  error.reason = net::ERR_FILE_NOT_FOUND;
  error.unreachableURL = GURL("http://example.com/suppress");
  WebLocalFrame* web_frame = GetMainFrame();

  // Start a load that will reach provisional state synchronously,
  // but won't complete synchronously.
  CommonNavigationParams common_params;
  common_params.navigation_type = FrameMsg_Navigate_Type::NORMAL;
  common_params.url = GURL("data:text/html,test data");
  NavigateMainFrame(common_params, StartNavigationParams(),
                    RequestNavigationParams());

  // An error occurred.
  view()->GetMainRenderFrame()->didFailProvisionalLoad(
      web_frame, error, blink::WebStandardCommit);
  const int kMaxOutputCharacters = 22;
  EXPECT_EQ("",
            base::UTF16ToASCII(web_frame->contentAsText(kMaxOutputCharacters)));
}

#if defined(OS_ANDROID)
// Crashing on Android: http://crbug.com/311341
#define MAYBE_DoesNotSuppress DISABLED_DoesNotSuppress
#else
#define MAYBE_DoesNotSuppress DoesNotSuppress
#endif

TEST_F(SuppressErrorPageTest, MAYBE_DoesNotSuppress) {
  WebURLError error;
  error.domain = WebString::fromUTF8(net::kErrorDomain);
  error.reason = net::ERR_FILE_NOT_FOUND;
  error.unreachableURL = GURL("http://example.com/dont-suppress");
  WebLocalFrame* web_frame = GetMainFrame();

  // Start a load that will reach provisional state synchronously,
  // but won't complete synchronously.
  CommonNavigationParams common_params;
  common_params.navigation_type = FrameMsg_Navigate_Type::NORMAL;
  common_params.url = GURL("data:text/html,test data");
  NavigateMainFrame(common_params, StartNavigationParams(),
                    RequestNavigationParams());

  // An error occurred.
  view()->GetMainRenderFrame()->didFailProvisionalLoad(
      web_frame, error, blink::WebStandardCommit);
  // The error page itself is loaded asynchronously.
  FrameLoadWaiter(frame()).Wait();
  const int kMaxOutputCharacters = 22;
  EXPECT_EQ("A suffusion of yellow.",
            base::UTF16ToASCII(web_frame->contentAsText(kMaxOutputCharacters)));
}

// Tests if IME API's candidatewindow* events sent from browser are handled
// in renderer.
TEST_F(RenderViewImplTest, SendCandidateWindowEvents) {
  // Sends an HTML with an <input> element and scripts to the renderer.
  // The script handles all 3 of candidatewindow* events for an
  // InputMethodContext object and once it received 'show', 'update', 'hide'
  // should appear in the result div.
  LoadHTML("<input id='test'>"
           "<div id='result'>Result: </div>"
           "<script>"
           "window.onload = function() {"
           "  var result = document.getElementById('result');"
           "  var test = document.getElementById('test');"
           "  test.focus();"
           "  var context = test.inputMethodContext;"
           "  if (context) {"
           "    context.oncandidatewindowshow = function() {"
           "        result.innerText += 'show'; };"
           "    context.oncandidatewindowupdate = function(){"
           "        result.innerText += 'update'; };"
           "    context.oncandidatewindowhide = function(){"
           "        result.innerText += 'hide'; };"
           "  }"
           "};"
           "</script>");

  // Fire candidatewindow events.
  view()->OnCandidateWindowShown();
  view()->OnCandidateWindowUpdated();
  view()->OnCandidateWindowHidden();

  // Retrieve the content and check if it is expected.
  const int kMaxOutputCharacters = 50;
  std::string output = base::UTF16ToUTF8(
      GetMainFrame()->contentAsText(kMaxOutputCharacters));
  EXPECT_EQ(output, "\nResult:showupdatehide");
}

// Ensure the render view sends favicon url update events correctly.
TEST_F(RenderViewImplTest, SendFaviconURLUpdateEvent) {
  // An event should be sent when a favicon url exists.
  LoadHTML("<html>"
           "<head>"
           "<link rel='icon' href='http://www.google.com/favicon.ico'>"
           "</head>"
           "</html>");
  EXPECT_TRUE(render_thread_->sink().GetFirstMessageMatching(
      ViewHostMsg_UpdateFaviconURL::ID));
  render_thread_->sink().ClearMessages();

  // An event should not be sent if no favicon url exists. This is an assumption
  // made by some of Chrome's favicon handling.
  LoadHTML("<html>"
           "<head>"
           "</head>"
           "</html>");
  EXPECT_FALSE(render_thread_->sink().GetFirstMessageMatching(
      ViewHostMsg_UpdateFaviconURL::ID));
}

TEST_F(RenderViewImplTest, FocusElementCallsFocusedNodeChanged) {
  LoadHTML("<input id='test1' value='hello1'></input>"
           "<input id='test2' value='hello2'></input>");

  ExecuteJavaScript("document.getElementById('test1').focus();");
  const IPC::Message* msg1 = render_thread_->sink().GetFirstMessageMatching(
      ViewHostMsg_FocusedNodeChanged::ID);
  EXPECT_TRUE(msg1);

  ViewHostMsg_FocusedNodeChanged::Param params;
  ViewHostMsg_FocusedNodeChanged::Read(msg1, &params);
  EXPECT_TRUE(get<0>(params));
  render_thread_->sink().ClearMessages();

  ExecuteJavaScript("document.getElementById('test2').focus();");
  const IPC::Message* msg2 = render_thread_->sink().GetFirstMessageMatching(
        ViewHostMsg_FocusedNodeChanged::ID);
  EXPECT_TRUE(msg2);
  ViewHostMsg_FocusedNodeChanged::Read(msg2, &params);
  EXPECT_TRUE(get<0>(params));
  render_thread_->sink().ClearMessages();

  view()->webview()->clearFocusedElement();
  const IPC::Message* msg3 = render_thread_->sink().GetFirstMessageMatching(
        ViewHostMsg_FocusedNodeChanged::ID);
  EXPECT_TRUE(msg3);
  ViewHostMsg_FocusedNodeChanged::Read(msg3, &params);
  EXPECT_FALSE(get<0>(params));
  render_thread_->sink().ClearMessages();
}

TEST_F(RenderViewImplTest, ServiceWorkerNetworkProviderSetup) {
  ServiceWorkerNetworkProvider* provider = NULL;
  RequestExtraData* extra_data = NULL;

  // Make sure each new document has a new provider and
  // that the main request is tagged with the provider's id.
  LoadHTML("<b>A Document</b>");
  ASSERT_TRUE(GetMainFrame()->dataSource());
  provider = ServiceWorkerNetworkProvider::FromDocumentState(
      DocumentState::FromDataSource(GetMainFrame()->dataSource()));
  ASSERT_TRUE(provider);
  extra_data = static_cast<RequestExtraData*>(
      GetMainFrame()->dataSource()->request().extraData());
  ASSERT_TRUE(extra_data);
  EXPECT_EQ(extra_data->service_worker_provider_id(),
            provider->provider_id());
  int provider1_id = provider->provider_id();

  LoadHTML("<b>New Document B Goes Here</b>");
  ASSERT_TRUE(GetMainFrame()->dataSource());
  provider = ServiceWorkerNetworkProvider::FromDocumentState(
      DocumentState::FromDataSource(GetMainFrame()->dataSource()));
  ASSERT_TRUE(provider);
  EXPECT_NE(provider1_id, provider->provider_id());
  extra_data = static_cast<RequestExtraData*>(
      GetMainFrame()->dataSource()->request().extraData());
  ASSERT_TRUE(extra_data);
  EXPECT_EQ(extra_data->service_worker_provider_id(),
            provider->provider_id());

  // See that subresource requests are also tagged with the provider's id.
  EXPECT_EQ(frame(), RenderFrameImpl::FromWebFrame(GetMainFrame()));
  blink::WebURLRequest request(GURL("http://foo.com"));
  request.setRequestContext(blink::WebURLRequest::RequestContextSubresource);
  blink::WebURLResponse redirect_response;
  frame()->willSendRequest(GetMainFrame(), 0, request, redirect_response);
  extra_data = static_cast<RequestExtraData*>(request.extraData());
  ASSERT_TRUE(extra_data);
  EXPECT_EQ(extra_data->service_worker_provider_id(),
            provider->provider_id());
}

TEST_F(RenderViewImplTest, OnSetAccessibilityMode) {
  ASSERT_EQ(AccessibilityModeOff, frame()->accessibility_mode());
  ASSERT_EQ((RendererAccessibility*) NULL, frame()->renderer_accessibility());

  SetAccessibilityMode(AccessibilityModeTreeOnly);
  ASSERT_EQ(AccessibilityModeTreeOnly, frame()->accessibility_mode());
  ASSERT_NE((RendererAccessibility*) NULL, frame()->renderer_accessibility());

  SetAccessibilityMode(AccessibilityModeOff);
  ASSERT_EQ(AccessibilityModeOff, frame()->accessibility_mode());
  ASSERT_EQ((RendererAccessibility*) NULL, frame()->renderer_accessibility());

  SetAccessibilityMode(AccessibilityModeComplete);
  ASSERT_EQ(AccessibilityModeComplete, frame()->accessibility_mode());
  ASSERT_NE((RendererAccessibility*) NULL, frame()->renderer_accessibility());
}

TEST_F(RenderViewImplTest, ScreenMetricsEmulation) {
  LoadHTML("<body style='min-height:1000px;'></body>");

  blink::WebDeviceEmulationParams params;
  base::string16 get_width = base::ASCIIToUTF16("Number(window.innerWidth)");
  base::string16 get_height = base::ASCIIToUTF16("Number(window.innerHeight)");
  int width, height;

  params.viewSize.width = 327;
  params.viewSize.height = 415;
  view()->OnEnableDeviceEmulation(params);
  EXPECT_TRUE(ExecuteJavaScriptAndReturnIntValue(get_width, &width));
  EXPECT_EQ(params.viewSize.width, width);
  EXPECT_TRUE(ExecuteJavaScriptAndReturnIntValue(get_height, &height));
  EXPECT_EQ(params.viewSize.height, height);

  params.viewSize.width = 1005;
  params.viewSize.height = 1102;
  view()->OnEnableDeviceEmulation(params);
  EXPECT_TRUE(ExecuteJavaScriptAndReturnIntValue(get_width, &width));
  EXPECT_EQ(params.viewSize.width, width);
  EXPECT_TRUE(ExecuteJavaScriptAndReturnIntValue(get_height, &height));
  EXPECT_EQ(params.viewSize.height, height);

  view()->OnDisableDeviceEmulation();

  view()->OnEnableDeviceEmulation(params);
  // Don't disable here to test that emulation is being shutdown properly.
}

// Sanity checks for the Navigation Timing API |navigationStart| override. We
// are asserting only most basic constraints, as TimeTicks (passed as the
// override) are not comparable with the wall time (returned by the Blink API).
TEST_F(RenderViewImplTest, NavigationStartOverride) {
  // Verify that a navigation that claims to have started at the earliest
  // possible TimeTicks is indeed reported as one that started before
  // OnNavigate() is called.
  base::Time before_navigation = base::Time::Now();
  CommonNavigationParams early_common_params;
  StartNavigationParams early_start_params;
  RequestNavigationParams early_request_params;
  early_common_params.url = GURL("data:text/html,<div>Page</div>");
  early_common_params.navigation_type = FrameMsg_Navigate_Type::NORMAL;
  early_common_params.transition = ui::PAGE_TRANSITION_TYPED;
  early_start_params.is_post = true;
  early_request_params.browser_navigation_start =
      base::TimeTicks::FromInternalValue(1);

  NavigateMainFrame(early_common_params, early_start_params,
                    early_request_params);
  ProcessPendingMessages();

  base::Time early_nav_reported_start =
      base::Time::FromDoubleT(GetMainFrame()->performance().navigationStart());
  EXPECT_LT(early_nav_reported_start, before_navigation);

  // Verify that a navigation that claims to have started in the future - 42
  // days from now is *not* reported as one that starts in the future; as we
  // sanitize the override allowing a maximum of ::Now().
  CommonNavigationParams late_common_params;
  RequestNavigationParams late_request_params;
  StartNavigationParams late_start_params;
  late_common_params.url = GURL("data:text/html,<div>Another page</div>");
  late_common_params.navigation_type = FrameMsg_Navigate_Type::NORMAL;
  late_common_params.transition = ui::PAGE_TRANSITION_TYPED;
  late_start_params.is_post = true;
  late_request_params.browser_navigation_start =
      base::TimeTicks::Now() + base::TimeDelta::FromDays(42);

  NavigateMainFrame(late_common_params, late_start_params, late_request_params);
  ProcessPendingMessages();
  base::Time after_navigation =
      base::Time::Now() + base::TimeDelta::FromDays(1);

  base::Time late_nav_reported_start =
      base::Time::FromDoubleT(GetMainFrame()->performance().navigationStart());
  EXPECT_LE(late_nav_reported_start, after_navigation);
}

TEST_F(RenderViewImplTest, PreferredSizeZoomed) {
  LoadHTML("<body style='margin:0;'><div style='display:inline-block; "
           "width:400px; height:400px;'/></body>");
  view()->webview()->mainFrame()->setCanHaveScrollbars(false);
  EnablePreferredSizeMode();

  gfx::Size size = GetPreferredSize();
  EXPECT_EQ(gfx::Size(400, 400), size);

  SetZoomLevel(ZoomFactorToZoomLevel(2.0));
  size = GetPreferredSize();
  EXPECT_EQ(gfx::Size(800, 800), size);
}

// Ensure the RenderViewImpl history list is properly updated when starting a
// new browser-initiated navigation.
TEST_F(RenderViewImplTest, HistoryIsProperlyUpdatedOnNavigation) {
  EXPECT_EQ(0, view()->historyBackListCount());
  EXPECT_EQ(0, view()->historyBackListCount() +
      view()->historyForwardListCount() + 1);

  // Receive a Navigate message with history parameters.
  RequestNavigationParams request_params;
  request_params.current_history_list_length = 2;
  request_params.current_history_list_offset = 1;
  request_params.pending_history_list_offset = 2;
  request_params.page_id = -1;
  NavigateMainFrame(CommonNavigationParams(), StartNavigationParams(),
                    request_params);

  // The history list in RenderView should have been updated.
  EXPECT_EQ(1, view()->historyBackListCount());
  EXPECT_EQ(2, view()->historyBackListCount() +
      view()->historyForwardListCount() + 1);
}

TEST_F(DevToolsAgentTest, DevToolsResumeOnClose) {
  Attach();
  EXPECT_FALSE(IsPaused());
  DispatchDevToolsMessage("{\"id\":1,\"method\":\"Debugger.enable\"}");

  // Executing javascript will pause the thread and create nested message loop.
  // Posting task simulates message coming from browser.
  base::MessageLoop::current()->PostTask(FROM_HERE, base::Bind(
      &DevToolsAgentTest::CloseWhilePaused, base::Unretained(this)));
  ExecuteJavaScript("debugger;");

  // CloseWhilePaused should resume execution and continue here.
  EXPECT_FALSE(IsPaused());
  Detach();
}

}  // namespace content
