// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/window_utils_efl.h"

#include "ui/gfx/rect.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebScreenInfo.h"

#include <Ecore_X.h>

namespace content {

namespace {

// Length of an inch in CSS's 1px unit.
const int kPixelsPerInch = 96;

int depthPerComponent(int depth)
{
    switch (depth) {
    case 0:
    case 24:
    case 32:
        return 8;
    case 8:
        return 2;
    default:
        return depth / 3;
    }
}

}

void GetScreenInfoEfl(WebKit::WebScreenInfo* results)
{
  Ecore_X_Display* display = ecore_x_display_get();
  Ecore_X_Screen* screen = ecore_x_default_screen_get();
  int width, height;
  ecore_x_screen_size_get(screen, &width, &height);
  int depth = ecore_x_default_depth_get(display, screen);
  results->deviceScaleFactor = ecore_x_dpi_get() / kPixelsPerInch;
  results->isMonochrome = depth == 1;
  results->depth = depth;
  results->depthPerComponent = depthPerComponent(depth);
  // FIXME: not sure how to get available rect.
  results->rect = WebKit::WebRect(0, 0, width, height);
  results->availableRect = results->rect;
}

} // namespace content
