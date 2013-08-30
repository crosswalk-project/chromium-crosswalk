// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_widget_host_view_android.h"

#include <android/bitmap.h>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/worker_pool.h"
#include "cc/layers/delegated_renderer_layer.h"
#include "cc/layers/layer.h"
#include "cc/layers/texture_layer.h"
#include "cc/output/compositor_frame.h"
#include "cc/output/compositor_frame_ack.h"
#include "content/browser/accessibility/browser_accessibility_manager_android.h"
#include "content/browser/android/content_view_core_impl.h"
#include "content/browser/android/in_process/synchronous_compositor_impl.h"
#include "content/browser/android/overscroll_glow.h"
#include "content/browser/gpu/gpu_surface_tracker.h"
#include "content/browser/renderer_host/compositor_impl_android.h"
#include "content/browser/renderer_host/image_transport_factory_android.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/surface_texture_transport_client_android.h"
#include "content/browser/renderer_host/touch_smooth_scroll_gesture_android.h"
#include "content/common/gpu/client/gl_helper_browser.h"
#include "content/common/gpu/gpu_messages.h"
#include "content/common/input_messages.h"
#include "content/common/view_messages.h"
#include "content/public/common/content_switches.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "third_party/khronos/GLES2/gl2ext.h"
#include "ui/gfx/android/device_display_info.h"
#include "ui/gfx/android/java_bitmap.h"
#include "ui/gfx/display.h"
#include "ui/gfx/screen.h"
#include "ui/gfx/size_conversions.h"

namespace content {

namespace {

void InsertSyncPointAndAckForGpu(
    int gpu_host_id, int route_id, const std::string& return_mailbox) {
  uint32 sync_point =
      ImageTransportFactoryAndroid::GetInstance()->InsertSyncPoint();
  AcceleratedSurfaceMsg_BufferPresented_Params ack_params;
  ack_params.mailbox_name = return_mailbox;
  ack_params.sync_point = sync_point;
  RenderWidgetHostImpl::AcknowledgeBufferPresent(
      route_id, gpu_host_id, ack_params);
}

void InsertSyncPointAndAckForCompositor(
    int renderer_host_id,
    int route_id,
    const gpu::Mailbox& return_mailbox,
    const gfx::Size return_size) {
  cc::CompositorFrameAck ack;
  ack.gl_frame_data.reset(new cc::GLFrameData());
  if (!return_mailbox.IsZero()) {
    ack.gl_frame_data->mailbox = return_mailbox;
    ack.gl_frame_data->size = return_size;
    ack.gl_frame_data->sync_point =
        ImageTransportFactoryAndroid::GetInstance()->InsertSyncPoint();
  }
  RenderWidgetHostImpl::SendSwapCompositorFrameAck(
      route_id, renderer_host_id, ack);
}

}  // anonymous namespace

RenderWidgetHostViewAndroid::RenderWidgetHostViewAndroid(
    RenderWidgetHostImpl* widget_host,
    ContentViewCoreImpl* content_view_core)
    : host_(widget_host),
      needs_begin_frame_(false),
      are_layers_attached_(true),
      content_view_core_(NULL),
      ime_adapter_android_(this),
      cached_background_color_(SK_ColorWHITE),
      texture_id_in_layer_(0),
      weak_ptr_factory_(this),
      overscroll_effect_enabled_(true) {
  if (CompositorImpl::UsesDirectGL()) {
    surface_texture_transport_.reset(new SurfaceTextureTransportClient());
    layer_ = surface_texture_transport_->Initialize();
    layer_->SetIsDrawable(true);
  } else {
    if (CommandLine::ForCurrentProcess()->HasSwitch(
            switches::kEnableDelegatedRenderer)) {
      delegated_renderer_layer_ = cc::DelegatedRendererLayer::Create(this);
      layer_ = delegated_renderer_layer_;
    } else {
      texture_layer_ = cc::TextureLayer::Create(this);
      layer_ = texture_layer_;
    }
  }

  layer_->SetContentsOpaque(true);

  overscroll_effect_enabled_ = !CommandLine::ForCurrentProcess()->
      HasSwitch(switches::kDisableOverscrollEdgeEffect);
  // Don't block the main thread with effect resource loading.
  // Actual effect creation is deferred until an overscroll event is received.
  if (overscroll_effect_enabled_) {
    base::WorkerPool::PostTask(FROM_HERE,
                               base::Bind(&OverscrollGlow::EnsureResources),
                               true);
  }

  host_->SetView(this);
  SetContentViewCore(content_view_core);
}

RenderWidgetHostViewAndroid::~RenderWidgetHostViewAndroid() {
  SetContentViewCore(NULL);
  DCHECK(ack_callbacks_.empty());
  if (texture_id_in_layer_) {
    ImageTransportFactoryAndroid::GetInstance()->DeleteTexture(
        texture_id_in_layer_);
  }

  if (texture_layer_.get())
    texture_layer_->ClearClient();
}


bool RenderWidgetHostViewAndroid::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(RenderWidgetHostViewAndroid, message)
    IPC_MESSAGE_HANDLER(ViewHostMsg_ImeBatchStateChanged_ACK,
                        OnProcessImeBatchStateAck)
    IPC_MESSAGE_HANDLER(ViewHostMsg_StartContentIntent, OnStartContentIntent)
    IPC_MESSAGE_HANDLER(ViewHostMsg_DidChangeBodyBackgroundColor,
                        OnDidChangeBodyBackgroundColor)
    IPC_MESSAGE_HANDLER(ViewHostMsg_SetNeedsBeginFrame,
                        OnSetNeedsBeginFrame)
    IPC_MESSAGE_HANDLER(ViewHostMsg_TextInputStateChanged,
                        OnTextInputStateChanged)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void RenderWidgetHostViewAndroid::InitAsChild(gfx::NativeView parent_view) {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewAndroid::InitAsPopup(
    RenderWidgetHostView* parent_host_view, const gfx::Rect& pos) {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewAndroid::InitAsFullscreen(
    RenderWidgetHostView* reference_host_view) {
  NOTIMPLEMENTED();
}

RenderWidgetHost*
RenderWidgetHostViewAndroid::GetRenderWidgetHost() const {
  return host_;
}

void RenderWidgetHostViewAndroid::WasShown() {
  if (!host_->is_hidden())
    return;

  host_->WasShown();
}

void RenderWidgetHostViewAndroid::WasHidden() {
  RunAckCallbacks();

  if (host_->is_hidden())
    return;

  // Inform the renderer that we are being hidden so it can reduce its resource
  // utilization.
  host_->WasHidden();
}

void RenderWidgetHostViewAndroid::WasResized() {
  if (surface_texture_transport_.get() && content_view_core_)
    surface_texture_transport_->SetSize(
        content_view_core_->GetPhysicalBackingSize());

  host_->WasResized();
}

void RenderWidgetHostViewAndroid::SetSize(const gfx::Size& size) {
  // Ignore the given size as only the Java code has the power to
  // resize the view on Android.
  WasResized();
}

void RenderWidgetHostViewAndroid::SetBounds(const gfx::Rect& rect) {
  SetSize(rect.size());
}

WebKit::WebGLId RenderWidgetHostViewAndroid::GetScaledContentTexture(
    float scale,
    gfx::Size* out_size) {
  gfx::Size size(gfx::ToCeiledSize(
      gfx::ScaleSize(texture_size_in_layer_, scale)));

  if (!CompositorImpl::IsInitialized() ||
      texture_id_in_layer_ == 0 ||
      texture_size_in_layer_.IsEmpty() ||
      size.IsEmpty()) {
    if (out_size)
        out_size->SetSize(0, 0);

    return 0;
  }

  if (out_size)
    *out_size = size;

  GLHelperBrowser* helper =
      ImageTransportFactoryAndroid::GetInstance()->GetGLHelperBrowser();
  return helper->CopyAndScaleTexture(texture_id_in_layer_,
                                     texture_size_in_layer_,
                                     size,
                                     true,
                                     GLHelperBrowser::SCALER_QUALITY_FAST);
}

bool RenderWidgetHostViewAndroid::PopulateBitmapWithContents(jobject jbitmap) {
  if (!CompositorImpl::IsInitialized() ||
      texture_id_in_layer_ == 0 ||
      texture_size_in_layer_.IsEmpty())
    return false;

  gfx::JavaBitmap bitmap(jbitmap);

  // TODO(dtrainor): Eventually add support for multiple formats here.
  DCHECK(bitmap.format() == ANDROID_BITMAP_FORMAT_RGBA_8888);

  GLHelperBrowser* helper =
      ImageTransportFactoryAndroid::GetInstance()->GetGLHelperBrowser();

  WebKit::WebGLId texture = helper->CopyAndScaleTexture(
      texture_id_in_layer_,
      texture_size_in_layer_,
      bitmap.size(),
      true,
      GLHelperBrowser::SCALER_QUALITY_FAST);
  if (texture == 0)
    return false;

  helper->ReadbackTextureSync(texture,
                              gfx::Rect(bitmap.size()),
                              static_cast<unsigned char*> (bitmap.pixels()));

  WebKit::WebGraphicsContext3D* context =
      ImageTransportFactoryAndroid::GetInstance()->GetContext3D();
  context->deleteTexture(texture);

  return true;
}

bool RenderWidgetHostViewAndroid::HasValidFrame() const {
  return texture_id_in_layer_ != 0 &&
      content_view_core_ &&
      !texture_size_in_layer_.IsEmpty();
}

gfx::NativeView RenderWidgetHostViewAndroid::GetNativeView() const {
  return content_view_core_->GetViewAndroid();
}

gfx::NativeViewId RenderWidgetHostViewAndroid::GetNativeViewId() const {
  return reinterpret_cast<gfx::NativeViewId>(
      const_cast<RenderWidgetHostViewAndroid*>(this));
}

gfx::NativeViewAccessible
RenderWidgetHostViewAndroid::GetNativeViewAccessible() {
  NOTIMPLEMENTED();
  return NULL;
}

void RenderWidgetHostViewAndroid::MovePluginWindows(
    const gfx::Vector2d& scroll_offset,
    const std::vector<webkit::npapi::WebPluginGeometry>& moves) {
  // We don't have plugin windows on Android. Do nothing. Note: this is called
  // from RenderWidgetHost::OnUpdateRect which is itself invoked while
  // processing the corresponding message from Renderer.
}

void RenderWidgetHostViewAndroid::Focus() {
  host_->Focus();
  host_->SetInputMethodActive(true);
  ResetClipping();
  if (overscroll_effect_)
    overscroll_effect_->SetEnabled(true);
}

void RenderWidgetHostViewAndroid::Blur() {
  host_->ExecuteEditCommand("Unselect", "");
  host_->SetInputMethodActive(false);
  host_->Blur();
  if (overscroll_effect_)
    overscroll_effect_->SetEnabled(false);
}

bool RenderWidgetHostViewAndroid::HasFocus() const {
  if (!content_view_core_)
    return false;  // ContentViewCore not created yet.

  return content_view_core_->HasFocus();
}

bool RenderWidgetHostViewAndroid::IsSurfaceAvailableForCopy() const {
  NOTIMPLEMENTED();
  return false;
}

void RenderWidgetHostViewAndroid::Show() {
  if (are_layers_attached_)
    return;

  are_layers_attached_ = true;
  AttachLayers();
}

void RenderWidgetHostViewAndroid::Hide() {
  if (!are_layers_attached_)
    return;

  are_layers_attached_ = false;
  RemoveLayers();
}

bool RenderWidgetHostViewAndroid::IsShowing() {
  // ContentViewCoreImpl represents the native side of the Java
  // ContentViewCore.  It being NULL means that it is not attached
  // to the View system yet, so we treat this RWHVA as hidden.
  return are_layers_attached_ && content_view_core_;
}

gfx::Rect RenderWidgetHostViewAndroid::GetViewBounds() const {
  if (!content_view_core_)
    return gfx::Rect();

  gfx::Size size = content_view_core_->GetViewportSizeDip();
  gfx::Size offset = content_view_core_->GetViewportSizeOffsetDip();
  size.Enlarge(-offset.width(), -offset.height());

  return gfx::Rect(size);
}

gfx::Size RenderWidgetHostViewAndroid::GetPhysicalBackingSize() const {
  if (!content_view_core_)
    return gfx::Size();

  return content_view_core_->GetPhysicalBackingSize();
}

float RenderWidgetHostViewAndroid::GetOverdrawBottomHeight() const {
  if (!content_view_core_)
    return 0.f;

  return content_view_core_->GetOverdrawBottomHeightDip();
}

void RenderWidgetHostViewAndroid::UpdateCursor(const WebCursor& cursor) {
  // There are no cursors on Android.
}

void RenderWidgetHostViewAndroid::SetIsLoading(bool is_loading) {
  // Do nothing. The UI notification is handled through ContentViewClient which
  // is TabContentsDelegate.
}

void RenderWidgetHostViewAndroid::TextInputTypeChanged(
    ui::TextInputType type,
    bool can_compose_inline) {
  // Unused on Android, which uses OnTextInputChanged instead.
}

int RenderWidgetHostViewAndroid::GetNativeImeAdapter() {
  return reinterpret_cast<int>(&ime_adapter_android_);
}

void RenderWidgetHostViewAndroid::OnTextInputStateChanged(
    const ViewHostMsg_TextInputState_Params& params) {
  if (!IsShowing())
    return;

  content_view_core_->UpdateImeAdapter(
      GetNativeImeAdapter(),
      static_cast<int>(params.type),
      params.value, params.selection_start, params.selection_end,
      params.composition_start, params.composition_end,
      params.show_ime_if_needed);
}

void RenderWidgetHostViewAndroid::OnProcessImeBatchStateAck(bool is_begin) {
  if (content_view_core_)
    content_view_core_->ProcessImeBatchStateAck(is_begin);
}

void RenderWidgetHostViewAndroid::OnDidChangeBodyBackgroundColor(
    SkColor color) {
  if (cached_background_color_ == color)
    return;

  cached_background_color_ = color;
  if (content_view_core_)
    content_view_core_->OnBackgroundColorChanged(color);
}

void RenderWidgetHostViewAndroid::SendBeginFrame(
    const cc::BeginFrameArgs& args) {
  TRACE_EVENT0("cc", "RenderWidgetHostViewAndroid::SendBeginFrame");
  if (host_)
    host_->Send(new ViewMsg_BeginFrame(host_->GetRoutingID(), args));
}

void RenderWidgetHostViewAndroid::OnSetNeedsBeginFrame(
    bool enabled) {
  TRACE_EVENT1("cc", "RenderWidgetHostViewAndroid::OnSetNeedsBeginFrame",
               "enabled", enabled);
  // ContentViewCoreImpl handles multiple subscribers to the BeginFrame, so
  // we have to make sure calls to ContentViewCoreImpl's SetNeedsBeginFrame
  // are balanced, even if RenderWidgetHostViewAndroid's may not be.
  if (content_view_core_ && needs_begin_frame_ != enabled) {
    content_view_core_->SetNeedsBeginFrame(enabled);
    needs_begin_frame_ = enabled;
  }
}

void RenderWidgetHostViewAndroid::OnStartContentIntent(
    const GURL& content_url) {
  if (content_view_core_)
    content_view_core_->StartContentIntent(content_url);
}

void RenderWidgetHostViewAndroid::ImeCancelComposition() {
  ime_adapter_android_.CancelComposition();
}

void RenderWidgetHostViewAndroid::ImeCompositionRangeChanged(
    const ui::Range& range,
    const std::vector<gfx::Rect>& character_bounds) {
}

void RenderWidgetHostViewAndroid::DidUpdateBackingStore(
    const gfx::Rect& scroll_rect,
    const gfx::Vector2d& scroll_delta,
    const std::vector<gfx::Rect>& copy_rects,
    const ui::LatencyInfo& latency_info) {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewAndroid::RenderViewGone(
    base::TerminationStatus status, int error_code) {
  Destroy();
}

void RenderWidgetHostViewAndroid::Destroy() {
  RemoveLayers();
  content_view_core_ = NULL;

  // The RenderWidgetHost's destruction led here, so don't call it.
  host_ = NULL;

  delete this;
}

void RenderWidgetHostViewAndroid::SetTooltipText(
    const string16& tooltip_text) {
  // Tooltips don't makes sense on Android.
}

void RenderWidgetHostViewAndroid::SelectionChanged(const string16& text,
                                                   size_t offset,
                                                   const ui::Range& range) {
  RenderWidgetHostViewBase::SelectionChanged(text, offset, range);

  if (text.empty() || range.is_empty() || !content_view_core_)
    return;
  size_t pos = range.GetMin() - offset;
  size_t n = range.length();

  DCHECK(pos + n <= text.length()) << "The text can not fully cover range.";
  if (pos >= text.length()) {
    NOTREACHED() << "The text can not cover range.";
    return;
  }

  std::string utf8_selection = UTF16ToUTF8(text.substr(pos, n));

  content_view_core_->OnSelectionChanged(utf8_selection);
}

void RenderWidgetHostViewAndroid::SelectionBoundsChanged(
    const ViewHostMsg_SelectionBounds_Params& params) {
  if (content_view_core_) {
    content_view_core_->OnSelectionBoundsChanged(params);
  }
}

void RenderWidgetHostViewAndroid::ScrollOffsetChanged() {
}

BackingStore* RenderWidgetHostViewAndroid::AllocBackingStore(
    const gfx::Size& size) {
  NOTIMPLEMENTED();
  return NULL;
}

void RenderWidgetHostViewAndroid::SetBackground(const SkBitmap& background) {
  RenderWidgetHostViewBase::SetBackground(background);
  host_->Send(new ViewMsg_SetBackground(host_->GetRoutingID(), background));
}

void RenderWidgetHostViewAndroid::CopyFromCompositingSurface(
    const gfx::Rect& src_subrect,
    const gfx::Size& dst_size,
    const base::Callback<void(bool, const SkBitmap&)>& callback) {
  NOTIMPLEMENTED();
  callback.Run(false, SkBitmap());
}

void RenderWidgetHostViewAndroid::CopyFromCompositingSurfaceToVideoFrame(
      const gfx::Rect& src_subrect,
      const scoped_refptr<media::VideoFrame>& target,
      const base::Callback<void(bool)>& callback) {
  NOTIMPLEMENTED();
  callback.Run(false);
}

bool RenderWidgetHostViewAndroid::CanCopyToVideoFrame() const {
  return false;
}

void RenderWidgetHostViewAndroid::ShowDisambiguationPopup(
    const gfx::Rect& target_rect, const SkBitmap& zoomed_bitmap) {
  if (!content_view_core_)
    return;

  content_view_core_->ShowDisambiguationPopup(target_rect, zoomed_bitmap);
}

SmoothScrollGesture* RenderWidgetHostViewAndroid::CreateSmoothScrollGesture(
    bool scroll_down, int pixels_to_scroll, int mouse_event_x,
    int mouse_event_y) {
  return new TouchSmoothScrollGestureAndroid(
      pixels_to_scroll,
      GetRenderWidgetHost(),
      content_view_core_->CreateSmoothScroller(
          scroll_down, mouse_event_x, mouse_event_y));
}

void RenderWidgetHostViewAndroid::OnAcceleratedCompositingStateChange() {
}

void RenderWidgetHostViewAndroid::SendDelegatedFrameAck() {
  cc::CompositorFrameAck ack;
  delegated_renderer_layer_->TakeUnusedResourcesForChildCompositor(
      &ack.resources);
  RenderWidgetHostImpl::SendSwapCompositorFrameAck(
      host_->GetRoutingID(), host_->GetProcess()->GetID(), ack);
}

void RenderWidgetHostViewAndroid::SwapDelegatedFrame(
    scoped_ptr<cc::DelegatedFrameData> frame_data) {
  bool has_frame = frame_data.get() && !frame_data->render_pass_list.empty();

  if (has_frame) {
    delegated_renderer_layer_->SetFrameData(frame_data.Pass());
    delegated_renderer_layer_->SetDisplaySize(texture_size_in_layer_);
    layer_->SetIsDrawable(true);
  }
  layer_->SetBounds(content_size_in_layer_);
  layer_->SetNeedsDisplay();

  base::Closure ack_callback =
      base::Bind(&RenderWidgetHostViewAndroid::SendDelegatedFrameAck,
                 weak_ptr_factory_.GetWeakPtr());

  if (host_->is_hidden())
    ack_callback.Run();
  else
    ack_callbacks_.push(ack_callback);
}

void RenderWidgetHostViewAndroid::ComputeContentsSize(
    const cc::CompositorFrameMetadata& frame_metadata) {
  // Calculate the content size.  This should be 0 if the texture_size is 0.
  gfx::Vector2dF offset;
  if (texture_size_in_layer_.GetArea() > 0)
    offset = frame_metadata.location_bar_content_translation;
  offset.set_y(offset.y() + frame_metadata.overdraw_bottom_height);
  offset.Scale(frame_metadata.device_scale_factor);
  content_size_in_layer_ =
      gfx::Size(texture_size_in_layer_.width() - offset.x(),
                texture_size_in_layer_.height() - offset.y());
  // Content size changes should be reflected in associated animation effects.
  UpdateAnimationSize(frame_metadata);
}

void RenderWidgetHostViewAndroid::OnSwapCompositorFrame(
    scoped_ptr<cc::CompositorFrame> frame) {
  // Always let ContentViewCore know about the new frame first, so it can decide
  // to schedule a Draw immediately when it sees the texture layer invalidation.
  UpdateContentViewCoreFrameMetadata(frame->metadata);

  if (frame->delegated_frame_data) {
    if (!frame->delegated_frame_data->render_pass_list.empty()) {
      texture_size_in_layer_ = frame->delegated_frame_data->render_pass_list
          .back()->output_rect.size();
    }
    ComputeContentsSize(frame->metadata);

    SwapDelegatedFrame(frame->delegated_frame_data.Pass());
    return;
  }

  if (!frame->gl_frame_data || frame->gl_frame_data->mailbox.IsZero())
    return;

  base::Closure callback = base::Bind(&InsertSyncPointAndAckForCompositor,
                                      host_->GetProcess()->GetID(),
                                      host_->GetRoutingID(),
                                      current_mailbox_,
                                      texture_size_in_layer_);
  ImageTransportFactoryAndroid::GetInstance()->WaitSyncPoint(
      frame->gl_frame_data->sync_point);

  texture_size_in_layer_ = frame->gl_frame_data->size;
  ComputeContentsSize(frame->metadata);

  BuffersSwapped(frame->gl_frame_data->mailbox, callback);
}

void RenderWidgetHostViewAndroid::SynchronousFrameMetadata(
    const cc::CompositorFrameMetadata& frame_metadata) {
  // This is a subset of OnSwapCompositorFrame() used in the synchronous
  // compositor flow.
  UpdateContentViewCoreFrameMetadata(frame_metadata);
  ComputeContentsSize(frame_metadata);
}

void RenderWidgetHostViewAndroid::UpdateContentViewCoreFrameMetadata(
    const cc::CompositorFrameMetadata& frame_metadata) {
  if (content_view_core_) {
    // All offsets and sizes are in CSS pixels.
    content_view_core_->UpdateFrameInfo(
        frame_metadata.root_scroll_offset,
        frame_metadata.page_scale_factor,
        gfx::Vector2dF(frame_metadata.min_page_scale_factor,
                       frame_metadata.max_page_scale_factor),
        frame_metadata.root_layer_size,
        frame_metadata.viewport_size,
        frame_metadata.location_bar_offset,
        frame_metadata.location_bar_content_translation,
        frame_metadata.overdraw_bottom_height);
  }
}

void RenderWidgetHostViewAndroid::AcceleratedSurfaceBuffersSwapped(
    const GpuHostMsg_AcceleratedSurfaceBuffersSwapped_Params& params,
    int gpu_host_id) {
  NOTREACHED() << "Deprecated. Use --composite-to-mailbox.";

  if (params.mailbox_name.empty())
    return;

  std::string return_mailbox;
  if (!current_mailbox_.IsZero()) {
    return_mailbox.assign(
        reinterpret_cast<const char*>(current_mailbox_.name),
        sizeof(current_mailbox_.name));
  }

  base::Closure callback = base::Bind(&InsertSyncPointAndAckForGpu,
                                      gpu_host_id, params.route_id,
                                      return_mailbox);

  gpu::Mailbox mailbox;
  std::copy(params.mailbox_name.data(),
            params.mailbox_name.data() + params.mailbox_name.length(),
            reinterpret_cast<char*>(mailbox.name));

  texture_size_in_layer_ = params.size;
  content_size_in_layer_ = params.size;

  BuffersSwapped(mailbox, callback);
}

void RenderWidgetHostViewAndroid::BuffersSwapped(
    const gpu::Mailbox& mailbox,
    const base::Closure& ack_callback) {
  ImageTransportFactoryAndroid* factory =
      ImageTransportFactoryAndroid::GetInstance();

  // TODO(sievers): When running the impl thread in the browser we
  // need to delay the ACK until after commit and use more than a single
  // texture.
  DCHECK(!CompositorImpl::IsThreadingEnabled());

  if (!texture_id_in_layer_) {
    texture_id_in_layer_ = factory->CreateTexture();
    texture_layer_->SetIsDrawable(true);
  }

  ImageTransportFactoryAndroid::GetInstance()->AcquireTexture(
      texture_id_in_layer_, mailbox.name);

  ResetClipping();

  current_mailbox_ = mailbox;

  if (host_->is_hidden())
    ack_callback.Run();
  else
    ack_callbacks_.push(ack_callback);
}

void RenderWidgetHostViewAndroid::AttachLayers() {
  if (!content_view_core_)
    return;

  content_view_core_->AttachLayer(layer_);

  if (overscroll_effect_)
    content_view_core_->AttachLayer(overscroll_effect_->root_layer());
}

void RenderWidgetHostViewAndroid::RemoveLayers() {
  if (!content_view_core_)
    return;

  if (overscroll_effect_)
    content_view_core_->RemoveLayer(overscroll_effect_->root_layer());

  content_view_core_->RemoveLayer(layer_);
}

bool RenderWidgetHostViewAndroid::Animate(base::TimeTicks frame_time) {
  if (!overscroll_effect_)
    return false;
  return overscroll_effect_->Animate(frame_time);
}

void RenderWidgetHostViewAndroid::CreateOverscrollEffectIfNecessary() {
  if (!overscroll_effect_enabled_ || overscroll_effect_)
    return;

  overscroll_effect_ = OverscrollGlow::Create(true);

  // Prevent future creation attempts on failure.
  if (!overscroll_effect_)
    overscroll_effect_enabled_ = false;

  if (overscroll_effect_ && content_view_core_ && are_layers_attached_)
    content_view_core_->AttachLayer(overscroll_effect_->root_layer());
}

void RenderWidgetHostViewAndroid::UpdateAnimationSize(
    const cc::CompositorFrameMetadata& frame_metadata) {
  if (!overscroll_effect_)
    return;
  // Disable edge effects for axes on which scrolling is impossible.
  gfx::SizeF ceiled_viewport_size =
      gfx::ToCeiledSize(frame_metadata.viewport_size);
  overscroll_effect_->set_horizontal_overscroll_enabled(
      ceiled_viewport_size.width() < frame_metadata.root_layer_size.width());
  overscroll_effect_->set_vertical_overscroll_enabled(
      ceiled_viewport_size.height() < frame_metadata.root_layer_size.height());
  overscroll_effect_->set_size(content_size_in_layer_);
}

void RenderWidgetHostViewAndroid::ScheduleAnimationIfNecessary() {
  if (!content_view_core_)
    return;
  if (overscroll_effect_ && overscroll_effect_->NeedsAnimate())
    content_view_core_->SetNeedsAnimate();
}

void RenderWidgetHostViewAndroid::AcceleratedSurfacePostSubBuffer(
    const GpuHostMsg_AcceleratedSurfacePostSubBuffer_Params& params,
    int gpu_host_id) {
  NOTREACHED();
}

void RenderWidgetHostViewAndroid::AcceleratedSurfaceSuspend() {
  NOTREACHED();
}

void RenderWidgetHostViewAndroid::AcceleratedSurfaceRelease() {
  // This tells us we should free the frontbuffer.
  if (texture_id_in_layer_) {
    texture_layer_->SetTextureId(0);
    texture_layer_->SetIsDrawable(false);
    ImageTransportFactoryAndroid::GetInstance()->DeleteTexture(
        texture_id_in_layer_);
    texture_id_in_layer_ = 0;
    current_mailbox_ = gpu::Mailbox();
  }
}

bool RenderWidgetHostViewAndroid::HasAcceleratedSurface(
    const gfx::Size& desired_size) {
  NOTREACHED();
  return false;
}

void RenderWidgetHostViewAndroid::GetScreenInfo(WebKit::WebScreenInfo* result) {
  // ScreenInfo isn't tied to the widget on Android. Always return the default.
  RenderWidgetHostViewBase::GetDefaultScreenInfo(result);
}

// TODO(jrg): Find out the implications and answer correctly here,
// as we are returning the WebView and not root window bounds.
gfx::Rect RenderWidgetHostViewAndroid::GetBoundsInRootWindow() {
  return GetViewBounds();
}

gfx::GLSurfaceHandle RenderWidgetHostViewAndroid::GetCompositingSurface() {
  if (surface_texture_transport_) {
    return surface_texture_transport_->GetCompositingSurface(
        host_->surface_id());
  } else {
    return gfx::GLSurfaceHandle(gfx::kNullPluginWindow, gfx::TEXTURE_TRANSPORT);
  }
}

void RenderWidgetHostViewAndroid::ProcessAckedTouchEvent(
    const TouchEventWithLatencyInfo& touch, InputEventAckState ack_result) {
  if (content_view_core_)
    content_view_core_->ConfirmTouchEvent(ack_result);
}

void RenderWidgetHostViewAndroid::SetHasHorizontalScrollbar(
    bool has_horizontal_scrollbar) {
  // intentionally empty, like RenderWidgetHostViewViews
}

void RenderWidgetHostViewAndroid::SetScrollOffsetPinning(
    bool is_pinned_to_left, bool is_pinned_to_right) {
  // intentionally empty, like RenderWidgetHostViewViews
}

void RenderWidgetHostViewAndroid::UnhandledWheelEvent(
    const WebKit::WebMouseWheelEvent& event) {
  // intentionally empty, like RenderWidgetHostViewViews
}

InputEventAckState RenderWidgetHostViewAndroid::FilterInputEvent(
    const WebKit::WebInputEvent& input_event) {
  if (host_) {
    SynchronousCompositorImpl* compositor =
        SynchronousCompositorImpl::FromID(host_->GetProcess()->GetID(),
                                          host_->GetRoutingID());
    if (compositor)
      return compositor->HandleInputEvent(input_event);
  }
  return INPUT_EVENT_ACK_STATE_NOT_CONSUMED;
}

void RenderWidgetHostViewAndroid::OnAccessibilityNotifications(
    const std::vector<AccessibilityHostMsg_NotificationParams>& params) {
  if (!GetBrowserAccessibilityManager()) {
    SetBrowserAccessibilityManager(
        new BrowserAccessibilityManagerAndroid(
            content_view_core_->GetJavaObject(),
            BrowserAccessibilityManagerAndroid::GetEmptyDocument(),
            this));
  }
  GetBrowserAccessibilityManager()->OnAccessibilityNotifications(params);
}

void RenderWidgetHostViewAndroid::SetAccessibilityFocus(int acc_obj_id) {
  if (!host_)
    return;

  host_->AccessibilitySetFocus(acc_obj_id);
}

void RenderWidgetHostViewAndroid::AccessibilityDoDefaultAction(int acc_obj_id) {
  if (!host_)
    return;

  host_->AccessibilityDoDefaultAction(acc_obj_id);
}

void RenderWidgetHostViewAndroid::AccessibilityScrollToMakeVisible(
    int acc_obj_id, gfx::Rect subfocus) {
  if (!host_)
    return;

  host_->AccessibilityScrollToMakeVisible(acc_obj_id, subfocus);
}

void RenderWidgetHostViewAndroid::AccessibilityScrollToPoint(
    int acc_obj_id, gfx::Point point) {
  if (!host_)
    return;

  host_->AccessibilityScrollToPoint(acc_obj_id, point);
}

void RenderWidgetHostViewAndroid::AccessibilitySetTextSelection(
    int acc_obj_id, int start_offset, int end_offset) {
  if (!host_)
    return;

  host_->AccessibilitySetTextSelection(
      acc_obj_id, start_offset, end_offset);
}

gfx::Point RenderWidgetHostViewAndroid::GetLastTouchEventLocation() const {
  NOTIMPLEMENTED();
  // Only used on Win8
  return gfx::Point();
}

void RenderWidgetHostViewAndroid::FatalAccessibilityTreeError() {
  if (!host_)
    return;

  host_->FatalAccessibilityTreeError();
  SetBrowserAccessibilityManager(NULL);
}

bool RenderWidgetHostViewAndroid::LockMouse() {
  NOTIMPLEMENTED();
  return false;
}

void RenderWidgetHostViewAndroid::UnlockMouse() {
  NOTIMPLEMENTED();
}

// Methods called from the host to the render

void RenderWidgetHostViewAndroid::SendKeyEvent(
    const NativeWebKeyboardEvent& event) {
  if (host_)
    host_->ForwardKeyboardEvent(event);
}

void RenderWidgetHostViewAndroid::SendTouchEvent(
    const WebKit::WebTouchEvent& event) {
  if (host_)
    host_->ForwardTouchEventWithLatencyInfo(event, ui::LatencyInfo());
}


void RenderWidgetHostViewAndroid::SendMouseEvent(
    const WebKit::WebMouseEvent& event) {
  if (host_)
    host_->ForwardMouseEvent(event);
}

void RenderWidgetHostViewAndroid::SendMouseWheelEvent(
    const WebKit::WebMouseWheelEvent& event) {
  if (host_)
    host_->ForwardWheelEvent(event);
}

void RenderWidgetHostViewAndroid::SendGestureEvent(
    const WebKit::WebGestureEvent& event) {
  // Sending a gesture that may trigger overscroll should resume the effect.
  if (overscroll_effect_)
    overscroll_effect_->SetEnabled(true);

  if (host_)
    host_->ForwardGestureEvent(event);
}

void RenderWidgetHostViewAndroid::SelectRange(const gfx::Point& start,
                                              const gfx::Point& end) {
  if (host_)
    host_->SelectRange(start, end);
}

void RenderWidgetHostViewAndroid::MoveCaret(const gfx::Point& point) {
  if (host_)
    host_->MoveCaret(point);
}

void RenderWidgetHostViewAndroid::RequestContentClipping(
    const gfx::Rect& clipping,
    const gfx::Size& content_size) {
  // A focused view provides its own clipping.
  if (HasFocus())
    return;

  ClipContents(clipping, content_size);
}

void RenderWidgetHostViewAndroid::ResetClipping() {
  ClipContents(gfx::Rect(gfx::Point(), content_size_in_layer_),
               content_size_in_layer_);
}

void RenderWidgetHostViewAndroid::ClipContents(const gfx::Rect& clipping,
                                               const gfx::Size& content_size) {
  if (!texture_id_in_layer_ || content_size_in_layer_.IsEmpty())
    return;

  gfx::Size clipped_content(content_size_in_layer_);
  clipped_content.SetToMin(clipping.size());
  texture_layer_->SetBounds(clipped_content);
  texture_layer_->SetNeedsDisplay();

  if (texture_size_in_layer_.IsEmpty()) {
    texture_layer_->SetUV(gfx::PointF(), gfx::PointF());
    return;
  }

  gfx::PointF offset(
      clipping.x() + content_size_in_layer_.width() - content_size.width(),
      clipping.y() + content_size_in_layer_.height() - content_size.height());
  offset.SetToMax(gfx::PointF());

  gfx::Vector2dF uv_scale(1.f / texture_size_in_layer_.width(),
                          1.f / texture_size_in_layer_.height());
  texture_layer_->SetUV(
      gfx::PointF(offset.x() * uv_scale.x(),
                  offset.y() * uv_scale.y()),
      gfx::PointF((offset.x() + clipped_content.width()) * uv_scale.x(),
                  (offset.y() + clipped_content.height()) * uv_scale.y()));
}

SkColor RenderWidgetHostViewAndroid::GetCachedBackgroundColor() const {
  return cached_background_color_;
}

void RenderWidgetHostViewAndroid::OnOverscrolled(
    gfx::Vector2dF accumulated_overscroll,
    gfx::Vector2dF current_fling_velocity) {
  CreateOverscrollEffectIfNecessary();
  if (!overscroll_effect_)
    return;

  overscroll_effect_->OnOverscrolled(base::TimeTicks::Now(),
                                     accumulated_overscroll,
                                     current_fling_velocity);
  ScheduleAnimationIfNecessary();
}

void RenderWidgetHostViewAndroid::SetContentViewCore(
    ContentViewCoreImpl* content_view_core) {
  RunAckCallbacks();

  if (are_layers_attached_)
    RemoveLayers();

  content_view_core_ = content_view_core;

  if (are_layers_attached_)
    AttachLayers();
}

void RenderWidgetHostViewAndroid::RunAckCallbacks() {
  while (!ack_callbacks_.empty()) {
    ack_callbacks_.front().Run();
    ack_callbacks_.pop();
  }
}

void RenderWidgetHostViewAndroid::HasTouchEventHandlers(
    bool need_touch_events) {
  if (content_view_core_)
    content_view_core_->HasTouchEventHandlers(need_touch_events);
}

unsigned RenderWidgetHostViewAndroid::PrepareTexture(
    cc::ResourceUpdateQueue* queue) {
  RunAckCallbacks();
  return texture_id_in_layer_;
}

void RenderWidgetHostViewAndroid::DidCommitFrameData() {
  RunAckCallbacks();
}

WebKit::WebGraphicsContext3D* RenderWidgetHostViewAndroid::Context3d() {
  return ImageTransportFactoryAndroid::GetInstance()->GetContext3D();
}

bool RenderWidgetHostViewAndroid::PrepareTextureMailbox(
    cc::TextureMailbox* mailbox) {
  return false;
}

// static
void RenderWidgetHostViewPort::GetDefaultScreenInfo(
    WebKit::WebScreenInfo* results) {
  const gfx::Display& display =
      gfx::Screen::GetNativeScreen()->GetPrimaryDisplay();
  results->rect = display.bounds();
  // TODO(husky): Remove any system controls from availableRect.
  results->availableRect = display.work_area();
  results->deviceScaleFactor = display.device_scale_factor();
  gfx::DeviceDisplayInfo info;
  results->depth = info.GetBitsPerPixel();
  results->depthPerComponent = info.GetBitsPerComponent();
  results->isMonochrome = (results->depthPerComponent == 0);
}

////////////////////////////////////////////////////////////////////////////////
// RenderWidgetHostView, public:

// static
RenderWidgetHostView*
RenderWidgetHostView::CreateViewForWidget(RenderWidgetHost* widget) {
  RenderWidgetHostImpl* rwhi = RenderWidgetHostImpl::From(widget);
  return new RenderWidgetHostViewAndroid(rwhi, NULL);
}

} // namespace content
