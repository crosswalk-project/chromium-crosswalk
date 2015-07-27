// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/command_line.h"
#include "base/strings/stringprintf.h"
#include "content/browser/frame_host/frame_navigation_entry.h"
#include "content/browser/frame_host/frame_tree.h"
#include "content/browser/frame_host/navigation_controller_impl.h"
#include "content/browser/frame_host/navigation_entry_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/resource_controller.h"
#include "content/public/browser/resource_dispatcher_host.h"
#include "content/public/browser/resource_dispatcher_host_delegate.h"
#include "content/public/browser/resource_throttle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/bindings_policy.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "content/shell/browser/shell.h"
#include "content/test/content_browser_test_utils_internal.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/url_request/url_request_failed_job.h"

namespace content {

class NavigationControllerBrowserTest : public ContentBrowserTest {
 protected:
  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());
  }
};

IN_PROC_BROWSER_TEST_F(NavigationControllerBrowserTest, LoadDataWithBaseURL) {
  const GURL base_url("http://baseurl");
  const GURL history_url("http://historyurl");
  const std::string data = "<html><body>foo</body></html>";

  const NavigationController& controller =
      shell()->web_contents()->GetController();
  // Load data. Blocks until it is done.
  content::LoadDataWithBaseURL(shell(), history_url, data, base_url);

  // We should use history_url instead of the base_url as the original url of
  // this navigation entry, because base_url is only used for resolving relative
  // paths in the data, or enforcing same origin policy.
  EXPECT_EQ(controller.GetVisibleEntry()->GetOriginalRequestURL(), history_url);
}

IN_PROC_BROWSER_TEST_F(NavigationControllerBrowserTest, UniqueIDs) {
  const NavigationControllerImpl& controller =
      static_cast<const NavigationControllerImpl&>(
          shell()->web_contents()->GetController());

  GURL main_url(embedded_test_server()->GetURL(
      "/navigation_controller/page_with_link_to_load_iframe.html"));
  NavigateToURL(shell(), main_url);
  ASSERT_EQ(1, controller.GetEntryCount());

  // Use JavaScript to click the link and load the iframe.
  std::string script = "document.getElementById('link').click()";
  EXPECT_TRUE(content::ExecuteScript(shell()->web_contents(), script));
  EXPECT_TRUE(WaitForLoadStop(shell()->web_contents()));
  ASSERT_EQ(2, controller.GetEntryCount());

  // Unique IDs should... um... be unique.
  ASSERT_NE(controller.GetEntryAtIndex(0)->GetUniqueID(),
            controller.GetEntryAtIndex(1)->GetUniqueID());
}

// This test used to make sure that a scheme used to prevent spoofs didn't ever
// interfere with navigations. We switched to a different scheme, so now this is
// just a test to make sure we can still navigate once we prune the history
// list.
IN_PROC_BROWSER_TEST_F(NavigationControllerBrowserTest,
                       DontIgnoreBackAfterNavEntryLimit) {
  NavigationController& controller =
      shell()->web_contents()->GetController();

  const int kMaxEntryCount =
      static_cast<int>(NavigationControllerImpl::max_entry_count());

  // Load up to the max count, all entries should be there.
  for (int url_index = 0; url_index < kMaxEntryCount; ++url_index) {
    GURL url(base::StringPrintf("data:text/html,page%d", url_index));
    EXPECT_TRUE(NavigateToURL(shell(), url));
  }

  EXPECT_EQ(controller.GetEntryCount(), kMaxEntryCount);

  // Navigate twice more more.
  for (int url_index = kMaxEntryCount;
       url_index < kMaxEntryCount + 2; ++url_index) {
    GURL url(base::StringPrintf("data:text/html,page%d", url_index));
    EXPECT_TRUE(NavigateToURL(shell(), url));
  }

  // We expect page0 and page1 to be gone.
  EXPECT_EQ(kMaxEntryCount, controller.GetEntryCount());
  EXPECT_EQ(GURL("data:text/html,page2"),
            controller.GetEntryAtIndex(0)->GetURL());

  // Now try to go back. This should not hang.
  ASSERT_TRUE(controller.CanGoBack());
  controller.GoBack();
  EXPECT_TRUE(WaitForLoadStop(shell()->web_contents()));

  // This should have successfully gone back.
  EXPECT_EQ(GURL(base::StringPrintf("data:text/html,page%d", kMaxEntryCount)),
            controller.GetLastCommittedEntry()->GetURL());
}

namespace {

int RendererHistoryLength(Shell* shell) {
  int value = 0;
  EXPECT_TRUE(ExecuteScriptAndExtractInt(
      shell->web_contents(),
      "domAutomationController.send(history.length)",
      &value));
  return value;
}

// Similar to the ones from content_browser_test_utils.
bool NavigateToURLAndReplace(Shell* shell, const GURL& url) {
  WebContents* web_contents = shell->web_contents();
  WaitForLoadStop(web_contents);
  TestNavigationObserver same_tab_observer(web_contents, 1);
  NavigationController::LoadURLParams params(url);
  params.should_replace_current_entry = true;
  web_contents->GetController().LoadURLWithParams(params);
  web_contents->Focus();
  same_tab_observer.Wait();
  if (!IsLastCommittedEntryOfPageType(web_contents, PAGE_TYPE_NORMAL))
    return false;
  return web_contents->GetLastCommittedURL() == url;
}

}  // namespace

// When loading a new page to replace an old page in the history list, make sure
// that the browser and renderer agree, and that both get it right.
IN_PROC_BROWSER_TEST_F(NavigationControllerBrowserTest,
                       CorrectLengthWithCurrentItemReplacement) {
  NavigationController& controller =
      shell()->web_contents()->GetController();

  EXPECT_TRUE(NavigateToURL(shell(), GURL("data:text/html,page1")));
  EXPECT_EQ(1, controller.GetEntryCount());
  EXPECT_EQ(1, RendererHistoryLength(shell()));

  EXPECT_TRUE(NavigateToURLAndReplace(shell(), GURL("data:text/html,page2")));
  EXPECT_EQ(1, controller.GetEntryCount());
  EXPECT_EQ(1, RendererHistoryLength(shell()));

  // Note that there's no way to access the renderer's notion of the history
  // offset via JavaScript. Checking just the history length, though, is enough;
  // if the replacement failed, there would be a new history entry and thus an
  // incorrect length.
}

// When spawning a new page from a WebUI page, make sure that the browser and
// renderer agree about the length of the history list, and that both get it
// right.
IN_PROC_BROWSER_TEST_F(NavigationControllerBrowserTest,
                       CorrectLengthWithNewTabNavigatingFromWebUI) {
  GURL web_ui_page(std::string(kChromeUIScheme) + "://" +
                   std::string(kChromeUIGpuHost));
  EXPECT_TRUE(NavigateToURL(shell(), web_ui_page));
  EXPECT_EQ(BINDINGS_POLICY_WEB_UI,
      shell()->web_contents()->GetRenderViewHost()->GetEnabledBindings());

  ShellAddedObserver observer;
  std::string page_url = embedded_test_server()->GetURL(
      "/navigation_controller/simple_page_1.html").spec();
  EXPECT_TRUE(ExecuteScript(shell()->web_contents(),
                            "window.open('" + page_url + "', '_blank')"));
  Shell* shell2 = observer.GetShell();
  EXPECT_TRUE(WaitForLoadStop(shell2->web_contents()));

  EXPECT_EQ(1, shell2->web_contents()->GetController().GetEntryCount());
  EXPECT_EQ(1, RendererHistoryLength(shell2));

  // Again, as above, there's no way to access the renderer's notion of the
  // history offset via JavaScript. Checking just the history length, again,
  // will have to suffice.
}

namespace {

class NoNavigationsObserver : public WebContentsObserver {
 public:
  // Observes navigation for the specified |web_contents|.
  explicit NoNavigationsObserver(WebContents* web_contents)
      : WebContentsObserver(web_contents) {}

 private:
  void DidNavigateAnyFrame(RenderFrameHost* render_frame_host,
                           const LoadCommittedDetails& details,
                           const FrameNavigateParams& params) override {
    FAIL() << "No navigations should occur";
  }
};

}  // namespace

// Some pages create a popup, then write an iframe into it. This causes a
// subframe navigation without having any committed entry. Such navigations
// just get thrown on the ground, but we shouldn't crash.
//
// This test actually hits NAVIGATION_TYPE_NAV_IGNORE three times. Two of them,
// the initial window.open() and the iframe creation, don't try to create
// navigation entries, and the third, the new navigation, tries to.
IN_PROC_BROWSER_TEST_F(NavigationControllerBrowserTest, SubframeOnEmptyPage) {
  NavigateToURL(shell(), GURL(url::kAboutBlankURL));
  EXPECT_TRUE(WaitForLoadStop(shell()->web_contents()));

  FrameTreeNode* root =
      static_cast<WebContentsImpl*>(shell()->web_contents())->
          GetFrameTree()->root();

  // Pop open a new window.
  ShellAddedObserver new_shell_observer;
  std::string script = "window.open()";
  EXPECT_TRUE(content::ExecuteScript(root->current_frame_host(), script));
  Shell* new_shell = new_shell_observer.GetShell();
  ASSERT_NE(new_shell->web_contents(), shell()->web_contents());
  FrameTreeNode* new_root =
      static_cast<WebContentsImpl*>(new_shell->web_contents())->
          GetFrameTree()->root();

  // Make a new iframe in it.
  NoNavigationsObserver observer(new_shell->web_contents());
  script = "var iframe = document.createElement('iframe');"
           "iframe.src = 'data:text/html,<p>some page</p>';"
           "document.body.appendChild(iframe);";
  EXPECT_TRUE(content::ExecuteScript(new_root->current_frame_host(), script));
  // The success check is of the last-committed entry, and there is none.
  WaitForLoadStopWithoutSuccessCheck(new_shell->web_contents());

  ASSERT_EQ(1U, new_root->child_count());
  ASSERT_NE(nullptr, new_root->child_at(0));

  // Navigate it.
  GURL frame_url = embedded_test_server()->GetURL(
      "/navigation_controller/simple_page_2.html");
  script = "location.assign('" + frame_url.spec() + "')";
  EXPECT_TRUE(content::ExecuteScript(
      new_root->child_at(0)->current_frame_host(), script));

  // Success is not crashing, and not navigating.
  EXPECT_EQ(nullptr,
            new_shell->web_contents()->GetController().GetLastCommittedEntry());
}

namespace {

class FrameNavigateParamsCapturer : public WebContentsObserver {
 public:
  // Observes navigation for the specified |node|.
  explicit FrameNavigateParamsCapturer(FrameTreeNode* node)
      : WebContentsObserver(
            node->current_frame_host()->delegate()->GetAsWebContents()),
        frame_tree_node_id_(node->frame_tree_node_id()),
        navigations_remaining_(1),
        wait_for_load_(true),
        message_loop_runner_(new MessageLoopRunner) {}

  void set_navigations_remaining(int count) {
    navigations_remaining_ = count;
  }

  void set_wait_for_load(bool ignore) {
    wait_for_load_ = ignore;
  }

  void Wait() {
    message_loop_runner_->Run();
  }

  const FrameNavigateParams& params() const {
    EXPECT_EQ(1U, params_.size());
    return params_[0];
  }

  const std::vector<FrameNavigateParams>& all_params() const {
    return params_;
  }

  const LoadCommittedDetails& details() const {
    EXPECT_EQ(1U, details_.size());
    return details_[0];
  }

  const std::vector<LoadCommittedDetails>& all_details() const {
    return details_;
  }

 private:
  void DidNavigateAnyFrame(RenderFrameHost* render_frame_host,
                           const LoadCommittedDetails& details,
                           const FrameNavigateParams& params) override {
    RenderFrameHostImpl* rfh =
        static_cast<RenderFrameHostImpl*>(render_frame_host);
    if (rfh->frame_tree_node()->frame_tree_node_id() != frame_tree_node_id_)
      return;

    --navigations_remaining_;
    params_.push_back(params);
    details_.push_back(details);
    if (!navigations_remaining_ &&
        (!web_contents()->IsLoading() || !wait_for_load_))
      message_loop_runner_->Quit();
  }

  void DidStopLoading() override {
    if (!navigations_remaining_)
      message_loop_runner_->Quit();
  }

  // The id of the FrameTreeNode whose navigations to observe.
  int frame_tree_node_id_;

  // How many navigations remain to capture.
  int navigations_remaining_;

  // Whether to also wait for the load to complete.
  bool wait_for_load_;

  // The params of the navigations.
  std::vector<FrameNavigateParams> params_;

  // The details of the navigations.
  std::vector<LoadCommittedDetails> details_;

  // The MessageLoopRunner used to spin the message loop.
  scoped_refptr<MessageLoopRunner> message_loop_runner_;
};

class LoadCommittedCapturer : public WebContentsObserver {
 public:
  // Observes the load commit for the specified |node|.
  explicit LoadCommittedCapturer(FrameTreeNode* node)
      : WebContentsObserver(
            node->current_frame_host()->delegate()->GetAsWebContents()),
        frame_tree_node_id_(node->frame_tree_node_id()),
        message_loop_runner_(new MessageLoopRunner) {}

  // Observes the load commit for the next created frame in the specified
  // |web_contents|.
  explicit LoadCommittedCapturer(WebContents* web_contents)
      : WebContentsObserver(web_contents),
        frame_tree_node_id_(0),
        message_loop_runner_(new MessageLoopRunner) {}

  void Wait() {
    message_loop_runner_->Run();
  }

  ui::PageTransition transition_type() const {
    return transition_type_;
  }

 private:
  void RenderFrameCreated(RenderFrameHost* render_frame_host) override {
    // If this object was created with a specified tree frame node, there
    // shouldn't be any frames being created.
    DCHECK_EQ(0, frame_tree_node_id_);
    RenderFrameHostImpl* rfh =
        static_cast<RenderFrameHostImpl*>(render_frame_host);
    frame_tree_node_id_ = rfh->frame_tree_node()->frame_tree_node_id();
  }

  void DidCommitProvisionalLoadForFrame(
      RenderFrameHost* render_frame_host,
      const GURL& url,
      ui::PageTransition transition_type) override {
    DCHECK_NE(0, frame_tree_node_id_);
    RenderFrameHostImpl* rfh =
        static_cast<RenderFrameHostImpl*>(render_frame_host);
    if (rfh->frame_tree_node()->frame_tree_node_id() != frame_tree_node_id_)
      return;

    transition_type_ = transition_type;
    if (!web_contents()->IsLoading())
      message_loop_runner_->Quit();
  }

  void DidStopLoading() override { message_loop_runner_->Quit(); }

  // The id of the FrameTreeNode whose navigations to observe.
  int frame_tree_node_id_;

  // The transition_type of the last navigation.
  ui::PageTransition transition_type_;

  // The MessageLoopRunner used to spin the message loop.
  scoped_refptr<MessageLoopRunner> message_loop_runner_;
};

}  // namespace

IN_PROC_BROWSER_TEST_F(NavigationControllerBrowserTest,
                       ErrorPageReplacement) {
  NavigationController& controller = shell()->web_contents()->GetController();
  GURL error_url(
      net::URLRequestFailedJob::GetMockHttpUrl(net::ERR_CONNECTION_RESET));
  net::URLRequestFailedJob::AddUrlHandler();

  NavigateToURL(shell(), GURL(url::kAboutBlankURL));
  EXPECT_EQ(1, controller.GetEntryCount());

  FrameTreeNode* root =
      static_cast<WebContentsImpl*>(shell()->web_contents())->
          GetFrameTree()->root();

  // Navigate to a page that fails to load. It must result in an error page, the
  // NEW_PAGE navigation type, and an addition to the history list.
  {
    FrameNavigateParamsCapturer capturer(root);
    NavigateFrameToURL(root, error_url);
    capturer.Wait();
    EXPECT_EQ(NAVIGATION_TYPE_NEW_PAGE, capturer.details().type);
    NavigationEntry* entry = controller.GetLastCommittedEntry();
    EXPECT_EQ(PAGE_TYPE_ERROR, entry->GetPageType());
    EXPECT_EQ(2, controller.GetEntryCount());
  }

  // Navigate again to the page that fails to load. It must result in an error
  // page, the EXISTING_PAGE navigation type, and no addition to the history
  // list. We do not use SAME_PAGE here; that case only differs in that it
  // clears the pending entry, and there is no pending entry after a load
  // failure.
  {
    FrameNavigateParamsCapturer capturer(root);
    NavigateFrameToURL(root, error_url);
    capturer.Wait();
    EXPECT_EQ(NAVIGATION_TYPE_EXISTING_PAGE, capturer.details().type);
    NavigationEntry* entry = controller.GetLastCommittedEntry();
    EXPECT_EQ(PAGE_TYPE_ERROR, entry->GetPageType());
    EXPECT_EQ(2, controller.GetEntryCount());
  }

  // Make a new entry ...
  NavigateToURL(shell(), GURL(url::kAboutBlankURL));
  EXPECT_EQ(3, controller.GetEntryCount());

  // ... and replace it with a failed load. (Note that when you set the
  // should_replace_current_entry flag, the navigation is classified as NEW_PAGE
  // because that is a classification of the renderer's behavior, and the flag
  // is a browser-side flag.)
  {
    FrameNavigateParamsCapturer capturer(root);
    NavigateToURLAndReplace(shell(), error_url);
    capturer.Wait();
    EXPECT_EQ(NAVIGATION_TYPE_NEW_PAGE, capturer.details().type);
    NavigationEntry* entry = controller.GetLastCommittedEntry();
    EXPECT_EQ(PAGE_TYPE_ERROR, entry->GetPageType());
    EXPECT_EQ(3, controller.GetEntryCount());
  }

  // Make a new web ui page to force a process swap ...
  GURL web_ui_page(std::string(kChromeUIScheme) + "://" +
                   std::string(kChromeUIGpuHost));
  NavigateToURL(shell(), web_ui_page);
  EXPECT_EQ(4, controller.GetEntryCount());

  // ... and replace it with a failed load. (It is NEW_PAGE for the reason noted
  // above.)
  {
    FrameNavigateParamsCapturer capturer(root);
    NavigateToURLAndReplace(shell(), error_url);
    capturer.Wait();
    EXPECT_EQ(NAVIGATION_TYPE_NEW_PAGE, capturer.details().type);
    NavigationEntry* entry = controller.GetLastCommittedEntry();
    EXPECT_EQ(PAGE_TYPE_ERROR, entry->GetPageType());
    EXPECT_EQ(4, controller.GetEntryCount());
  }
}

// Various tests for navigation type classifications. TODO(avi): It's rather
// bogus that the same info is in two different enums; http://crbug.com/453555.

// Verify that navigations for NAVIGATION_TYPE_NEW_PAGE are correctly
// classified.
IN_PROC_BROWSER_TEST_F(NavigationControllerBrowserTest,
                       NavigationTypeClassification_NewPage) {
  NavigateToURL(shell(), GURL(url::kAboutBlankURL));

  FrameTreeNode* root =
      static_cast<WebContentsImpl*>(shell()->web_contents())->
          GetFrameTree()->root();

  {
    // Simple load.
    FrameNavigateParamsCapturer capturer(root);
    GURL frame_url(embedded_test_server()->GetURL(
        "/navigation_controller/page_with_links.html"));
    NavigateFrameToURL(root, frame_url);
    capturer.Wait();
    // TODO(avi,creis): Why is this (and quite a few others below) a "link"
    // transition? Lots of these transitions should be cleaned up.
    EXPECT_EQ(ui::PAGE_TRANSITION_LINK, capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_NEW_PAGE, capturer.details().type);
  }

  {
    // Load via a fragment link click.
    FrameNavigateParamsCapturer capturer(root);
    std::string script = "document.getElementById('fraglink').click()";
    EXPECT_TRUE(content::ExecuteScript(root->current_frame_host(), script));
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_LINK, capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_NEW_PAGE, capturer.details().type);
  }

  {
    // Load via link click.
    FrameNavigateParamsCapturer capturer(root);
    std::string script = "document.getElementById('thelink').click()";
    EXPECT_TRUE(content::ExecuteScript(root->current_frame_host(), script));
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_LINK, capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_NEW_PAGE, capturer.details().type);
  }

  {
    // location.assign().
    FrameNavigateParamsCapturer capturer(root);
    GURL frame_url(embedded_test_server()->GetURL(
        "/navigation_controller/simple_page_2.html"));
    std::string script = "location.assign('" + frame_url.spec() + "')";
    EXPECT_TRUE(content::ExecuteScript(root->current_frame_host(), script));
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_LINK | ui::PAGE_TRANSITION_CLIENT_REDIRECT,
              capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_NEW_PAGE, capturer.details().type);
  }

  {
    // history.pushState().
    FrameNavigateParamsCapturer capturer(root);
    std::string script =
        "history.pushState({}, 'page 1', 'simple_page_1.html')";
    EXPECT_TRUE(content::ExecuteScript(root->current_frame_host(), script));
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_LINK | ui::PAGE_TRANSITION_CLIENT_REDIRECT,
              capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_NEW_PAGE, capturer.details().type);
  }
}

// Verify that navigations for NAVIGATION_TYPE_EXISTING_PAGE are correctly
// classified.
IN_PROC_BROWSER_TEST_F(NavigationControllerBrowserTest,
                       NavigationTypeClassification_ExistingPage) {
  GURL url1(embedded_test_server()->GetURL(
      "/navigation_controller/simple_page_1.html"));
  NavigateToURL(shell(), url1);
  GURL url2(embedded_test_server()->GetURL(
      "/navigation_controller/simple_page_2.html"));
  NavigateToURL(shell(), url2);

  FrameTreeNode* root =
      static_cast<WebContentsImpl*>(shell()->web_contents())->
          GetFrameTree()->root();

  {
    // Back from the browser side.
    FrameNavigateParamsCapturer capturer(root);
    shell()->web_contents()->GetController().GoBack();
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_TYPED
              | ui::PAGE_TRANSITION_FORWARD_BACK
              | ui::PAGE_TRANSITION_FROM_ADDRESS_BAR,
              capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_EXISTING_PAGE, capturer.details().type);
  }

  {
    // Forward from the browser side.
    FrameNavigateParamsCapturer capturer(root);
    shell()->web_contents()->GetController().GoForward();
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_TYPED
              | ui::PAGE_TRANSITION_FORWARD_BACK
              | ui::PAGE_TRANSITION_FROM_ADDRESS_BAR,
              capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_EXISTING_PAGE, capturer.details().type);
  }

  {
    // Back from the renderer side.
    FrameNavigateParamsCapturer capturer(root);
    EXPECT_TRUE(content::ExecuteScript(root->current_frame_host(),
                                       "history.back()"));
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_TYPED
              | ui::PAGE_TRANSITION_FORWARD_BACK
              | ui::PAGE_TRANSITION_FROM_ADDRESS_BAR,
              capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_EXISTING_PAGE, capturer.details().type);
  }

  {
    // Forward from the renderer side.
    FrameNavigateParamsCapturer capturer(root);
    EXPECT_TRUE(content::ExecuteScript(root->current_frame_host(),
                                       "history.forward()"));
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_TYPED
              | ui::PAGE_TRANSITION_FORWARD_BACK
              | ui::PAGE_TRANSITION_FROM_ADDRESS_BAR,
              capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_EXISTING_PAGE, capturer.details().type);
  }

  {
    // Back from the renderer side via history.go().
    FrameNavigateParamsCapturer capturer(root);
    EXPECT_TRUE(content::ExecuteScript(root->current_frame_host(),
                                       "history.go(-1)"));
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_TYPED
              | ui::PAGE_TRANSITION_FORWARD_BACK
              | ui::PAGE_TRANSITION_FROM_ADDRESS_BAR,
              capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_EXISTING_PAGE, capturer.details().type);
  }

  {
    // Forward from the renderer side via history.go().
    FrameNavigateParamsCapturer capturer(root);
    EXPECT_TRUE(content::ExecuteScript(root->current_frame_host(),
                                       "history.go(1)"));
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_TYPED
              | ui::PAGE_TRANSITION_FORWARD_BACK
              | ui::PAGE_TRANSITION_FROM_ADDRESS_BAR,
              capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_EXISTING_PAGE, capturer.details().type);
  }

  {
    // Reload from the browser side.
    FrameNavigateParamsCapturer capturer(root);
    shell()->web_contents()->GetController().Reload(false);
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_RELOAD, capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_EXISTING_PAGE, capturer.details().type);
  }

  {
    // Reload from the renderer side.
    FrameNavigateParamsCapturer capturer(root);
    EXPECT_TRUE(content::ExecuteScript(root->current_frame_host(),
                                       "location.reload()"));
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_LINK | ui::PAGE_TRANSITION_CLIENT_REDIRECT,
              capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_EXISTING_PAGE, capturer.details().type);
  }

  {
    // location.replace().
    FrameNavigateParamsCapturer capturer(root);
    GURL frame_url(embedded_test_server()->GetURL(
        "/navigation_controller/simple_page_1.html"));
    std::string script = "location.replace('" + frame_url.spec() + "')";
    EXPECT_TRUE(content::ExecuteScript(root->current_frame_host(), script));
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_LINK | ui::PAGE_TRANSITION_CLIENT_REDIRECT,
              capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_EXISTING_PAGE, capturer.details().type);
  }
}

// Verify that navigations for NAVIGATION_TYPE_SAME_PAGE are correctly
// classified.
IN_PROC_BROWSER_TEST_F(NavigationControllerBrowserTest,
                       NavigationTypeClassification_SamePage) {
  GURL url1(embedded_test_server()->GetURL(
      "/navigation_controller/simple_page_1.html"));
  NavigateToURL(shell(), url1);

  FrameTreeNode* root =
      static_cast<WebContentsImpl*>(shell()->web_contents())->
          GetFrameTree()->root();

  {
    // Simple load.
    FrameNavigateParamsCapturer capturer(root);
    GURL frame_url(embedded_test_server()->GetURL(
        "/navigation_controller/simple_page_1.html"));
    NavigateFrameToURL(root, frame_url);
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_LINK, capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_SAME_PAGE, capturer.details().type);
  }
}

// Verify that navigations for NAVIGATION_TYPE_IN_PAGE are correctly
// classified.
IN_PROC_BROWSER_TEST_F(NavigationControllerBrowserTest,
                       NavigationTypeClassification_InPage) {
  GURL url1(embedded_test_server()->GetURL(
      "/navigation_controller/simple_page_1.html"));
  NavigateToURL(shell(), url1);

  FrameTreeNode* root =
      static_cast<WebContentsImpl*>(shell()->web_contents())->
          GetFrameTree()->root();

  {
    // history.replaceState().
    FrameNavigateParamsCapturer capturer(root);
    std::string script =
        "history.replaceState({}, 'page 1', 'simple_page_2.html')";
    EXPECT_TRUE(content::ExecuteScript(root->current_frame_host(), script));
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_LINK, capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_IN_PAGE, capturer.details().type);
  }

  // Back and forward across a fragment navigation.

  GURL url2(embedded_test_server()->GetURL(
      "/navigation_controller/page_with_links.html"));
  NavigateToURL(shell(), url2);
  std::string script = "document.getElementById('fraglink').click()";
  EXPECT_TRUE(content::ExecuteScript(root->current_frame_host(), script));
  EXPECT_TRUE(WaitForLoadStop(shell()->web_contents()));

  {
    // Back.
    FrameNavigateParamsCapturer capturer(root);
    shell()->web_contents()->GetController().GoBack();
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_TYPED
              | ui::PAGE_TRANSITION_FORWARD_BACK
              | ui::PAGE_TRANSITION_FROM_ADDRESS_BAR,
              capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_IN_PAGE, capturer.details().type);
  }

  {
    // Forward.
    FrameNavigateParamsCapturer capturer(root);
    shell()->web_contents()->GetController().GoForward();
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_LINK | ui::PAGE_TRANSITION_FORWARD_BACK,
              capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_IN_PAGE, capturer.details().type);
  }

  // Back and forward across a pushState-created navigation.

  NavigateToURL(shell(), url1);
  script = "history.pushState({}, 'page 2', 'simple_page_2.html')";
  EXPECT_TRUE(content::ExecuteScript(root->current_frame_host(), script));
  EXPECT_TRUE(WaitForLoadStop(shell()->web_contents()));

  {
    // Back.
    FrameNavigateParamsCapturer capturer(root);
    shell()->web_contents()->GetController().GoBack();
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_TYPED
              | ui::PAGE_TRANSITION_FORWARD_BACK
              | ui::PAGE_TRANSITION_FROM_ADDRESS_BAR,
              capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_IN_PAGE, capturer.details().type);
  }

  {
    // Forward.
    FrameNavigateParamsCapturer capturer(root);
    shell()->web_contents()->GetController().GoForward();
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_LINK | ui::PAGE_TRANSITION_FORWARD_BACK,
              capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_IN_PAGE, capturer.details().type);
  }
}

// Verify that navigations for NAVIGATION_TYPE_NEW_SUBFRAME and
// NAVIGATION_TYPE_AUTO_SUBFRAME are properly classified.
IN_PROC_BROWSER_TEST_F(NavigationControllerBrowserTest,
                       NavigationTypeClassification_NewAndAutoSubframe) {
  GURL main_url(embedded_test_server()->GetURL(
      "/navigation_controller/page_with_iframe.html"));
  NavigateToURL(shell(), main_url);

  // It is safe to obtain the root frame tree node here, as it doesn't change.
  FrameTreeNode* root =
      static_cast<WebContentsImpl*>(shell()->web_contents())->
          GetFrameTree()->root();

  ASSERT_EQ(1U, root->child_count());
  ASSERT_NE(nullptr, root->child_at(0));

  {
    // Simple load.
    FrameNavigateParamsCapturer capturer(root->child_at(0));
    GURL frame_url(embedded_test_server()->GetURL(
        "/navigation_controller/simple_page_1.html"));
    NavigateFrameToURL(root->child_at(0), frame_url);
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_MANUAL_SUBFRAME,
              capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_NEW_SUBFRAME, capturer.details().type);
  }

  {
    // Back.
    FrameNavigateParamsCapturer capturer(root->child_at(0));
    shell()->web_contents()->GetController().GoBack();
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_AUTO_SUBFRAME, capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_AUTO_SUBFRAME, capturer.details().type);
  }

  {
    // Forward.
    FrameNavigateParamsCapturer capturer(root->child_at(0));
    shell()->web_contents()->GetController().GoForward();
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_AUTO_SUBFRAME, capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_AUTO_SUBFRAME, capturer.details().type);
  }

  {
    // Simple load.
    FrameNavigateParamsCapturer capturer(root->child_at(0));
    GURL frame_url(embedded_test_server()->GetURL(
        "/navigation_controller/page_with_links.html"));
    NavigateFrameToURL(root->child_at(0), frame_url);
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_MANUAL_SUBFRAME,
              capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_NEW_SUBFRAME, capturer.details().type);
  }

  {
    // Load via a fragment link click.
    FrameNavigateParamsCapturer capturer(root->child_at(0));
    std::string script = "document.getElementById('fraglink').click()";
    EXPECT_TRUE(content::ExecuteScript(root->child_at(0)->current_frame_host(),
                                       script));
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_MANUAL_SUBFRAME,
              capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_NEW_SUBFRAME, capturer.details().type);
  }

  {
    // location.assign().
    FrameNavigateParamsCapturer capturer(root->child_at(0));
    GURL frame_url(embedded_test_server()->GetURL(
        "/navigation_controller/simple_page_1.html"));
    std::string script = "location.assign('" + frame_url.spec() + "')";
    EXPECT_TRUE(content::ExecuteScript(root->child_at(0)->current_frame_host(),
                                       script));
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_MANUAL_SUBFRAME,
              capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_NEW_SUBFRAME, capturer.details().type);
  }

  {
    // location.replace().
    LoadCommittedCapturer capturer(root->child_at(0));
    GURL frame_url(embedded_test_server()->GetURL(
        "/navigation_controller/simple_page_2.html"));
    std::string script = "location.replace('" + frame_url.spec() + "')";
    EXPECT_TRUE(content::ExecuteScript(root->child_at(0)->current_frame_host(),
                                       script));
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_AUTO_SUBFRAME, capturer.transition_type());
  }

  {
    // history.pushState().
    FrameNavigateParamsCapturer capturer(root->child_at(0));
    std::string script =
        "history.pushState({}, 'page 1', 'simple_page_1.html')";
    EXPECT_TRUE(content::ExecuteScript(root->child_at(0)->current_frame_host(),
                                       script));
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_MANUAL_SUBFRAME,
              capturer.params().transition);
    EXPECT_EQ(NAVIGATION_TYPE_NEW_SUBFRAME, capturer.details().type);
  }

  {
    // history.replaceState().
    LoadCommittedCapturer capturer(root->child_at(0));
    std::string script =
        "history.replaceState({}, 'page 2', 'simple_page_2.html')";
    EXPECT_TRUE(content::ExecuteScript(root->child_at(0)->current_frame_host(),
                                       script));
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_AUTO_SUBFRAME, capturer.transition_type());
  }

  {
    // Reload.
    LoadCommittedCapturer capturer(root->child_at(0));
    EXPECT_TRUE(content::ExecuteScript(root->child_at(0)->current_frame_host(),
                                       "location.reload()"));
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_AUTO_SUBFRAME, capturer.transition_type());
  }

  {
    // Create an iframe.
    LoadCommittedCapturer capturer(shell()->web_contents());
    GURL frame_url(embedded_test_server()->GetURL(
        "/navigation_controller/simple_page_1.html"));
    std::string script = "var iframe = document.createElement('iframe');"
                         "iframe.src = '" + frame_url.spec() + "';"
                         "document.body.appendChild(iframe);";
    EXPECT_TRUE(content::ExecuteScript(root->current_frame_host(), script));
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_AUTO_SUBFRAME, capturer.transition_type());
  }
}

// Verify that navigations caused by client-side redirects are correctly
// classified.
IN_PROC_BROWSER_TEST_F(NavigationControllerBrowserTest,
                       NavigationTypeClassification_ClientSideRedirect) {
  NavigateToURL(shell(), GURL(url::kAboutBlankURL));
  EXPECT_TRUE(WaitForLoadStop(shell()->web_contents()));

  FrameTreeNode* root =
      static_cast<WebContentsImpl*>(shell()->web_contents())->
          GetFrameTree()->root();

  {
    // Load the redirecting page.
    FrameNavigateParamsCapturer capturer(root);
    capturer.set_navigations_remaining(2);
    GURL frame_url(embedded_test_server()->GetURL(
        "/navigation_controller/client_redirect.html"));
    NavigateFrameToURL(root, frame_url);
    capturer.Wait();

    std::vector<FrameNavigateParams> params = capturer.all_params();
    std::vector<LoadCommittedDetails> details = capturer.all_details();
    ASSERT_EQ(2U, params.size());
    ASSERT_EQ(2U, details.size());
    EXPECT_EQ(ui::PAGE_TRANSITION_LINK, params[0].transition);
    EXPECT_EQ(NAVIGATION_TYPE_NEW_PAGE, details[0].type);
    EXPECT_EQ(ui::PAGE_TRANSITION_LINK | ui::PAGE_TRANSITION_CLIENT_REDIRECT,
              params[1].transition);
    EXPECT_EQ(NAVIGATION_TYPE_EXISTING_PAGE, details[1].type);
  }
}

// Verify the tree of FrameNavigationEntries after NAVIGATION_TYPE_AUTO_SUBFRAME
// commits.
// TODO(creis): Test cross-site iframes.
// TODO(creis): Test updating entries for history auto subframe navigations.
IN_PROC_BROWSER_TEST_F(NavigationControllerBrowserTest,
                       FrameNavigationEntry_AutoSubframe) {
  GURL main_url(embedded_test_server()->GetURL(
      "/navigation_controller/simple_page_1.html"));
  NavigateToURL(shell(), main_url);
  const NavigationControllerImpl& controller =
      static_cast<const NavigationControllerImpl&>(
          shell()->web_contents()->GetController());
  FrameTreeNode* root =
      static_cast<WebContentsImpl*>(shell()->web_contents())->
          GetFrameTree()->root();

  // Create an iframe.
  GURL frame_url(embedded_test_server()->GetURL(
      "/navigation_controller/simple_page_2.html"));
  {
    LoadCommittedCapturer capturer(shell()->web_contents());
    std::string script = "var iframe = document.createElement('iframe');"
                         "iframe.src = '" + frame_url.spec() + "';"
                         "document.body.appendChild(iframe);";
    EXPECT_TRUE(content::ExecuteScript(root->current_frame_host(), script));
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_AUTO_SUBFRAME, capturer.transition_type());
  }

  // Check last committed NavigationEntry.
  EXPECT_EQ(1, controller.GetEntryCount());
  NavigationEntryImpl* entry = controller.GetLastCommittedEntry();
  EXPECT_EQ(main_url, entry->GetURL());
  FrameNavigationEntry* root_entry = entry->root_node()->frame_entry.get();
  EXPECT_EQ(main_url, root_entry->url());

  // Verify subframe entries if we're in --site-per-process mode.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kSitePerProcess)) {
    // The entry should now have a subframe FrameNavigationEntry.
    ASSERT_EQ(1U, entry->root_node()->children.size());
    FrameNavigationEntry* frame_entry =
        entry->root_node()->children[0]->frame_entry.get();
    EXPECT_EQ(frame_url, frame_entry->url());
  } else {
    // There are no subframe FrameNavigationEntries by default.
    EXPECT_EQ(0U, entry->root_node()->children.size());
  }

  // Create a second iframe.
  {
    LoadCommittedCapturer capturer(shell()->web_contents());
    std::string script = "var iframe = document.createElement('iframe');"
                         "iframe.src = '" + frame_url.spec() + "';"
                         "document.body.appendChild(iframe);";
    EXPECT_TRUE(content::ExecuteScript(root->current_frame_host(), script));
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_AUTO_SUBFRAME, capturer.transition_type());
  }

  // The last committed NavigationEntry shouldn't have changed.
  EXPECT_EQ(1, controller.GetEntryCount());
  entry = controller.GetLastCommittedEntry();
  EXPECT_EQ(main_url, entry->GetURL());
  root_entry = entry->root_node()->frame_entry.get();
  EXPECT_EQ(main_url, root_entry->url());

  // Verify subframe entries if we're in --site-per-process mode.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kSitePerProcess)) {
    // The entry should now have 2 subframe FrameNavigationEntries.
    ASSERT_EQ(2U, entry->root_node()->children.size());
    FrameNavigationEntry* frame_entry =
        entry->root_node()->children[1]->frame_entry.get();
    EXPECT_EQ(frame_url, frame_entry->url());
  } else {
    // There are no subframe FrameNavigationEntries by default.
    EXPECT_EQ(0U, entry->root_node()->children.size());
  }

  // Create a nested iframe in the second subframe.
  {
    LoadCommittedCapturer capturer(shell()->web_contents());
    std::string script = "var iframe = document.createElement('iframe');"
                         "iframe.src = '" + frame_url.spec() + "';"
                         "document.body.appendChild(iframe);";
    EXPECT_TRUE(content::ExecuteScript(root->child_at(1)->current_frame_host(),
                                       script));
    capturer.Wait();
    EXPECT_EQ(ui::PAGE_TRANSITION_AUTO_SUBFRAME, capturer.transition_type());
  }

  // The last committed NavigationEntry shouldn't have changed.
  EXPECT_EQ(1, controller.GetEntryCount());
  entry = controller.GetLastCommittedEntry();
  EXPECT_EQ(main_url, entry->GetURL());
  root_entry = entry->root_node()->frame_entry.get();
  EXPECT_EQ(main_url, root_entry->url());

  // Verify subframe entries if we're in --site-per-process mode.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kSitePerProcess)) {
    // The entry should now have 2 subframe FrameNavigationEntries.
    ASSERT_EQ(2U, entry->root_node()->children.size());
    ASSERT_EQ(1U, entry->root_node()->children[1]->children.size());
    FrameNavigationEntry* frame_entry =
        entry->root_node()->children[1]->children[0]->frame_entry.get();
    EXPECT_EQ(frame_url, frame_entry->url());
  } else {
    // There are no subframe FrameNavigationEntries by default.
    EXPECT_EQ(0U, entry->root_node()->children.size());
  }
}

namespace {

class HttpThrottle : public ResourceThrottle {
 public:
  // ResourceThrottle
  void WillStartRequest(bool* defer) override {
    *defer = true;
  }

  const char* GetNameForLogging() const override {
    return "HttpThrottle";
  }
};

class StallDelegate : public ResourceDispatcherHostDelegate {
  // ResourceDispatcherHostDelegate
  void RequestBeginning(
      net::URLRequest* request,
      content::ResourceContext* resource_context,
      content::AppCacheService* appcache_service,
      ResourceType resource_type,
      ScopedVector<content::ResourceThrottle>* throttles) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    throttles->push_back(new HttpThrottle);
  }
};

// Loads |start_url|, then loads |stalled_url| which stalls. While the page is
// stalled, an in-page navigation happens. Make sure that all the navigations
// are properly classified.
void DoReplaceStateWhilePending(Shell* shell,
                                const GURL& start_url,
                                const GURL& stalled_url,
                                const std::string& replace_state_filename) {
  NavigationControllerImpl& controller =
      static_cast<NavigationControllerImpl&>(
          shell->web_contents()->GetController());

  FrameTreeNode* root =
      static_cast<WebContentsImpl*>(shell->web_contents())->
          GetFrameTree()->root();

  // Start with one page.
  EXPECT_TRUE(NavigateToURL(shell, start_url));

  // Have the user decide to go to a different page which is very slow.
  StallDelegate stall_delegate;
  ResourceDispatcherHost::Get()->SetDelegate(&stall_delegate);
  controller.LoadURL(
      stalled_url, Referrer(), ui::PAGE_TRANSITION_LINK, std::string());

  // That should be the pending entry.
  NavigationEntryImpl* entry = controller.GetPendingEntry();
  ASSERT_NE(nullptr, entry);
  EXPECT_EQ(stalled_url, entry->GetURL());

  {
    // Now the existing page uses history.replaceState().
    FrameNavigateParamsCapturer capturer(root);
    capturer.set_wait_for_load(false);
    std::string script =
        "history.replaceState({}, '', '" + replace_state_filename + "')";
    EXPECT_TRUE(content::ExecuteScript(root->current_frame_host(), script));
    capturer.Wait();

    // The fact that there was a pending entry shouldn't interfere with the
    // classification.
    EXPECT_EQ(NAVIGATION_TYPE_IN_PAGE, capturer.details().type);
  }

  ResourceDispatcherHost::Get()->SetDelegate(nullptr);
}

}  // namespace

IN_PROC_BROWSER_TEST_F(
    NavigationControllerBrowserTest,
    NavigationTypeClassification_On1InPageToXWhile2Pending) {
  GURL url1(embedded_test_server()->GetURL(
      "/navigation_controller/simple_page_1.html"));
  GURL url2(embedded_test_server()->GetURL(
      "/navigation_controller/simple_page_2.html"));
  DoReplaceStateWhilePending(shell(), url1, url2, "x");
}

IN_PROC_BROWSER_TEST_F(
    NavigationControllerBrowserTest,
    NavigationTypeClassification_On1InPageTo2While2Pending) {
  GURL url1(embedded_test_server()->GetURL(
      "/navigation_controller/simple_page_1.html"));
  GURL url2(embedded_test_server()->GetURL(
      "/navigation_controller/simple_page_2.html"));
  DoReplaceStateWhilePending(shell(), url1, url2, "simple_page_2.html");
}

IN_PROC_BROWSER_TEST_F(NavigationControllerBrowserTest,
                       NavigationTypeClassification_On1InPageToXWhile1Pending) {
  GURL url(embedded_test_server()->GetURL(
      "/navigation_controller/simple_page_1.html"));
  DoReplaceStateWhilePending(shell(), url, url, "x");
}

IN_PROC_BROWSER_TEST_F(NavigationControllerBrowserTest,
                       NavigationTypeClassification_On1InPageTo1While1Pending) {
  GURL url(embedded_test_server()->GetURL(
      "/navigation_controller/simple_page_1.html"));
  DoReplaceStateWhilePending(shell(), url, url, "simple_page_1.html");
}

// Ensure the renderer process does not get confused about the current entry
// due to subframes and replaced entries.  See https://crbug.com/480201.
IN_PROC_BROWSER_TEST_F(NavigationControllerBrowserTest,
                       PreventSpoofFromSubframeAndReplace) {
  // Start at an initial URL.
  GURL url1(embedded_test_server()->GetURL(
      "/navigation_controller/simple_page_1.html"));
  NavigateToURL(shell(), url1);

  // Now go to a page with a real iframe.
  GURL url2(embedded_test_server()->GetURL(
      "/navigation_controller/page_with_data_iframe.html"));
  NavigateToURL(shell(), url2);

  // It is safe to obtain the root frame tree node here, as it doesn't change.
  FrameTreeNode* root = static_cast<WebContentsImpl*>(shell()->web_contents())
                            ->GetFrameTree()
                            ->root();
  ASSERT_EQ(1U, root->child_count());
  ASSERT_NE(nullptr, root->child_at(0));

  {
    // Navigate in the iframe.
    FrameNavigateParamsCapturer capturer(root->child_at(0));
    GURL frame_url(embedded_test_server()->GetURL(
        "/navigation_controller/simple_page_2.html"));
    NavigateFrameToURL(root->child_at(0), frame_url);
    capturer.Wait();
    EXPECT_EQ(NAVIGATION_TYPE_NEW_SUBFRAME, capturer.details().type);
  }

  {
    // Go back in the iframe.
    TestNavigationObserver back_load_observer(shell()->web_contents());
    shell()->web_contents()->GetController().GoBack();
    back_load_observer.Wait();
  }

  {
    // Go forward in the iframe.
    TestNavigationObserver forward_load_observer(shell()->web_contents());
    shell()->web_contents()->GetController().GoForward();
    forward_load_observer.Wait();
  }

  GURL url3(embedded_test_server()->GetURL(
      "/navigation_controller/page_with_iframe.html"));
  {
    // location.replace() to cause an inert commit.
    TestNavigationObserver replace_load_observer(shell()->web_contents());
    std::string script = "location.replace('" + url3.spec() + "')";
    EXPECT_TRUE(content::ExecuteScript(root->current_frame_host(), script));
    replace_load_observer.Wait();
  }

  {
    // Go back to url2.
    TestNavigationObserver back_load_observer(shell()->web_contents());
    shell()->web_contents()->GetController().GoBack();
    back_load_observer.Wait();

    // Make sure the URL is correct for both the entry and the main frame, and
    // that the process hasn't been killed for showing a spoof.
    EXPECT_TRUE(root->current_frame_host()->IsRenderFrameLive());
    EXPECT_EQ(url2, shell()->web_contents()->GetLastCommittedURL());
    EXPECT_EQ(url2, root->current_url());
  }

  {
    // Go back to reset main frame entirely.
    TestNavigationObserver back_load_observer(shell()->web_contents());
    shell()->web_contents()->GetController().GoBack();
    back_load_observer.Wait();
    EXPECT_EQ(url1, shell()->web_contents()->GetLastCommittedURL());
    EXPECT_EQ(url1, root->current_url());
  }

  {
    // Go forward.
    TestNavigationObserver back_load_observer(shell()->web_contents());
    shell()->web_contents()->GetController().GoForward();
    back_load_observer.Wait();
    EXPECT_EQ(url2, shell()->web_contents()->GetLastCommittedURL());
    EXPECT_EQ(url2, root->current_url());
  }

  {
    // Go forward to the replaced URL.
    TestNavigationObserver forward_load_observer(shell()->web_contents());
    shell()->web_contents()->GetController().GoForward();
    forward_load_observer.Wait();

    // Make sure the URL is correct for both the entry and the main frame, and
    // that the process hasn't been killed for showing a spoof.
    EXPECT_TRUE(root->current_frame_host()->IsRenderFrameLive());
    EXPECT_EQ(url3, shell()->web_contents()->GetLastCommittedURL());
    EXPECT_EQ(url3, root->current_url());
  }
}

// Ensure the renderer process does not get killed if the main frame URL's path
// changes when going back in a subframe, since this is currently possible after
// a replaceState in the main frame (thanks to https://crbug.com/373041).
// See https:///crbug.com/486916.
IN_PROC_BROWSER_TEST_F(NavigationControllerBrowserTest,
                       SubframeBackFromReplaceState) {
  // Start at a page with a real iframe.
  GURL url1(embedded_test_server()->GetURL(
      "/navigation_controller/page_with_data_iframe.html"));
  NavigateToURL(shell(), url1);

  // It is safe to obtain the root frame tree node here, as it doesn't change.
  FrameTreeNode* root = static_cast<WebContentsImpl*>(shell()->web_contents())
                            ->GetFrameTree()
                            ->root();
  ASSERT_EQ(1U, root->child_count());
  ASSERT_NE(nullptr, root->child_at(0));

  {
    // Navigate in the iframe.
    FrameNavigateParamsCapturer capturer(root->child_at(0));
    GURL frame_url(embedded_test_server()->GetURL(
        "/navigation_controller/simple_page_2.html"));
    NavigateFrameToURL(root->child_at(0), frame_url);
    capturer.Wait();
    EXPECT_EQ(NAVIGATION_TYPE_NEW_SUBFRAME, capturer.details().type);
  }

  {
    // history.replaceState().
    FrameNavigateParamsCapturer capturer(root);
    std::string script =
        "history.replaceState({}, 'replaced', 'replaced')";
    EXPECT_TRUE(content::ExecuteScript(root->current_frame_host(), script));
    capturer.Wait();
  }

  {
    // Go back in the iframe.
    TestNavigationObserver back_load_observer(shell()->web_contents());
    shell()->web_contents()->GetController().GoBack();
    back_load_observer.Wait();
  }

  // For now, we expect the main frame's URL to revert.  This won't happen once
  // https://crbug.com/373041 is fixed.
  EXPECT_EQ(url1, shell()->web_contents()->GetLastCommittedURL());

  // Make sure the renderer process has not been killed.
  EXPECT_TRUE(root->current_frame_host()->IsRenderFrameLive());
}

namespace {

class FailureWatcher : public WebContentsObserver {
 public:
  // Observes failure for the specified |node|.
  explicit FailureWatcher(FrameTreeNode* node)
      : WebContentsObserver(
            node->current_frame_host()->delegate()->GetAsWebContents()),
        frame_tree_node_id_(node->frame_tree_node_id()),
        message_loop_runner_(new MessageLoopRunner) {}

  void Wait() {
    message_loop_runner_->Run();
  }

 private:
  void DidFailLoad(RenderFrameHost* render_frame_host,
                   const GURL& validated_url,
                   int error_code,
                   const base::string16& error_description) override {
    RenderFrameHostImpl* rfh =
        static_cast<RenderFrameHostImpl*>(render_frame_host);
    if (rfh->frame_tree_node()->frame_tree_node_id() != frame_tree_node_id_)
      return;

    message_loop_runner_->Quit();
  }

  void DidFailProvisionalLoad(
      RenderFrameHost* render_frame_host,
      const GURL& validated_url,
      int error_code,
      const base::string16& error_description) override {
    RenderFrameHostImpl* rfh =
        static_cast<RenderFrameHostImpl*>(render_frame_host);
    if (rfh->frame_tree_node()->frame_tree_node_id() != frame_tree_node_id_)
      return;

    message_loop_runner_->Quit();
  }

  // The id of the FrameTreeNode whose navigations to observe.
  int frame_tree_node_id_;

  // The MessageLoopRunner used to spin the message loop.
  scoped_refptr<MessageLoopRunner> message_loop_runner_;
};

}  // namespace

IN_PROC_BROWSER_TEST_F(NavigationControllerBrowserTest,
                       StopCausesFailureDespiteJavaScriptURL) {
  NavigationControllerImpl& controller =
      static_cast<NavigationControllerImpl&>(
          shell()->web_contents()->GetController());

  FrameTreeNode* root =
      static_cast<WebContentsImpl*>(shell()->web_contents())->
          GetFrameTree()->root();

  // Start with a normal page.
  GURL url1(embedded_test_server()->GetURL(
      "/navigation_controller/simple_page_1.html"));
  EXPECT_TRUE(NavigateToURL(shell(), url1));

  // Have the user decide to go to a different page which is very slow.
  StallDelegate stall_delegate;
  ResourceDispatcherHost::Get()->SetDelegate(&stall_delegate);
  GURL url2(embedded_test_server()->GetURL(
      "/navigation_controller/simple_page_2.html"));
  controller.LoadURL(url2, Referrer(), ui::PAGE_TRANSITION_LINK, std::string());

  // That should be the pending entry.
  NavigationEntryImpl* entry = controller.GetPendingEntry();
  ASSERT_NE(nullptr, entry);
  EXPECT_EQ(url2, entry->GetURL());

  // Loading a JavaScript URL shouldn't affect the ability to stop.
  {
    FailureWatcher watcher(root);
    GURL js("javascript:(function(){})()");
    controller.LoadURL(js, Referrer(), ui::PAGE_TRANSITION_LINK, std::string());
    // This LoadURL ends up purging the pending entry, which is why this is
    // tricky.
    EXPECT_EQ(nullptr, controller.GetPendingEntry());
    shell()->web_contents()->Stop();
    watcher.Wait();
  }

  ResourceDispatcherHost::Get()->SetDelegate(nullptr);
}

}  // namespace content
