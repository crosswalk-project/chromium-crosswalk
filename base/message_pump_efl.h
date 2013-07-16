// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_MESSAGE_PUMP_EFL_H_
#define BASE_MESSAGE_PUMP_EFL_H_

#include "base/base_export.h"
#include "base/event_types.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_pump.h"
#include "base/message_pump_dispatcher.h"
#include "base/message_pump_observer.h"
#include "base/observer_list.h"
#include "base/time.h"

typedef struct _XDisplay Display;

namespace base {

class BASE_EXPORT MessagePumpEfl : public MessagePump {
public:
  MessagePumpEfl();

  // Like MessagePump::Run, but events are routed through dispatcher.
  virtual void RunWithDispatcher(Delegate* delegate,
                                 MessagePumpDispatcher* dispatcher);

  void HandleDispatch();

  // Adds an Observer, which will start receiving notifications immediately.
  void AddObserver(MessagePumpObserver* observer);

  // Removes an Observer.  It is safe to call this method while an Observer is
  // receiving a notification callback.
  void RemoveObserver(MessagePumpObserver* observer);

  // Overridden from MessagePump:
  virtual void Run(Delegate* delegate) OVERRIDE;
  virtual void Quit() OVERRIDE;
  virtual void ScheduleWork() OVERRIDE;
  virtual void ScheduleDelayedWork(const TimeTicks& delayed_work_time) OVERRIDE;

  // Returns default X Display.
  static Display* GetDefaultXDisplay();

protected:
  virtual ~MessagePumpEfl();

  // Returns the dispatcher for the current run state (|state_->dispatcher|).
  MessagePumpDispatcher* GetDispatcher();

  ObserverList<MessagePumpObserver>& observers();

private:
  struct Private;
  scoped_ptr<Private> private_;

  DISALLOW_COPY_AND_ASSIGN(MessagePumpEfl);
};

typedef MessagePumpEfl MessagePumpForUI;

}  // namespace base

#endif  // BASE_MESSAGE_PUMP_EFL_H_
