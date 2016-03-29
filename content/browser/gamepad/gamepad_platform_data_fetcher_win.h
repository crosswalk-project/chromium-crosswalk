// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_GAMEPAD_GAMEPAD_PLATFORM_DATA_FETCHER_WIN_H_
#define CONTENT_BROWSER_GAMEPAD_GAMEPAD_PLATFORM_DATA_FETCHER_WIN_H_

#include "build/build_config.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <stdlib.h>
#include <Unknwn.h>
#include <WinDef.h>
#include <windows.h>
#include <XInput.h>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/scoped_native_library.h"
#include "content/browser/gamepad/gamepad_data_fetcher.h"
#include "content/browser/gamepad/gamepad_standard_mappings.h"
#include "content/browser/gamepad/raw_input_data_fetcher_win.h"
#include "third_party/WebKit/public/platform/WebGamepads.h"

namespace content {

class GamepadPlatformDataFetcherWin : public GamepadDataFetcher {
 public:
  GamepadPlatformDataFetcherWin();
  ~GamepadPlatformDataFetcherWin() override;
  void GetGamepadData(blink::WebGamepads* pads,
                      bool devices_changed_hint) override;
  void PauseHint(bool paused) override;

 private:
  // XInput-specific implementation for GetGamepadData.
  bool GetXInputGamepadData(blink::WebGamepads* pads,
                            bool devices_changed_hint);

  // The three function types we use from xinput1_3.dll.
  typedef void (WINAPI *XInputEnableFunc)(BOOL enable);
  typedef DWORD (WINAPI *XInputGetCapabilitiesFunc)(
    DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES* pCapabilities);
  typedef DWORD (WINAPI *XInputGetStateFunc)(
      DWORD dwUserIndex, XINPUT_STATE* pState);

  // Get functions from dynamically loaded xinputX_Y.dll. We don't use
  // DELAYLOAD because XInputEnable is not available on all versions (it is
  // marked as deprecated on Win10) and thus the symbol is resolved in runtime.
  // Returns true if loading was successful.
  bool GetXInputDllFunctions();

  // Scan for connected XInput and DirectInput gamepads.
  void EnumerateDevices();
  bool GetXInputPadConnectivity(int i, blink::WebGamepad* pad) const;

  void GetXInputPadData(int i, blink::WebGamepad* pad);
  void GetRawInputPadData(int i, blink::WebGamepad* pad);

  int FirstAvailableGamepadId() const;
  bool HasXInputGamepad(int index) const;
  bool HasRawInputGamepad(const HANDLE handle) const;

  base::ScopedNativeLibrary xinput_dll_;
  bool xinput_available_;

  // Function pointers to XInput functionality, retrieved in
  // |GetXinputDllFunctions|.
  XInputGetCapabilitiesFunc xinput_get_capabilities_;
  XInputGetStateFunc xinput_get_state_;

  enum PadConnectionStatus {
    DISCONNECTED,
    XINPUT_CONNECTED,
    RAWINPUT_CONNECTED
  };

  struct PlatformPadState {
    PadConnectionStatus status;

    int xinput_index; // XInput-only
    HANDLE raw_input_handle;  // RawInput-only fields.
  };
  PlatformPadState platform_pad_state_[blink::WebGamepads::itemsLengthCap];

  scoped_ptr<RawInputDataFetcher> raw_input_fetcher_;

  DISALLOW_COPY_AND_ASSIGN(GamepadPlatformDataFetcherWin);
};

}  // namespace content

#endif  // CONTENT_BROWSER_GAMEPAD_GAMEPAD_PLATFORM_DATA_FETCHER_WIN_H_
