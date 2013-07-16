// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_EFL_EVENT_H_
#define UI_GFX_EFL_EVENT_H_

#include "ui/base/ui_export.h"

namespace gfx {

class UI_EXPORT EflEvent {
  public:

  enum EflEventType {
    EventTypeKeyUp,
    EventTypeKeyDown
  };

  EflEvent(EflEventType type, void* efl_event) : type_(type),
                                                 efl_event_(efl_event) {}

  EflEventType eventType() const { return type_; }
  void* eflEvent() { return efl_event_; }

  private:
    EflEventType type_;
    void* efl_event_;
};

} // namespace gfx

#endif // UI_GFX_EFL_EVENT_H_
