// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "base/time.h"
#include "base/string_number_conversions.h"
#include "base/string_util.h"
#include "content/browser/renderer_host/web_input_event_factory_efl.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"
#include "ui/base/events/event_constants.h"
#include "ui/base/keycodes/keyboard_codes_posix.h"
#include "ui/gfx/efl_event.h"

#include <Evas.h>
#include <map>

namespace content {

// Taken from web_input_event_aura.h
const int kPixelsPerTick = 53;

// Keycode map efl->windows vkeys.
static std::map<std::string, ui::KeyboardCode> efl_keycode_map;

static int eflModifierToWebEventModifiers(const Evas_Modifier* efl_modidier,
                                          int buttons = 0) {
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

static inline void addKey(std::string key, ui::KeyboardCode key_code)
{
  efl_keycode_map.insert(std::pair<std::string, ui::KeyboardCode>(key, key_code));
}

static inline void addRangeToKeycodeMap(const char from, const char to, int rangeStart)
{
  for (char c = from; c <= to; c++) {
    addKey(std::string(1,c), (ui::KeyboardCode)rangeStart++);
  }
}

static void createEflToWindowsKeyMap()
{
  // Unmapped codes.

  // VKEY_CLEAR = 0x0C,
  // VKEY_MENU = 0x12,
  // VKEY_PAUSE = 0x13,
  // VKEY_CAPITAL = 0x14,
  // VKEY_KANA = 0x15,
  // VKEY_HANGUL = 0x15,
  // VKEY_JUNJA = 0x17,
  // VKEY_FINAL = 0x18,
  // VKEY_HANJA = 0x19,
  // VKEY_KANJI = 0x19,
  // VKEY_CONVERT = 0x1C,
  // VKEY_NONCONVERT = 0x1D,
  // VKEY_ACCEPT = 0x1E,
  // VKEY_MODECHANGE = 0x1F,
  // VKEY_SELECT = 0x29,
  // VKEY_EXECUTE = 0x2B,
  // VKEY_SNAPSHOT = 0x2C,
  // VKEY_HELP = 0x2F,
  // VKEY_LWIN = 0x5B,
  // VKEY_COMMAND = VKEY_LWIN,  // Provide the Mac name for convenience.
  // VKEY_RWIN = 0x5C,
  // VKEY_APPS = 0x5D,
  // VKEY_SLEEP = 0x5F,
  // VKEY_SEPARATOR = 0x6C,
  // VKEY_F13 = 0x7C,
  // VKEY_F14 = 0x7D,
  // VKEY_F15 = 0x7E,
  // VKEY_F16 = 0x7F,
  // VKEY_F17 = 0x80,
  // VKEY_F18 = 0x81,
  // VKEY_F19 = 0x82,
  // VKEY_F20 = 0x83,
  // VKEY_F21 = 0x84,
  // VKEY_F22 = 0x85,
  // VKEY_F23 = 0x86,
  // VKEY_F24 = 0x87,
  // VKEY_LMENU = 0xA4,
  // VKEY_RMENU = 0xA5,
  // VKEY_BROWSER_BACK = 0xA6,
  // VKEY_BROWSER_FORWARD = 0xA7,
  // VKEY_BROWSER_REFRESH = 0xA8,
  // VKEY_BROWSER_STOP = 0xA9,
  // VKEY_BROWSER_SEARCH = 0xAA,
  // VKEY_BROWSER_FAVORITES = 0xAB,
  // VKEY_BROWSER_HOME = 0xAC,
  // VKEY_VOLUME_MUTE = 0xAD,
  // VKEY_VOLUME_DOWN = 0xAE,
  // VKEY_VOLUME_UP = 0xAF,
  // VKEY_MEDIA_NEXT_TRACK = 0xB0,
  // VKEY_MEDIA_PREV_TRACK = 0xB1,
  // VKEY_MEDIA_STOP = 0xB2,
  // VKEY_MEDIA_PLAY_PAUSE = 0xB3,
  // VKEY_MEDIA_LAUNCH_MAIL = 0xB4,
  // VKEY_MEDIA_LAUNCH_MEDIA_SELECT = 0xB5,
  // VKEY_MEDIA_LAUNCH_APP1 = 0xB6,
  // VKEY_MEDIA_LAUNCH_APP2 = 0xB7,
  // VKEY_OEM_1 = 0xBA,
  // VKEY_OEM_PLUS = 0xBB,
  // VKEY_OEM_COMMA = 0xBC,
  // VKEY_OEM_MINUS = 0xBD,
  // VKEY_OEM_PERIOD = 0xBE,
  // VKEY_OEM_2 = 0xBF,
  // VKEY_OEM_3 = 0xC0,
  // VKEY_OEM_4 = 0xDB,
  // VKEY_OEM_5 = 0xDC,
  // VKEY_OEM_6 = 0xDD,
  // VKEY_OEM_7 = 0xDE,
  // VKEY_OEM_8 = 0xDF,
  // VKEY_OEM_102 = 0xE2,
  // VKEY_PROCESSKEY = 0xE5,
  // VKEY_PACKET = 0xE7,
  // VKEY_DBE_SBCSCHAR = 0xF3,
  // VKEY_DBE_DBCSCHAR = 0xF4,
  // VKEY_ATTN = 0xF6,
  // VKEY_CRSEL = 0xF7,
  // VKEY_EXSEL = 0xF8,
  // VKEY_EREOF = 0xF9,
  // VKEY_PLAY = 0xFA,
  // VKEY_ZOOM = 0xFB,
  // VKEY_NONAME = 0xFC,
  // VKEY_PA1 = 0xFD,
  // VKEY_OEM_CLEAR = 0xFE,

  addKey(std::string("BackSpace"), ui::VKEY_BACK);
  addKey(std::string("Tab"), ui::VKEY_TAB);
  addKey(std::string("ISO_Left_Tab"), ui::VKEY_BACKTAB);
  addKey(std::string("Return"), ui::VKEY_RETURN);
  addKey(std::string("Escape"), ui::VKEY_ESCAPE);
  addKey(std::string("space"), ui::VKEY_SPACE);
  addKey(std::string("Prior"), ui::VKEY_PRIOR);
  addKey(std::string("KP_Prior"), ui::VKEY_PRIOR);
  addKey(std::string("Next"), ui::VKEY_NEXT);
  addKey(std::string("Next"), ui::VKEY_NEXT);
  addKey(std::string("End"), ui::VKEY_END);
  addKey(std::string("KP_End"), ui::VKEY_END);
  addKey(std::string("Home"), ui::VKEY_HOME);
  addKey(std::string("KP_Home"), ui::VKEY_HOME);
  addKey(std::string("Left"), ui::VKEY_LEFT);
  addKey(std::string("KP_Left"), ui::VKEY_LEFT);
  addKey(std::string("Up"), ui::VKEY_UP);
  addKey(std::string("KP_Up"), ui::VKEY_UP);
  addKey(std::string("Right"), ui::VKEY_RIGHT);
  addKey(std::string("KP_Rright"), ui::VKEY_RIGHT);
  addKey(std::string("Down"), ui::VKEY_DOWN);
  addKey(std::string("KP_Down"), ui::VKEY_DOWN);
  addKey(std::string("Print"), ui::VKEY_PRINT);
  addKey(std::string("Insert"), ui::VKEY_INSERT);
  addKey(std::string("KP_Insert"), ui::VKEY_INSERT);
  addKey(std::string("Delete"), ui::VKEY_DELETE);
  addKey(std::string("KP_Delete"), ui::VKEY_DELETE);
  addKey(std::string("Shift_L"), ui::VKEY_LSHIFT);
  addKey(std::string("Shift_R"), ui::VKEY_RSHIFT);
  addKey(std::string("Control_R"), ui::VKEY_RCONTROL);
  addKey(std::string("Control_L"), ui::VKEY_LCONTROL);
  addKey(std::string("KP_Multiply"), ui::VKEY_MULTIPLY);
  addKey(std::string("KP_Add"), ui::VKEY_ADD);
  addKey(std::string("KP_Subtract"), ui::VKEY_SUBTRACT);
  addKey(std::string("KP_Decimal"), ui::VKEY_DECIMAL);
  addKey(std::string("KP_Divide"), ui::VKEY_DIVIDE);
  addKey(std::string("Num_Lock"), ui::VKEY_NUMLOCK);
  addKey(std::string("Scroll_Lock"), ui::VKEY_SCROLL);

  // Numpad 0-9
  for (int i = 0; i < 9; i++) {
    addKey(std::string("KP_") + base::IntToString(i), (ui::KeyboardCode)(ui::VKEY_NUMPAD0 + i));
  }

  // F1-F12
  for (int i = 0; i < 12; i++) {
    addKey(std::string("F") + base::IntToString(i), (ui::KeyboardCode)(ui::VKEY_F1 + i));
  }

  addRangeToKeycodeMap('0', '9', ui::VKEY_0);
  addRangeToKeycodeMap('a', 'z', ui::VKEY_A);
  addRangeToKeycodeMap('A', 'Z', ui::VKEY_A);
}

// temporarily copied from WebKeyboardEvent::windowsKeyCodeWithoutLocation
static int windowsKeyCodeWithoutLocation(ui::KeyboardCode keycode)
{
    switch (keycode) {
    case ui::VKEY_LCONTROL:
    case ui::VKEY_RCONTROL:
        return ui::VKEY_CONTROL;
    case ui::VKEY_LSHIFT:
    case ui::VKEY_RSHIFT:
        return ui::VKEY_SHIFT;
    case ui::VKEY_LMENU:
    case ui::VKEY_RMENU:
        return ui::VKEY_MENU;
    default:
        return keycode;
    }
}

// temporarily copied from WebKeyboardEvent::windowsKeyCodeWithoutLocation
static int locationModifiersFromWindowsKeyCode(ui::KeyboardCode keycode)
{
    switch (keycode) {
    case ui::VKEY_LCONTROL:
    case ui::VKEY_LSHIFT:
    case ui::VKEY_LMENU:
    case ui::VKEY_LWIN:
        return WebKit::WebKeyboardEvent::IsLeft;
    case ui::VKEY_RCONTROL:
    case ui::VKEY_RSHIFT:
    case ui::VKEY_RMENU:
    case ui::VKEY_RWIN:
        return WebKit::WebKeyboardEvent::IsRight;
    default:
        return 0;
    }
}

static void setKeyText(ui::KeyboardCode code,
                       WebKit::WebKeyboardEvent& webkit_event,
                       const char* efl_event_string)
{
  switch (code) {
    case ui::VKEY_RETURN:
      webkit_event.unmodifiedText[0] = '\r';
      break;
    case ui::VKEY_BACK:
      webkit_event.unmodifiedText[0] = '\x8';
      break;
    case ui::VKEY_TAB:
      webkit_event.unmodifiedText[0] = '\t';
      break;
    default:
      if (efl_event_string)
        webkit_event.unmodifiedText[0] = efl_event_string[0];
  }
}

ui::KeyboardCode windowsKeyboardCodeFromEvasKeyEventName(const char* eventName)
{
  if (efl_keycode_map.empty()) {
    createEflToWindowsKeyMap();
  }

  const std::map<std::string, ui::KeyboardCode>::iterator it =
                                   efl_keycode_map.find(std::string(eventName));
  return it != efl_keycode_map.end() ? it->second : ui::VKEY_UNKNOWN;
}

template<typename T>
WebKit::WebKeyboardEvent toWebKeyboardEvent(T* event,
                                            gfx::EflEvent::EflEventType type)
{
  WebKit::WebKeyboardEvent result;

  result.timeStampSeconds = event->timestamp / 1000;
  result.modifiers = eflModifierToWebEventModifiers(event->modifiers);
  result.type = type == gfx::EflEvent::EventTypeKeyUp ? WebKit::WebInputEvent::KeyUp :
                                                        WebKit::WebInputEvent::KeyDown;

  ui::KeyboardCode windowsKeyCode = windowsKeyboardCodeFromEvasKeyEventName(event->key);
  result.windowsKeyCode = windowsKeyCodeWithoutLocation(windowsKeyCode);
  result.modifiers |= locationModifiersFromWindowsKeyCode(windowsKeyCode);

  setKeyText(windowsKeyCode, result, event->string);

  result.text[0] = result.unmodifiedText[0];

  result.setKeyIdentifierFromWindowsKeyCode();

  if (event->key && StartsWithASCII(std::string(event->key), std::string("KP_"), true))
    result.modifiers |= WebKit::WebInputEvent::IsKeyPad;

  return result;
}

WebKit::WebKeyboardEvent keyboardEvent(gfx::NativeEvent event)
{
  DCHECK(event);

  switch(event->eventType()) {
    case gfx::EflEvent::EventTypeKeyUp:
      return toWebKeyboardEvent(static_cast<Evas_Event_Key_Up*>(event->eflEvent()),
                                gfx::EflEvent::EventTypeKeyUp);
    case gfx::EflEvent::EventTypeKeyDown:
      return toWebKeyboardEvent(static_cast<Evas_Event_Key_Down*>(event->eflEvent()),
                                gfx::EflEvent::EventTypeKeyDown);
    default:
      NOTREACHED();
  }

  return WebKit::WebKeyboardEvent();
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
  event.modifiers = eflModifierToWebEventModifiers(mouse_wheel->modifiers);
  event.timeStampSeconds = base::Time::UnixEpoch().ToDoubleT();
  event.deltaX = x_offset * kPixelsPerTick;
  event.deltaY = y_offset * kPixelsPerTick;
  event.wheelTicksX = x_offset;
  event.wheelTicksY = y_offset;

  return event;
}

} // namespace content
