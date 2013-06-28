// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/snapshot/snapshot.h"

#include "base/logging.h"
#include "ui/base/x/x11_util.h"
#include "ui/gfx/rect.h"

namespace ui {

bool GrabViewSnapshot(gfx::NativeView view_handle,
                        std::vector<unsigned char>* png_representation,
                        const gfx::Rect& snapshot_bounds) {
  return true;
}

}  // namespace ui
