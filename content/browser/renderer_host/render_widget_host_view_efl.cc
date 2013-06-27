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

#include <Ecore_X.h>

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
#include "content/browser/accessibility/browser_accessibility_gtk.h"
#include "content/browser/accessibility/browser_accessibility_manager_gtk.h"
#include "content/browser/renderer_host/backing_store_gtk.h"
#include "content/browser/renderer_host/render_view_host_delegate.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/renderer_host/window_utils_efl.h"
#include "content/common/edit_command.h"
#include "content/common/gpu/gpu_messages.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/common/content_switches.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebScreenInfo.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/gtk/WebInputEventFactory.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/x11/WebScreenInfoFactory.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/base/gtk/gtk_compat.h"
#include "ui/base/text/text_elider.h"
#include "ui/base/x/active_window_watcher_x.h"
#include "ui/base/x/x11_util.h"
#include "ui/gfx/preserve_window_efl.h"
#include "webkit/glue/webcursor_gtk_data.h"
#include "webkit/plugins/npapi/webplugin.h"

using WebKit::WebInputEventFactory;
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

const GdkColor kBGColor =
#if defined(NDEBUG)
    { 0, 0xff * 257, 0xff * 257, 0xff * 257 };
#else
    { 0, 0x00 * 257, 0xff * 257, 0x00 * 257 };
#endif

// Returns the spinning cursor used for loading state.
GdkCursor* GetMozSpinningCursor() {
  static GdkCursor* moz_spinning_cursor = NULL;
  if (!moz_spinning_cursor) {
    const GdkColor fg = { 0, 0, 0, 0 };
    const GdkColor bg = { 65535, 65535, 65535, 65535 };
    GdkPixmap* source = gdk_bitmap_create_from_data(
        NULL, reinterpret_cast<const gchar*>(moz_spinning_bits), 32, 32);
    GdkPixmap* mask = gdk_bitmap_create_from_data(
        NULL, reinterpret_cast<const gchar*>(moz_spinning_mask_bits), 32, 32);
    moz_spinning_cursor =
        gdk_cursor_new_from_pixmap(source, mask, &fg, &bg, 2, 2);
    g_object_unref(source);
    g_object_unref(mask);
  }
  return moz_spinning_cursor;
}

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

bool RenderWidgetHostViewEfl::PreserveWindowMouseDown(Evas_Event_Mouse_Down* event)
{
	return false;
}

bool RenderWidgetHostViewEfl::PreserveWindowMouseUp(Evas_Event_Mouse_Up* event)
{
	return false;
}

bool RenderWidgetHostViewEfl::PreserveWindowMouseMove(Evas_Event_Mouse_Move* event)
{
	return false;
}

bool RenderWidgetHostViewEfl::PreserveWindowMouseWheel(Evas_Event_Mouse_Wheel* event)
{
	return false;
}

bool RenderWidgetHostViewEfl::PreserveWindowKeyDown(Evas_Event_Key_Down* event)
{
	return false;
}

bool RenderWidgetHostViewEfl::PreserveWindowKeyUp(Evas_Event_Key_Up* event)
{
	return false;
}

void RenderWidgetHostViewEfl::PreserveWindowFocusIn()
{
}

void RenderWidgetHostViewEfl::PreserveWindowFocusOut()
{
}

void RenderWidgetHostViewEfl::PreserveWindowShow()
{
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
   Evas* evas = evas_object_evas_get (elm_box);
   preserve_window_ = gfx::PreserveWindow::Create(this, evas);
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

//  DoSharedInit();
//  parent_ = parent_host_view->GetNativeView();
//  GtkWindow* window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_POPUP));
//  gtk_container_add(GTK_CONTAINER(window), view_);
//  DoPopupOrFullscreenInit(window, pos);
//
//  // Grab all input for the app. If a click lands outside the bounds of the
//  // popup, WebKit will notice and destroy us. The underlying X window needs to
//  // be created and mapped by the above code before we can grab the input
//  // devices.
//  if (NeedsInputGrab()) {
//    // If our parent is in a widget hierarchy that ends with a window, add
//    // ourselves to the same window group to make sure that our GTK grab
//    // covers it.
//    GtkWidget* toplevel = gtk_widget_get_toplevel(parent_);
//    if (toplevel &&
//        GTK_WIDGET_TOPLEVEL(toplevel) &&
//        GTK_IS_WINDOW(toplevel)) {
//      gtk_window_group_add_window(
//          gtk_window_get_group(GTK_WINDOW(toplevel)), window);
//    }
//
//    // Install an application-level GTK grab to make sure that we receive all of
//    // the app's input.
//    gtk_grab_add(view_);
//
//    // We need to install an X grab as well. However if the app already has an X
//    // grab (as in the case of extension popup), an app grab will suffice.
//    do_x_grab_ = !gdk_pointer_is_grabbed();
//    if (do_x_grab_) {
//      // Install the grab on behalf our parent window if it and all of its
//      // ancestors are mapped; otherwise, just use ourselves (maybe we're being
//      // shown on behalf of an inactive tab).
//      GdkWindow* grab_window = gtk_widget_get_window(parent_);
//      if (!grab_window || !gdk_window_is_viewable(grab_window))
//        grab_window = gtk_widget_get_window(view_);
//
//      gdk_pointer_grab(
//          grab_window,
//          TRUE,  // Only events outside of the window are reported with
//                 // respect to |parent_->window|.
//          static_cast<GdkEventMask>(GDK_BUTTON_PRESS_MASK |
//              GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK),
//          NULL,
//          NULL,
//          GDK_CURRENT_TIME);
//      // We grab keyboard events too so things like alt+tab are eaten.
//      gdk_keyboard_grab(grab_window, TRUE, GDK_CURRENT_TIME);
//    }
//  }
}

void RenderWidgetHostViewEfl::InitAsFullscreen(
    RenderWidgetHostView* reference_host_view) {
  DCHECK(reference_host_view);
//  DoSharedInit();
//
//  is_fullscreen_ = true;
//  GtkWindow* window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
//  gtk_window_set_decorated(window, FALSE);
//  destroy_handler_id_ = g_signal_connect(GTK_WIDGET(window),
//                                         "destroy",
//                                         G_CALLBACK(OnDestroyThunk),
//                                         this);
//  gtk_container_add(GTK_CONTAINER(window), view_);
//
//  // Try to move and resize the window to cover the screen in case the window
//  // manager doesn't support _NET_WM_STATE_FULLSCREEN.
//  GdkScreen* screen = gtk_window_get_screen(window);
//  GdkWindow* ref_gdk_window = gtk_widget_get_window(
//      reference_host_view->GetNativeView());
//
//  gfx::Rect bounds;
//  if (ref_gdk_window) {
//    const int monitor_id = gdk_screen_get_monitor_at_window(screen,
//                                                            ref_gdk_window);
//    GdkRectangle monitor_rect;
//    gdk_screen_get_monitor_geometry(screen, monitor_id, &monitor_rect);
//    bounds = gfx::Rect(monitor_rect);
//  } else {
//    bounds = gfx::Rect(
//        0, 0, gdk_screen_get_width(screen), gdk_screen_get_height(screen));
//  }
//  gtk_window_move(window, bounds.x(), bounds.y());
//  gtk_window_resize(window, bounds.width(), bounds.height());
//  gtk_window_fullscreen(window);
//  DoPopupOrFullscreenInit(window, bounds);
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

void RenderWidgetHostViewEfl::ActiveWindowChanged(GdkWindow* window) {
/*  GdkWindow* our_window = gtk_widget_get_parent_window(view_);

  if (our_window == window)
    made_active_ = true;

  // If the window was previously active, but isn't active anymore, shut it
  // down.
  if (is_fullscreen_ && our_window != window && made_active_)
    host_->Shutdown();*/
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
//  GdkWindow* gdk_window = gtk_widget_get_window(view_);
//  if (!gdk_window)
//    return gfx::Rect(requested_size_);
//  GdkRectangle window_rect;
//  gdk_window_get_origin(gdk_window, &window_rect.x, &window_rect.y);
//  return gfx::Rect(window_rect.x, window_rect.y,
//                   requested_size_.width(), requested_size_.height());
}

void RenderWidgetHostViewEfl::UpdateCursor(const WebCursor& cursor) {
  // Optimize the common case, where the cursor hasn't changed.
  // However, we can switch between different pixmaps, so only on the
  // non-pixmap branch.
  if (current_cursor_.GetCursorType() != GDK_CURSOR_IS_PIXMAP &&
      current_cursor_.GetCursorType() == cursor.GetCursorType()) {
    return;
  }

  current_cursor_ = cursor;
  ShowCurrentCursor();
}

void RenderWidgetHostViewEfl::SetIsLoading(bool is_loading) {
  is_loading_ = is_loading;
  // Only call ShowCurrentCursor() when it will actually change the cursor.
  if (current_cursor_.GetCursorType() == GDK_LAST_CURSOR)
    ShowCurrentCursor();
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
  if (compositing_surface_ != gfx::kNullPluginWindow) {
//    GtkNativeViewManager* manager = GtkNativeViewManager::GetInstance();
//    manager->ReleasePermanentXID(compositing_surface_);
  }

  if (do_x_grab_) {
    // Undo the X grab.
//    GdkDisplay* display = gtk_widget_get_display(parent_);
//    gdk_display_pointer_ungrab(display, GDK_CURRENT_TIME);
//    gdk_display_keyboard_ungrab(display, GDK_CURRENT_TIME);
  }

  if (preserve_window_->SmartObject()) {
    // If this is a popup or fullscreen widget, then we need to destroy the
    // window that we created to hold it.
    if (IsPopup() || is_fullscreen_) {
      // GtkWidget* window = gtk_widget_get_parent(view_);

      ui::ActiveWindowWatcherX::RemoveObserver(this);

      // Disconnect the destroy handler so that we don't try to shutdown twice.
//      if (is_fullscreen_)
//        g_signal_handler_disconnect(window, destroy_handler_id_);
//
//      gtk_widget_destroy(window);
    }

    // Remove |view_| from all containers now, so nothing else can hold a
    // reference to |view_|'s widget except possibly a gtk signal handler if
    // this code is currently executing within the context of a gtk signal
    // handler.  Note that |view_| is still alive after this call.  It will be
    // deallocated in the destructor.
    // See http://crbug.com/11847 for details.
//    gtk_widget_destroy(view_);
  }

  // The RenderWidgetHost's destruction led here, so don't call it.
  host_ = NULL;

  MessageLoop::current()->DeleteSoon(FROM_HERE, this);
}

void RenderWidgetHostViewEfl::SetTooltipText(const string16& tooltip_text) {
/*  // Maximum number of characters we allow in a tooltip.
  const int kMaxTooltipLength = 8 << 10;
  // Clamp the tooltip length to kMaxTooltipLength so that we don't
  // accidentally DOS the user with a mega tooltip (since GTK doesn't do
  // this itself).
  // I filed https://bugzilla.gnome.org/show_bug.cgi?id=604641 upstream.
  const string16 clamped_tooltip =
      ui::TruncateString(tooltip_text, kMaxTooltipLength);

  if (clamped_tooltip.empty()) {
    gtk_widget_set_has_tooltip(view_, FALSE);
  } else {
    gtk_widget_set_tooltip_text(view_,
                                UTF16ToUTF8(clamped_tooltip).c_str());
  }*/
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
//  im_context_->UpdateCaretBounds(
//      gfx::UnionRects(params.anchor_rect, params.focus_rect));
}

void RenderWidgetHostViewEfl::ScrollOffsetChanged() {
}

GdkEventButton* RenderWidgetHostViewEfl::GetLastMouseDown() {
//  return last_mouse_down_;
  return 0;
}

gfx::NativeView RenderWidgetHostViewEfl::BuildInputMethodsGtkMenu() {
  // return im_context_->BuildInputMethodsGtkMenu();
  return 0;
}

// TODO: Where did this original OnDestroy get declared?
//void RenderWidgetHostViewEfl::OnDestroy(GtkWidget* widget) {
//  DCHECK(is_fullscreen_);
//  host_->Shutdown();
//}

bool RenderWidgetHostViewEfl::NeedsInputGrab() {
  return popup_type_ == WebKit::WebPopupTypeSelect;
}

bool RenderWidgetHostViewEfl::IsPopup() const {
  return popup_type_ != WebKit::WebPopupTypeNone;
}

void RenderWidgetHostViewEfl::DoSharedInit(Evas_Object* parent) {
/*	  view_ = evas_object_rectangle_add(evas_object_evas_get(parent));
	  evas_object_color_set(view_, 0, 255, 0, 255);
	  evas_object_size_hint_weight_set(view_, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	  evas_object_show(view_);*/

  // TODO: (Important) Assign view_ with a newly created EFL widget here.
  // view_.Own(RenderWidgetHostViewEflEvasObject::CreateNewWidget(this));

  // TODO: Find EFL alternatives for these
  // im_context_.reset(new GtkIMContextWrapper(this));
  // plugin_container_manager_.set_host_widget(view_);
  // key_bindings_handler_.reset(new GtkKeyBindingsHandler(view_));
}

void RenderWidgetHostViewEfl::DoPopupOrFullscreenInit(GtkWindow* window,
                                                      const gfx::Rect& bounds) {
/*  requested_size_.SetSize(std::min(bounds.width(), kMaxWindowWidth),
                          std::min(bounds.height(), kMaxWindowHeight));
  host_->WasResized();

  ui::ActiveWindowWatcherX::AddObserver(this);

  // Don't set the size when we're going fullscreen. This can confuse the
  // window manager into thinking we're resizing a fullscreen window and
  // therefore not fullscreen anymore.
  if (!is_fullscreen_) {
    gtk_widget_set_size_request(
        view_, requested_size_.width(), requested_size_.height());

    // Don't allow the window to be resized. This also forces the window to
    // shrink down to the size of its child contents.
    gtk_window_set_resizable(window, FALSE);
    gtk_window_set_default_size(window, -1, -1);
    gtk_window_move(window, bounds.x(), bounds.y());
  }

  gtk_widget_show_all(GTK_WIDGET(window));*/
}

BackingStore* RenderWidgetHostViewEfl::AllocBackingStore(
    const gfx::Size& size) {

  Ecore_X_Display* display = ecore_x_display_get();
  Ecore_X_Screen* screen = ecore_x_default_screen_get();
  int depth = ecore_x_default_depth_get(display, screen);
  Ecore_X_Visual visual = ecore_x_default_visual_get(display, screen);

  return new BackingStoreGtk(host_, size, visual, depth);
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

void RenderWidgetHostViewEfl::ModifyEventForEdgeDragging(
    GtkWidget* widget, GdkEventMotion* event) {
  // If the widget is aligned with an edge of the monitor its on and the user
  // attempts to drag past that edge we track the number of times it has
  // occurred, so that we can force the widget to scroll when it otherwise
  // would be unable to, by modifying the (x,y) position in the drag
  // event that we forward on to webkit. If we get a move that's no longer a
  // drag or a drag indicating the user is no longer at that edge we stop
  // altering the drag events.
  int new_dragged_at_horizontal_edge = 0;
  int new_dragged_at_vertical_edge = 0;
  // Used for checking the edges of the monitor. We cache the values to save
  // roundtrips to the X server.
  CR_DEFINE_STATIC_LOCAL(gfx::Size, drag_monitor_size, ());
  if (event->state & GDK_BUTTON1_MASK) {
    if (drag_monitor_size.IsEmpty()) {
      // We can safely cache the monitor size for the duration of a drag.
      GdkScreen* screen = gtk_widget_get_screen(widget);
      int monitor =
          gdk_screen_get_monitor_at_point(screen, event->x_root, event->y_root);
      GdkRectangle geometry;
      gdk_screen_get_monitor_geometry(screen, monitor, &geometry);
      drag_monitor_size.SetSize(geometry.width, geometry.height);
    }
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    // Check X and Y independently, as the user could be dragging into a corner.
    if (event->x == 0 && event->x_root == 0) {
      new_dragged_at_horizontal_edge = dragged_at_horizontal_edge_ - 1;
    } else if (allocation.width - 1 == static_cast<gint>(event->x) &&
        drag_monitor_size.width() - 1 == static_cast<gint>(event->x_root)) {
      new_dragged_at_horizontal_edge = dragged_at_horizontal_edge_ + 1;
    }

    if (event->y == 0 && event->y_root == 0) {
      new_dragged_at_vertical_edge = dragged_at_vertical_edge_ - 1;
    } else if (allocation.height - 1 == static_cast<gint>(event->y) &&
        drag_monitor_size.height() - 1 == static_cast<gint>(event->y_root)) {
      new_dragged_at_vertical_edge = dragged_at_vertical_edge_ + 1;
    }

    event->x_root += new_dragged_at_horizontal_edge;
    event->x += new_dragged_at_horizontal_edge;
    event->y_root += new_dragged_at_vertical_edge;
    event->y += new_dragged_at_vertical_edge;
  } else {
    // Clear whenever we get a non-drag mouse move.
    drag_monitor_size.SetSize(0, 0);
  }
  dragged_at_horizontal_edge_ = new_dragged_at_horizontal_edge;
  dragged_at_vertical_edge_ = new_dragged_at_vertical_edge;
}

void RenderWidgetHostViewEfl::Paint(const gfx::Rect& damage_rect) {

  // If the GPU process is rendering directly into the View,
  // call the compositor directly.
  RenderWidgetHostImpl* render_widget_host =
      RenderWidgetHostImpl::From(GetRenderWidgetHost());
  if (render_widget_host->is_accelerated_compositing_active()) {
    host_->ScheduleComposite();
    return;
  }

//  GdkWindow* window = gtk_widget_get_window(view_);

  //DCHECK(!about_to_validate_and_paint_);

  invalid_rect_ = damage_rect;
  about_to_validate_and_paint_ = true;


  // If the size of our canvas is (0,0), then we don't want to block here. We
  // are doing one of our first paints and probably have animations going on.
  bool force_create = !host_->empty();
  BackingStoreGtk* backing_store = static_cast<BackingStoreGtk*>(
      host_->GetBackingStore(force_create));
  // Calling GetBackingStore maybe have changed |invalid_rect_|...
  about_to_validate_and_paint_ = false;

  gfx::Rect paint_rect = gfx::Rect(0, 0, kMaxWindowWidth, kMaxWindowHeight);
  paint_rect.Intersect(invalid_rect_);

  if (backing_store) {
    // Only render the widget if it is attached to a window; there's a short
    // period where this object isn't attached to a window but hasn't been
    // Destroy()ed yet and it receives paint messages...
//    if (window) {
      fprintf(stderr, "ahh %u %u \n", elm_win_xwindow_get(preserve_window_->EvasWindow()), elm_win_xwindow_get(preserve_window_->SmartObject()));
      backing_store->XShowRect(gfx::Point(0, 0),
          paint_rect, elm_win_xwindow_get(preserve_window_->EvasWindow()));
//    }
    if (!whiteout_start_time_.is_null()) {
      base::TimeDelta whiteout_duration = base::TimeTicks::Now() -
          whiteout_start_time_;
      UMA_HISTOGRAM_TIMES("MPArch.RWHH_WhiteoutDuration", whiteout_duration);

      // Reset the start time to 0 so that we start recording again the next
      // time the backing store is NULL...
      whiteout_start_time_ = base::TimeTicks();
    }
    if (!web_contents_switch_paint_time_.is_null()) {
      base::TimeDelta web_contents_switch_paint_duration =
          base::TimeTicks::Now() - web_contents_switch_paint_time_;
      UMA_HISTOGRAM_TIMES("MPArch.RWH_TabSwitchPaintDuration",
          web_contents_switch_paint_duration);
      // Reset web_contents_switch_paint_time_ to 0 so future tab selections are
      // recorded.
      web_contents_switch_paint_time_ = base::TimeTicks();
    }
  } else {
//    if (window)
//      gdk_window_clear(window);
    if (whiteout_start_time_.is_null())
      whiteout_start_time_ = base::TimeTicks::Now();
  }
}

void RenderWidgetHostViewEfl::ShowCurrentCursor() {
//  // The widget may not have a window. If that's the case, abort mission. This
//  // is the same issue as that explained above in Paint().
//  if (!gtk_widget_get_window(view_))
//    return;
//
//  // TODO(port): WebKit bug https://bugs.webkit.org/show_bug.cgi?id=16388 is
//  // that calling gdk_window_set_cursor repeatedly is expensive.  We should
//  // avoid it here where possible.
//  GdkCursor* gdk_cursor;
//  if (current_cursor_.GetCursorType() == GDK_LAST_CURSOR) {
//    // Use MOZ_CURSOR_SPINNING if we are showing the default cursor and
//    // the page is loading.
//    gdk_cursor = is_loading_ ? GetMozSpinningCursor() : NULL;
//  } else {
//    gdk_cursor = current_cursor_.GetNativeCursor();
//  }
//  gdk_window_set_cursor(gtk_widget_get_window(view_), gdk_cursor);
}

void RenderWidgetHostViewEfl::SetHasHorizontalScrollbar(
    bool has_horizontal_scrollbar) {
}

void RenderWidgetHostViewEfl::SetScrollOffsetPinning(
    bool is_pinned_to_left, bool is_pinned_to_right) {
}


void RenderWidgetHostViewEfl::OnAcceleratedCompositingStateChange() {
//  bool activated = host_->is_accelerated_compositing_active();

  // TODO: Replace with dshwang's EflPreserveWindow?
//  GtkPreserveWindow* widget = reinterpret_cast<GtkPreserveWindow*>(view_);
//
//  gtk_preserve_window_delegate_resize(widget, activated);
}

void RenderWidgetHostViewEfl::GetScreenInfo(WebScreenInfo* results) {

	content::GetScreenInfoEfl(results);
//  GdkWindow* gdk_window = gtk_widget_get_window(view_);
//  if (!gdk_window) {
//    GdkDisplay* display = gdk_display_get_default();
//    gdk_window = gdk_display_get_default_group(display);
//  }
//  if (!gdk_window)
//    return;
//  GetScreenInfoFromNativeWindow(gdk_window, results);
}

gfx::Rect RenderWidgetHostViewEfl::GetBoundsInRootWindow() {
//  return gfx::Rect();
    return GetViewBounds();
//  GtkWidget* toplevel = gtk_widget_get_toplevel(view_);
//  if (!toplevel)
//    return GetViewBounds();
//
//  GdkRectangle frame_extents;
//  GdkWindow* gdk_window = gtk_widget_get_window(toplevel);
//  if (!gdk_window)
//    return GetViewBounds();
//
//  gdk_window_get_frame_extents(gdk_window, &frame_extents);
//  return gfx::Rect(frame_extents.x, frame_extents.y,
//                   frame_extents.width, frame_extents.height);
}

gfx::GLSurfaceHandle RenderWidgetHostViewEfl::GetCompositingSurface() {
//  if (compositing_surface_ == gfx::kNullPluginWindow) {
//    GtkNativeViewManager* manager = GtkNativeViewManager::GetInstance();
//    gfx::NativeViewId view_id = GetNativeViewId();
//
//    if (!manager->GetPermanentXIDForId(&compositing_surface_, view_id)) {
//      DLOG(ERROR) << "Can't find XID for view id " << view_id;
//    }
//  }
  return gfx::GLSurfaceHandle(compositing_surface_, gfx::NATIVE_TRANSPORT);
}

bool RenderWidgetHostViewEfl::LockMouse() {


  return false;
}

void RenderWidgetHostViewEfl::UnlockMouse() {
}

void RenderWidgetHostViewEfl::ForwardKeyboardEvent(
    const NativeWebKeyboardEvent& event) {
//  if (!host_)
//    return;
//
//  EditCommands edit_commands;
//  if (!event.skip_in_browser &&
//      key_bindings_handler_->Match(event, &edit_commands)) {
//    Send(new ViewMsg_SetEditCommandsForNextKeyEvent(
//        host_->GetRoutingID(), edit_commands));
//    NativeWebKeyboardEvent copy_event(event);
//    copy_event.match_edit_command = true;
//    host_->ForwardKeyboardEvent(copy_event);
//    return;
//  }
//
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
//  if (widget_center_valid_)
//    return widget_center_;
//
//  GdkWindow* window = gtk_widget_get_window(view_);
//  gint window_x = 0;
//  gint window_y = 0;
//  gdk_window_get_origin(window, &window_x, &window_y);
//  gint window_w = gdk_window_get_width(window);
//  gint window_h = gdk_window_get_height(window);
//  widget_center_.SetPoint(window_x + window_w / 2,
//                          window_y + window_h / 2);
//  widget_center_valid_ = true;
//  return widget_center_;
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
//  GdkWindow* gdk_window =
//      gdk_display_get_default_group(gdk_display_get_default());
//  GetScreenInfoFromNativeWindow(gdk_window, results);
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
    const std::vector<AccessibilityHostMsg_NotificationParams>& params) {
/*  if (!browser_accessibility_manager_) {
    GtkWidget* parent = gtk_widget_get_parent(view_);
    browser_accessibility_manager_.reset(
        new BrowserAccessibilityManagerGtk(
            parent,
            BrowserAccessibilityManagerGtk::GetEmptyDocument(),
            this));
  }
  browser_accessibility_manager_->OnAccessibilityNotifications(params);*/
}

AtkObject* RenderWidgetHostViewEfl::GetAccessible() {
//  if (!browser_accessibility_manager_) {
//    GtkWidget* parent = gtk_widget_get_parent(view_);
//    browser_accessibility_manager_.reset(
//        new BrowserAccessibilityManagerGtk(
//            parent,
//            BrowserAccessibilityManagerGtk::GetEmptyDocument(),
//            this));
//  }
//  BrowserAccessibilityGtk* root =
//      browser_accessibility_manager_->GetRoot()->ToBrowserAccessibilityGtk();
//
//  atk_object_set_role(root->GetAtkObject(), ATK_ROLE_HTML_CONTAINER);
//  return root->GetAtkObject();
  return 0;
}

void RenderWidgetHostViewEfl::OnCreatePluginContainer(
    gfx::PluginWindowHandle id) {
  // plugin_container_manager_.CreatePluginContainer(id);
}

void RenderWidgetHostViewEfl::OnDestroyPluginContainer(
    gfx::PluginWindowHandle id) {
  // plugin_container_manager_.DestroyPluginContainer(id);
}

}  // namespace content
