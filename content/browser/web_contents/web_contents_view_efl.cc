// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_contents/web_contents_view_efl.h"

#include <algorithm>

#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "build/build_config.h"
#include "content/browser/renderer_host/render_view_host_factory.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_efl.h"
#include "content/browser/web_contents/interstitial_page_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_view_delegate.h"
#include "ui/base/efl/ewk_view_wrapper.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/size.h"
#include "webkit/glue/webdropdata.h"

using WebKit::WebDragOperation;
using WebKit::WebDragOperationsMask;

namespace content {

WebContentsViewPort* CreateWebContentsView(
    WebContentsImpl* web_contents,
    WebContentsViewDelegate* delegate,
    RenderViewHostDelegateView** render_view_host_delegate_view) {
  WebContentsViewEfl* rv = new WebContentsViewEfl(web_contents, delegate);
  *render_view_host_delegate_view = rv;
  return rv;
}

WebContentsViewEfl::WebContentsViewEfl(
    WebContentsImpl* web_contents,
    WebContentsViewDelegate* delegate)
    : web_contents_(web_contents),
      delegate_(delegate),
      view_container_box_(0) {
  /*
   * TODO: Connect the evas smart callback signals for size changes to
   * callback functions in this file.
   */

  /* TODO: do we need a focus store for our EFL widget */

  // TODO: Initializiation function for delegate that takes an evas_object
  // if (delegate_)
  //    delegate_->Initialize(expanded_.get(), &focus_store_);
}

WebContentsViewEfl::~WebContentsViewEfl() {
  // TODO: destory evas_object
  // expanded_.Destroy();
}

gfx::NativeView WebContentsViewEfl::GetNativeView() const {
  return GetContentNativeView();
}

gfx::NativeView WebContentsViewEfl::GetContentNativeView() const {
  RenderWidgetHostView* rwhv = web_contents_->GetRenderWidgetHostView();
  if (!rwhv)
    return NULL;
  return rwhv->GetNativeView();
}

gfx::NativeWindow WebContentsViewEfl::GetTopLevelNativeWindow() const {
  /* TODO: traverse to top level evas_object or similar */

  /*
  GtkWidget* window = gtk_widget_get_ancestor(GetNativeView(), GTK_TYPE_WINDOW);
  return window ? GTK_WINDOW(window) : NULL;
  */
  return 0;
}

void WebContentsViewEfl::GetContainerBounds(gfx::Rect* out) const {
  // This is used for positioning the download shelf arrow animation,
  // as well as sizing some other widgets in Windows.  In GTK the size is
  // managed for us, so it appears to be only used for the download shelf
  // animation.
}

void WebContentsViewEfl::OnTabCrashed(base::TerminationStatus status,
                                      int error_code) {
}

void WebContentsViewEfl::Focus() {
  if (delegate_) {
    delegate_->Focus();
  }
}

void WebContentsViewEfl::SetInitialFocus() {
  if (web_contents_->FocusLocationBarByDefault())
    web_contents_->SetFocusToLocationBar(false);
  else
    Focus();
}

void WebContentsViewEfl::StoreFocus() {
  // TODO: Do we need that for EFL?
  // focus_store_.Store(GetNativeView());
}

void WebContentsViewEfl::RestoreFocus() {
  // TODO: Do we need focus storage for EFL?
    SetInitialFocus();
}

WebDropData* WebContentsViewEfl::GetDropData() const {
  return 0;
}

gfx::Rect WebContentsViewEfl::GetViewBounds() const {
  gfx::Rect rect;
  return rect;
}

void WebContentsViewEfl::CreateView(
    const gfx::Size& initial_size, gfx::NativeView context) {
  requested_size_ = initial_size;
}

RenderWidgetHostView* WebContentsViewEfl::CreateViewForWidget(
    RenderWidgetHost* render_widget_host) {
  if (render_widget_host->GetView()) {
    // During testing, the view will already be set up in most cases to the
    // test view, so we don't want to clobber it with a real one. To verify that
    // this actually is happening (and somebody isn't accidentally creating the
    // view twice), we check for the RVH Factory, which will be set when we're
    // making special ones (which go along with the special views).
    DCHECK(RenderViewHostFactory::has_factory());
    return render_widget_host->GetView();
  }

  RenderWidgetHostView* view =
      RenderWidgetHostView::CreateViewForWidget(render_widget_host);
  view->InitAsChild(reinterpret_cast<gfx::NativeView>(view_container_box_));
  // gfx::NativeView content_view = view->GetNativeView();

  // TODO: Connect EFL focus event to self
  // InsertIntoContentArea(content_view);

  // We don't want to change any state in this class for swapped out RVHs
  // because they will not be visible at this time.
  if (render_widget_host->IsRenderView()) {
    RenderViewHost* rvh = RenderViewHost::From(render_widget_host);
    if (!static_cast<RenderViewHostImpl*>(rvh)->is_swapped_out())
      UpdateDragDest(rvh);
  }

  return view;
}

RenderWidgetHostView* WebContentsViewEfl::CreateViewForPopupWidget(
    RenderWidgetHost* render_widget_host) {
  return RenderWidgetHostViewPort::CreateViewForWidget(render_widget_host);
}

void WebContentsViewEfl::SetPageTitle(const string16& title) {
  // Set the window name to include the page title so it's easier to spot
  // when debugging (e.g. via xwininfo -tree).
  // gfx::NativeView content_view = GetContentNativeView();

  // TODO: AS the name of the function says, set the underlying X window title
  // to the title argument.
}

void WebContentsViewEfl::SizeContents(const gfx::Size& size) {
  // We don't need to manually set the size of of widgets in GTK+, but we do
  // need to pass the sizing information on to the RWHV which will pass the
  // sizing information on to the renderer.
  requested_size_ = size;
  RenderWidgetHostView* rwhv = web_contents_->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->SetSize(size);
}

void WebContentsViewEfl::RenderViewCreated(RenderViewHost* host) {
}

void WebContentsViewEfl::RenderViewSwappedIn(RenderViewHost* host) {
  UpdateDragDest(host);
}

void WebContentsViewEfl::SetOverscrollControllerEnabled(bool enabled) {
}

WebContents* WebContentsViewEfl::web_contents() {
  return web_contents_;
}

void WebContentsViewEfl::UpdateDragCursor(WebDragOperation operation) {
  // drag_dest_->UpdateDragStatus(operation);
}

void WebContentsViewEfl::GotFocus() {
  // This is only used in the views FocusManager stuff but it bleeds through
  // all subclasses. http://crbug.com/21875
}

// This is called when the renderer asks us to take focus back (i.e., it has
// iterated past the last focusable element on the page).
void WebContentsViewEfl::TakeFocus(bool reverse) {
  if (!web_contents_->GetDelegate())
    return;
  DCHECK(web_contents_->GetDelegate()->TakeFocus(web_contents_, reverse));
}

void WebContentsViewEfl::UpdateDragDest(RenderViewHost* host) {
}

void WebContentsViewEfl::ShowContextMenu(
    const ContextMenuParams& params,
    ContextMenuSourceType type) {
  if (delegate_)
    delegate_->ShowContextMenu(params, type);
  else
    DLOG(ERROR) << "Cannot show context menus without a delegate.";
}

void WebContentsViewEfl::ShowPopupMenu(const gfx::Rect& bounds,
                                       int item_height,
                                       double item_font_size,
                                       int selected_item,
                                       const std::vector<WebMenuItem>& items,
                                       bool right_aligned,
                                       bool allow_multiple_selection) {
  // External popup menus are only used on Mac and Android.
  NOTIMPLEMENTED();
}

// Render view DnD -------------------------------------------------------------

void WebContentsViewEfl::StartDragging(const WebDropData& drop_data,
                                       WebDragOperationsMask ops,
                                       const gfx::ImageSkia& image,
                                       const gfx::Vector2d& image_offset,
                                       const DragEventSourceInfo& event_info) {
}

}  // namespace content
