// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/screen.h"

#include <Ecore_X.h>
#include <Ecore_Evas.h>

#include "base/logging.h"
#include "ui/gfx/display.h"

namespace {

class ScreenEfl : public gfx::Screen {
 public:
  ScreenEfl() {
  }

  virtual ~ScreenEfl() {
  }

  virtual bool IsDIPEnabled() OVERRIDE {
    return false;
  }

  virtual gfx::Point GetCursorScreenPoint() OVERRIDE {
    return gfx::Point();
  }

  virtual gfx::NativeWindow GetWindowAtCursorScreenPoint() OVERRIDE {
    NOTIMPLEMENTED();
    return NULL;
  }

  virtual int GetNumDisplays() OVERRIDE {
    return ecore_x_screen_count_get();
  }

  virtual gfx::Display GetDisplayNearestWindow(
      gfx::NativeView view) const OVERRIDE {
    return GetPrimaryDisplay();
  }

  virtual gfx::Display GetDisplayNearestPoint(
      const gfx::Point& point) const OVERRIDE {
    return GetPrimaryDisplay();
  }

  virtual gfx::Display GetDisplayMatching(
      const gfx::Rect& match_rect) const OVERRIDE {
    return GetPrimaryDisplay();
  }

  virtual gfx::Display GetPrimaryDisplay() const OVERRIDE {
    int width = 0, height = 0;
    Ecore_X_Screen* screen = ecore_x_default_screen_get();
    ecore_x_screen_size_get(screen, &width, &height);
    // fprintf(stdout, "%dx%d\n", width, height);
    return gfx::Display(0, gfx::Rect(width, height));
  }

  virtual void AddObserver(gfx::DisplayObserver* observer) OVERRIDE {
  }

  virtual void RemoveObserver(gfx::DisplayObserver* observer) OVERRIDE {
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ScreenEfl);
};

}  // namespace

namespace gfx {

Screen* CreateNativeScreen() {
  return new ScreenEfl;
}

} // namespace gfx
