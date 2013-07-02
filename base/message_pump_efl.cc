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

struct MessagePumpEfl::Private {
  MessagePump::Delegate* delegate;

  bool should_quit;

  // This is the time when we need to do delayed work.
  TimeTicks delayed_work_time;

  // List of observers.
  ObserverList<MessagePumpObserver> observers;

  Ecore_Pipe* wakeup_pipe;
};

MessagePumpEfl::MessagePumpEfl()
    : private_(new Private) {
  private_->wakeup_pipe = ecore_pipe_add(WakeUpEvent, this);
  private_->delegate = base::MessageLoopForUI::current();
  private_->should_quit = false;
}

MessagePumpEfl::~MessagePumpEfl() {
  ecore_pipe_del(private_->wakeup_pipe);
}

void MessagePumpEfl::RunWithDispatcher(Delegate* delegate,
                                       MessagePumpDispatcher* dispatcher) {
  NOTREACHED();
}

void MessagePumpEfl::HandleDispatch() {
  // FIXME: dshwang does not have confidence about this implementation. Need to check by efl experts.
  ecore_main_loop_iterate();

  bool more_work_is_plausible = private_->delegate->DoWork();
  if (private_->should_quit)
    return;

  more_work_is_plausible |= private_->delegate->DoDelayedWork(&private_->delayed_work_time);
  if (private_->should_quit)
    return;

  if (!more_work_is_plausible)
    more_work_is_plausible |= private_->delegate->DoIdleWork();

  if (more_work_is_plausible)
    ScheduleWork();
}

void MessagePumpEfl::AddObserver(MessagePumpObserver* observer) {
  private_->observers.AddObserver(observer);
}

void MessagePumpEfl::RemoveObserver(MessagePumpObserver* observer) {
  private_->observers.RemoveObserver(observer);
}

void MessagePumpEfl::Run(Delegate* delegate) {
  NOTREACHED();
}

void MessagePumpEfl::Quit() {
  private_->should_quit = true;
}

void MessagePumpEfl::ScheduleWork() {
  // This can be called on any thread, so we don't want to touch any state
  // variables as we would then need locks all over.  This ensures that if
  // we are sleeping in a poll that we will wake up.
  if (HANDLE_EINTR(ecore_pipe_write(private_->wakeup_pipe, wakupEcorePipeMessage, ecorePipeMessageSize)) != 1) {
    NOTREACHED() << "Could not write to the UI message loop wakeup pipe!";
  }
}

void MessagePumpEfl::ScheduleDelayedWork(const TimeTicks& delayed_work_time) {
  // We need to wake up the loop in case the poll timeout needs to be
  // adjusted.  This will cause us to try to do work, but that's ok.
  private_->delayed_work_time = delayed_work_time;
  ScheduleWork();
}

ObserverList<MessagePumpObserver>& MessagePumpEfl::observers() {
  return private_->observers;
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
