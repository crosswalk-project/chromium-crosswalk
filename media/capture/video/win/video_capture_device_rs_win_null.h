// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_WIN_VIDEO_CAPTURE_DEVICE_RS_WIN_NULL_H_
#define MEDIA_CAPTURE_VIDEO_WIN_VIDEO_CAPTURE_DEVICE_RS_WIN_NULL_H_

#include "base/threading/non_thread_safe.h"
#include "media/base/media_export.h"
#include "media/capture/video/video_capture_device.h"

namespace media {

// The NULL implementation of VideoCaptureDevice.
class MEDIA_EXPORT VideoCaptureDeviceRSWin : public base::NonThreadSafe,
                                             public VideoCaptureDevice {
 public:
  explicit VideoCaptureDeviceRSWin(const Name& device_name) {}
  ~VideoCaptureDeviceRSWin() override {}

  // Opens the device driver for this device.
  bool Init() { return false; }

  // VideoCaptureDevice implementation.
  void AllocateAndStart(const VideoCaptureParams& params,
                        scoped_ptr<Client> client) override {}
  void StopAndDeAllocate() override {}

  // Utiliies used by VideoCaptureDeviceFactoryWin.
  static bool IsSupported();
  static void GetDeviceNames(Names* device_names);
  static void GetDeviceSupportedFormats(const Name& device,
                                        VideoCaptureFormats* formats);
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(VideoCaptureDeviceRSWin);
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_WIN_VIDEO_CAPTURE_DEVICE_RS_WIN_NULL_H_
