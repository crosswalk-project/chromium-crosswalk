// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/screen_orientation/screen_orientation_delegate_win.h"

#include <windows.h>
#include "content/public/browser/screen_orientation_provider.h"

namespace content {

ScreenOrientationDelegateWin::ScreenOrientationDelegateWin() {
  content::ScreenOrientationProvider::SetDelegate(this);
}

ScreenOrientationDelegateWin::~ScreenOrientationDelegateWin() {
  content::ScreenOrientationProvider::SetDelegate(nullptr);
}

bool ScreenOrientationDelegateWin::FullScreenRequired(
    content::WebContents* web_contents) {
  return false;
}

// SetDisplayAutoRotationPreferences is available on Windows 8 and after.
static void SetDisplayAutoRotationPreferencesWrapper(
  ORIENTATION_PREFERENCE orientation) {
  typedef void(WINAPI *SetDisplayAutoRotationPreferencesPtr)(
    ORIENTATION_PREFERENCE);
  static SetDisplayAutoRotationPreferencesPtr
    set_display_auto_rotation_preferences_func =
    reinterpret_cast<SetDisplayAutoRotationPreferencesPtr>(
    GetProcAddress(GetModuleHandleA("user32.dll"),
    "SetDisplayAutoRotationPreferences"));
  if (set_display_auto_rotation_preferences_func)
    set_display_auto_rotation_preferences_func(orientation);
}

// GetAutoRotationState is available on Windows 8 and after.
static BOOL GetAutoRotationStateWrapper(PAR_STATE pState) {
  typedef BOOL(WINAPI *GetAutoRotationStatePtr)(PAR_STATE);
  static GetAutoRotationStatePtr get_auto_rotation_state_func =
    reinterpret_cast<GetAutoRotationStatePtr>(
    GetProcAddress(GetModuleHandleA("user32.dll"),
    "GetAutoRotationState"));
  if (get_auto_rotation_state_func)
    return get_auto_rotation_state_func(pState);
  return FALSE;
}

static void GetCurrentDisplaySettings(bool *landscape, bool *flipped) {
  DEVMODE dm;
  ZeroMemory(&dm, sizeof(dm));
  dm.dmSize = sizeof(dm);
  if (!EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm))
    return;
  if (flipped) {
    *flipped = (dm.dmDisplayOrientation == DMDO_270
                || dm.dmDisplayOrientation == DMDO_180);
  }
  if (landscape)
    *landscape = (dm.dmPelsWidth > dm.dmPelsHeight);
}

void ScreenOrientationDelegateWin::Lock(
  content::WebContents* web_contents,
  blink::WebScreenOrientationLockType lock_orientation) {
  ORIENTATION_PREFERENCE prefs = ORIENTATION_PREFERENCE_NONE;
  bool landscape = true;
  bool flipped = false;
  switch (lock_orientation) {
  case blink::WebScreenOrientationLockPortraitPrimary:
    prefs = ORIENTATION_PREFERENCE_PORTRAIT;
    break;
  case blink::WebScreenOrientationLockPortraitSecondary:
    prefs = ORIENTATION_PREFERENCE_PORTRAIT_FLIPPED;
    break;
  case blink::WebScreenOrientationLockLandscapePrimary:
    prefs = ORIENTATION_PREFERENCE_LANDSCAPE;
    break;
  case blink::WebScreenOrientationLockLandscapeSecondary:
    prefs = ORIENTATION_PREFERENCE_LANDSCAPE_FLIPPED;
    break;
  case blink::WebScreenOrientationLockPortrait:
    GetCurrentDisplaySettings(&landscape, &flipped);
    prefs = (flipped && !landscape) ? ORIENTATION_PREFERENCE_PORTRAIT_FLIPPED
                                    : ORIENTATION_PREFERENCE_PORTRAIT;
    break;
  case blink::WebScreenOrientationLockLandscape:
    GetCurrentDisplaySettings(&landscape, &flipped);
    prefs = (flipped && landscape) ? ORIENTATION_PREFERENCE_LANDSCAPE_FLIPPED
                                   : ORIENTATION_PREFERENCE_LANDSCAPE;
    break;
  case blink::WebScreenOrientationLockNatural:
    GetCurrentDisplaySettings(&landscape, &flipped);
    prefs = landscape ? ORIENTATION_PREFERENCE_LANDSCAPE
                      : ORIENTATION_PREFERENCE_PORTRAIT;
    break;
  case blink::WebScreenOrientationLockAny:
    GetCurrentDisplaySettings(&landscape, &flipped);
    if (landscape) {
      prefs = flipped ? ORIENTATION_PREFERENCE_LANDSCAPE_FLIPPED
                      : ORIENTATION_PREFERENCE_LANDSCAPE;
    } else {
      prefs = flipped ? ORIENTATION_PREFERENCE_PORTRAIT_FLIPPED
                      : ORIENTATION_PREFERENCE_PORTRAIT;
    }
    break;
  case blink::WebScreenOrientationLockDefault:
  default:
    break;
  }
  SetDisplayAutoRotationPreferencesWrapper(prefs);
}

bool ScreenOrientationDelegateWin::ScreenOrientationProviderSupported() {
  AR_STATE autoRotationState;
  ZeroMemory(&autoRotationState, sizeof(AR_STATE));
  return (GetAutoRotationStateWrapper(&autoRotationState)
          && !(autoRotationState & AR_NOSENSOR)
          && !(autoRotationState & AR_NOT_SUPPORTED));
}

void ScreenOrientationDelegateWin::Unlock(
    content::WebContents* web_contents) {
  SetDisplayAutoRotationPreferencesWrapper(ORIENTATION_PREFERENCE_NONE);
}

}  // namespace content
