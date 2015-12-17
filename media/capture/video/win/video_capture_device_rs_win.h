// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_WIN_VIDEO_CAPTURE_DEVICE_RS_WIN_H_
#define MEDIA_CAPTURE_VIDEO_WIN_VIDEO_CAPTURE_DEVICE_RS_WIN_H_

#include <list>
#include <string>

#include "base/synchronization/lock.h"
#include "base/threading/non_thread_safe.h"
#include "media/base/media_export.h"
#include "media/capture/video/video_capture_device.h"
#include "third_party/libpxc/include/pxccapture.h"
#include "third_party/libpxc/include/pxcsensemanager.h"
#include "third_party/libpxc/include/pxcsession.h"

namespace media {

class SenseManagerHandler;

// Intel RealSense SDK (RSSDK) based implementation of VideoCaptureDevice.
class MEDIA_EXPORT VideoCaptureDeviceRSWin : public base::NonThreadSafe,
                                             public VideoCaptureDevice {
 public:
  explicit VideoCaptureDeviceRSWin(const Name& device_name);
  ~VideoCaptureDeviceRSWin() override;

  // Opens the device driver for this device.
  bool Init();

  // VideoCaptureDevice implementation.
  void AllocateAndStart(const VideoCaptureParams& params,
                        scoped_ptr<Client> client) override;
  void StopAndDeAllocate() override;

  // Utiliies used by VideoCaptureDeviceFactoryWin.
  static bool IsSupported();
  static void GetDeviceNames(Names* device_names);
  static void GetDeviceSupportedFormats(const Name& device,
                                        VideoCaptureFormats* formats);

  // Callbacks for RSSDK device API.
  bool OnNewSample(PXCCapture::Sample* sample);
  void SetErrorState(const std::string& reason);

 private:
  typedef std::list<PXCCapture::Device::StreamProfile> ProfileList;

  bool CompareProfile(
    const VideoCaptureFormat& requested,
    const PXCCapture::Device::StreamProfile& lhs,
    const PXCCapture::Device::StreamProfile& rhs);

  const PXCCapture::Device::StreamProfile& GetBestMatchedProfile(
    const VideoCaptureFormat& requested,
    const ProfileList& profiles);

  Name name_;
  PXCSession* session_;
  PXCCapture* capture_;
  PXCCapture::Device* capture_device_;
  PXCSenseManager* sense_manager_;
  ProfileList profiles_;
  scoped_ptr<SenseManagerHandler> sense_manager_handler_;

  base::Lock lock_;  // Used to guard the below variables.
  scoped_ptr<VideoCaptureDevice::Client> client_;
  VideoCaptureFormat capture_format_;
  bool capturing_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(VideoCaptureDeviceRSWin);
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_WIN_VIDEO_CAPTURE_DEVICE_RS_WIN_H_
