// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/basictypes.h"
#include "base/logging.h"

#include "ui/native_theme/native_theme.h"

namespace ui {

// static
NativeTheme* NativeTheme::instance() {
  return 0;
}

}  // namespace ui
