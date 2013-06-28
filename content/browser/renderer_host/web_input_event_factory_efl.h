// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_WEB_INPUT_EVENT_FACTORY_EFL_H_
#define CONTENT_BROWSER_RENDERER_HOST_WEB_INPUT_EVENT_FACTORY_EFL_H_

#include "content/common/content_export.h"
#include "ui/gfx/native_widget_types.h"

typedef struct _Evas_Event_Mouse_Down Evas_Event_Mouse_Down;
typedef struct _Evas_Event_Mouse_Up Evas_Event_Mouse_Up;
typedef struct _Evas_Event_Mouse_Move Evas_Event_Mouse_Move;
typedef struct _Evas_Event_Mouse_Wheel Evas_Event_Mouse_Wheel;

namespace WebKit {
struct WebMouseEvent;
struct WebMouseWheelEvent;
struct WebKeyboardEvent;
}

namespace content {

CONTENT_EXPORT WebKit::WebKeyboardEvent keyboardEvent(gfx::NativeEvent);
CONTENT_EXPORT WebKit::WebMouseEvent mouseEvent(Evas_Event_Mouse_Down*);
CONTENT_EXPORT WebKit::WebMouseEvent mouseEvent(Evas_Event_Mouse_Up*);
CONTENT_EXPORT WebKit::WebMouseEvent mouseEvent(Evas_Event_Mouse_Move*);
CONTENT_EXPORT WebKit::WebMouseWheelEvent mouseWheelEvent(Evas_Event_Mouse_Wheel*);

} // namespace content

#endif // CONTENT_BROWSER_RENDERER_HOST_WEB_INPUT_EVENT_FACTORY_EFL_H_
