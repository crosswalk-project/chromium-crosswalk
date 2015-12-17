// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/win/video_capture_device_rs_win_null.h"

namespace media {

bool VideoCaptureDeviceRSWin::IsSupported() {
  return false;
}

void VideoCaptureDeviceRSWin::GetDeviceNames(Names* device_names) {
}

void VideoCaptureDeviceRSWin::GetDeviceSupportedFormats(
    const Name& device,
    VideoCaptureFormats* formats) {
}

}  // namespace media
