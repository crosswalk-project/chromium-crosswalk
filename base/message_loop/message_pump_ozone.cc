// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_loop/message_pump_ozone.h"

#include "base/logging.h"
#include "base/message_loop/message_loop.h"

namespace base {

MessagePumpOzone::MessagePumpOzone()
    : MessagePumpLibevent() {
}

MessagePumpOzone::~MessagePumpOzone() {
}

void MessagePumpOzone::AddObserver(MessagePumpObserver* observer) {
  observers_.AddObserver(observer);
}

void MessagePumpOzone::RemoveObserver(MessagePumpObserver* observer) {
  observers_.RemoveObserver(observer);
}

void MessagePumpOzone::WillProcessEvent(const NativeEvent& event) {
  FOR_EACH_OBSERVER(MessagePumpObserver,
                    observers_,
                    WillProcessEvent(event));
}

void MessagePumpOzone::DidProcessEvent(const NativeEvent& event) {
  FOR_EACH_OBSERVER(MessagePumpObserver,
                    observers_,
                    DidProcessEvent(event));
}

// static
MessagePumpOzone* MessagePumpOzone::Current() {
  MessageLoopForUI* loop = MessageLoopForUI::current();
  return static_cast<MessagePumpOzone*>(loop->pump_ui());
}

void MessagePumpOzone::AddDispatcherForRootWindow(
    MessagePumpDispatcher* dispatcher) {
  // Only one root window is supported.
  DCHECK_EQ(dispatcher_.size(), 0U);
  dispatcher_.insert(dispatcher_.begin(), dispatcher);
}

void MessagePumpOzone::RemoveDispatcherForRootWindow(
      MessagePumpDispatcher* dispatcher) {
  DCHECK_EQ(dispatcher_.size(), 1U);
  dispatcher_.pop_back();
}

uint32_t MessagePumpOzone::Dispatch(const NativeEvent& dev) {
  uint32_t result = POST_DISPATCH_NONE;
  WillProcessEvent(dev);
  if (!dispatcher_.empty()) {
    result = dispatcher_[0]->Dispatch(dev);
  }
  DidProcessEvent(dev);
  return result;
}

// This code assumes that the caller tracks the lifetime of the |dispatcher|.
void MessagePumpOzone::RunWithDispatcher(
    Delegate* delegate, MessagePumpDispatcher* dispatcher) {
  dispatcher_.push_back(dispatcher);
  Run(delegate);
  dispatcher_.pop_back();
}

}  // namespace base
