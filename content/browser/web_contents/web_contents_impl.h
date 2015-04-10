// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_IMPL_H_
#define CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_IMPL_H_

#include <map>
#include <set>
#include <string>

#include "base/compiler_specific.h"
#include "base/containers/scoped_ptr_hash_map.h"
#include "base/gtest_prod_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/observer_list.h"
#include "base/process/process.h"
#include "base/values.h"
#include "content/browser/frame_host/frame_tree.h"
#include "content/browser/frame_host/navigation_controller_delegate.h"
#include "content/browser/frame_host/navigation_controller_impl.h"
#include "content/browser/frame_host/navigator_delegate.h"
#include "content/browser/frame_host/render_frame_host_delegate.h"
#include "content/browser/frame_host/render_frame_host_manager.h"
#include "content/browser/media/audio_state_provider.h"
#include "content/browser/renderer_host/render_view_host_delegate.h"
#include "content/browser/renderer_host/render_widget_host_delegate.h"
#include "content/common/accessibility_mode_enums.h"
#include "content/common/content_export.h"
#include "content/public/browser/color_chooser.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/renderer_preferences.h"
#include "content/public/common/resource_type.h"
#include "content/public/common/three_d_api_types.h"
#include "net/base/load_states.h"
#include "net/http/http_response_headers.h"
#include "third_party/WebKit/public/web/WebDragOperation.h"
#include "ui/base/page_transition_types.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/size.h"

struct BrowserPluginHostMsg_ResizeGuest_Params;
struct ViewHostMsg_DateTimeDialogValue_Params;
struct ViewMsg_PostMessage_Params;

#if defined(OS_TIZEN) && defined(ENABLE_MURPHY)
namespace tizen {
class MediaWebContentsObserver;
}
#endif

namespace content {
class BrowserPluginEmbedder;
class BrowserPluginGuest;
class BrowserPluginGuestManager;
class DateTimeChooserAndroid;
class DownloadItem;
class GeolocationServiceContext;
class InterstitialPageImpl;
class JavaScriptDialogManager;
class ManifestManagerHost;
class MediaWebContentsObserver;
class PluginContentOriginWhitelist;
class PowerSaveBlocker;
class RenderViewHost;
class RenderViewHostDelegateView;
class RenderViewHostImpl;
class RenderWidgetHostImpl;
class SavePackage;
class ScreenOrientationDispatcherHost;
class SiteInstance;
class TestWebContents;
class WebContentsAudioMuter;
class WebContentsDelegate;
class WebContentsImpl;
class WebContentsObserver;
class WebContentsView;
class WebContentsViewDelegate;
struct AXEventNotificationDetails;
struct ColorSuggestion;
struct FaviconURL;
struct LoadNotificationDetails;
struct ResourceRedirectDetails;
struct ResourceRequestDetails;

#if defined(OS_ANDROID)
class WebContentsAndroid;
#endif

// Factory function for the implementations that content knows about. Takes
// ownership of |delegate|.
WebContentsView* CreateWebContentsView(
    WebContentsImpl* web_contents,
    WebContentsViewDelegate* delegate,
    RenderViewHostDelegateView** render_view_host_delegate_view);

class CONTENT_EXPORT WebContentsImpl
    : public NON_EXPORTED_BASE(WebContents),
      public NON_EXPORTED_BASE(RenderFrameHostDelegate),
      public RenderViewHostDelegate,
      public RenderWidgetHostDelegate,
      public RenderFrameHostManager::Delegate,
      public NotificationObserver,
      public NON_EXPORTED_BASE(NavigationControllerDelegate),
      public NON_EXPORTED_BASE(NavigatorDelegate) {
 public:
  class FriendZone;

  ~WebContentsImpl() override;

  static WebContentsImpl* CreateWithOpener(
      const WebContents::CreateParams& params,
      WebContentsImpl* opener);

  static std::vector<WebContentsImpl*> GetAllWebContents();

  // Returns the opener WebContentsImpl, if any. This can be set to null if the
  // opener is closed or the page clears its window.opener.
  WebContentsImpl* opener() const { return opener_; }

  // Creates a swapped out RenderView. This is used by the browser plugin to
  // create a swapped out RenderView in the embedder render process for the
  // guest, to expose the guest's window object to the embedder.
  // This returns the routing ID of the newly created swapped out RenderView.
  int CreateSwappedOutRenderView(SiteInstance* instance);

  // Complex initialization here. Specifically needed to avoid having
  // members call back into our virtual functions in the constructor.
  virtual void Init(const WebContents::CreateParams& params);

  // Returns the SavePackage which manages the page saving job. May be NULL.
  SavePackage* save_package() const { return save_package_.get(); }

#if defined(OS_ANDROID)
  // In Android WebView, the RenderView needs created even there is no
  // navigation entry, this allows Android WebViews to use
  // javascript: URLs that load into the DOMWindow before the first page
  // load. This is not safe to do in any context that a web page could get a
  // reference to the DOMWindow before the first page load.
  bool CreateRenderViewForInitialEmptyDocument();
#endif

  // Expose the render manager for testing.
  // TODO(creis): Remove this now that we can get to it via FrameTreeNode.
  RenderFrameHostManager* GetRenderManagerForTesting();

  // Returns guest browser plugin object, or NULL if this WebContents is not a
  // guest.
  BrowserPluginGuest* GetBrowserPluginGuest() const;

  // Sets a BrowserPluginGuest object for this WebContents. If this WebContents
  // has a BrowserPluginGuest then that implies that it is being hosted by
  // a BrowserPlugin object in an embedder renderer process.
  void SetBrowserPluginGuest(BrowserPluginGuest* guest);

  // Returns embedder browser plugin object, or NULL if this WebContents is not
  // an embedder.
  BrowserPluginEmbedder* GetBrowserPluginEmbedder() const;

  // Creates a BrowserPluginEmbedder object for this WebContents if one doesn't
  // already exist.
  void CreateBrowserPluginEmbedderIfNecessary();

  // Gets the current fullscreen render widget's routing ID. Returns
  // MSG_ROUTING_NONE when there is no fullscreen render widget.
  int GetFullscreenWidgetRoutingID() const;

  // Invoked when visible SSL state (as defined by SSLStatus) changes.
  void DidChangeVisibleSSLState();

  // Informs the render view host and the BrowserPluginEmbedder, if present, of
  // a Drag Source End.
  void DragSourceEndedAt(int client_x, int client_y, int screen_x,
      int screen_y, blink::WebDragOperation operation);

  // A response has been received for a resource request.
  void DidGetResourceResponseStart(
      const ResourceRequestDetails& details);

  // A redirect was received while requesting a resource.
  void DidGetRedirectForResourceRequest(
      RenderFrameHost* render_frame_host,
      const ResourceRedirectDetails& details);

  WebContentsView* GetView() const;

  ScreenOrientationDispatcherHost* screen_orientation_dispatcher_host() {
    return screen_orientation_dispatcher_host_.get();
  }

  bool should_normally_be_visible() { return should_normally_be_visible_; }

  // Indicate if the window has been occluded, and pass this to the views, only
  // if there is no active capture going on (otherwise it is dropped on the
  // floor).
  void WasOccluded();
  void WasUnOccluded();

  // Broadcasts the mode change to all frames.
  void SetAccessibilityMode(AccessibilityMode mode);

  // Adds the given accessibility mode to the current accessibility mode
  // bitmap.
  void AddAccessibilityMode(AccessibilityMode mode);

  // Removes the given accessibility mode from the current accessibility
  // mode bitmap, managing the bits that are shared with other modes such
  // that a bit will only be turned off when all modes that depend on it
  // have been removed.
  void RemoveAccessibilityMode(AccessibilityMode mode);

  // Clear the navigation transition data when the user navigates back to Chrome
  // from a native app.
  void ClearNavigationTransitionData();

  // WebContents ------------------------------------------------------
  ScreenOrientationDispatcherHost* GetScreenOrientationDispatcherHost() override;
  WebContentsDelegate* GetDelegate() override;
  void SetDelegate(WebContentsDelegate* delegate) override;
  NavigationControllerImpl& GetController() override;
  const NavigationControllerImpl& GetController() const override;
  BrowserContext* GetBrowserContext() const override;
  const GURL& GetURL() const override;
  const GURL& GetVisibleURL() const override;
  const GURL& GetLastCommittedURL() const override;
  RenderProcessHost* GetRenderProcessHost() const override;
  RenderFrameHostImpl* GetMainFrame() override;
  RenderFrameHost* GetFocusedFrame() override;
  void ForEachFrame(
      const base::Callback<void(RenderFrameHost*)>& on_frame) override;
  void SendToAllFrames(IPC::Message* message) override;
  RenderViewHost* GetRenderViewHost() const override;
  int GetRoutingID() const override;
  RenderWidgetHostView* GetRenderWidgetHostView() const override;
  RenderWidgetHostView* GetFullscreenRenderWidgetHostView() const override;
  SkColor GetThemeColor() const override;
  WebUI* CreateWebUI(const GURL& url) override;
  WebUI* GetWebUI() const override;
  WebUI* GetCommittedWebUI() const override;
  void SetUserAgentOverride(const std::string& override) override;
  const std::string& GetUserAgentOverride() const override;
  void EnableTreeOnlyAccessibilityMode() override;
  bool IsTreeOnlyAccessibilityModeForTesting() const override;
  bool IsFullAccessibilityModeForTesting() const override;
#if defined(OS_WIN)
  virtual void SetParentNativeViewAccessible(
      gfx::NativeViewAccessible accessible_parent) override;
#endif
  const base::string16& GetTitle() const override;
  int32 GetMaxPageID() override;
  int32 GetMaxPageIDForSiteInstance(SiteInstance* site_instance) override;
  SiteInstanceImpl* GetSiteInstance() const override;
  SiteInstanceImpl* GetPendingSiteInstance() const override;
  bool IsLoading() const override;
  bool IsLoadingToDifferentDocument() const override;
  bool IsWaitingForResponse() const override;
  const net::LoadStateWithParam& GetLoadState() const override;
  const base::string16& GetLoadStateHost() const override;
  uint64 GetUploadSize() const override;
  uint64 GetUploadPosition() const override;
  std::set<GURL> GetSitesInTab() const override;
  const std::string& GetEncoding() const override;
  bool DisplayedInsecureContent() const override;
  void IncrementCapturerCount(const gfx::Size& capture_size) override;
  void DecrementCapturerCount() override;
  int GetCapturerCount() const override;
  bool IsAudioMuted() const override;
  void SetAudioMuted(bool mute) override;
  bool IsCrashed() const override;
  void SetIsCrashed(base::TerminationStatus status, int error_code) override;
  base::TerminationStatus GetCrashedStatus() const override;
  bool IsBeingDestroyed() const override;
  void NotifyNavigationStateChanged(InvalidateTypes changed_flags) override;
  base::TimeTicks GetLastActiveTime() const override;
  void WasShown() override;
  void WasHidden() override;
  bool NeedToFireBeforeUnload() override;
  void DispatchBeforeUnload(bool for_cross_site_transition) override;
  void Stop() override;
  WebContents* Clone() override;
  void ReloadFocusedFrame(bool ignore_cache) override;
  void Undo() override;
  void Redo() override;
  void Cut() override;
  void Copy() override;
  void CopyToFindPboard() override;
  void Paste() override;
  void PasteAndMatchStyle() override;
  void Delete() override;
  void SelectAll() override;
  void Unselect() override;
  void Replace(const base::string16& word) override;
  void ReplaceMisspelling(const base::string16& word) override;
  void NotifyContextMenuClosed(
      const CustomContextMenuContext& context) override;
  void ExecuteCustomContextMenuCommand(
      int action,
      const CustomContextMenuContext& context) override;
  gfx::NativeView GetNativeView() override;
  gfx::NativeView GetContentNativeView() override;
  gfx::NativeWindow GetTopLevelNativeWindow() override;
  gfx::Rect GetContainerBounds() override;
  gfx::Rect GetViewBounds() override;
  DropData* GetDropData() override;
  void Focus() override;
  void SetInitialFocus() override;
  void StoreFocus() override;
  void RestoreFocus() override;
  void FocusThroughTabTraversal(bool reverse) override;
  bool ShowingInterstitialPage() const override;
  InterstitialPage* GetInterstitialPage() const override;
  bool IsSavable() override;
  void OnSavePage() override;
  bool SavePage(const base::FilePath& main_file,
                const base::FilePath& dir_path,
                SavePageType save_type) override;
  void SaveFrame(const GURL& url, const Referrer& referrer) override;
  void SaveFrameWithHeaders(const GURL& url,
                            const Referrer& referrer,
                            const std::string& headers) override;
  void GenerateMHTML(const base::FilePath& file,
                     const base::Callback<void(int64)>& callback) override;
  const std::string& GetContentsMimeType() const override;
  bool WillNotifyDisconnection() const override;
  void SetOverrideEncoding(const std::string& encoding) override;
  void ResetOverrideEncoding() override;
  RendererPreferences* GetMutableRendererPrefs() override;
  void Close() override;
  void SystemDragEnded() override;
  void UserGestureDone() override;
  void SetClosedByUserGesture(bool value) override;
  bool GetClosedByUserGesture() const override;
  void ViewSource() override;
  void ViewFrameSource(const GURL& url, const PageState& page_state) override;
  int GetMinimumZoomPercent() const override;
  int GetMaximumZoomPercent() const override;
  void ResetPageScale() override;
  gfx::Size GetPreferredSize() const override;
  bool GotResponseToLockMouseRequest(bool allowed) override;
  bool HasOpener() const override;
  WebContents* GetOpener() const override;
  void DidChooseColorInColorChooser(SkColor color) override;
  void DidEndColorChooser() override;
  int DownloadImage(const GURL& url,
                    bool is_favicon,
                    uint32_t max_bitmap_size,
                    bool bypass_cache,
                    const ImageDownloadCallback& callback) override;
  bool IsSubframe() const override;
  void Find(int request_id,
            const base::string16& search_text,
            const blink::WebFindOptions& options) override;
  void StopFinding(StopFindAction action) override;
  void InsertCSS(const std::string& css) override;
  bool WasRecentlyAudible() override;
  void GetManifest(const GetManifestCallback&) override;
  void ExitFullscreen() override;
#if defined(OS_ANDROID)
  base::android::ScopedJavaLocalRef<jobject> GetJavaWebContents() override;
  virtual WebContentsAndroid* GetWebContentsAndroid();
#elif defined(OS_MACOSX)
  void SetAllowOtherViews(bool allow) override;
  bool GetAllowOtherViews() override;
#endif

  // Implementation of PageNavigator.
  WebContents* OpenURL(const OpenURLParams& params) override;

  // Implementation of IPC::Sender.
  bool Send(IPC::Message* message) override;

  // RenderFrameHostDelegate ---------------------------------------------------
  bool OnMessageReceived(RenderFrameHost* render_frame_host,
                         const IPC::Message& message) override;
  const GURL& GetMainFrameLastCommittedURL() const override;
  void RenderFrameCreated(RenderFrameHost* render_frame_host) override;
  void RenderFrameDeleted(RenderFrameHost* render_frame_host) override;
  void DidStartLoading(RenderFrameHost* render_frame_host,
                       bool to_different_document) override;
  void DidStopLoading() override;
  void SwappedOut(RenderFrameHost* render_frame_host) override;
  void DidDeferAfterResponseStarted(
      const TransitionLayerData& transition_data) override;
  bool WillHandleDeferAfterResponseStarted() override;
  void WorkerCrashed(RenderFrameHost* render_frame_host) override;
  void ShowContextMenu(RenderFrameHost* render_frame_host,
                       const ContextMenuParams& params) override;
  void RunJavaScriptMessage(RenderFrameHost* render_frame_host,
                            const base::string16& message,
                            const base::string16& default_prompt,
                            const GURL& frame_url,
                            JavaScriptMessageType type,
                            IPC::Message* reply_msg) override;
  void RunBeforeUnloadConfirm(RenderFrameHost* render_frame_host,
                              const base::string16& message,
                              bool is_reload,
                              IPC::Message* reply_msg) override;
  void DidAccessInitialDocument() override;
  void DidChangeName(RenderFrameHost* render_frame_host,
                     const std::string& name) override;
  void DidDisownOpener(RenderFrameHost* render_frame_host) override;
  void DocumentOnLoadCompleted(RenderFrameHost* render_frame_host) override;
  void UpdateTitle(RenderFrameHost* render_frame_host,
                   int32 page_id,
                   const base::string16& title,
                   base::i18n::TextDirection title_direction) override;
  void UpdateEncoding(RenderFrameHost* render_frame_host,
                      const std::string& encoding) override;
  WebContents* GetAsWebContents() override;
  bool IsNeverVisible() override;
  AccessibilityMode GetAccessibilityMode() const override;
  void AccessibilityEventReceived(
      const std::vector<AXEventNotificationDetails>& details) override;
  RenderFrameHost* GetGuestByInstanceID(
      RenderFrameHost* render_frame_host,
      int browser_plugin_instance_id) override;
  GeolocationServiceContext* GetGeolocationServiceContext() override;
  void EnterFullscreenMode(const GURL& origin) override;
  void ExitFullscreenMode() override;
#if defined(OS_WIN)
  gfx::NativeViewAccessible GetParentNativeViewAccessible() override;
#endif

  // RenderViewHostDelegate ----------------------------------------------------
  RenderViewHostDelegateView* GetDelegateView() override;
  bool OnMessageReceived(RenderViewHost* render_view_host,
                         const IPC::Message& message) override;
  // RenderFrameHostDelegate has the same method, so list it there because this
  // interface is going away.
  // virtual WebContents* GetAsWebContents() override;
  gfx::Rect GetRootWindowResizerRect() const override;
  void RenderViewCreated(RenderViewHost* render_view_host) override;
  void RenderViewReady(RenderViewHost* render_view_host) override;
  void RenderViewTerminated(RenderViewHost* render_view_host,
                            base::TerminationStatus status,
                            int error_code) override;
  void RenderViewDeleted(RenderViewHost* render_view_host) override;
  void UpdateState(RenderViewHost* render_view_host,
                   int32 page_id,
                   const PageState& page_state) override;
  void UpdateTargetURL(RenderViewHost* render_view_host,
                       const GURL& url) override;
  void Close(RenderViewHost* render_view_host) override;
  void RequestMove(const gfx::Rect& new_bounds) override;
  void DidCancelLoading() override;
  void DocumentAvailableInMainFrame(RenderViewHost* render_view_host) override;
  void RouteCloseEvent(RenderViewHost* rvh) override;
  void RouteMessageEvent(RenderViewHost* rvh,
                         const ViewMsg_PostMessage_Params& params) override;
  bool AddMessageToConsole(int32 level,
                           const base::string16& message,
                           int32 line_no,
                           const base::string16& source_id) override;
  RendererPreferences GetRendererPrefs(
      BrowserContext* browser_context) const override;
  void OnUserGesture() override;
  void OnIgnoredUIEvent() override;
  void RendererUnresponsive(RenderViewHost* render_view_host) override;
  void RendererResponsive(RenderViewHost* render_view_host) override;
  void LoadStateChanged(const GURL& url,
                        const net::LoadStateWithParam& load_state,
                        uint64 upload_position,
                        uint64 upload_size) override;
  void Activate() override;
  void Deactivate() override;
  void LostCapture() override;
  void RunFileChooser(RenderViewHost* render_view_host,
                      const FileChooserParams& params) override;
  bool IsFullscreenForCurrentTab() const override;
  void UpdatePreferredSize(const gfx::Size& pref_size) override;
  void ResizeDueToAutoResize(const gfx::Size& new_size) override;
  void RequestToLockMouse(bool user_gesture,
                          bool last_unlocked_by_target) override;
  void LostMouseLock() override;
  void CreateNewWindow(
      int render_process_id,
      int route_id,
      int main_frame_route_id,
      const ViewHostMsg_CreateWindow_Params& params,
      SessionStorageNamespace* session_storage_namespace) override;
  void CreateNewWidget(int render_process_id,
                       int route_id,
                       blink::WebPopupType popup_type) override;
  void CreateNewFullscreenWidget(int render_process_id, int route_id) override;
  void ShowCreatedWindow(int route_id,
                         WindowOpenDisposition disposition,
                         const gfx::Rect& initial_rect,
                         bool user_gesture) override;
  void ShowCreatedWidget(int route_id, const gfx::Rect& initial_rect) override;
  void ShowCreatedFullscreenWidget(int route_id) override;
  void RequestMediaAccessPermission(
      const MediaStreamRequest& request,
      const MediaResponseCallback& callback) override;
  bool CheckMediaAccessPermission(const GURL& security_origin,
                                  MediaStreamType type) override;
  SessionStorageNamespace* GetSessionStorageNamespace(
      SiteInstance* instance) override;
  SessionStorageNamespaceMap GetSessionStorageNamespaceMap() override;
  FrameTree* GetFrameTree() override;
  void SetIsVirtualKeyboardRequested(bool requested) override;
  bool IsVirtualKeyboardRequested() override;


  // NavigatorDelegate ---------------------------------------------------------

  void DidStartProvisionalLoad(RenderFrameHostImpl* render_frame_host,
                               const GURL& validated_url,
                               bool is_error_page,
                               bool is_iframe_srcdoc) override;
  void DidStartNavigationTransition(
      RenderFrameHostImpl* render_frame_host) override;
  void DidFailProvisionalLoadWithError(
      RenderFrameHostImpl* render_frame_host,
      const FrameHostMsg_DidFailProvisionalLoadWithError_Params& params)
      override;
  void DidFailLoadWithError(RenderFrameHostImpl* render_frame_host,
                            const GURL& url,
                            int error_code,
                            const base::string16& error_description) override;
  void DidCommitProvisionalLoad(RenderFrameHostImpl* render_frame_host,
                                const GURL& url,
                                ui::PageTransition transition_type) override;
  void DidNavigateMainFramePreCommit(bool navigation_is_within_page) override;
  void DidNavigateMainFramePostCommit(
      RenderFrameHostImpl* render_frame_host,
      const LoadCommittedDetails& details,
      const FrameHostMsg_DidCommitProvisionalLoad_Params& params) override;
  void DidNavigateAnyFramePostCommit(
      RenderFrameHostImpl* render_frame_host,
      const LoadCommittedDetails& details,
      const FrameHostMsg_DidCommitProvisionalLoad_Params& params) override;
  void SetMainFrameMimeType(const std::string& mime_type) override;
  bool CanOverscrollContent() const override;
  void NotifyChangedNavigationState(InvalidateTypes changed_flags) override;
  void AboutToNavigateRenderFrame(
      RenderFrameHostImpl* old_host,
      RenderFrameHostImpl* new_host) override;
  void DidStartNavigationToPendingEntry(
      const GURL& url,
      NavigationController::ReloadType reload_type) override;
  void RequestOpenURL(RenderFrameHostImpl* render_frame_host,
                      const OpenURLParams& params) override;
  bool ShouldPreserveAbortedURLs() override;

  // RenderWidgetHostDelegate --------------------------------------------------

  void RenderWidgetDeleted(RenderWidgetHostImpl* render_widget_host) override;
  void RenderWidgetGotFocus(RenderWidgetHostImpl* render_widget_host) override;
  void RenderWidgetWasResized(RenderWidgetHostImpl* render_widget_host,
                              bool width_changed) override;
  void ScreenInfoChanged() override;
  bool PreHandleKeyboardEvent(const NativeWebKeyboardEvent& event,
                              bool* is_keyboard_shortcut) override;
  void HandleKeyboardEvent(const NativeWebKeyboardEvent& event) override;
  bool HandleWheelEvent(const blink::WebMouseWheelEvent& event) override;
  bool PreHandleGestureEvent(const blink::WebGestureEvent& event) override;
  void DidSendScreenRects(RenderWidgetHostImpl* rwh) override;
  BrowserAccessibilityManager* GetRootBrowserAccessibilityManager() override;
  BrowserAccessibilityManager* GetOrCreateRootBrowserAccessibilityManager()
      override;

  // RenderFrameHostManager::Delegate ------------------------------------------

  bool CreateRenderViewForRenderManager(
      RenderViewHost* render_view_host,
      int opener_route_id,
      int proxy_routing_id,
      bool for_main_frame_navigation) override;
  bool CreateRenderFrameForRenderManager(RenderFrameHost* render_frame_host,
                                         int parent_routing_id,
                                         int proxy_routing_id) override;
  void BeforeUnloadFiredFromRenderManager(
      bool proceed,
      const base::TimeTicks& proceed_time,
      bool* proceed_to_fire_unload) override;
  void RenderProcessGoneFromRenderManager(
      RenderViewHost* render_view_host) override;
  void UpdateRenderViewSizeForRenderManager() override;
  void CancelModalDialogsForRenderManager() override;
  void NotifySwappedFromRenderManager(RenderFrameHost* old_host,
                                      RenderFrameHost* new_host,
                                      bool is_main_frame) override;
  void NotifyMainFrameSwappedFromRenderManager(
      RenderViewHost* old_host,
      RenderViewHost* new_host) override;
  int CreateOpenerRenderViewsForRenderManager(SiteInstance* instance) override;
  NavigationControllerImpl& GetControllerForRenderManager() override;
  scoped_ptr<WebUIImpl> CreateWebUIForRenderManager(const GURL& url) override;
  NavigationEntry* GetLastCommittedNavigationEntryForRenderManager() override;
  bool FocusLocationBarByDefault() override;
  void SetFocusToLocationBar(bool select_all) override;
  bool IsHidden() override;

  // NotificationObserver ------------------------------------------------------

  void Observe(int type,
               const NotificationSource& source,
               const NotificationDetails& details) override;

  // NavigationControllerDelegate ----------------------------------------------

  WebContents* GetWebContents() override;
  void NotifyNavigationEntryCommitted(
      const LoadCommittedDetails& load_details) override;

  // Invoked before a form repost warning is shown.
  void NotifyBeforeFormRepostWarningShow() override;

  // Activate this WebContents and show a form repost warning.
  void ActivateAndShowRepostFormWarningDialog() override;

  // Whether the initial empty page of this view has been accessed by another
  // page, making it unsafe to show the pending URL. Always false after the
  // first commit.
  bool HasAccessedInitialDocument() override;

  // Updates the max page ID for the current SiteInstance in this
  // WebContentsImpl to be at least |page_id|.
  void UpdateMaxPageID(int32 page_id) override;

  // Updates the max page ID for the given SiteInstance in this WebContentsImpl
  // to be at least |page_id|.
  void UpdateMaxPageIDForSiteInstance(SiteInstance* site_instance,
                                      int32 page_id) override;

  // Copy the current map of SiteInstance ID to max page ID from another tab.
  // This is necessary when this tab adopts the NavigationEntries from
  // |web_contents|.
  void CopyMaxPageIDsFrom(WebContents* web_contents) override;

  // Called by the NavigationController to cause the WebContentsImpl to navigate
  // to the current pending entry. The NavigationController should be called
  // back with RendererDidNavigate on success or DiscardPendingEntry on failure.
  // The callbacks can be inside of this function, or at some future time.
  //
  // The entry has a PageID of -1 if newly created (corresponding to navigation
  // to a new URL).
  //
  // If this method returns false, then the navigation is discarded (equivalent
  // to calling DiscardPendingEntry on the NavigationController).
  bool NavigateToPendingEntry(
      NavigationController::ReloadType reload_type) override;

  // Sets the history for this WebContentsImpl to |history_length| entries, with
  // an offset of |history_offset|.
  void SetHistoryOffsetAndLength(int history_offset,
                                 int history_length) override;

  // Called by InterstitialPageImpl when it creates a RenderFrameHost.
  void RenderFrameForInterstitialPageCreated(
      RenderFrameHost* render_frame_host) override;

  // Sets the passed interstitial as the currently showing interstitial.
  // No interstitial page should already be attached.
  void AttachInterstitialPage(InterstitialPageImpl* interstitial_page) override;

  // Unsets the currently showing interstitial.
  void DetachInterstitialPage() override;

  // Changes the IsLoading state and notifies the delegate as needed.
  // |details| is used to provide details on the load that just finished
  // (but can be null if not applicable).
  void SetIsLoading(bool is_loading,
                    bool to_different_document,
                    LoadNotificationDetails* details) override;

  typedef base::Callback<void(WebContents*)> CreatedCallback;

  // Requests the renderer to move the selection extent to a new position.
  void MoveRangeSelectionExtent(const gfx::Point& extent);

  // Requests the renderer to select the region between two points in the
  // currently focused frame.
  void SelectRange(const gfx::Point& base, const gfx::Point& extent);

  // Notifies the main frame that it can continue navigation (if it was deferred
  // immediately at first response).
  void ResumeResponseDeferredAtStart();

  // Forces overscroll to be disabled (used by touch emulation).
  void SetForceDisableOverscrollContent(bool force_disable);

  AudioStateProvider* audio_state_provider() {
    return audio_state_provider_.get();
  }

  bool has_audio_power_save_blocker_for_testing() const {
    return audio_power_save_blocker_;
  }

  bool has_video_power_save_blocker_for_testing() const {
    return video_power_save_blocker_;
  }

#if defined(ENABLE_BROWSER_CDMS)
  MediaWebContentsObserver* media_web_contents_observer() {
    return media_web_contents_observer_.get();
  }
#endif

 private:
  friend class WebContentsObserver;
  friend class WebContents;  // To implement factory methods.

  FRIEND_TEST_ALL_PREFIXES(WebContentsImplTest, NoJSMessageOnInterstitials);
  FRIEND_TEST_ALL_PREFIXES(WebContentsImplTest, UpdateTitle);
  FRIEND_TEST_ALL_PREFIXES(WebContentsImplTest, FindOpenerRVHWhenPending);
  FRIEND_TEST_ALL_PREFIXES(WebContentsImplTest,
                           CrossSiteCantPreemptAfterUnload);
  FRIEND_TEST_ALL_PREFIXES(WebContentsImplTest, PendingContents);
  FRIEND_TEST_ALL_PREFIXES(WebContentsImplTest, FrameTreeShape);
  FRIEND_TEST_ALL_PREFIXES(WebContentsImplTest, GetLastActiveTime);
  FRIEND_TEST_ALL_PREFIXES(FormStructureBrowserTest, HTMLFiles);
  FRIEND_TEST_ALL_PREFIXES(NavigationControllerTest, HistoryNavigate);
  FRIEND_TEST_ALL_PREFIXES(RenderFrameHostManagerTest, PageDoesBackAndReload);
  FRIEND_TEST_ALL_PREFIXES(SitePerProcessBrowserTest, CrossSiteIframe);
  FRIEND_TEST_ALL_PREFIXES(SitePerProcessAccessibilityBrowserTest,
                           CrossSiteIframeAccessibility);

  // So InterstitialPageImpl can access SetIsLoading.
  friend class InterstitialPageImpl;

  // TODO(brettw) TestWebContents shouldn't exist!
  friend class TestWebContents;

  class DestructionObserver;

  // See WebContents::Create for a description of these parameters.
  WebContentsImpl(BrowserContext* browser_context,
                  WebContentsImpl* opener);

  // Add and remove observers for page navigation notifications. The order in
  // which notifications are sent to observers is undefined. Clients must be
  // sure to remove the observer before they go away.
  void AddObserver(WebContentsObserver* observer);
  void RemoveObserver(WebContentsObserver* observer);

  // Clears this tab's opener if it has been closed.
  void OnWebContentsDestroyed(WebContentsImpl* web_contents);

  // Creates and adds to the map a destruction observer watching |web_contents|.
  // No-op if such an observer already exists.
  void AddDestructionObserver(WebContentsImpl* web_contents);

  // Deletes and removes from the map a destruction observer
  // watching |web_contents|. No-op if there is no such observer.
  void RemoveDestructionObserver(WebContentsImpl* web_contents);

  // Traverses all the RenderFrameHosts in the FrameTree and creates a set
  // all the unique RenderWidgetHostViews.
  std::set<RenderWidgetHostView*> GetRenderWidgetHostViewsInTree();

  // Callback function when showing JavaScript dialogs.  Takes in a routing ID
  // pair to identify the RenderFrameHost that opened the dialog, because it's
  // possible for the RenderFrameHost to be deleted by the time this is called.
  void OnDialogClosed(int render_process_id,
                      int render_frame_id,
                      IPC::Message* reply_msg,
                      bool dialog_was_suppressed,
                      bool success,
                      const base::string16& user_input);

  bool OnMessageReceived(RenderViewHost* render_view_host,
                         RenderFrameHost* render_frame_host,
                         const IPC::Message& message);

  // Checks whether render_frame_message_source_ is set to non-null value,
  // otherwise it terminates the main frame renderer process.
  bool HasValidFrameSource();

  // IPC message handlers.
  void OnThemeColorChanged(SkColor theme_color);
  void OnDidLoadResourceFromMemoryCache(const GURL& url,
                                        const std::string& security_info,
                                        const std::string& http_request,
                                        const std::string& mime_type,
                                        ResourceType resource_type);
  void OnDidDisplayInsecureContent();
  void OnDidRunInsecureContent(const std::string& security_origin,
                               const GURL& target_url);
  void OnDocumentLoadedInFrame();
  void OnDidFinishLoad(const GURL& url);
  void OnDidStartLoading(bool to_different_document);
  void OnDidStopLoading();
  void OnDidChangeLoadProgress(double load_progress);
  void OnGoToEntryAtOffset(int offset);
  void OnUpdateZoomLimits(int minimum_percent,
                          int maximum_percent);
  void OnEnumerateDirectory(int request_id, const base::FilePath& path);

  void OnRegisterProtocolHandler(const std::string& protocol,
                                 const GURL& url,
                                 const base::string16& title,
                                 bool user_gesture);
  void OnUnregisterProtocolHandler(const std::string& protocol,
                                   const GURL& url,
                                   bool user_gesture);
  void OnFindReply(int request_id,
                   int number_of_matches,
                   const gfx::Rect& selection_rect,
                   int active_match_ordinal,
                   bool final_update);
#if defined(OS_ANDROID)
  void OnFindMatchRectsReply(int version,
                             const std::vector<gfx::RectF>& rects,
                             const gfx::RectF& active_rect);

  void OnOpenDateTimeDialog(
      const ViewHostMsg_DateTimeDialogValue_Params& value);
#endif
  void OnDomOperationResponse(const std::string& json_string,
                              int automation_id);
  void OnAppCacheAccessed(const GURL& manifest_url, bool blocked_by_policy);
  void OnOpenColorChooser(int color_chooser_id,
                          SkColor color,
                          const std::vector<ColorSuggestion>& suggestions);
  void OnEndColorChooser(int color_chooser_id);
  void OnSetSelectedColorInColorChooser(int color_chooser_id, SkColor color);
  void OnWebUISend(const GURL& source_url,
                   const std::string& name,
                   const base::ListValue& args);
#if defined(ENABLE_PLUGINS)
  void OnPepperPluginHung(int plugin_child_id,
                          const base::FilePath& path,
                          bool is_hung);
  void OnPluginCrashed(const base::FilePath& plugin_path,
                       base::ProcessId plugin_pid);
  void OnRequestPpapiBrokerPermission(int routing_id,
                                      const GURL& url,
                                      const base::FilePath& plugin_path);

  // Callback function when requesting permission to access the PPAPI broker.
  // |result| is true if permission was granted.
  void OnPpapiBrokerPermissionResult(int routing_id, bool result);

  void OnBrowserPluginMessage(RenderFrameHost* render_frame_host,
                              const IPC::Message& message);
#endif  // defined(ENABLE_PLUGINS)
  void OnDidDownloadImage(int id,
                          int http_status_code,
                          const GURL& image_url,
                          const std::vector<SkBitmap>& bitmaps,
                          const std::vector<gfx::Size>& original_bitmap_sizes);
  void OnUpdateFaviconURL(const std::vector<FaviconURL>& candidates);
  void OnFirstVisuallyNonEmptyPaint();
  void OnMediaPlayingNotification(int64 player_cookie,
                                  bool has_video,
                                  bool has_audio,
                                  bool is_remote);
  void OnMediaPausedNotification(int64 player_cookie);
  void OnShowValidationMessage(const gfx::Rect& anchor_in_root_view,
                               const base::string16& main_text,
                               const base::string16& sub_text);
  void OnHideValidationMessage();
  void OnMoveValidationMessage(const gfx::Rect& anchor_in_root_view);

  // Called by derived classes to indicate that we're no longer waiting for a
  // response. This won't actually update the throbber, but it will get picked
  // up at the next animation step if the throbber is going.
  void SetNotWaitingForResponse() { waiting_for_response_ = false; }

  // Navigation helpers --------------------------------------------------------
  //
  // These functions are helpers for Navigate() and DidNavigate().

  // Handles post-navigation tasks in DidNavigate AFTER the entry has been
  // committed to the navigation controller. Note that the navigation entry is
  // not provided since it may be invalid/changed after being committed. The
  // current navigation entry is in the NavigationController at this point.

  // If our controller was restored, update the max page ID associated with the
  // given RenderViewHost to be larger than the number of restored entries.
  // This is called in CreateRenderView before any navigations in the RenderView
  // have begun, to prevent any races in updating RenderView::next_page_id.
  void UpdateMaxPageIDIfNecessary(RenderViewHost* rvh);

  // Saves the given title to the navigation entry and does associated work. It
  // will update history and the view for the new title, and also synthesize
  // titles for file URLs that have none (so we require that the URL of the
  // entry already be set).
  //
  // This is used as the backend for state updates, which include a new title,
  // or the dedicated set title message. It returns true if the new title is
  // different and was therefore updated.
  bool UpdateTitleForEntry(NavigationEntryImpl* entry,
                           const base::string16& title);

  // Recursively creates swapped out RenderViews for this tab's opener chain
  // (including this tab) in the given SiteInstance, allowing other tabs to send
  // cross-process JavaScript calls to their opener(s).  Returns the route ID of
  // this tab's RenderView for |instance|.
  int CreateOpenerRenderViews(SiteInstance* instance);

  // Helper for CreateNewWidget/CreateNewFullscreenWidget.
  void CreateNewWidget(int render_process_id,
                       int route_id,
                       bool is_fullscreen,
                       blink::WebPopupType popup_type);

  // Helper for ShowCreatedWidget/ShowCreatedFullscreenWidget.
  void ShowCreatedWidget(int route_id,
                         bool is_fullscreen,
                         const gfx::Rect& initial_rect);

  // Finds the new RenderWidgetHost and returns it. Note that this can only be
  // called once as this call also removes it from the internal map.
  RenderWidgetHostView* GetCreatedWidget(int route_id);

  // Finds the new WebContentsImpl by route_id, initializes it for
  // renderer-initiated creation, and returns it. Note that this can only be
  // called once as this call also removes it from the internal map.
  WebContentsImpl* GetCreatedWindow(int route_id);

  // Tracking loading progress -------------------------------------------------

  // Resets the tracking state of the current load.
  void ResetLoadProgressState();

  // Calculates the progress of the current load and notifies the delegate.
  void SendLoadProgressChanged();

  // Misc non-view stuff -------------------------------------------------------

  // Sets the history for a specified RenderViewHost to |history_length|
  // entries, with an offset of |history_offset|.
  void SetHistoryOffsetAndLengthForView(RenderViewHost* render_view_host,
                                        int history_offset,
                                        int history_length);

  // Helper functions for sending notifications.
  void NotifyViewSwapped(RenderViewHost* old_host, RenderViewHost* new_host);
  void NotifyFrameSwapped(RenderFrameHost* old_host, RenderFrameHost* new_host);
  void NotifyDisconnected();

  void SetEncoding(const std::string& encoding);

  // TODO(creis): This should take in a FrameTreeNode to know which node's
  // render manager to return.  For now, we just return the root's.
  RenderFrameHostManager* GetRenderManager() const;

  RenderViewHostImpl* GetRenderViewHostImpl();

  // Removes browser plugin embedder if there is one.
  void RemoveBrowserPluginEmbedder();

  // Clear |render_frame_host|'s tracking entry for its power save blockers.
  void ClearPowerSaveBlockers(RenderFrameHost* render_frame_host);

  // Clear tracking entries for all RenderFrameHosts, clears
  // |audio_power_save_blocker_| and |video_power_save_blocker_|.
  void ClearAllPowerSaveBlockers();

  // Creates an audio or video power save blocker respectively.
  void CreateAudioPowerSaveBlocker();
  void CreateVideoPowerSaveBlocker();

  // Releases the audio power save blockers if |active_audio_players_| is empty.
  // Likewise, releases the video power save blockers if |active_video_players_|
  // is empty.
  void MaybeReleasePowerSaveBlockers();

  // Helper function to invoke WebContentsDelegate::GetSizeForNewRenderView().
  gfx::Size GetSizeForNewRenderView();

  void OnFrameRemoved(RenderFrameHost* render_frame_host);

  // Helper method that's called whenever |preferred_size_| or
  // |preferred_size_for_capture_| changes, to propagate the new value to the
  // |delegate_|.
  void OnPreferredSizeChanged(const gfx::Size& old_size);

  // Helper methods for adding or removing player entries in |player_map| under
  // the key |render_frame_message_source_|.
  typedef std::vector<int64> PlayerList;
  typedef std::map<uintptr_t, PlayerList> ActiveMediaPlayerMap;
  void AddMediaPlayerEntry(int64 player_cookie,
                           ActiveMediaPlayerMap* player_map);
  void RemoveMediaPlayerEntry(int64 player_cookie,
                              ActiveMediaPlayerMap* player_map);
  // Removes all entries from |player_map| for |render_frame_host|.
  void RemoveAllMediaPlayerEntries(RenderFrameHost* render_frame_host,
                                   ActiveMediaPlayerMap* player_map);

  // Data for core operation ---------------------------------------------------

  // Delegate for notifying our owner about stuff. Not owned by us.
  WebContentsDelegate* delegate_;

  // Handles the back/forward list and loading.
  NavigationControllerImpl controller_;

  // The corresponding view.
  scoped_ptr<WebContentsView> view_;

  // The view of the RVHD. Usually this is our WebContentsView implementation,
  // but if an embedder uses a different WebContentsView, they'll need to
  // provide this.
  RenderViewHostDelegateView* render_view_host_delegate_view_;

  // Tracks created WebContentsImpl objects that have not been shown yet. They
  // are identified by the route ID passed to CreateNewWindow.
  typedef std::map<int, WebContentsImpl*> PendingContents;
  PendingContents pending_contents_;

  // These maps hold on to the widgets that we created on behalf of the renderer
  // that haven't shown yet.
  typedef std::map<int, RenderWidgetHostView*> PendingWidgetViews;
  PendingWidgetViews pending_widget_views_;

  typedef std::map<WebContentsImpl*, DestructionObserver*> DestructionObservers;
  DestructionObservers destruction_observers_;

  // A list of observers notified when page state changes. Weak references.
  // This MUST be listed above frame_tree_ since at destruction time the
  // latter might cause RenderViewHost's destructor to call us and we might use
  // the observer list then.
  ObserverList<WebContentsObserver> observers_;

  // The tab that opened this tab, if any.  Will be set to null if the opener
  // is closed.
  WebContentsImpl* opener_;

  // True if this tab was opened by another tab. This is not unset if the opener
  // is closed.
  bool created_with_opener_;

#if defined(OS_WIN)
  gfx::NativeViewAccessible accessible_parent_;
#endif

  // Helper classes ------------------------------------------------------------

  // Tracking variables and associated power save blockers for media playback.
  ActiveMediaPlayerMap active_audio_players_;
  ActiveMediaPlayerMap active_video_players_;
  scoped_ptr<PowerSaveBlocker> audio_power_save_blocker_;
  scoped_ptr<PowerSaveBlocker> video_power_save_blocker_;

  // Tells whether this WebContents is actively producing sound.
  // Order is important: the |frame_tree_| destruction uses
  // |audio_state_provider_|.
  scoped_ptr<AudioStateProvider> audio_state_provider_;

  // Manages the frame tree of the page and process swaps in each node.
  FrameTree frame_tree_;

  // SavePackage, lazily created.
  scoped_refptr<SavePackage> save_package_;

  // Data for loading state ----------------------------------------------------

  // Indicates whether we're currently loading a resource.
  bool is_loading_;

  // Indicates whether the current load is to a different document. Only valid
  // if is_loading_ is true.
  bool is_load_to_different_document_;

  // Indicates if the tab is considered crashed.
  base::TerminationStatus crashed_status_;
  int crashed_error_code_;

  // Whether this WebContents is waiting for a first-response for the
  // main resource of the page. This controls whether the throbber state is
  // "waiting" or "loading."
  bool waiting_for_response_;

  // Map of SiteInstance ID to max page ID for this tab. A page ID is specific
  // to a given tab and SiteInstance, and must be valid for the lifetime of the
  // WebContentsImpl.
  std::map<int32, int32> max_page_ids_;

  // The current load state and the URL associated with it.
  net::LoadStateWithParam load_state_;
  base::string16 load_state_host_;

  double loading_total_progress_;

  base::TimeTicks loading_last_progress_update_;

  // Upload progress, for displaying in the status bar.
  // Set to zero when there is no significant upload happening.
  uint64 upload_size_;
  uint64 upload_position_;

  // Data for current page -----------------------------------------------------

  // When a title cannot be taken from any entry, this title will be used.
  base::string16 page_title_when_no_navigation_entry_;

  // When a navigation occurs, we record its contents MIME type. It can be
  // used to check whether we can do something for some special contents.
  std::string contents_mime_type_;

  // The last reported character encoding, not canonicalized.
  std::string last_reported_encoding_;

  // The canonicalized character encoding.
  std::string canonical_encoding_;

  // True if this is a secure page which displayed insecure content.
  bool displayed_insecure_content_;

  // Whether the initial empty page has been accessed by another page, making it
  // unsafe to show the pending URL. Usually false unless another window tries
  // to modify the blank page.  Always false after the first commit.
  bool has_accessed_initial_document_;

  // The theme color for the underlying document as specified
  // by theme-color meta tag.
  SkColor theme_color_;

  // The last published theme color.
  SkColor last_sent_theme_color_;

  // Data for misc internal state ----------------------------------------------

  // When > 0, the WebContents is currently being captured (e.g., for
  // screenshots or mirroring); and the underlying RenderWidgetHost should not
  // be told it is hidden.
  int capturer_count_;

  // Tracks whether RWHV should be visible once capturer_count_ becomes zero.
  bool should_normally_be_visible_;

  // See getter above.
  bool is_being_destroyed_;

  // Indicates whether we should notify about disconnection of this
  // WebContentsImpl. This is used to ensure disconnection notifications only
  // happen if a connection notification has happened and that they happen only
  // once.
  bool notify_disconnection_;

  // Pointer to the JavaScript dialog manager, lazily assigned. Used because the
  // delegate of this WebContentsImpl is nulled before its destructor is called.
  JavaScriptDialogManager* dialog_manager_;

  // Set to true when there is an active "before unload" dialog.  When true,
  // we've forced the throbber to start in Navigate, and we need to remember to
  // turn it off in OnJavaScriptMessageBoxClosed if the navigation is canceled.
  bool is_showing_before_unload_dialog_;

  // Settings that get passed to the renderer process.
  RendererPreferences renderer_preferences_;

  // The time that this WebContents was last made active. The initial value is
  // the WebContents creation time.
  base::TimeTicks last_active_time_;

  // See description above setter.
  bool closed_by_user_gesture_;

  // Minimum/maximum zoom percent.
  int minimum_zoom_percent_;
  int maximum_zoom_percent_;

  // The intrinsic size of the page.
  gfx::Size preferred_size_;

  // The preferred size for content screen capture.  When |capturer_count_| > 0,
  // this overrides |preferred_size_|.
  gfx::Size preferred_size_for_capture_;

#if defined(OS_ANDROID)
  // Date time chooser opened by this tab.
  // Only used in Android since all other platforms use a multi field UI.
  scoped_ptr<DateTimeChooserAndroid> date_time_chooser_;
#endif

  // Holds information about a current color chooser dialog, if one is visible.
  struct ColorChooserInfo {
    ColorChooserInfo(int render_process_id,
                     int render_frame_id,
                     ColorChooser* chooser,
                     int identifier);
    ~ColorChooserInfo();

    int render_process_id;
    int render_frame_id;

    // Color chooser that was opened by this tab.
    scoped_ptr<ColorChooser> chooser;

    // A unique identifier for the current color chooser.  Identifiers are
    // unique across a renderer process.  This avoids race conditions in
    // synchronizing the browser and renderer processes.  For example, if a
    // renderer closes one chooser and opens another, and simultaneously the
    // user picks a color in the first chooser, the IDs can be used to drop the
    // "chose a color" message rather than erroneously tell the renderer that
    // the user picked a color in the second chooser.
    int identifier;
  };

  scoped_ptr<ColorChooserInfo> color_chooser_info_;

  // Manages the embedder state for browser plugins, if this WebContents is an
  // embedder; NULL otherwise.
  scoped_ptr<BrowserPluginEmbedder> browser_plugin_embedder_;
  // Manages the guest state for browser plugin, if this WebContents is a guest;
  // NULL otherwise.
  scoped_ptr<BrowserPluginGuest> browser_plugin_guest_;

#if defined(ENABLE_PLUGINS)
  // Manages the whitelist of plugin content origins exempt from power saving.
  scoped_ptr<PluginContentOriginWhitelist> plugin_content_origin_whitelist_;
#endif

  // This must be at the end, or else we might get notifications and use other
  // member variables that are gone.
  NotificationRegistrar registrar_;

  // Used during IPC message dispatching from the RenderView/RenderFrame so that
  // the handlers can get a pointer to the RVH through which the message was
  // received.
  RenderViewHost* render_view_message_source_;
  RenderFrameHost* render_frame_message_source_;

  // All live RenderWidgetHostImpls that are created by this object and may
  // outlive it.
  std::set<RenderWidgetHostImpl*> created_widgets_;

  // Routing id of the shown fullscreen widget or MSG_ROUTING_NONE otherwise.
  int fullscreen_widget_routing_id_;

  // At the time the fullscreen widget was being shut down, did it have focus?
  // This is used to restore focus to the WebContentsView after both: 1) the
  // fullscreen widget is destroyed, and 2) the WebContentsDelegate has
  // completed making layout changes to effect an exit from fullscreen mode.
  bool fullscreen_widget_had_focus_at_shutdown_;

  // Maps the ids of pending image downloads to their callbacks
  typedef std::map<int, ImageDownloadCallback> ImageDownloadMap;
  ImageDownloadMap image_download_map_;

  // Whether this WebContents is responsible for displaying a subframe in a
  // different process from its parent page.
  bool is_subframe_;

  // Whether overscroll should be unconditionally disabled.
  bool force_disable_overscroll_content_;

  // Whether the last JavaScript dialog shown was suppressed. Used for testing.
  bool last_dialog_suppressed_;

  scoped_ptr<GeolocationServiceContext> geolocation_service_context_;

  scoped_ptr<ScreenOrientationDispatcherHost>
      screen_orientation_dispatcher_host_;

  scoped_ptr<ManifestManagerHost> manifest_manager_host_;

  // The accessibility mode for all frames. This is queried when each frame
  // is created, and broadcast to all frames when it changes.
  AccessibilityMode accessibility_mode_;

  // Created on-demand to mute all audio output from this WebContents.
  scoped_ptr<WebContentsAudioMuter> audio_muter_;

  bool virtual_keyboard_requested_;

#if defined(ENABLE_BROWSER_CDMS)
  // Manages all the media player and CDM managers and forwards IPCs to them.
  scoped_ptr<MediaWebContentsObserver> media_web_contents_observer_;
#endif

#if defined(OS_TIZEN) && defined(ENABLE_MURPHY)
  // Manages all the media player managers and forwards IPCs to them.
  scoped_ptr<tizen::MediaWebContentsObserver> media_webcontents_observer_;
#endif

  base::WeakPtrFactory<WebContentsImpl> loading_weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsImpl);
};

// Dangerous methods which should never be made part of the public API, so we
// grant their use only to an explicit friend list (c++ attorney/client idiom).
class CONTENT_EXPORT WebContentsImpl::FriendZone {
 private:
  friend class TestNavigationObserver;
  friend class WebContentsAddedObserver;
  friend class ContentBrowserSanityChecker;

  FriendZone();  // Not instantiable.

  // Adds/removes a callback called on creation of each new WebContents.
  static void AddCreatedCallbackForTesting(const CreatedCallback& callback);
  static void RemoveCreatedCallbackForTesting(const CreatedCallback& callback);

  DISALLOW_COPY_AND_ASSIGN(FriendZone);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_IMPL_H_
