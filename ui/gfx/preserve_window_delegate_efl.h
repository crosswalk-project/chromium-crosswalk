// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_BROWSER_PRESERVE_WINDOW_DELEGATE_EFL_H_
#define UI_GFX_BROWSER_PRESERVE_WINDOW_DELEGATE_EFL_H_

#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/size.h"
#include <Evas.h>

namespace gfx {

// A private interface used by RootWindowHost implementations to communicate input events
// with their owning PreserveWindow.
class PreserveWindowDelegate {
 public:
  virtual bool PreserveWindowMouseDown(Evas_Event_Mouse_Down* event) = 0;
  virtual bool PreserveWindowMouseUp(Evas_Event_Mouse_Up* event) = 0;
  virtual bool PreserveWindowMouseMove(Evas_Event_Mouse_Move* event) = 0;
  virtual bool PreserveWindowMouseWheel(Evas_Event_Mouse_Wheel* event) = 0;
  virtual bool PreserveWindowKeyDown(Evas_Event_Key_Down* event) = 0;
  virtual bool PreserveWindowKeyUp(Evas_Event_Key_Up* event) = 0;

  // Called when the windowing system activates the window.
  virtual void PreserveWindowFocusIn() = 0;

  // Called when system focus is changed to another window.
  virtual void PreserveWindowFocusOut() = 0;

  virtual void PreserveWindowShow() = 0;
  virtual void PreserveWindowHide() = 0;

  virtual void PreserveWindowMove(const gfx::Point& origin) = 0;
  virtual void PreserveWindowResize(const gfx::Size& size) = 0;
  virtual void PreserveWindowRepaint(const gfx::Rect& damage_rect) = 0;

 protected:
  virtual ~PreserveWindowDelegate() {}
};

}  // namespace gfx

#endif  // UI_GFX_BROWSER_PRESERVE_WINDOW_DELEGATE_EFL_H_
