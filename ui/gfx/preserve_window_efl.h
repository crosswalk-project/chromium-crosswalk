// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_BROWSER_PRESERVE_WINDOW_EFL_H_
#define UI_GFX_BROWSER_PRESERVE_WINDOW_EFL_H_

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "ui/base/ui_export.h"
#include "ui/gfx/preserve_window_delegate_efl.h"

#include <Evas.h>

namespace gfx {

class UI_EXPORT PreserveWindow {
 public:
  static PreserveWindow* Create(PreserveWindowDelegate*, Evas*);

  ~PreserveWindow();

  Evas_Object* SmartObject() const { return smart_object_; }
  Evas_Object* EvasWindow();

 private:
  PreserveWindow(PreserveWindowDelegate*, Evas*);

  PreserveWindowDelegate* delegate_;
  Evas_Object* smart_object_;

  DISALLOW_COPY_AND_ASSIGN(PreserveWindow);
};

}  // namespace gfx

#endif  // UI_GFX_BROWSER_PRESERVE_WINDOW_EFL_H_
