// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_BROWSER_PRESERVE_WINDOW_EFL_H_
#define UI_GFX_BROWSER_PRESERVE_WINDOW_EFL_H_

#include <Evas.h>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "ui/base/ui_export.h"
#include "ui/gfx/preserve_window_delegate_efl.h"

namespace gfx {

class UI_EXPORT PreserveWindow : public PreserveWindowDelegate {
 public:
  static PreserveWindow* Create(PreserveWindowDelegate*, Evas_Object*);

  virtual ~PreserveWindow();

  Evas_Object* SmartObject() const { return smart_object_; }
  Evas_Object* EvasWindow();

 private:
  PreserveWindow(PreserveWindowDelegate*, Evas_Object*);

  // PreserveWindowDelegate
  virtual void PreserveWindowMouseDown(Evas_Event_Mouse_Down* event) OVERRIDE;
  virtual void PreserveWindowMouseUp(Evas_Event_Mouse_Up* event) OVERRIDE;
  virtual void PreserveWindowMouseMove(Evas_Event_Mouse_Move* event) OVERRIDE;
  virtual void PreserveWindowMouseWheel(Evas_Event_Mouse_Wheel* event) OVERRIDE;
  virtual void PreserveWindowKeyDown(Evas_Event_Key_Down* event) OVERRIDE;
  virtual void PreserveWindowKeyUp(Evas_Event_Key_Up* event) OVERRIDE;
  virtual void PreserveWindowFocusIn() OVERRIDE;
  virtual void PreserveWindowFocusOut() OVERRIDE;
  virtual void PreserveWindowShow() OVERRIDE;
  virtual void PreserveWindowHide() OVERRIDE;
  virtual void PreserveWindowMove(const gfx::Point& origin) OVERRIDE;
  virtual void PreserveWindowResize(const gfx::Size& size) OVERRIDE;
  virtual void PreserveWindowRepaint(const gfx::Rect& damage_rect) OVERRIDE;

  PreserveWindowDelegate* delegate_;
  Evas_Object* container_object_;
  Evas_Object* smart_object_;

  DISALLOW_COPY_AND_ASSIGN(PreserveWindow);
};

}  // namespace gfx

#endif  // UI_GFX_BROWSER_PRESERVE_WINDOW_EFL_H_
