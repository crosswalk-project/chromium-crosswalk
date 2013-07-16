// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_widget_host_view_efl.h"

#include <Elementary.h>

// If this gets included after the gtk headers, then a bunch of compiler
// errors happen because of a "#define Status int" in Xlib.h, which interacts
// badly with net::URLRequestStatus::Status.
#include "content/common/view_messages.h"

#include <cairo/cairo.h>

#include <algorithm>
#include <string>

#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/debug/trace_event.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/metrics/histogram.h"
#include "base/string_number_conversions.h"
#include "base/strings/utf_offset_string_conversions.h"
#include "base/time.h"
#include "base/utf_string_conversions.h"
#include "content/browser/renderer_host/render_view_host_delegate.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/renderer_host/window_utils_efl.h"
#include "content/browser/renderer_host/web_input_event_factory_efl.h"
#include "content/common/edit_command.h"
#include "content/common/gpu/gpu_messages.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/common/content_switches.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebScreenInfo.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/x11/WebScreenInfoFactory.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/base/text/text_elider.h"
#include "ui/base/x/active_window_watcher_x.h"
#include "ui/base/x/x11_util.h"
#include "ui/gfx/efl_event.h"
#include "ui/gfx/preserve_window_efl.h"
#include "webkit/plugins/npapi/webplugin.h"

using WebKit::WebMouseWheelEvent;
using WebKit::WebScreenInfo;

namespace content {

namespace {
// Paint rects on Linux are bounded by the maximum size of a shared memory
// region. By default that's 32MB, but many distros increase it significantly
// (i.e. to 256MB).
//
// We fetch the maximum value from /proc/sys/kernel/shmmax at runtime and, if
// we exceed that, then we limit the height of the paint rect in the renderer.
//
// These constants are here to ensure that, in the event that we exceed it, we
// end up with something a little more square. Previously we had 4000x4000, but
// people's monitor setups are actually exceeding that these days.
const int kMaxWindowWidth = 10000;
const int kMaxWindowHeight = 10000;

// See WebInputEventFactor.cpp for a reason for this being the default
// scroll size for linux.
const float kDefaultScrollPixelsPerTick = 160.0f / 3.0f;

bool MovedToPoint(const WebKit::WebMouseEvent& mouse_event,
                   const gfx::Point& center) {
  return mouse_event.globalX == center.x() &&
         mouse_event.globalY == center.y();
}

}  // namespace

RenderWidgetHostViewEfl::RenderWidgetHostViewEfl(RenderWidgetHost* widget_host)
    : host_(RenderWidgetHostImpl::From(widget_host)),
      about_to_validate_and_paint_(false),
      is_hidden_(false),
      is_loading_(false),
      parent_(NULL),
      is_popup_first_mouse_release_(true),
      was_imcontext_focused_before_grab_(false),
      do_x_grab_(false),
      is_fullscreen_(false),
      made_active_(false),
      mouse_is_being_warped_to_unlocked_position_(false),
      destroy_handler_id_(0),
      dragged_at_horizontal_edge_(0),
      dragged_at_vertical_edge_(0),
      compositing_surface_(gfx::kNullPluginWindow),
      preserve_window_(0) {
  host_->SetView(this);
}

RenderWidgetHostViewEfl::~RenderWidgetHostViewEfl() {
  UnlockMouse();
 // view_.Destroy();
}

void RenderWidgetHostViewEfl::PreserveWindowMouseDown(Evas_Event_Mouse_Down* mouse_down)
{
  host_->ForwardMouseEvent(content::mouseEvent(mouse_down));
}

void RenderWidgetHostViewEfl::PreserveWindowMouseUp(Evas_Event_Mouse_Up* mouse_up)
{
  host_->ForwardMouseEvent(content::mouseEvent(mouse_up));
}

void RenderWidgetHostViewEfl::PreserveWindowMouseMove(Evas_Event_Mouse_Move* mouse_move)
{
  host_->ForwardMouseEvent(content::mouseEvent(mouse_move));
}

void RenderWidgetHostViewEfl::PreserveWindowMouseWheel(Evas_Event_Mouse_Wheel* mouse_wheel)
{
  host_->ForwardWheelEvent(content::mouseWheelEvent(mouse_wheel));
}

void RenderWidgetHostViewEfl::PreserveWindowKeyDown(Evas_Event_Key_Down* key_down)
{
  gfx::EflEvent event = gfx::EflEvent(gfx::EflEvent::EventTypeKeyDown,
                                           key_down);
  host_->ForwardKeyboardEvent(NativeWebKeyboardEvent(&event));
}

void RenderWidgetHostViewEfl::PreserveWindowKeyUp(Evas_Event_Key_Up* key_up)
{
  gfx::EflEvent event = gfx::EflEvent(gfx::EflEvent::EventTypeKeyUp,
                                           key_up);
  host_->ForwardKeyboardEvent(NativeWebKeyboardEvent(&event));
}

void RenderWidgetHostViewEfl::PreserveWindowFocusIn()
{
  ShowCurrentCursor();
  if (!host_)
      return;

  host_->GotFocus();
  host_->SetActive(true);
}

void RenderWidgetHostViewEfl::PreserveWindowFocusOut()
{
  if (host_ && !IsShowingContextMenu()) {
    host_->SetActive(false);
    host_->Blur();
  }
}

void RenderWidgetHostViewEfl::PreserveWindowShow()
{
//  root_window_host_delegate_->OnHostActivated();
}

void RenderWidgetHostViewEfl::PreserveWindowHide()
{
}

void RenderWidgetHostViewEfl::PreserveWindowMove(const gfx::Point& origin)
{
}

void RenderWidgetHostViewEfl::PreserveWindowResize(const gfx::Size& size)
{
    SetSize(size);
}

void RenderWidgetHostViewEfl::PreserveWindowRepaint(const gfx::Rect& damage_rect)
{
    Paint(damage_rect);
}

bool RenderWidgetHostViewEfl::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(RenderWidgetHostViewEfl, message)
    IPC_MESSAGE_HANDLER(ViewHostMsg_CreatePluginContainer,
                        OnCreatePluginContainer)
    IPC_MESSAGE_HANDLER(ViewHostMsg_DestroyPluginContainer,
                        OnDestroyPluginContainer)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void RenderWidgetHostViewEfl::InitAsChild(
    gfx::NativeView parent_view) {
   Evas_Object* elm_box = reinterpret_cast<Evas_Object*>(parent_view);
   preserve_window_ = gfx::PreserveWindow::Create(this, elm_box);
   evas_object_size_hint_align_set(preserve_window_->SmartObject(), EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(preserve_window_->SmartObject(), EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(elm_box, preserve_window_->SmartObject());
   evas_object_show(preserve_window_->SmartObject());
   compositing_surface_ = elm_win_xwindow_get(preserve_window_->EvasWindow());
}

void RenderWidgetHostViewEfl::InitAsPopup(
    RenderWidgetHostView* parent_host_view, const gfx::Rect& pos) {
  // If we aren't a popup, then |window| will be leaked.
  DCHECK(IsPopup());
}

void RenderWidgetHostViewEfl::InitAsFullscreen(
    RenderWidgetHostView* reference_host_view) {
  DCHECK(reference_host_view);
}

RenderWidgetHost* RenderWidgetHostViewEfl::GetRenderWidgetHost() const {
  return host_;
}

void RenderWidgetHostViewEfl::WasShown() {
  if (!is_hidden_)
    return;

  if (web_contents_switch_paint_time_.is_null())
    web_contents_switch_paint_time_ = base::TimeTicks::Now();
  is_hidden_ = false;
  host_->WasShown();
}

void RenderWidgetHostViewEfl::WasHidden() {
  if (is_hidden_)
    return;

  // If we receive any more paint messages while we are hidden, we want to
  // ignore them so we don't re-allocate the backing store.  We will paint
  // everything again when we become selected again.
  is_hidden_ = true;

  // If we have a renderer, then inform it that we are being hidden so it can
  // reduce its resource utilization.
  host_->WasHidden();

  web_contents_switch_paint_time_ = base::TimeTicks();
}

void RenderWidgetHostViewEfl::SetSize(const gfx::Size& size) {
  int width = std::min(size.width(), kMaxWindowWidth);
  int height = std::min(size.height(), kMaxWindowHeight);
  if (IsPopup()) {
    // We're a popup, honor the size request.
    // gtk_widget_set_size_request(view_, width, height);
  }

  // Update the size of the RWH.
  if (requested_size_.width() != width ||
      requested_size_.height() != height) {
    requested_size_ = gfx::Size(width, height);
    host_->SendScreenRects();
    host_->WasResized();
  }
}

void RenderWidgetHostViewEfl::SetBounds(const gfx::Rect& rect) {
  // TODO: Implement popup moving.
//  // This is called when webkit has sent us a Move message.
//  if (IsPopup()) {
//    gtk_window_move(GTK_WINDOW(gtk_widget_get_toplevel(view_)),
//                    rect.x(), rect.y());
//  }

  SetSize(rect.size());
}

gfx::NativeView RenderWidgetHostViewEfl::GetNativeView() const {
  return reinterpret_cast<gfx::NativeView>(preserve_window_->SmartObject());
}

gfx::NativeViewId RenderWidgetHostViewEfl::GetNativeViewId() const {
  // TODO: Will we need an equivalent for this one?
  // return GtkNativeViewManager::GetInstance()->GetIdForWidget(view_);
  return 0;
}

gfx::NativeViewAccessible RenderWidgetHostViewEfl::GetNativeViewAccessible() {
  NOTIMPLEMENTED();
  return NULL;
}

void RenderWidgetHostViewEfl::MovePluginWindows(
    const gfx::Vector2d& scroll_offset,
    const std::vector<webkit::npapi::WebPluginGeometry>& moves) {
//  for (size_t i = 0; i < moves.size(); ++i) {
//    plugin_container_manager_.MovePluginContainer(moves[i]);
//  }
}

void RenderWidgetHostViewEfl::Focus() {
  // gtk_widget_grab_focus(view_);
}

void RenderWidgetHostViewEfl::Blur() {
  // TODO(estade): We should be clearing native focus as well, but I know of no
  // way to do that without focusing another widget.
  host_->Blur();
}

bool RenderWidgetHostViewEfl::HasFocus() const {
  return true;
  // gtk_widget_has_focus(view_);
}

bool RenderWidgetHostViewEfl::Send(IPC::Message* message) {
  return host_->Send(message);
}

bool RenderWidgetHostViewEfl::IsSurfaceAvailableForCopy() const {
  return true;
}

void RenderWidgetHostViewEfl::Show() {
  // gtk_widget_show(view_);
}

void RenderWidgetHostViewEfl::Hide() {
  // gtk_widget_hide(view_);
}

bool RenderWidgetHostViewEfl::IsShowing() {
  return true;
  // gtk_widget_get_visible(view_);
}

gfx::Rect RenderWidgetHostViewEfl::GetViewBounds() const {
  return gfx::Rect(requested_size_);
}

void RenderWidgetHostViewEfl::UpdateCursor(const WebCursor& cursor) {
  current_cursor_ = cursor;
  ShowCurrentCursor();
}

void RenderWidgetHostViewEfl::SetIsLoading(bool is_loading) {

}

void RenderWidgetHostViewEfl::TextInputStateChanged(
    const ViewHostMsg_TextInputState_Params& params) {
  // im_context_->UpdateInputMethodState(params.type, params.can_compose_inline);
}

void RenderWidgetHostViewEfl::ImeCancelComposition() {
  // im_context_->CancelComposition();
}

void RenderWidgetHostViewEfl::ImeCompositionRangeChanged(
    const ui::Range& range,
    const std::vector<gfx::Rect>& character_bounds) {
}

void RenderWidgetHostViewEfl::DidUpdateBackingStore(
    const gfx::Rect& scroll_rect,
    const gfx::Vector2d& scroll_delta,
    const std::vector<gfx::Rect>& copy_rects) {
  TRACE_EVENT0("ui::efl", "RenderWidgetHostViewEfl::DidUpdateBackingStore");

  if (is_hidden_)
    return;

  // TODO(darin): Implement the equivalent of Win32's ScrollWindowEX.  Can that
  // be done using XCopyArea?  Perhaps similar to
  // BackingStore::ScrollBackingStore?
  if (about_to_validate_and_paint_)
    invalid_rect_.Union(scroll_rect);
  else
    Paint(scroll_rect);

  for (size_t i = 0; i < copy_rects.size(); ++i) {
    // Avoid double painting.  NOTE: This is only relevant given the call to
    // Paint(scroll_rect) above.
    gfx::Rect rect = gfx::SubtractRects(copy_rects[i], scroll_rect);
    if (rect.IsEmpty())
      continue;

    if (about_to_validate_and_paint_)
      invalid_rect_.Union(rect);
    else
      Paint(rect);
  }
}

void RenderWidgetHostViewEfl::RenderViewGone(base::TerminationStatus status,
                                             int error_code) {
  Destroy();
  // plugin_container_manager_.set_host_widget(NULL);
}

void RenderWidgetHostViewEfl::Destroy() {
  // FIXME: Check what should be deleted.
  // The RenderWidgetHost's destruction led here, so don't call it.
  host_ = NULL;

  MessageLoop::current()->DeleteSoon(FROM_HERE, this);
}

void RenderWidgetHostViewEfl::SetTooltipText(const string16& tooltip_text) {
}

void RenderWidgetHostViewEfl::SelectionChanged(const string16& text,
                                               size_t offset,
                                               const ui::Range& range) {
  RenderWidgetHostViewBase::SelectionChanged(text, offset, range);

  if (text.empty() || range.is_empty())
    return;
  size_t pos = range.GetMin() - offset;
  size_t n = range.length();

  DCHECK(pos + n <= text.length()) << "The text can not fully cover range.";
  if (pos >= text.length()) {
    NOTREACHED() << "The text can not cover range.";
    return;
  }

  BrowserContext* browser_context = host_->GetProcess()->GetBrowserContext();
  // Set the BUFFER_SELECTION to the ui::Clipboard.
  ui::ScopedClipboardWriter clipboard_writer(
      ui::Clipboard::GetForCurrentThread(),
      ui::Clipboard::BUFFER_SELECTION,
      BrowserContext::GetMarkerForOffTheRecordContext(browser_context));
  clipboard_writer.WriteText(text.substr(pos, n));
}

void RenderWidgetHostViewEfl::SelectionBoundsChanged(
    const ViewHostMsg_SelectionBounds_Params& params) {
}

void RenderWidgetHostViewEfl::ScrollOffsetChanged() {
}

gfx::NativeView RenderWidgetHostViewEfl::BuildInputMethodsGtkMenu() {
  // return im_context_->BuildInputMethodsGtkMenu();
  return 0;
}

bool RenderWidgetHostViewEfl::NeedsInputGrab() {
  return popup_type_ == WebKit::WebPopupTypeSelect;
}

bool RenderWidgetHostViewEfl::IsPopup() const {
  return popup_type_ != WebKit::WebPopupTypeNone;
}

void RenderWidgetHostViewEfl::DoSharedInit(Evas_Object* parent) {
}

BackingStore* RenderWidgetHostViewEfl::AllocBackingStore(
    const gfx::Size& /*size*/) {

    return 0; // We're using accelerated path.
}

// NOTE: |output| is initialized with the size of |src_subrect|, and |dst_size|
// is ignored on GTK.
void RenderWidgetHostViewEfl::CopyFromCompositingSurface(
    const gfx::Rect& src_subrect,
    const gfx::Size& /* dst_size */,
    const base::Callback<void(bool, const SkBitmap&)>& callback) {
  // Grab the snapshot from the renderer as that's the only reliable way to
  // readback from the GPU for this platform right now.
  GetRenderWidgetHost()->GetSnapshotFromRenderer(src_subrect, callback);
}

void RenderWidgetHostViewEfl::CopyFromCompositingSurfaceToVideoFrame(
      const gfx::Rect& src_subrect,
      const scoped_refptr<media::VideoFrame>& target,
      const base::Callback<void(bool)>& callback) {
  NOTIMPLEMENTED();
  callback.Run(false);
}

bool RenderWidgetHostViewEfl::CanCopyToVideoFrame() const {
  return false;
}

void RenderWidgetHostViewEfl::AcceleratedSurfaceBuffersSwapped(
    const GpuHostMsg_AcceleratedSurfaceBuffersSwapped_Params& params,
    int gpu_host_id) {
   AcceleratedSurfaceMsg_BufferPresented_Params ack_params;
   ack_params.sync_point = 0;
   RenderWidgetHostImpl::AcknowledgeBufferPresent(
      params.route_id, gpu_host_id, ack_params);
}

void RenderWidgetHostViewEfl::AcceleratedSurfacePostSubBuffer(
    const GpuHostMsg_AcceleratedSurfacePostSubBuffer_Params& params,
    int gpu_host_id) {
   AcceleratedSurfaceMsg_BufferPresented_Params ack_params;
   ack_params.sync_point = 0;
   RenderWidgetHostImpl::AcknowledgeBufferPresent(
      params.route_id, gpu_host_id, ack_params);
}

void RenderWidgetHostViewEfl::AcceleratedSurfaceSuspend() {
}

void RenderWidgetHostViewEfl::AcceleratedSurfaceRelease() {
}

bool RenderWidgetHostViewEfl::HasAcceleratedSurface(
      const gfx::Size& desired_size) {
  // TODO(jbates) Implement this so this view can use GetBackingStore for both
  // software and GPU frames. Defaulting to false just makes GetBackingStore
  // only useable for software frames.
  return false;
}

void RenderWidgetHostViewEfl::SetBackground(const SkBitmap& background) {
  RenderWidgetHostViewBase::SetBackground(background);
  Send(new ViewMsg_SetBackground(host_->GetRoutingID(), background));
}

void RenderWidgetHostViewEfl::Paint(const gfx::Rect& /*damage_rect*/) {

  // If the GPU process is rendering directly into the View,
  // call the compositor directly.
  host_->ScheduleComposite();
}

void RenderWidgetHostViewEfl::ShowCurrentCursor() {
}

void RenderWidgetHostViewEfl::SetHasHorizontalScrollbar(
    bool has_horizontal_scrollbar) {
}

void RenderWidgetHostViewEfl::SetScrollOffsetPinning(
    bool is_pinned_to_left, bool is_pinned_to_right) {
}


void RenderWidgetHostViewEfl::OnAcceleratedCompositingStateChange() {
}

void RenderWidgetHostViewEfl::GetScreenInfo(WebScreenInfo* results) {
	content::GetScreenInfoEfl(results);
}

gfx::Rect RenderWidgetHostViewEfl::GetBoundsInRootWindow() {
    return GetViewBounds();
}

gfx::GLSurfaceHandle RenderWidgetHostViewEfl::GetCompositingSurface() {
  return gfx::GLSurfaceHandle(compositing_surface_, gfx::NATIVE_TRANSPORT);
}

bool RenderWidgetHostViewEfl::LockMouse() {


  return false;
}

void RenderWidgetHostViewEfl::UnlockMouse() {
}

void RenderWidgetHostViewEfl::ForwardKeyboardEvent(
    const NativeWebKeyboardEvent& event) {
//  host_->ForwardKeyboardEvent(event);
}

bool RenderWidgetHostViewEfl::RetrieveSurrounding(std::string* text,
                                                  size_t* cursor_index) {
  if (!selection_range_.IsValid())
    return false;

  size_t offset = selection_range_.GetMin() - selection_text_offset_;
  DCHECK(offset <= selection_text_.length());

  if (offset == selection_text_.length()) {
    *text = UTF16ToUTF8(selection_text_);
    *cursor_index = text->length();
    return true;
  }

  *text = base::UTF16ToUTF8AndAdjustOffset(
      base::StringPiece16(selection_text_), &offset);
  if (offset == string16::npos) {
    NOTREACHED() << "Invalid offset in UTF16 string.";
    return false;
  }
  *cursor_index = offset;
  return true;
}

void RenderWidgetHostViewEfl::MarkCachedWidgetCenterStale() {
  widget_center_valid_ = false;
  mouse_has_been_warped_to_new_center_ = false;
}

gfx::Point RenderWidgetHostViewEfl::GetWidgetCenter() {
  return gfx::Point();
}

void RenderWidgetHostViewEfl::ModifyEventMovementAndCoords(
    WebKit::WebMouseEvent* event) {
  // Movement is computed by taking the difference of the new cursor position
  // and the previous. Under mouse lock the cursor will be warped back to the
  // center so that we are not limited by clipping boundaries.
  // We do not measure movement as the delta from cursor to center because
  // we may receive more mouse movement events before our warp has taken
  // effect.
  event->movementX = event->globalX - global_mouse_position_.x();
  event->movementY = event->globalY - global_mouse_position_.y();

  // While the cursor is being warped back to the unlocked position, suppress
  // the movement member data.
  if (mouse_is_being_warped_to_unlocked_position_) {
    event->movementX = 0;
    event->movementY = 0;
    if (MovedToPoint(*event, unlocked_global_mouse_position_))
      mouse_is_being_warped_to_unlocked_position_ = false;
  }

  global_mouse_position_.SetPoint(event->globalX, event->globalY);

  // Under mouse lock, coordinates of mouse are locked to what they were when
  // mouse lock was entered.
  if (mouse_locked_) {
    event->x = unlocked_mouse_position_.x();
    event->y = unlocked_mouse_position_.y();
    event->windowX = unlocked_mouse_position_.x();
    event->windowY = unlocked_mouse_position_.y();
    event->globalX = unlocked_global_mouse_position_.x();
    event->globalY = unlocked_global_mouse_position_.y();
  } else {
    unlocked_mouse_position_.SetPoint(event->windowX, event->windowY);
    unlocked_global_mouse_position_.SetPoint(event->globalX, event->globalY);
  }
}

////////////////////////////////////////////////////////////////////////////////
// RenderWidgetHostView, public:

// static
RenderWidgetHostView* RenderWidgetHostView::CreateViewForWidget(
    RenderWidgetHost* widget) {
  return new RenderWidgetHostViewEfl(widget);
}

// static
void RenderWidgetHostViewPort::GetDefaultScreenInfo(WebScreenInfo* results) {
}

void RenderWidgetHostViewEfl::SetAccessibilityFocus(int acc_obj_id) {
  if (!host_)
    return;

  host_->AccessibilitySetFocus(acc_obj_id);
}

void RenderWidgetHostViewEfl::AccessibilityDoDefaultAction(int acc_obj_id) {
  if (!host_)
    return;

  host_->AccessibilityDoDefaultAction(acc_obj_id);
}

void RenderWidgetHostViewEfl::AccessibilityScrollToMakeVisible(
    int acc_obj_id, gfx::Rect subfocus) {
  if (!host_)
    return;

  host_->AccessibilityScrollToMakeVisible(acc_obj_id, subfocus);
}

void RenderWidgetHostViewEfl::AccessibilityScrollToPoint(
    int acc_obj_id, gfx::Point point) {
  if (!host_)
    return;

  host_->AccessibilityScrollToPoint(acc_obj_id, point);
}

void RenderWidgetHostViewEfl::AccessibilitySetTextSelection(
    int acc_obj_id, int start_offset, int end_offset) {
  if (!host_)
    return;

  host_->AccessibilitySetTextSelection(acc_obj_id, start_offset, end_offset);
}

gfx::Point RenderWidgetHostViewEfl::GetLastTouchEventLocation() const {
  // Not needed on Linux.
  return gfx::Point();
}

void RenderWidgetHostViewEfl::FatalAccessibilityTreeError() {
  if (host_) {
    host_->FatalAccessibilityTreeError();
    SetBrowserAccessibilityManager(NULL);
  } else {
    CHECK(FALSE);
  }
}

void RenderWidgetHostViewEfl::OnAccessibilityNotifications(
    const std::vector<AccessibilityHostMsg_NotificationParams>& /*params*/) {
}

void RenderWidgetHostViewEfl::OnCreatePluginContainer(
    gfx::PluginWindowHandle id) {
}

void RenderWidgetHostViewEfl::OnDestroyPluginContainer(
    gfx::PluginWindowHandle id) {
}

}  // namespace content
