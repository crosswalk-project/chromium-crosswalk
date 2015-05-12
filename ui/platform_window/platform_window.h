// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_PLATFORM_WINDOW_PLATFORM_WINDOW_H_
#define UI_PLATFORM_WINDOW_PLATFORM_WINDOW_H_

#include "base/memory/scoped_ptr.h"
#include "ui/base/cursor/cursor.h"

namespace gfx {
class Rect;
}

namespace ui {

class PlatformWindowDelegate;

// Platform window.
//
// Each instance of PlatformWindow represents a single window in the
// underlying platform windowing system (i.e. X11/Win/OSX).
class PlatformWindow {
 public:
  enum PlatformWindowType {
    PLATFORM_WINDOW_UNKNOWN,
    PLATFORM_WINDOW_TYPE_TOOLTIP,
    PLATFORM_WINDOW_TYPE_POPUP,
    PLATFORM_WINDOW_TYPE_MENU,
    PLATFORM_WINDOW_TYPE_BUBBLE,
    PLATFORM_WINDOW_TYPE_WINDOW,
    PLATFORM_WINDOW_TYPE_WINDOW_FRAMELESS
  };

  virtual ~PlatformWindow() {}

  virtual void InitPlatformWindow(PlatformWindowType type,
                                  gfx::AcceleratedWidget parent_window) { }

  virtual void Show() = 0;
  virtual void Hide() = 0;
  virtual void Close() = 0;

  // Sets and gets the bounds of the platform-window. Note that the bounds is in
  // physical pixel coordinates.
  virtual void SetBounds(const gfx::Rect& bounds) = 0;
  virtual gfx::Rect GetBounds() = 0;

  virtual void SetCapture() = 0;
  virtual void ReleaseCapture() = 0;

  virtual void ToggleFullscreen() = 0;
  virtual void Maximize() = 0;
  virtual void Minimize() = 0;
  virtual void Restore() = 0;

  virtual void SetCursor(PlatformCursor cursor) = 0;

  // Moves the cursor to |location|. Location is in platform window coordinates.
  virtual void MoveCursorTo(const gfx::Point& location) = 0;

  // Confines the cursor to |bounds| when it is in the platform window. |bounds|
  // is in platform window coordinates.
  virtual void ConfineCursorToBounds(const gfx::Rect& bounds) = 0;
};

}  // namespace ui

#endif  // UI_PLATFORM_WINDOW_PLATFORM_WINDOW_H_
