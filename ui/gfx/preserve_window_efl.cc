// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/preserve_window_efl.h"

#include "base/logging.h"
#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/size.h"
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_X.h>
#include <Elementary.h>

namespace gfx {

namespace {
#define evas_smart_preserve_window_type "Evas_Smart_Preserve_Window"

const char PRESERVE_WINDOW_MOVE[] = "preserve,moved";
const char PRESERVE_WINDOW_RESIZE[] = "preserve,resized";
const char PRESERVE_WINDOW_REPAINT[] = "preserve,repainted";
const Evas_Smart_Cb_Description g_smart_callbacks[] = {
    {PRESERVE_WINDOW_MOVE, "(ii)"},
    {PRESERVE_WINDOW_RESIZE, "(ii)"},
    {PRESERVE_WINDOW_REPAINT, "(iiii)"},
    {NULL, NULL}};

enum PreserveWindowSmartEventType {
  PreserveWindowMoveType,
  PreserveWindowResizeType,
  PreserveWindowRepaintType,
};

const char* PreserveWindowSmartEvent(PreserveWindowSmartEventType type) {
  switch (type) {
    case PreserveWindowMoveType:
      return PRESERVE_WINDOW_MOVE;
      break;
    case PreserveWindowResizeType:
      return PRESERVE_WINDOW_RESIZE;
      break;
    case PreserveWindowRepaintType:
      return PRESERVE_WINDOW_REPAINT;
      break;
  }
  NOTREACHED();
  return "";
}

struct PreserveWindowData
{
   Evas_Object_Smart_Clipped_Data base;
   Evas_Object* window_;
   // FIXME: It is dummy to receive input events. We must find proper event handling way.
   Evas_Object* background_;
};

bool IsPreserveWindowEvasObject(const Evas_Object* evas_object)
{
    DCHECK(evas_object);

    const char* evas_object_type = evas_object_type_get(evas_object);
    if (!evas_object_smart_type_check(evas_object, evas_smart_preserve_window_type)) {
      LOG(ERROR) << evas_object << " is not of an "<< evas_object_type << "!";
        return false;
    }

    const Evas_Smart* evas_smart = evas_object_smart_smart_get(evas_object);
    if (!evas_smart) {
        LOG(ERROR) << evas_object << "(" << evas_object_type << ") is not a smart object!";
        return false;
    }

    const Evas_Smart_Class* smart_class = evas_smart_class_get(evas_smart);
    if (!smart_class) {
      LOG(ERROR) << evas_object << "(" << evas_object_type << ") is not a smart class object!";
        return false;
    }

    return true;
}

inline PreserveWindowData* ToSmartData(Evas_Object* evas_object)
{
  DCHECK(evas_object);
  DCHECK(IsPreserveWindowEvasObject(evas_object));
  CHECK(evas_object_smart_data_get(evas_object));
  return static_cast<PreserveWindowData*>(evas_object_smart_data_get(evas_object));
}

EVAS_SMART_SUBCLASS_NEW(evas_smart_preserve_window_type, evas_smart_preserve_window,
                        Evas_Smart_Class, Evas_Smart_Class,
                        evas_object_smart_clipped_class_get, g_smart_callbacks);

/* create and setup a new preserve window smart object's internals */
void evas_smart_preserve_window_smart_add(Evas_Object* o) {
  // Don't use EVAS_SMART_DATA_ALLOC(o, PreserveWindowData) because [-fpermissive] does not allow invalid conversion from 'void*' to 'PreserveWindowData*'.
  PreserveWindowData* smart_data;
  smart_data = static_cast<PreserveWindowData*>(evas_object_smart_data_get(o));
  if (!smart_data) {
    smart_data = static_cast<PreserveWindowData*>(calloc(1, sizeof(PreserveWindowData)));
    if (!smart_data) {
      return;
    }
    evas_object_smart_data_set(o, smart_data);
  }

  int x, y, w, h = 0;
  evas_object_geometry_get(o, &x, &y, &w, &h);

  smart_data->window_ = elm_win_add(o, "preserve-window", ELM_WIN_DOCK);
  evas_object_resize(smart_data->window_, w, h);
  evas_object_show(smart_data->window_);

  Ecore_X_Window x_window = elm_win_xwindow_get(smart_data->window_);
  Ecore_X_Window root_x_window = elm_win_xwindow_get(o);
  ecore_x_window_reparent(x_window, root_x_window, x, y);

  smart_data->background_ = evas_object_rectangle_add(evas_object_evas_get(smart_data->window_));
  evas_object_color_set(smart_data->background_, 0, 0, 0, 0);
  //evas_object_color_set(smart_data->window_, 0, 255, 0, 255);
  evas_object_size_hint_weight_set(smart_data->background_, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  elm_win_resize_object_add(smart_data->window_, smart_data->background_);
  evas_object_focus_set(smart_data->background_, EINA_TRUE);
  evas_smart_preserve_window_parent_sc->add(o);
}

void evas_smart_preserve_window_smart_del(Evas_Object* o) {
  PreserveWindowData* smart_data = ToSmartData(o);
  Ecore_X_Window x_window = elm_win_xwindow_get(smart_data->window_);
  ecore_x_window_reparent(x_window, 0 /* default root window */, 0 /* x */, 0 /* y */);
  evas_object_del(smart_data->background_);
  evas_object_del(smart_data->window_);
  evas_smart_preserve_window_parent_sc->del(o);
}

void evas_smart_preserve_window_smart_show(Evas_Object* o) {
  PreserveWindowData* smart_data = ToSmartData(o);
  evas_object_show(smart_data->window_);
  evas_object_show(smart_data->background_);
  evas_smart_preserve_window_parent_sc->show(o);
}

void evas_smart_preserve_window_smart_hide(Evas_Object* o) {
  PreserveWindowData* smart_data = ToSmartData(o);
  evas_object_hide(smart_data->window_);
  evas_object_hide(smart_data->background_);
  evas_smart_preserve_window_parent_sc->hide(o);
}

void evas_smart_preserve_window_smart_move(Evas_Object* o,
                                             Evas_Coord x,
                                             Evas_Coord y) {
  Evas_Coord ox, oy;
  evas_object_geometry_get(o, &ox, &oy, NULL, NULL);
  if ((ox == x) && (oy == y))
    return;

  PreserveWindowData* smart_data = ToSmartData(o);
  Ecore_X_Window x_window = elm_win_xwindow_get(smart_data->window_);
  Ecore_X_Window root_x_window = elm_win_xwindow_get(o);
  ecore_x_window_reparent(x_window, root_x_window, x, y);

  int position[2] = {x, y};
  evas_object_smart_callback_call(
      o, PRESERVE_WINDOW_MOVE, static_cast<void*>(position));

  /* this will trigger recalculation */
  evas_object_smart_changed(o);
}

void evas_smart_preserve_window_smart_resize(Evas_Object* o,
                                             Evas_Coord w,
                                             Evas_Coord h) {
  Evas_Coord ow, oh;
  evas_object_geometry_get(o, NULL, NULL, &ow, &oh);
  if ((ow == w) && (oh == h))
    return;

  PreserveWindowData* smart_data = ToSmartData(o);
  evas_object_resize(smart_data->window_, w, h);

  int size[2] = {w, h};
  evas_object_smart_callback_call(
      o, PRESERVE_WINDOW_RESIZE, static_cast<void*>(size));

  /* this will trigger recalculation */
  evas_object_smart_changed(o);
}

/* act on child objects' properties, before rendering */
void evas_smart_preserve_window_smart_calculate(Evas_Object* o) {
  int dirty_rect[4] = { 0, };
  // FIXME: how to know dirty rect actually.
  evas_object_geometry_get(o, &dirty_rect[0], &dirty_rect[1], &dirty_rect[2], &dirty_rect[3]);
  evas_object_smart_callback_call(
      o, PRESERVE_WINDOW_REPAINT, static_cast<void*>(dirty_rect));
}

/* setting our smart interface */
void evas_smart_preserve_window_smart_set_user(Evas_Smart_Class* sc) {
  /* specializing these two */
  sc->add = evas_smart_preserve_window_smart_add;
  sc->del = evas_smart_preserve_window_smart_del;
  sc->show = evas_smart_preserve_window_smart_show;
  sc->hide = evas_smart_preserve_window_smart_hide;

  /* clipped smart object has no hook on move, resize and calculation */
  sc->move = evas_smart_preserve_window_smart_move;
  sc->resize = evas_smart_preserve_window_smart_resize;
  sc->calculate = evas_smart_preserve_window_smart_calculate;
}

// SmartObjectEventHandler implementation.
template <Evas_Callback_Type EventType> class SmartObjectEventHandler {
 public:
  static void Subscribe(Evas_Object* evas_object, PreserveWindowDelegate* delegate) {
    evas_object_event_callback_add(evas_object, EventType, HandleEvent, delegate);
  }

  static void Unsubscribe(Evas_Object* evas_object) {
    evas_object_event_callback_del(evas_object, EventType, HandleEvent);
  }

  static void HandleEvent(void* data, Evas*, Evas_Object*, void* event_info);
};

template <>
void SmartObjectEventHandler<EVAS_CALLBACK_MOUSE_DOWN>::HandleEvent(void* data, Evas*, Evas_Object*, void* event_info)
{
  PreserveWindowDelegate* delegate = static_cast<PreserveWindowDelegate*>(data);
  delegate->PreserveWindowMouseDown(static_cast<Evas_Event_Mouse_Down*>(event_info));
}

template <>
void SmartObjectEventHandler<EVAS_CALLBACK_MOUSE_UP>::HandleEvent(void* data, Evas*, Evas_Object*, void* event_info)
{
  PreserveWindowDelegate* delegate = static_cast<PreserveWindowDelegate*>(data);
  delegate->PreserveWindowMouseUp(static_cast<Evas_Event_Mouse_Up*>(event_info));
}

template <>
void SmartObjectEventHandler<EVAS_CALLBACK_MOUSE_MOVE>::HandleEvent(void* data, Evas*, Evas_Object*, void* event_info)
{
  PreserveWindowDelegate* delegate = static_cast<PreserveWindowDelegate*>(data);
  delegate->PreserveWindowMouseMove(static_cast<Evas_Event_Mouse_Move*>(event_info));
}

template <>
void SmartObjectEventHandler<EVAS_CALLBACK_MOUSE_WHEEL>::HandleEvent(void* data, Evas*, Evas_Object*, void* event_info)
{
  PreserveWindowDelegate* delegate = static_cast<PreserveWindowDelegate*>(data);
  delegate->PreserveWindowMouseWheel(static_cast<Evas_Event_Mouse_Wheel*>(event_info));
}

template <>
void SmartObjectEventHandler<EVAS_CALLBACK_KEY_DOWN>::HandleEvent(void* data, Evas*, Evas_Object*, void* event_info)
{
  PreserveWindowDelegate* delegate = static_cast<PreserveWindowDelegate*>(data);
  delegate->PreserveWindowKeyDown(static_cast<Evas_Event_Key_Down*>(event_info));
}

template <>
void SmartObjectEventHandler<EVAS_CALLBACK_KEY_UP>::HandleEvent(void* data, Evas*, Evas_Object*, void* event_info)
{
  PreserveWindowDelegate* delegate = static_cast<PreserveWindowDelegate*>(data);
  delegate->PreserveWindowKeyUp(static_cast<Evas_Event_Key_Up*>(event_info));
}

template <>
void SmartObjectEventHandler<EVAS_CALLBACK_FOCUS_IN>::HandleEvent(void* data, Evas*, Evas_Object*, void*)
{
  PreserveWindowDelegate* delegate = static_cast<PreserveWindowDelegate*>(data);
  delegate->PreserveWindowFocusIn();
}

template <>
void SmartObjectEventHandler<EVAS_CALLBACK_FOCUS_OUT>::HandleEvent(void* data, Evas*, Evas_Object*, void*)
{
  PreserveWindowDelegate* delegate = static_cast<PreserveWindowDelegate*>(data);
  delegate->PreserveWindowFocusOut();
}

template <>
void SmartObjectEventHandler<EVAS_CALLBACK_SHOW>::HandleEvent(void* data, Evas*, Evas_Object*, void*)
{
  PreserveWindowDelegate* delegate = static_cast<PreserveWindowDelegate*>(data);
  delegate->PreserveWindowShow();
}

template <>
void SmartObjectEventHandler<EVAS_CALLBACK_HIDE>::HandleEvent(void* data, Evas*, Evas_Object*, void*)
{
  PreserveWindowDelegate* delegate = static_cast<PreserveWindowDelegate*>(data);
  delegate->PreserveWindowHide();
}

// SmartObjectSmartHandler implementation.
template <PreserveWindowSmartEventType EventType> class SmartObjectSmartHandler {
 public:
  static void Subscribe(Evas_Object* evas_object, PreserveWindowDelegate* delegate) {
    evas_object_smart_callback_add(evas_object, PreserveWindowSmartEvent(EventType), HandleEvent, delegate);
  }

  static void Unsubscribe(Evas_Object* evas_object) {
    evas_object_smart_callback_del(evas_object, PreserveWindowSmartEvent(EventType), HandleEvent);
  }

  static void HandleEvent(void* data, Evas_Object*, void* event_info);
};

template <PreserveWindowSmartEventType EventType>
void SmartObjectSmartHandler<EventType>::HandleEvent(void* data, Evas_Object*, void* event_info)
{
  PreserveWindowDelegate* delegate = static_cast<PreserveWindowDelegate*>(data);
  switch (EventType) { // Mikhail FIXME: this should be done during compile time!
    case PreserveWindowMoveType: {
      int* position = static_cast<int*>(event_info);
      delegate->PreserveWindowMove(gfx::Point(position[0], position[1]));
      break;
    }
    case PreserveWindowResizeType: {
      int* size = static_cast<int*>(event_info);
      delegate->PreserveWindowResize(gfx::Size(size[0], size[1]));
      break;
    }
    case PreserveWindowRepaintType: {
      int* dirty_rect = static_cast<int*>(event_info);
      delegate->PreserveWindowRepaint(gfx::Rect(dirty_rect[0], dirty_rect[1], dirty_rect[2], dirty_rect[3]));
      break;
    }
    default:
      NOTREACHED();
      break;
  }
}
} // namespace

// static
PreserveWindow* PreserveWindow::Create(
    PreserveWindowDelegate* delegate, Evas_Object* object) {
  return new PreserveWindow(delegate, object);
}

PreserveWindow::PreserveWindow(
    PreserveWindowDelegate* delegate, Evas_Object* object)
    : delegate_(delegate)
    , container_object_(object) {
  Evas* evas = evas_object_evas_get (container_object_);
  smart_object_ = evas_object_smart_add(evas, evas_smart_preserve_window_smart_class_new());
  evas_object_show(smart_object_);

  SmartObjectSmartHandler<PreserveWindowMoveType>::Subscribe(smart_object_, delegate_);
  SmartObjectSmartHandler<PreserveWindowResizeType>::Subscribe(smart_object_, delegate_);
  SmartObjectSmartHandler<PreserveWindowRepaintType>::Subscribe(smart_object_, delegate_);

  PreserveWindowData* smart_data = ToSmartData(smart_object_);
  SmartObjectEventHandler<EVAS_CALLBACK_MOUSE_DOWN>::Subscribe(smart_data->background_, delegate_);
  SmartObjectEventHandler<EVAS_CALLBACK_MOUSE_UP>::Subscribe(smart_data->background_, delegate_);
  SmartObjectEventHandler<EVAS_CALLBACK_MOUSE_MOVE>::Subscribe(smart_data->background_, delegate_);
  SmartObjectEventHandler<EVAS_CALLBACK_MOUSE_WHEEL>::Subscribe(smart_data->background_, delegate_);
  SmartObjectEventHandler<EVAS_CALLBACK_KEY_DOWN>::Subscribe(smart_data->background_, delegate_);
  SmartObjectEventHandler<EVAS_CALLBACK_KEY_UP>::Subscribe(smart_data->background_, delegate_);
  SmartObjectEventHandler<EVAS_CALLBACK_FOCUS_IN>::Subscribe(smart_data->background_, delegate_);
  SmartObjectEventHandler<EVAS_CALLBACK_FOCUS_OUT>::Subscribe(smart_data->background_, delegate_);
  SmartObjectEventHandler<EVAS_CALLBACK_SHOW>::Subscribe(smart_data->background_, delegate_);
  SmartObjectEventHandler<EVAS_CALLBACK_HIDE>::Subscribe(smart_data->background_, delegate_);

  // FIXME: presever_window creates Evas_Object,
  // not elm_object, so this ugly focus handling is needed.
  SmartObjectEventHandler<EVAS_CALLBACK_MOUSE_DOWN>::
      Subscribe(smart_data->background_, this);
  SmartObjectEventHandler<EVAS_CALLBACK_FOCUS_IN>::
      Subscribe(container_object_, this);
  SmartObjectEventHandler<EVAS_CALLBACK_FOCUS_OUT>::
      Subscribe(container_object_, this);

  // FIXME: After creation, request redraw.
  evas_object_smart_changed(smart_object_);
}

PreserveWindow::~PreserveWindow() {
  SmartObjectSmartHandler<PreserveWindowMoveType>::Unsubscribe(smart_object_);
  SmartObjectSmartHandler<PreserveWindowResizeType>::Unsubscribe(smart_object_);
  SmartObjectSmartHandler<PreserveWindowRepaintType>::Unsubscribe(smart_object_);

  PreserveWindowData* smart_data = ToSmartData(smart_object_);
  SmartObjectEventHandler<EVAS_CALLBACK_MOUSE_DOWN>::Unsubscribe(smart_data->background_);
  SmartObjectEventHandler<EVAS_CALLBACK_MOUSE_UP>::Unsubscribe(smart_data->background_);
  SmartObjectEventHandler<EVAS_CALLBACK_MOUSE_MOVE>::Unsubscribe(smart_data->background_);
  SmartObjectEventHandler<EVAS_CALLBACK_MOUSE_WHEEL>::Unsubscribe(smart_data->background_);
  SmartObjectEventHandler<EVAS_CALLBACK_KEY_DOWN>::Unsubscribe(smart_data->background_);
  SmartObjectEventHandler<EVAS_CALLBACK_KEY_UP>::Unsubscribe(smart_data->background_);
  SmartObjectEventHandler<EVAS_CALLBACK_FOCUS_IN>::Unsubscribe(smart_data->background_);
  SmartObjectEventHandler<EVAS_CALLBACK_FOCUS_OUT>::Unsubscribe(smart_data->background_);
  SmartObjectEventHandler<EVAS_CALLBACK_SHOW>::Unsubscribe(smart_data->background_);
  SmartObjectEventHandler<EVAS_CALLBACK_HIDE>::Unsubscribe(smart_data->background_);

  SmartObjectEventHandler<EVAS_CALLBACK_MOUSE_DOWN>::
      Unsubscribe(smart_data->background_);
  SmartObjectEventHandler<EVAS_CALLBACK_FOCUS_IN>::
      Unsubscribe(container_object_);
  SmartObjectEventHandler<EVAS_CALLBACK_FOCUS_OUT>::
      Unsubscribe(container_object_);

  evas_object_del(smart_object_);
}


Evas_Object* PreserveWindow::EvasWindow() {
  PreserveWindowData* smart_data = ToSmartData(smart_object_);
  return smart_data->window_;
}

void PreserveWindow::PreserveWindowMouseDown(Evas_Event_Mouse_Down* event) {
  // FIXME: elm_box is not focusable although setting true via
  // elm_object_focus_allow_set. If we find focusable elm container,
  // PreserveWindowFocusIn() can be removed.
  PreserveWindowFocusIn();
  //elm_object_focus_set(container_object_, EINA_TRUE);
}

void PreserveWindow::PreserveWindowMouseUp(Evas_Event_Mouse_Up* event) {
  NOTIMPLEMENTED();
}

void PreserveWindow::PreserveWindowMouseMove(Evas_Event_Mouse_Move* event) {
  NOTIMPLEMENTED();
}

void PreserveWindow::PreserveWindowMouseWheel(Evas_Event_Mouse_Wheel* event) {
  NOTIMPLEMENTED();
}

void PreserveWindow::PreserveWindowKeyDown(Evas_Event_Key_Down* event) {
  NOTIMPLEMENTED();
}

void PreserveWindow::PreserveWindowKeyUp(Evas_Event_Key_Up* event) {
  NOTIMPLEMENTED();
}

void PreserveWindow::PreserveWindowFocusIn() {
  PreserveWindowData* smart_data = ToSmartData(smart_object_);
  evas_object_focus_set(smart_data->background_, EINA_TRUE);
}

void PreserveWindow::PreserveWindowFocusOut() {
  PreserveWindowData* smart_data = ToSmartData(smart_object_);
  evas_object_focus_set(smart_data->background_, EINA_FALSE);
}

void PreserveWindow::PreserveWindowShow() {
  NOTIMPLEMENTED();
}

void PreserveWindow::PreserveWindowHide() {
  NOTIMPLEMENTED();
}

void PreserveWindow::PreserveWindowMove(const gfx::Point& origin) {
  NOTIMPLEMENTED();
}

void PreserveWindow::PreserveWindowResize(const gfx::Size& size) {
  NOTIMPLEMENTED();
}

void PreserveWindow::PreserveWindowRepaint(const gfx::Rect& damage_rect) {
  NOTIMPLEMENTED();
}

}  // namespace gfx
