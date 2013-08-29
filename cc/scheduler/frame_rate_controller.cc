// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/scheduler/frame_rate_controller.h"

#include "base/debug/trace_event.h"
#include "base/logging.h"
#include "cc/base/thread.h"
#include "cc/debug/frame_rate_counter.h"
#include "cc/scheduler/delay_based_time_source.h"
#include "cc/scheduler/time_source.h"
#include "cc/trees/layer_tree_host_impl.h"

namespace {

// Assume the default Maximum FPS is 60.
static const double kDefaultMaxFPS = 60.0;

// kRetroactiveTickThreshold is a threshold to determine whether it needs to
// switch to non-throttle ticking from throttle ticking mode. For example,
// when the current average FPS is less than MaxFPS*kRetroactiveTickThreshold,
// it will need to switch to non-throttle ticking mode.
static const double kRetroactiveTickThreshold = 0.950;

// kThrottlingTickThreshold is a threshold to determine whether it needs to
// switch back to throttle ticking mode from non-throttle ticking mode.
// For example, when the current average FPS is bigger than
// MaxFPS*kThrottlingTickThreshold, it will need to switch back to throttle
// ticking mode.
static const double kThrottlingTickThreshold = 0.966;

}  // namespace

namespace cc {

class FrameRateControllerTimeSourceAdapter : public TimeSourceClient {
 public:
  static scoped_ptr<FrameRateControllerTimeSourceAdapter> Create(
      FrameRateController* frame_rate_controller) {
    return make_scoped_ptr(
        new FrameRateControllerTimeSourceAdapter(frame_rate_controller));
  }
  virtual ~FrameRateControllerTimeSourceAdapter() {}

  virtual void OnTimerTick() OVERRIDE { frame_rate_controller_->OnTimerTick(); }

 private:
  explicit FrameRateControllerTimeSourceAdapter(
      FrameRateController* frame_rate_controller)
      : frame_rate_controller_(frame_rate_controller) {}

  FrameRateController* frame_rate_controller_;
};

FrameRateController::FrameRateController(scoped_refptr<TimeSource> timer,
                                         Thread* thread,
                                         LayerTreeHostImpl* layerTreeHostImpl)
    : client_(NULL),
      num_frames_pending_(0),
      max_frames_pending_(0),
      time_source_(timer),
      active_(false),
      swap_buffers_complete_supported_(true),
      is_time_source_throttling_(true),
      weak_factory_(this),
      thread_(thread),
      retroactive_tick_(false),
      layer_tree_host_impl_(layerTreeHostImpl),
      max_fps_(kDefaultMaxFPS) {
  time_source_client_adapter_ =
      FrameRateControllerTimeSourceAdapter::Create(this);
  time_source_->SetClient(time_source_client_adapter_.get());
}

FrameRateController::FrameRateController(Thread* thread)
    : client_(NULL),
      num_frames_pending_(0),
      max_frames_pending_(0),
      active_(false),
      swap_buffers_complete_supported_(true),
      is_time_source_throttling_(false),
      weak_factory_(this),
      thread_(thread),
      retroactive_tick_(false),
      layer_tree_host_impl_(NULL),
      max_fps_(kDefaultMaxFPS) {}

FrameRateController::~FrameRateController() {
  if (is_time_source_throttling_)
    time_source_->SetActive(false);
}

void FrameRateController::SetActive(bool active) {
  if (active_ == active)
    return;
  TRACE_EVENT1("cc", "FrameRateController::SetActive", "active", active);
  active_ = active;

  if (is_time_source_throttling_ && !retroactive_tick_) {
    time_source_->SetActive(active);
  } else {
    if (active)
      PostManualTick();
    else
      weak_factory_.InvalidateWeakPtrs();
  }
}

void FrameRateController::SetMaxFramesPending(int max_frames_pending) {
  DCHECK_GE(max_frames_pending, 0);
  max_frames_pending_ = max_frames_pending;
}

void FrameRateController::SetTimebaseAndInterval(base::TimeTicks timebase,
                                                 base::TimeDelta interval) {
  if (is_time_source_throttling_)
    time_source_->SetTimebaseAndInterval(timebase, interval);

  double time_interval = interval.InSecondsF();
  max_fps_ = (time_interval > 0.0) ? 1.0 / time_interval : kDefaultMaxFPS;
}

void FrameRateController::SetSwapBuffersCompleteSupported(bool supported) {
  swap_buffers_complete_supported_ = supported;
}

void FrameRateController::OnTimerTick() {
  DCHECK(active_);

  // Check if we have too many frames in flight.
  bool throttled =
      max_frames_pending_ && num_frames_pending_ >= max_frames_pending_;
  TRACE_COUNTER_ID1("cc", "ThrottledVSyncInterval", thread_, throttled);

  if (client_)
    client_->VSyncTick(throttled);

  if (is_time_source_throttling_ && layer_tree_host_impl_) {
    double fps = layer_tree_host_impl_->fps_counter()->GetAverageFPS();
    if (!retroactive_tick_ && fps > 0.0
        && fps < (max_fps_ * kRetroactiveTickThreshold)) {
      time_source_->SetActive(false);
      retroactive_tick_ = true;
    } else if (retroactive_tick_
               && fps > (max_fps_ * kThrottlingTickThreshold)) {
      time_source_->SetActive(true);
      retroactive_tick_ = false;
    }
  }

  if (swap_buffers_complete_supported_ && ((!is_time_source_throttling_ &&
      !throttled) || retroactive_tick_))
    PostManualTick();
}

void FrameRateController::PostManualTick() {
  if (active_) {
    thread_->PostTask(base::Bind(&FrameRateController::ManualTick,
                                 weak_factory_.GetWeakPtr()));
  }
}

void FrameRateController::ManualTick() { OnTimerTick(); }

void FrameRateController::DidBeginFrame() {
  if (swap_buffers_complete_supported_)
    num_frames_pending_++;
  else if (!is_time_source_throttling_ || retroactive_tick_)
    PostManualTick();
}

void FrameRateController::DidFinishFrame() {
  DCHECK(swap_buffers_complete_supported_);

  num_frames_pending_--;
  if (!is_time_source_throttling_ || retroactive_tick_)
    PostManualTick();
}

void FrameRateController::DidAbortAllPendingFrames() {
  num_frames_pending_ = 0;
}

base::TimeTicks FrameRateController::NextTickTime() {
  if (is_time_source_throttling_ && !retroactive_tick_)
    return time_source_->NextTickTime();

  return base::TimeTicks();
}

base::TimeTicks FrameRateController::LastTickTime() {
  if (is_time_source_throttling_ && !retroactive_tick_)
    return time_source_->LastTickTime();

  return base::TimeTicks::Now();
}

}  // namespace cc
