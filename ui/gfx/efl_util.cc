// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/efl_util.h"

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_X.h>
#include <Edje.h>
#include <Eina.h>
#include <Evas.h>

namespace gfx {

// TODO(rakuco): Properly check the return values from each function.
void EflInit() {
  eina_init();
  evas_init();
  ecore_init();
  ecore_evas_init();

  ecore_x_init(NULL);

  edje_init();

  ecore_main_loop_glib_integrate();
}

void EflShutdown() {
  edje_shutdown();

  ecore_x_shutdown();

  ecore_evas_shutdown();
  ecore_shutdown();
  evas_shutdown();
  eina_shutdown();
}

}  // namespace gfx
