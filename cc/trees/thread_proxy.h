// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TREES_THREAD_PROXY_H_
#define CC_TREES_THREAD_PROXY_H_

#include <string>

#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time.h"
#include "cc/animation/animation_events.h"
#include "cc/base/completion_event.h"
#include "cc/resources/resource_update_controller.h"
#include "cc/scheduler/rolling_time_delta_history.h"
#include "cc/scheduler/scheduler.h"
#include "cc/trees/layer_tree_host_impl.h"
#include "cc/trees/proxy.h"

namespace base { class SingleThreadTaskRunner; }

namespace cc {

class ContextProvider;
class InputHandlerClient;
class LayerTreeHost;
class ResourceUpdateQueue;
class Scheduler;
class ScopedThreadProxy;

class ThreadProxy : public Proxy,
                    LayerTreeHostImplClient,
                    SchedulerClient,
                    ResourceUpdateControllerClient {
 public:
  static scoped_ptr<Proxy> Create(
      LayerTreeHost* layer_tree_host,
      scoped_refptr<base::SingleThreadTaskRunner> impl_task_runner);

  virtual ~ThreadProxy();

  // Proxy implementation
  virtual bool CompositeAndReadback(void* pixels, gfx::Rect rect) OVERRIDE;
  virtual void FinishAllRendering() OVERRIDE;
  virtual bool IsStarted() const OVERRIDE;
  virtual void SetLayerTreeHostClientReady() OVERRIDE;
  virtual void SetVisible(bool visible) OVERRIDE;
  virtual void CreateAndInitializeOutputSurface() OVERRIDE;
  virtual const RendererCapabilities& GetRendererCapabilities() const OVERRIDE;
  virtual void SetNeedsAnimate() OVERRIDE;
  virtual void SetNeedsCommit() OVERRIDE;
  virtual void SetNeedsRedraw(gfx::Rect damage_rect) OVERRIDE;
  virtual void SetDeferCommits(bool defer_commits) OVERRIDE;
  virtual bool CommitRequested() const OVERRIDE;
  virtual void MainThreadHasStoppedFlinging() OVERRIDE;
  virtual void Start(scoped_ptr<OutputSurface> first_output_surface) OVERRIDE;
  virtual void Stop() OVERRIDE;
  virtual size_t MaxPartialTextureUpdates() const OVERRIDE;
  virtual void AcquireLayerTextures() OVERRIDE;
  virtual void ForceSerializeOnSwapBuffers() OVERRIDE;
  virtual skia::RefPtr<SkPicture> CapturePicture() OVERRIDE;
  virtual scoped_ptr<base::Value> AsValue() const OVERRIDE;
  virtual bool CommitPendingForTesting() OVERRIDE;
  virtual std::string SchedulerStateAsStringForTesting() OVERRIDE;

  // LayerTreeHostImplClient implementation
  virtual void DidTryInitializeRendererOnImplThread(
      bool success,
      scoped_refptr<ContextProvider> offscreen_context_provider) OVERRIDE;
  virtual void DidLoseOutputSurfaceOnImplThread() OVERRIDE;
  virtual void OnSwapBuffersCompleteOnImplThread() OVERRIDE;
  virtual void BeginFrameOnImplThread(const BeginFrameArgs& args) OVERRIDE;
  virtual void OnCanDrawStateChanged(bool can_draw) OVERRIDE;
  virtual void OnHasPendingTreeStateChanged(bool has_pending_tree) OVERRIDE;
  virtual void SetNeedsRedrawOnImplThread() OVERRIDE;
  virtual void SetNeedsRedrawRectOnImplThread(gfx::Rect dirty_rect) OVERRIDE;
  virtual void DidInitializeVisibleTileOnImplThread() OVERRIDE;
  virtual void SetNeedsCommitOnImplThread() OVERRIDE;
  virtual void PostAnimationEventsToMainThreadOnImplThread(
      scoped_ptr<AnimationEventsVector> queue,
      base::Time wall_clock_time) OVERRIDE;
  virtual bool ReduceContentsTextureMemoryOnImplThread(size_t limit_bytes,
                                                       int priority_cutoff)
      OVERRIDE;
  virtual void ReduceWastedContentsTextureMemoryOnImplThread() OVERRIDE;
  virtual void SendManagedMemoryStats() OVERRIDE;
  virtual bool IsInsideDraw() OVERRIDE;
  virtual void RenewTreePriority() OVERRIDE;
  virtual void RequestScrollbarAnimationOnImplThread(base::TimeDelta delay)
      OVERRIDE;
  virtual void DidActivatePendingTree() OVERRIDE;

  // SchedulerClient implementation
  virtual void SetNeedsBeginFrameOnImplThread(bool enable) OVERRIDE;
  virtual void ScheduledActionSendBeginFrameToMainThread() OVERRIDE;
  virtual ScheduledActionDrawAndSwapResult
      ScheduledActionDrawAndSwapIfPossible() OVERRIDE;
  virtual ScheduledActionDrawAndSwapResult ScheduledActionDrawAndSwapForced()
      OVERRIDE;
  virtual void ScheduledActionCommit() OVERRIDE;
  virtual void ScheduledActionCheckForCompletedTileUploads() OVERRIDE;
  virtual void ScheduledActionActivatePendingTreeIfNeeded() OVERRIDE;
  virtual void ScheduledActionBeginOutputSurfaceCreation() OVERRIDE;
  virtual void ScheduledActionAcquireLayerTexturesForMainThread() OVERRIDE;
  virtual void DidAnticipatedDrawTimeChange(base::TimeTicks time) OVERRIDE;
  virtual base::TimeDelta DrawDurationEstimate() OVERRIDE;

  // ResourceUpdateControllerClient implementation
  virtual void ReadyToFinalizeTextureUpdates() OVERRIDE;

 private:
  ThreadProxy(LayerTreeHost* layer_tree_host,
              scoped_refptr<base::SingleThreadTaskRunner> impl_task_runner);

  struct BeginFrameAndCommitState {
    BeginFrameAndCommitState();
    ~BeginFrameAndCommitState();

    base::TimeTicks monotonic_frame_begin_time;
    scoped_ptr<ScrollAndScaleSet> scroll_info;
    size_t memory_allocation_limit_bytes;
  };

  // Called on main thread.
  void BeginFrameOnMainThread(
      scoped_ptr<BeginFrameAndCommitState> begin_frame_state);
  void DidCommitAndDrawFrame();
  void DidCompleteSwapBuffers();
  void SetAnimationEvents(scoped_ptr<AnimationEventsVector> queue,
                          base::Time wall_clock_time);
  void DoCreateAndInitializeOutputSurface();
  // |capabilities| is set only when |success| is true.
  void OnOutputSurfaceInitializeAttempted(
      bool success,
      const RendererCapabilities& capabilities);

  // Called on impl thread.
  struct ReadbackRequest;
  struct CommitPendingRequest;
  struct SchedulerStateRequest;

  void ForceCommitOnImplThread(CompletionEvent* completion);
  void StartCommitOnImplThread(
      CompletionEvent* completion,
      ResourceUpdateQueue* queue,
      scoped_refptr<cc::ContextProvider> offscreen_context_provider);
  void BeginFrameAbortedByMainThreadOnImplThread();
  void RequestReadbackOnImplThread(ReadbackRequest* request);
  void FinishAllRenderingOnImplThread(CompletionEvent* completion);
  void InitializeImplOnImplThread(CompletionEvent* completion);
  void SetLayerTreeHostClientReadyOnImplThread();
  void SetVisibleOnImplThread(CompletionEvent* completion, bool visible);
  void HasInitializedOutputSurfaceOnImplThread(
      CompletionEvent* completion,
      bool* has_initialized_output_surface);
  void InitializeOutputSurfaceOnImplThread(
      CompletionEvent* completion,
      scoped_ptr<OutputSurface> output_surface,
      scoped_refptr<ContextProvider> offscreen_context_provider,
      bool* success,
      RendererCapabilities* capabilities);
  void FinishGLOnImplThread(CompletionEvent* completion);
  void LayerTreeHostClosedOnImplThread(CompletionEvent* completion);
  void AcquireLayerTexturesForMainThreadOnImplThread(
      CompletionEvent* completion);
  ScheduledActionDrawAndSwapResult ScheduledActionDrawAndSwapInternal(
      bool forced_draw);
  void ForceSerializeOnSwapBuffersOnImplThread(CompletionEvent* completion);
  void CheckOutputSurfaceStatusOnImplThread();
  void CommitPendingOnImplThreadForTesting(CommitPendingRequest* request);
  void SchedulerStateAsStringOnImplThreadForTesting(
      SchedulerStateRequest* request);
  void CapturePictureOnImplThread(CompletionEvent* completion,
                                  skia::RefPtr<SkPicture>* picture);
  void AsValueOnImplThread(CompletionEvent* completion,
                           base::DictionaryValue* state) const;
  void RenewTreePriorityOnImplThread();
  void DidSwapUseIncompleteTileOnImplThread();
  void StartScrollbarAnimationOnImplThread();
  void MainThreadHasStoppedFlingingOnImplThread();
  void ProactiveBeginFrameOnImplThread();

  // Accessed on main thread only.

  // Set only when SetNeedsAnimate is called.
  bool animate_requested_;
  // Set only when SetNeedsCommit is called.
  bool commit_requested_;
  // Set by SetNeedsCommit and SetNeedsAnimate.
  bool commit_request_sent_to_impl_thread_;
  // Set by BeginFrameOnMainThread
  bool created_offscreen_context_provider_;
  base::CancelableClosure output_surface_creation_callback_;
  LayerTreeHost* layer_tree_host_;
  RendererCapabilities renderer_capabilities_main_thread_copy_;
  bool started_;
  bool textures_acquired_;
  bool in_composite_and_readback_;
  bool manage_tiles_pending_;
  // Weak pointer to use when posting tasks to the impl thread.
  base::WeakPtr<ThreadProxy> impl_thread_weak_ptr_;
  // Holds the first output surface passed from Start. Should not be used for
  // anything else.
  scoped_ptr<OutputSurface> first_output_surface_;

  base::WeakPtrFactory<ThreadProxy> weak_factory_on_impl_thread_;

  base::WeakPtr<ThreadProxy> main_thread_weak_ptr_;
  base::WeakPtrFactory<ThreadProxy> weak_factory_;

  scoped_ptr<LayerTreeHostImpl> layer_tree_host_impl_;

  scoped_ptr<Scheduler> scheduler_on_impl_thread_;

  // Set when the main thread is waiting on a
  // ScheduledActionSendBeginFrameToMainThread to be issued.
  CompletionEvent*
      begin_frame_sent_to_main_thread_completion_event_on_impl_thread_;

  // Set when the main thread is waiting on a readback.
  ReadbackRequest* readback_request_on_impl_thread_;

  // Set when the main thread is waiting on a commit to complete.
  CompletionEvent* commit_completion_event_on_impl_thread_;

  // Set when the main thread is waiting on a pending tree activation.
  CompletionEvent* completion_event_for_commit_held_on_tree_activation_;

  // Set when the main thread is waiting on layers to be drawn.
  CompletionEvent* texture_acquisition_completion_event_on_impl_thread_;

  scoped_ptr<ResourceUpdateController>
      current_resource_update_controller_on_impl_thread_;

  // Set when the next draw should post DidCommitAndDrawFrame to the main
  // thread.
  bool next_frame_is_newly_committed_frame_on_impl_thread_;

  bool throttle_frame_production_;
  bool begin_frame_scheduling_enabled_;
  bool using_synchronous_renderer_compositor_;

  bool inside_draw_;

  bool defer_commits_;
  scoped_ptr<BeginFrameAndCommitState> pending_deferred_commit_;

  base::TimeTicks smoothness_takes_priority_expiration_time_;
  bool renew_tree_priority_on_impl_thread_pending_;

  RollingTimeDeltaHistory draw_duration_history_;

  DISALLOW_COPY_AND_ASSIGN(ThreadProxy);
};

}  // namespace cc

#endif  // CC_TREES_THREAD_PROXY_H_
