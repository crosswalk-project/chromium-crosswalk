// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_pump_efl.h"

#include <Ecore.h>
#include <Ecore_X.h>
#include <X11/Xlib.h>

#include "base/debug/trace_event.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "base/threading/platform_thread.h"

namespace {
static const int ecorePipeMessageSize = 1;
static const char wakupEcorePipeMessage[] = "W";

void WakeUpEvent(void* data, void*, unsigned int)
{
  static_cast<base::MessagePumpEfl*>(data)->HandleDispatch();
}
}  // namespace


namespace base {

// We may make recursive calls to Run, so we save state that needs to be
// separate between them in this structure type.
struct RunState {
  MessagePump::Delegate* delegate;
  MessagePumpDispatcher* dispatcher;

  // Used to flag that the current Run() invocation should return ASAP.
  bool should_quit;

  // Used to count how many Run() invocations are on the stack.
  int run_depth;
};

struct MessagePumpEfl::Private {
  RunState* state_;

  // This is the time when we need to do delayed work.
  TimeTicks delayed_work_time_;

  // List of observers.
  ObserverList<MessagePumpObserver> observers_;

  Ecore_Pipe* wakeup_pipe_;
};

MessagePumpEfl::MessagePumpEfl()
    : private_(new Private) {
  private_->state_ = NULL;
  private_->wakeup_pipe_ = ecore_pipe_add(WakeUpEvent, this);
}

MessagePumpEfl::~MessagePumpEfl() {
  ecore_pipe_del(private_->wakeup_pipe_);
}

void MessagePumpEfl::RunWithDispatcher(Delegate* delegate,
                                       MessagePumpDispatcher* dispatcher) {
#ifndef NDEBUG
  // Make sure we only run this on one thread. X/GTK only has one message pump
  // so we can only have one UI loop per process.
  static base::PlatformThreadId thread_id = base::PlatformThread::CurrentId();
  DCHECK(thread_id == base::PlatformThread::CurrentId()) <<
      "Running MessagePumpEfl on two different threads; "
      "this is unsupported by Ecore!";
#endif

  RunState state;
  state.delegate = delegate;
  state.dispatcher = dispatcher;
  state.should_quit = false;
  state.run_depth = private_->state_ ? private_->state_->run_depth + 1 : 1;

  RunState* previous_state = private_->state_;
  private_->state_ = &state;

  bool more_work_is_plausible = true;

  for (;;) {
    ecore_main_loop_iterate();
    more_work_is_plausible = false;
    if (private_->state_->should_quit)
      break;

    more_work_is_plausible |= private_->state_->delegate->DoWork();
    if (private_->state_->should_quit)
      break;

    more_work_is_plausible |=
        private_->state_->delegate->DoDelayedWork(&private_->delayed_work_time_);
    if (private_->state_->should_quit)
      break;

    if (more_work_is_plausible)
      continue;

    more_work_is_plausible = private_->state_->delegate->DoIdleWork();
    if (private_->state_->should_quit)
      break;
  }

  private_->state_ = previous_state;
}

void MessagePumpEfl::HandleDispatch() {
  private_->state_->delegate->DoWork();
  if (private_->state_->should_quit)
    return;

  private_->state_->delegate->DoDelayedWork(&private_->delayed_work_time_);
}

void MessagePumpEfl::AddObserver(MessagePumpObserver* observer) {
  private_->observers_.AddObserver(observer);
}

void MessagePumpEfl::RemoveObserver(MessagePumpObserver* observer) {
  private_->observers_.RemoveObserver(observer);
}

void MessagePumpEfl::Run(Delegate* delegate) {
  RunWithDispatcher(delegate, NULL);
}

void MessagePumpEfl::Quit() {
  if (private_->state_) {
    private_->state_->should_quit = true;
  } else {
    NOTREACHED() << "Quit called outside Run!";
  }
}

void MessagePumpEfl::ScheduleWork() {
  // This can be called on any thread, so we don't want to touch any state
  // variables as we would then need locks all over.  This ensures that if
  // we are sleeping in a poll that we will wake up.
  if (HANDLE_EINTR(ecore_pipe_write(private_->wakeup_pipe_, wakupEcorePipeMessage, ecorePipeMessageSize)) != 1) {
    NOTREACHED() << "Could not write to the UI message loop wakeup pipe!";
  }
}

void MessagePumpEfl::ScheduleDelayedWork(const TimeTicks& delayed_work_time) {
  // We need to wake up the loop in case the poll timeout needs to be
  // adjusted.  This will cause us to try to do work, but that's ok.
  private_->delayed_work_time_ = delayed_work_time;
  ScheduleWork();
}

MessagePumpDispatcher* MessagePumpEfl::GetDispatcher() {
  return private_->state_ ? private_->state_->dispatcher : NULL;
}

ObserverList<MessagePumpObserver>& MessagePumpEfl::observers() {
  return private_->observers_;
}

// static
Display* MessagePumpEfl::GetDefaultXDisplay() {
  static Ecore_X_Display* display = ecore_x_display_get();
  if (!display) {
    // Ecore_X has not been initialized, which is a decision we wish to
    // support, for example for the GPU process.
    static Display* xdisplay = XOpenDisplay(NULL);
    return xdisplay;
  }
  return static_cast<Display*>(display);
}

}  // namespace base
