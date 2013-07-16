// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "efl_webview/lib/message_pump_xwalk.h"

#include <Ecore.h>
#include <Ecore_X.h>

#include "base/debug/trace_event.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "base/threading/platform_thread.h"

namespace {

const int ecorePipeMessageSize = 1;
const char wakupEcorePipeMessage[] = "W";

void WakeUpEvent(void* data, void*, unsigned int)
{
  static_cast<xwalk::MessagePumpXWalk*>(data)->HandleDispatch();
}

}  // namespace

namespace xwalk {

struct MessagePumpXWalk::Private {
  MessagePump::Delegate* delegate;

  bool should_quit;

  // This is the time when we need to do delayed work.
  base::TimeTicks delayed_work_time;

  // List of observers.
  ObserverList<base::MessagePumpObserver> observers;

  Ecore_Pipe* wakeup_pipe;
};

MessagePumpXWalk::MessagePumpXWalk()
    : private_(new Private) {
  private_->wakeup_pipe = ecore_pipe_add(WakeUpEvent, this);
  private_->delegate = base::MessageLoopForUI::current();
  private_->should_quit = false;
}

MessagePumpXWalk::~MessagePumpXWalk() {
  ecore_pipe_del(private_->wakeup_pipe);
}

void MessagePumpXWalk::RunWithDispatcher(Delegate* delegate,
    base::MessagePumpDispatcher* dispatcher) {
  NOTREACHED();
}

void MessagePumpXWalk::HandleDispatch() {
  // FIXME: dshwang does not have confidence about this implementation.
  // Need to check by efl experts.
  ecore_main_loop_iterate();

  bool more_work_is_plausible = private_->delegate->DoWork();
  if (private_->should_quit)
    return;

  more_work_is_plausible |=
      private_->delegate->DoDelayedWork(&private_->delayed_work_time);
  if (private_->should_quit)
    return;

  if (!more_work_is_plausible)
    more_work_is_plausible |= private_->delegate->DoIdleWork();

  if (more_work_is_plausible)
    ScheduleWork();
}

void MessagePumpXWalk::AddObserver(base::MessagePumpObserver* observer) {
  private_->observers.AddObserver(observer);
}

void MessagePumpXWalk::RemoveObserver(base::MessagePumpObserver* observer) {
  private_->observers.RemoveObserver(observer);
}

void MessagePumpXWalk::Run(Delegate* delegate) {
  NOTREACHED();
}

void MessagePumpXWalk::Quit() {
  private_->should_quit = true;
}

void MessagePumpXWalk::ScheduleWork() {
  // This can be called on any thread, so we don't want to touch any state
  // variables as we would then need locks all over.  This ensures that if
  // we are sleeping in a poll that we will wake up.
  if (HANDLE_EINTR(ecore_pipe_write(private_->wakeup_pipe,
                                    wakupEcorePipeMessage,
                                    ecorePipeMessageSize)) != 1) {
    NOTREACHED() << "Could not write to the UI message loop wakeup pipe!";
  }
}

void MessagePumpXWalk::ScheduleDelayedWork(
    const base::TimeTicks& delayed_work_time) {
  // We need to wake up the loop in case the poll timeout needs to be
  // adjusted.  This will cause us to try to do work, but that's ok.
  private_->delayed_work_time = delayed_work_time;
  ScheduleWork();
}

ObserverList<base::MessagePumpObserver>& MessagePumpXWalk::observers() {
  return private_->observers;
}

}  // namespace xwalk
