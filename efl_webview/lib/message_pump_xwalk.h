// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EFL_WEBVIEW_LIBMESSAGE_PUMP_XWALK_H_
#define EFL_WEBVIEW_LIBMESSAGE_PUMP_XWALK_H_

#include "base/base_export.h"
#include "base/event_types.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_pump.h"
#include "base/message_pump_dispatcher.h"
#include "base/message_pump_observer.h"
#include "base/observer_list.h"
#include "base/time.h"

namespace xwalk {

class MessagePumpXWalk : public base::MessagePump {
public:
  MessagePumpXWalk();

  // Like MessagePump::Run, but events are routed through dispatcher.
  virtual void RunWithDispatcher(Delegate* delegate,
                                 base::MessagePumpDispatcher* dispatcher);

  void HandleDispatch();

  // Adds an Observer, which will start receiving notifications immediately.
  void AddObserver(base::MessagePumpObserver* observer);

  // Removes an Observer.  It is safe to call this method while an Observer is
  // receiving a notification callback.
  void RemoveObserver(base::MessagePumpObserver* observer);

  // Overridden from MessagePump:
  virtual void Run(Delegate* delegate) OVERRIDE;
  virtual void Quit() OVERRIDE;
  virtual void ScheduleWork() OVERRIDE;
  virtual void ScheduleDelayedWork(
      const base::TimeTicks& delayed_work_time) OVERRIDE;

protected:
  virtual ~MessagePumpXWalk();

  ObserverList<base::MessagePumpObserver>& observers();

private:
  struct Private;
  scoped_ptr<Private> private_;

  DISALLOW_COPY_AND_ASSIGN(MessagePumpXWalk);
};

}  // namespace xwalk

#endif  // EFL_WEBVIEW_LIBMESSAGE_PUMP_XWALK_H_
