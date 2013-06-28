// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/time.h"
#include "content/browser/renderer_host/web_input_event_factory_efl.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"

#include <Evas.h>

namespace content {

namespace {

// Taken from web_input_event_aura.h
const int kPixelsPerTick = 53;

static int eflModifierToWebEventModifiers(const Evas_Modifier* efl_modidier,
                                          int buttons) {
  int modifiers = 0;
  // FIXME: check what else we need to handle from WebKit::WebInputEvent::Modifiers
  if (evas_key_modifier_is_set(efl_modidier, "Shift"))
    modifiers |= WebKit::WebInputEvent::ShiftKey;
  if (evas_key_modifier_is_set(efl_modidier, "Control"))
    modifiers |= WebKit::WebInputEvent::ControlKey;
  if (evas_key_modifier_is_set(efl_modidier, "Alt"))
    modifiers |= WebKit::WebInputEvent::AltKey;
  if (evas_key_modifier_is_set(efl_modidier, "Meta"))
    modifiers |= WebKit::WebInputEvent::MetaKey;
  if(buttons & 1)
    modifiers |= WebKit::WebInputEvent::LeftButtonDown;
  if(buttons & 2)
    modifiers |= WebKit::WebInputEvent::MiddleButtonDown;
  if(buttons & 3)
    modifiers |= WebKit::WebInputEvent::RightButtonDown;
  return modifiers;
}

static int clickCountFromEflFlags(unsigned int flags)
{
    if (flags & EVAS_BUTTON_TRIPLE_CLICK)
        return 3;
    else if (flags & EVAS_BUTTON_DOUBLE_CLICK)
        return 2;
    else
        return 1;
}

template<typename T>
WebKit::WebMouseEvent toWebMouseEvent(WebKit::WebInputEvent::Type type, T* event)
{
  WebKit::WebMouseEvent result;

  result.timeStampSeconds = base::Time::UnixEpoch().ToDoubleT();
  result.modifiers = eflModifierToWebEventModifiers(event->modifiers, event->button);
  result.x = event->canvas.x;
  result.y = event->canvas.y;
  result.windowX = result.x;
  result.windowY = result.y;
  // FIXME: initialise global position.
  //  result.globalX = ;
  //  result.globalY = ;
  result.clickCount = clickCountFromEflFlags(event->flags);
  result.type = type;
  result.button = static_cast<WebKit::WebMouseEvent::Button>(event->button - 1);

  return result;
}

/*
template<typename T>
WebKit::WebKeyboardEvent toWebKeyboardEvent(T* key)
{
  WebKit::WebKeyboardEvent event;
  return event;
}
*/

} // namespace

NativeWebKeyboardEvent keyboardEvent(Evas_Event_Key_Down* key_down)
{
  DCHECK(key_down);
  // FIXME: todo
  NativeWebKeyboardEvent event;
  return event;
}

NativeWebKeyboardEvent keyboardEvent(Evas_Event_Key_Up* key_up)
{
  DCHECK(key_up);
  // FIXME: todo
  NativeWebKeyboardEvent event;
  return event;
}

WebKit::WebMouseEvent mouseEvent(Evas_Event_Mouse_Down* mouse_down)
{
  DCHECK(mouse_down);
  return toWebMouseEvent(WebKit::WebInputEvent::MouseDown, mouse_down);
}

WebKit::WebMouseEvent mouseEvent(Evas_Event_Mouse_Up* mouse_up)
{
  DCHECK(mouse_up);
  return toWebMouseEvent(WebKit::WebInputEvent::MouseUp, mouse_up);
}

WebKit::WebMouseEvent mouseEvent(Evas_Event_Mouse_Move* mouse_move)
{
  DCHECK(mouse_move);

  WebKit::WebMouseEvent result;

  result.timeStampSeconds = base::Time::UnixEpoch().ToDoubleT();
  result.modifiers = eflModifierToWebEventModifiers(mouse_move->modifiers, mouse_move->buttons);
  result.x = mouse_move->cur.canvas.x;
  result.y = mouse_move->cur.canvas.y;
  result.windowX = result.x;
  result.windowY = result.y;
  result.clickCount = 0;
  result.type = WebKit::WebInputEvent::MouseMove;
  result.button = static_cast<WebKit::WebMouseEvent::Button>(mouse_move->buttons - 1);

  return result;
}

WebKit::WebMouseWheelEvent mouseWheelEvent(Evas_Event_Mouse_Wheel* mouse_wheel)
{
  DCHECK(mouse_wheel);

  int x_offset = 0;
  int y_offset = 0;
  if (mouse_wheel->direction == 0) {
    y_offset = -mouse_wheel->z;
  } else if (mouse_wheel->direction == 1) {
    x_offset = -mouse_wheel->z;
  }

  WebKit::WebMouseWheelEvent event;

  event.type = WebKit::WebInputEvent::MouseWheel;
  event.button = WebKit::WebMouseEvent::ButtonNone;
  event.modifiers = eflModifierToWebEventModifiers(mouse_wheel->modifiers, 0);
  event.timeStampSeconds = base::Time::UnixEpoch().ToDoubleT();
  event.deltaX = x_offset * kPixelsPerTick;
  event.deltaY = y_offset * kPixelsPerTick;
  event.wheelTicksX = x_offset;
  event.wheelTicksY = y_offset;

  return event;
}

} // namespace content
