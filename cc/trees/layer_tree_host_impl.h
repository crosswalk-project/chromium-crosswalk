// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TREES_LAYER_TREE_HOST_IMPL_H_
#define CC_TREES_LAYER_TREE_HOST_IMPL_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/time.h"
#include "cc/animation/animation_events.h"
#include "cc/animation/animation_registrar.h"
#include "cc/base/cc_export.h"
#include "cc/input/input_handler.h"
#include "cc/input/layer_scroll_offset_delegate.h"
#include "cc/input/top_controls_manager_client.h"
#include "cc/layers/layer_lists.h"
#include "cc/layers/render_pass_sink.h"
#include "cc/output/begin_frame_args.h"
#include "cc/output/output_surface_client.h"
#include "cc/output/renderer.h"
#include "cc/quads/render_pass.h"
#include "cc/resources/tile_manager.h"
#include "skia/ext/refptr.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "ui/gfx/rect.h"

namespace cc {

class CompletionEvent;
class CompositorFrameMetadata;
class DebugRectHistory;
class FrameRateCounter;
class LayerImpl;
class LayerTreeHostImplTimeSourceAdapter;
class LayerTreeImpl;
class PageScaleAnimation;
class PaintTimeCounter;
class MemoryHistory;
class RenderingStatsInstrumentation;
class RenderPassDrawQuad;
class ResourceProvider;
class TopControlsManager;
struct RendererCapabilities;

// LayerTreeHost->Proxy callback interface.
class LayerTreeHostImplClient {
 public:
  virtual void DidTryInitializeRendererOnImplThread(
      bool success,
      scoped_refptr<ContextProvider> offscreen_context_provider) = 0;
  virtual void DidLoseOutputSurfaceOnImplThread() = 0;
  virtual void OnSwapBuffersCompleteOnImplThread() = 0;
  virtual void BeginFrameOnImplThread(const BeginFrameArgs& args) = 0;
  virtual void OnCanDrawStateChanged(bool can_draw) = 0;
  virtual void OnHasPendingTreeStateChanged(bool has_pending_tree) = 0;
  virtual void SetNeedsRedrawOnImplThread() = 0;
  virtual void SetNeedsRedrawRectOnImplThread(gfx::Rect damage_rect) = 0;
  virtual void DidInitializeVisibleTileOnImplThread() = 0;
  virtual void SetNeedsCommitOnImplThread() = 0;
  virtual void PostAnimationEventsToMainThreadOnImplThread(
      scoped_ptr<AnimationEventsVector> events,
      base::Time wall_clock_time) = 0;
  // Returns true if resources were deleted by this call.
  virtual bool ReduceContentsTextureMemoryOnImplThread(
      size_t limit_bytes,
      int priority_cutoff) = 0;
  virtual void ReduceWastedContentsTextureMemoryOnImplThread() = 0;
  virtual void SendManagedMemoryStats() = 0;
  virtual bool IsInsideDraw() = 0;
  virtual void RenewTreePriority() = 0;
  virtual void RequestScrollbarAnimationOnImplThread(base::TimeDelta delay) = 0;
  virtual void DidActivatePendingTree() = 0;

 protected:
  virtual ~LayerTreeHostImplClient() {}
};

// LayerTreeHostImpl owns the LayerImpl trees as well as associated rendering
// state.
class CC_EXPORT LayerTreeHostImpl
    : public InputHandler,
      public RendererClient,
      public TileManagerClient,
      public OutputSurfaceClient,
      public TopControlsManagerClient,
      public base::SupportsWeakPtr<LayerTreeHostImpl> {
 public:
  static scoped_ptr<LayerTreeHostImpl> Create(
      const LayerTreeSettings& settings,
      LayerTreeHostImplClient* client,
      Proxy* proxy,
      RenderingStatsInstrumentation* rendering_stats_instrumentation);
  virtual ~LayerTreeHostImpl();

  // InputHandler implementation
  virtual void BindToClient(InputHandlerClient* client) OVERRIDE;
  virtual InputHandler::ScrollStatus ScrollBegin(
      gfx::Point viewport_point,
      InputHandler::ScrollInputType type) OVERRIDE;
  virtual bool ScrollBy(gfx::Point viewport_point,
                        gfx::Vector2dF scroll_delta) OVERRIDE;
  virtual bool ScrollVerticallyByPage(gfx::Point viewport_point,
                                      ScrollDirection direction) OVERRIDE;
  virtual void SetRootLayerScrollOffsetDelegate(
      LayerScrollOffsetDelegate* root_layer_scroll_offset_delegate) OVERRIDE;
  virtual void OnRootLayerDelegatedScrollOffsetChanged() OVERRIDE;
  virtual void ScrollEnd() OVERRIDE;
  virtual InputHandler::ScrollStatus FlingScrollBegin() OVERRIDE;
  virtual void NotifyCurrentFlingVelocity(gfx::Vector2dF velocity) OVERRIDE;
  virtual void PinchGestureBegin() OVERRIDE;
  virtual void PinchGestureUpdate(float magnify_delta,
                                  gfx::Point anchor) OVERRIDE;
  virtual void PinchGestureEnd() OVERRIDE;
  virtual void StartPageScaleAnimation(gfx::Vector2d target_offset,
                                       bool anchor_point,
                                       float page_scale,
                                       base::TimeTicks start_time,
                                       base::TimeDelta duration) OVERRIDE;
  virtual void ScheduleAnimation() OVERRIDE;
  virtual bool HaveTouchEventHandlersAt(gfx::Point viewport_port) OVERRIDE;
  virtual void SetLatencyInfoForInputEvent(const ui::LatencyInfo& latency_info)
      OVERRIDE;

  // TopControlsManagerClient implementation.
  virtual void DidChangeTopControlsPosition() OVERRIDE;
  virtual bool HaveRootScrollLayer() const OVERRIDE;

  void StartScrollbarAnimation();

  struct CC_EXPORT FrameData : public RenderPassSink {
    FrameData();
    virtual ~FrameData();

    std::vector<gfx::Rect> occluding_screen_space_rects;
    std::vector<gfx::Rect> non_occluding_screen_space_rects;
    RenderPassList render_passes;
    RenderPassIdHashMap render_passes_by_id;
    const LayerImplList* render_surface_layer_list;
    LayerImplList will_draw_layers;
    bool contains_incomplete_tile;
    bool has_no_damage;

    // RenderPassSink implementation.
    virtual void AppendRenderPass(scoped_ptr<RenderPass> render_pass) OVERRIDE;
  };

  virtual void BeginCommit();
  virtual void CommitComplete();
  virtual void Animate(base::TimeTicks monotonic_time,
                       base::Time wall_clock_time);
  virtual void UpdateAnimationState(bool start_ready_animations);
  void MainThreadHasStoppedFlinging();
  void UpdateBackgroundAnimateTicking(bool should_background_tick);
  void SetViewportDamage(gfx::Rect damage_rect);

  void ManageTiles();

  // Returns false if problems occured preparing the frame, and we should try
  // to avoid displaying the frame. If PrepareToDraw is called, DidDrawAllLayers
  // must also be called, regardless of whether DrawLayers is called between the
  // two.
  virtual bool PrepareToDraw(FrameData* frame,
                             gfx::Rect device_viewport_damage_rect);
  virtual void DrawLayers(FrameData* frame, base::TimeTicks frame_begin_time);
  // Must be called if and only if PrepareToDraw was called.
  void DidDrawAllLayers(const FrameData& frame);

  const LayerTreeSettings& settings() const { return settings_; }

  // Returns the currently visible viewport size in DIP. This value excludes
  // the URL bar and non-overlay scrollbars.
  gfx::SizeF VisibleViewportSize() const;

  // RendererClient implementation
  virtual gfx::Rect DeviceViewport() const OVERRIDE;
 private:
  virtual float DeviceScaleFactor() const OVERRIDE;
  virtual const LayerTreeSettings& Settings() const OVERRIDE;
 public:
  virtual void SetFullRootLayerDamage() OVERRIDE;
  virtual void SetManagedMemoryPolicy(const ManagedMemoryPolicy& policy)
      OVERRIDE;
  virtual void EnforceManagedMemoryPolicy(const ManagedMemoryPolicy& policy)
      OVERRIDE;
  virtual bool HasImplThread() const OVERRIDE;
  virtual bool ShouldClearRootRenderPass() const OVERRIDE;
  virtual CompositorFrameMetadata MakeCompositorFrameMetadata() const OVERRIDE;
  virtual bool AllowPartialSwap() const OVERRIDE;

  // TileManagerClient implementation.
  virtual void DidInitializeVisibleTile() OVERRIDE;
  virtual void NotifyReadyToActivate() OVERRIDE;

  // OutputSurfaceClient implementation.
  virtual bool DeferredInitialize(
      scoped_refptr<ContextProvider> offscreen_context_provider) OVERRIDE;
  virtual void SetNeedsRedrawRect(gfx::Rect rect) OVERRIDE;
  virtual void BeginFrame(const BeginFrameArgs& args)
      OVERRIDE;
  virtual void SetExternalDrawConstraints(const gfx::Transform& transform,
                                          gfx::Rect viewport) OVERRIDE;
  virtual void DidLoseOutputSurface() OVERRIDE;
  virtual void OnSwapBuffersComplete(const CompositorFrameAck* ack) OVERRIDE;

  // Called from LayerTreeImpl.
  void OnCanDrawStateChangedForTree();

  // Implementation
  bool CanDraw();
  OutputSurface* output_surface() const { return output_surface_.get(); }

  std::string LayerTreeAsJson() const;

  void FinishAllRendering();
  int SourceAnimationFrameNumber() const;

  virtual bool InitializeRenderer(scoped_ptr<OutputSurface> output_surface);
  bool IsContextLost();
  TileManager* tile_manager() { return tile_manager_.get(); }
  Renderer* renderer() { return renderer_.get(); }
  const RendererCapabilities& GetRendererCapabilities() const;

  virtual bool SwapBuffers(const FrameData& frame);
  void SetNeedsBeginFrame(bool enable);
  void SetNeedsManageTiles() { manage_tiles_needed_ = true; }

  void Readback(void* pixels, gfx::Rect rect_in_device_viewport);

  LayerTreeImpl* active_tree() { return active_tree_.get(); }
  const LayerTreeImpl* active_tree() const { return active_tree_.get(); }
  LayerTreeImpl* pending_tree() { return pending_tree_.get(); }
  const LayerTreeImpl* pending_tree() const { return pending_tree_.get(); }
  const LayerTreeImpl* recycle_tree() const { return recycle_tree_.get(); }
  virtual void CreatePendingTree();
  void CheckForCompletedTileUploads();
  virtual void ActivatePendingTreeIfNeeded();

  // Shortcuts to layers on the active tree.
  LayerImpl* RootLayer() const;
  LayerImpl* RootScrollLayer() const;
  LayerImpl* CurrentlyScrollingLayer() const;

  virtual void SetVisible(bool visible);
  bool visible() const { return visible_; }

  void SetNeedsCommit() { client_->SetNeedsCommitOnImplThread(); }
  void SetNeedsRedraw() { client_->SetNeedsRedrawOnImplThread(); }

  ManagedMemoryPolicy ActualManagedMemoryPolicy() const;

  size_t memory_allocation_limit_bytes() const {
    return managed_memory_policy_.bytes_limit_when_visible;
  }

  void SetViewportSize(gfx::Size device_viewport_size);
  gfx::Size device_viewport_size() const { return device_viewport_size_; }

  void SetOverdrawBottomHeight(float overdraw_bottom_height);
  float overdraw_bottom_height() const { return overdraw_bottom_height_; }

  void SetDeviceScaleFactor(float device_scale_factor);
  float device_scale_factor() const { return device_scale_factor_; }

  const gfx::Transform& DeviceTransform() const;

  scoped_ptr<ScrollAndScaleSet> ProcessScrollDeltas();

  bool needs_animate_layers() const {
    return !animation_registrar_->active_animation_controllers().empty();
  }

  void SendManagedMemoryStats(
      size_t memory_visible_bytes,
      size_t memory_visible_and_nearby_bytes,
      size_t memory_use_bytes);

  void set_max_memory_needed_bytes(size_t bytes) {
    max_memory_needed_bytes_ = bytes;
  }

  FrameRateCounter* fps_counter() {
    return fps_counter_.get();
  }
  PaintTimeCounter* paint_time_counter() {
    return paint_time_counter_.get();
  }
  MemoryHistory* memory_history() {
    return memory_history_.get();
  }
  DebugRectHistory* debug_rect_history() {
    return debug_rect_history_.get();
  }
  ResourceProvider* resource_provider() {
    return resource_provider_.get();
  }
  TopControlsManager* top_controls_manager() {
    return top_controls_manager_.get();
  }

  Proxy* proxy() const { return proxy_; }

  AnimationRegistrar* animation_registrar() const {
    return animation_registrar_.get();
  }

  void SetDebugState(const LayerTreeDebugState& debug_state);
  const LayerTreeDebugState& debug_state() const { return debug_state_; }

  class CC_EXPORT CullRenderPassesWithCachedTextures {
 public:
    bool ShouldRemoveRenderPass(const RenderPassDrawQuad& quad,
                                const FrameData& frame) const;

    // Iterates from the root first, in order to remove the surfaces closest
    // to the root with cached textures, and all surfaces that draw into
    // them.
    size_t RenderPassListBegin(const RenderPassList& list) const {
      return list.size() - 1;
    }
    size_t RenderPassListEnd(const RenderPassList& list) const { return 0 - 1; }
    size_t RenderPassListNext(size_t it) const { return it - 1; }

    explicit CullRenderPassesWithCachedTextures(Renderer* renderer)
        : renderer_(renderer) {}
 private:
    Renderer* renderer_;
  };

  class CC_EXPORT CullRenderPassesWithNoQuads {
 public:
    bool ShouldRemoveRenderPass(const RenderPassDrawQuad& quad,
                                const FrameData& frame) const;

    // Iterates in draw order, so that when a surface is removed, and its
    // target becomes empty, then its target can be removed also.
    size_t RenderPassListBegin(const RenderPassList& list) const { return 0; }
    size_t RenderPassListEnd(const RenderPassList& list) const {
      return list.size();
    }
    size_t RenderPassListNext(size_t it) const { return it + 1; }
  };

  template <typename RenderPassCuller>
      static void RemoveRenderPasses(RenderPassCuller culler, FrameData* frame);

  skia::RefPtr<SkPicture> CapturePicture();

  gfx::Vector2dF accumulated_root_overscroll() const {
    return accumulated_root_overscroll_;
  }
  gfx::Vector2dF current_fling_velocity() const {
    return current_fling_velocity_;
  }

  bool pinch_gesture_active() const { return pinch_gesture_active_; }
  bool animate_layers_active() const { return animate_layers_active_; }

  void SetTreePriority(TreePriority priority);

  void ResetCurrentFrameTimeForNextFrame();
  base::TimeTicks CurrentFrameTimeTicks();
  base::Time CurrentFrameTime();

  virtual base::TimeTicks CurrentPhysicalTimeTicks() const;

  scoped_ptr<base::Value> AsValue() const;
  scoped_ptr<base::Value> ActivationStateAsValue() const;

  bool page_scale_animation_active() const { return !!page_scale_animation_; }

 protected:
  LayerTreeHostImpl(
      const LayerTreeSettings& settings,
      LayerTreeHostImplClient* client,
      Proxy* proxy,
      RenderingStatsInstrumentation* rendering_stats_instrumentation);
  virtual void ActivatePendingTree();

  // Virtual for testing.
  virtual void AnimateLayers(base::TimeTicks monotonic_time,
                             base::Time wall_clock_time);

  // Virtual for testing.
  virtual base::TimeDelta LowFrequencyAnimationInterval() const;

  const AnimationRegistrar::AnimationControllerMap&
      active_animation_controllers() const {
    return animation_registrar_->active_animation_controllers();
  }

  LayerTreeHostImplClient* client_;
  Proxy* proxy_;

 private:
  void CreateAndSetRenderer(OutputSurface* output_surface,
                            ResourceProvider* resource_provider);
  void ReleaseTreeResources();
  void EnforceZeroBudget(bool zero_budget);

  void AnimatePageScale(base::TimeTicks monotonic_time);
  void AnimateScrollbars(base::TimeTicks monotonic_time);
  void AnimateTopControls(base::TimeTicks monotonic_time);

  gfx::Vector2dF ScrollLayerWithViewportSpaceDelta(
      LayerImpl* layer_impl,
      float scale_from_viewport_to_screen_space,
      gfx::PointF viewport_point,
      gfx::Vector2dF viewport_delta);

  void UpdateMaxScrollOffset();
  void TrackDamageForAllSurfaces(
      LayerImpl* root_draw_layer,
      const LayerImplList& render_surface_layer_list);

  void UpdateTileManagerMemoryPolicy(const ManagedMemoryPolicy& policy);

  // Returns false if the frame should not be displayed. This function should
  // only be called from PrepareToDraw, as DidDrawAllLayers must be called
  // if this helper function is called.
  bool CalculateRenderPasses(FrameData* frame);

  void SendReleaseResourcesRecursive(LayerImpl* current);
  void ClearRenderSurfaces();
  bool EnsureRenderSurfaceLayerList();
  void ClearCurrentlyScrollingLayer();

  void AnimateScrollbarsRecursive(LayerImpl* layer,
                                  base::TimeTicks time);

  static LayerImpl* GetNonCompositedContentLayerRecursive(LayerImpl* layer);

  void UpdateCurrentFrameTime(base::TimeTicks* ticks, base::Time* now) const;

  void StartScrollbarAnimationRecursive(LayerImpl* layer, base::TimeTicks time);

  scoped_ptr<OutputSurface> output_surface_;

  // |resource_provider_| and |tile_manager_| can be NULL, e.g. when using tile-
  // free rendering - see OutputSurface::ForcedDrawToSoftwareDevice().
  scoped_ptr<ResourceProvider> resource_provider_;
  scoped_ptr<TileManager> tile_manager_;
  scoped_ptr<Renderer> renderer_;

  // Tree currently being drawn.
  scoped_ptr<LayerTreeImpl> active_tree_;

  // In impl-side painting mode, tree with possibly incomplete rasterized
  // content. May be promoted to active by ActivatePendingTreeIfNeeded().
  scoped_ptr<LayerTreeImpl> pending_tree_;

  // In impl-side painting mode, inert tree with layers that can be recycled
  // by the next sync from the main thread.
  scoped_ptr<LayerTreeImpl> recycle_tree_;

  InputHandlerClient* input_handler_client_;
  bool did_lock_scrolling_layer_;
  bool should_bubble_scrolls_;
  bool wheel_scrolling_;

  bool manage_tiles_needed_;

  // The optional delegate for the root layer scroll offset.
  LayerScrollOffsetDelegate* root_layer_scroll_offset_delegate_;
  LayerTreeSettings settings_;
  LayerTreeDebugState debug_state_;
  bool visible_;
  ManagedMemoryPolicy managed_memory_policy_;

  gfx::Vector2dF accumulated_root_overscroll_;
  gfx::Vector2dF current_fling_velocity_;

  bool pinch_gesture_active_;
  gfx::Point previous_pinch_anchor_;

  // This is set by AnimateLayers() and used by UpdateAnimationState()
  // when sending animation events to the main thread.
  base::Time last_animation_time_;
  bool animate_layers_active_;

  scoped_ptr<TopControlsManager> top_controls_manager_;

  scoped_ptr<PageScaleAnimation> page_scale_animation_;

  // This is used for ticking animations slowly when hidden.
  scoped_ptr<LayerTreeHostImplTimeSourceAdapter> time_source_client_adapter_;

  scoped_ptr<FrameRateCounter> fps_counter_;
  scoped_ptr<PaintTimeCounter> paint_time_counter_;
  scoped_ptr<MemoryHistory> memory_history_;
  scoped_ptr<DebugRectHistory> debug_rect_history_;

  // The maximum memory that would be used by the prioritized resource
  // manager, if there were no limit on memory usage.
  size_t max_memory_needed_bytes_;

  size_t last_sent_memory_visible_bytes_;
  size_t last_sent_memory_visible_and_nearby_bytes_;
  size_t last_sent_memory_use_bytes_;
  bool zero_budget_;

  // Viewport size passed in from the main thread, in physical pixels.
  gfx::Size device_viewport_size_;

  // Conversion factor from CSS pixels to physical pixels when
  // pageScaleFactor=1.
  float device_scale_factor_;

  // Vertical amount of the viewport size that's known to covered by a
  // browser-side UI element, such as an on-screen-keyboard.  This affects
  // scrollable size since we want to still be able to scroll to the bottom of
  // the page when the keyboard is up.
  float overdraw_bottom_height_;

  // Optional top-level constraints that can be set by the OutputSurface.  The
  // external_viewport_'s size takes precedence over device_viewport_size_ for
  // DrawQuad generation and Renderer; however, device_viewport_size_ is still
  // used for scrollable size.
  gfx::Transform external_transform_;
  gfx::Rect external_viewport_;

  gfx::Rect viewport_damage_rect_;

  base::TimeTicks current_frame_timeticks_;
  base::Time current_frame_time_;

  scoped_ptr<AnimationRegistrar> animation_registrar_;

  RenderingStatsInstrumentation* rendering_stats_instrumentation_;

  DISALLOW_COPY_AND_ASSIGN(LayerTreeHostImpl);
};

}  // namespace cc

#endif  // CC_TREES_LAYER_TREE_HOST_IMPL_H_
