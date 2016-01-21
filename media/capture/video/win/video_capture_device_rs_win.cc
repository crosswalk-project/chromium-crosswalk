// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/win/video_capture_device_rs_win.h"

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/libpxc/include/pxcimage.h"

namespace media {

#define PXC_SUCCEEDED(status) (((pxcStatus)(status)) >= PXC_STATUS_NO_ERROR)
#define PXC_FAILED(status) (((pxcStatus)(status)) < PXC_STATUS_NO_ERROR)

// Map of color pixel formats of PXCImage::PixelFormat to Chromium ones.
static const struct {
  PXCImage::PixelFormat pxc_format;
  VideoPixelFormat chromium_format;
} pixel_formats[] = {
    {PXCImage::PIXEL_FORMAT_RGB24, PIXEL_FORMAT_RGB24},
    {PXCImage::PIXEL_FORMAT_RGB32, PIXEL_FORMAT_ARGB},
    {PXCImage::PIXEL_FORMAT_NV12, PIXEL_FORMAT_NV12},
    {PXCImage::PIXEL_FORMAT_YUY2, PIXEL_FORMAT_YUY2}
};

static VideoPixelFormat PixelFormatPxcToChromium(
    const PXCImage::PixelFormat pxc_format) {
  for (size_t i = 0; i < arraysize(pixel_formats); ++i) {
    if (pxc_format == pixel_formats[i].pxc_format)
      return pixel_formats[i].chromium_format;
  }
  DLOG(ERROR) << "Device supports an unknown media type of Chromium "
     << PXCImage::PixelFormatToString(pxc_format);
  return PIXEL_FORMAT_UNKNOWN;
}

static PXCImage::PixelFormat PixelFormatChromiumToPxc(
    const VideoPixelFormat chromium_format) {
  for (size_t i = 0; i < arraysize(pixel_formats); ++i) {
    if (chromium_format == pixel_formats[i].chromium_format)
      return pixel_formats[i].pxc_format;
  }
  DLOG(ERROR) << "Chromium requests an unknown media type of device "
     << VideoPixelFormatToString(chromium_format);
  return PXCImage::PIXEL_FORMAT_ANY;
}

bool VideoCaptureDeviceRSWin::IsSupported() {
  PXCSession* session = PXCSession_Create();
  if (!session) return false;
  return true;
}

void VideoCaptureDeviceRSWin::GetDeviceNames(Names* device_names) {
  PXCSession* session = PXCSession_Create();
  if (!session) {
    DLOG(ERROR) << "Failed to create PXCSession";
    return;
  }

  PXCSession::ImplDesc templat = {};
  templat.group = PXCSession::IMPL_GROUP_SENSOR;
  templat.subgroup = PXCSession::IMPL_SUBGROUP_VIDEO_CAPTURE;
  PXCSession::ImplDesc desc;
  int module_index = 0;
  while (PXC_SUCCEEDED(session->QueryImpl(&templat, module_index, &desc))) {
    PXCCapture* capture = NULL;
    if (PXC_FAILED(session->CreateImpl<PXCCapture>(&desc, &capture)))
      continue;

    DVLOG(1) << "RSSDK capture module: " << desc.friendlyName;

    for (int i = 0; i < capture->QueryDeviceNum(); i++) {
      PXCCapture::DeviceInfo device_info;
      if (PXC_FAILED(capture->QueryDeviceInfo(i, &device_info))) break;

      Name name(base::SysWideToUTF8(device_info.name),
                base::SysWideToUTF8(device_info.did),
                Name::RSSDK);
      name.set_capabilities_id(base::IntToString(desc.iuid));
      device_names->push_back(name);

      DVLOG(1) << "RSSDK capture device: "
          << base::SysWideToUTF8(device_info.name) << " "
          << base::SysWideToUTF8(device_info.did);
    }
    module_index++;
    capture->Release();
  }
  session->Release();
}

void VideoCaptureDeviceRSWin::GetDeviceSupportedFormats(
    const Name& device_name, VideoCaptureFormats* formats) {
  // RSSDK requires to create and initialize device to get supported formats.
  // Postpone this task to VideoCaptureDeviceRSWin::Init() as it is too
  // expensive to do here.
}

class SenseManagerHandler : public PXCSenseManager::Handler {
 public:
  explicit SenseManagerHandler(VideoCaptureDeviceRSWin* observer)
      : observer_(observer) {}
  virtual ~SenseManagerHandler() {}

  // PXCSenseManager::Handler implementation.
  virtual pxcStatus PXCAPI OnNewSample(pxcUID mid, PXCCapture::Sample* sample) {
    if (observer_->OnNewSample(sample))
      return PXC_STATUS_NO_ERROR;
    else
      return PXC_STATUS_EXEC_ABORTED;
  }
  virtual void PXCAPI OnStatus(pxcUID mid, pxcStatus status) {
    if (status < PXC_STATUS_NO_ERROR) {
      observer_->SetErrorState("On PXCSenseManager error status");
      return;
    }
  }
 private:
  VideoCaptureDeviceRSWin* observer_;
};

VideoCaptureDeviceRSWin::VideoCaptureDeviceRSWin(const Name& device_name)
    : name_(device_name),
      sense_manager_(NULL),
      capturing_(false) {
  DetachFromThread();
}

VideoCaptureDeviceRSWin::~VideoCaptureDeviceRSWin() {
  DCHECK(CalledOnValidThread());
}

bool VideoCaptureDeviceRSWin::Init() {
  DCHECK(CalledOnValidThread());
  DVLOG(1) << "VideoCaptureDeviceRSWin:Init for device " << name_.name();

  session_ = PXCSession_Create();
  if (!session_) {
    DLOG(ERROR) << "Failed to create PXCSession";
    return false;
  }

  int iuid;
  base::StringToInt(name_.capabilities_id(), &iuid);
  if (PXC_FAILED(session_->CreateImpl<PXCCapture>(iuid, &capture_))) {
    DLOG(ERROR) << "PXCSession failed to create PXCCapture with iuid " << iuid;
    return false;
  }

  for (int i = 0; i < capture_->QueryDeviceNum(); i++) {
    PXCCapture::DeviceInfo device_info;
    if (PXC_FAILED(capture_->QueryDeviceInfo(i, &device_info))) break;
    Name name(base::SysWideToUTF8(device_info.name),
              base::SysWideToUTF8(device_info.did),
              VideoCaptureDevice::Name::RSSDK);
    if (name == name_) {
      capture_device_ = capture_->CreateDevice(device_info.didx);
      break;
    }
  }

  if (!capture_device_) {
    DLOG(ERROR) << "Failed to find PXCCapture::Device";
    return false;
  }

  int num_profiles = capture_device_->QueryStreamProfileSetNum(
      PXCCapture::STREAM_TYPE_COLOR);
  DVLOG(1) << "Found " << num_profiles << " profiles.";
  for (int profile_index = 0; profile_index < num_profiles; ++profile_index) {
    PXCCapture::Device::StreamProfileSet profile_set = {};
    if (PXC_FAILED(capture_device_->QueryStreamProfileSet(
        PXCCapture::STREAM_TYPE_COLOR, profile_index, &profile_set)))
      break;
    PXCCapture::Device::StreamProfile profile = profile_set.color;
    if ((profile.imageInfo.format != PXCImage::PIXEL_FORMAT_RGB32) ||
        (profile.options &
            PXCCapture::Device::StreamOption::STREAM_OPTION_UNRECTIFIED)) {
      // Only support RGB32 and rectified streams.
      continue;
    }
    DVLOG(1) << "Profile[" << profile_index << "]: "
      << PXCImage::PixelFormatToString(profile.imageInfo.format)
      << " (" << profile.imageInfo.width << "x"
      << profile.imageInfo.height << ")"
      << " @" << profile.frameRate.max << "fps";
    profiles_.push_back(profile);
  }
  return true;
}

void VideoCaptureDeviceRSWin::AllocateAndStart(
    const VideoCaptureParams& params,
    scoped_ptr<Client> client) {
  DCHECK(CalledOnValidThread());

  base::AutoLock lock(lock_);

  client_ = client.Pass();
  DCHECK_EQ(capturing_, false);

  sense_manager_ = PXCSenseManager::CreateInstance();
  if (!sense_manager_) {
    SetErrorState("Failed to create PXCSenseManager");
    return;
  }

  DVLOG(1) << "Requested foramt: "
      << VideoCaptureFormat::ToString(params.requested_format)
      << " for device " << name_.name();

  // The PXCCaptureManager instance is owned by the SenseManager.
  PXCCaptureManager* capture_manager = sense_manager_->QueryCaptureManager();
  if (!capture_manager) {
    SetErrorState("Failed to get PXCCaptureManager");
    return;
  }

  capture_manager->FilterByDeviceInfo(
      const_cast<wchar_t*>(base::SysUTF8ToWide(name_.name()).c_str()),
      const_cast<wchar_t*>(base::SysUTF8ToWide(name_.id()).c_str()),
      0);

  PXCCapture::Device::StreamProfile best_profile;
  best_profile = GetBestMatchedProfile(params.requested_format, profiles_);

  DVLOG(1) << "Best matched profile: "
      << PXCImage::PixelFormatToString(best_profile.imageInfo.format)
      << " (" << best_profile.imageInfo.width << "x"
      << best_profile.imageInfo.height << ")"
      << " @" << best_profile.frameRate.max << "fps";

  PXCCapture::Device::StreamProfileSet requested_profile_set = {};
  requested_profile_set.color = best_profile;
  capture_manager->FilterByStreamProfiles(&requested_profile_set);

  PXCVideoModule::DataDesc desc = {};
  desc.streams.color.frameRate.min = desc.streams.color.frameRate.max =
      best_profile.frameRate.max;
  desc.streams.color.sizeMin.height = desc.streams.color.sizeMax.height =
      best_profile.imageInfo.height;
  desc.streams.color.sizeMin.width = desc.streams.color.sizeMax.width =
      best_profile.imageInfo.width;
  desc.streams.color.options = best_profile.options;
  if (PXC_FAILED(sense_manager_->EnableStreams(&desc))) {
    SetErrorState("Failed to enable streams");
    return;
  }

  sense_manager_handler_.reset(new SenseManagerHandler(this));

  if (PXC_FAILED(sense_manager_->Init(sense_manager_handler_.get()))) {
    SetErrorState("Failed to init PXCSenseManager.");
    return;
  }

  if (PXC_FAILED(sense_manager_->StreamFrames(false))) {
    SetErrorState("Failed to stream frames");
    return;
  }

  PXCCapture::Device* pxc_device = capture_manager->QueryDevice();
  if (!pxc_device) {
    SetErrorState("Failed to query PXCCapture::Device");
    return;
  }

  PXCCapture::Device::StreamProfileSet profiles = {};
  if (PXC_FAILED(pxc_device->QueryStreamProfileSet(&profiles))) {
    SetErrorState("Failed to query stream profiles");
    return;
  }

  PXCCapture::Device::StreamProfile profile = profiles.color;
  if (!profile.imageInfo.format) {
    SetErrorState("Invalid image format");
    return;
  }

  capture_format_.frame_size.SetSize(profile.imageInfo.width,
                                     profile.imageInfo.height);
  capture_format_.frame_rate = profile.frameRate.max;
  capture_format_.pixel_format =
      PixelFormatPxcToChromium(profile.imageInfo.format);

  DVLOG(1) << "Set format: " << VideoCaptureFormat::ToString(capture_format_);

  capturing_ = true;
}

void VideoCaptureDeviceRSWin::StopAndDeAllocate() {
  DCHECK(CalledOnValidThread());
  {
    base::AutoLock lock(lock_);

    if (capturing_) {
      capturing_ = false;

      sense_manager_->Close();
      sense_manager_->Release();
      capture_device_->Release();
      capture_->Release();
      session_->Release();

      sense_manager_handler_.reset();
      sense_manager_ = NULL;
    }
    client_.reset();
  }
}

bool VideoCaptureDeviceRSWin::OnNewSample(PXCCapture::Sample* sample) {
  if (sample->color) {
    PXCImage* image = sample->color;
    PXCImage::ImageInfo info = image->QueryInfo();

    PXCImage::ImageData data;
    if (PXC_FAILED(image->AcquireAccess(
        PXCImage::ACCESS_READ, info.format, &data))) {
      SetErrorState("Failed to access image data");
      return false;
    }

    client_->OnIncomingCapturedData(
        static_cast<uint8*> (data.planes[0]),
        capture_format_.ImageAllocationSize(),
        capture_format_, 0, base::TimeTicks::Now());

    image->ReleaseAccess(&data);
  }

  return true;
}

void VideoCaptureDeviceRSWin::SetErrorState(const std::string& reason) {
  DVLOG(1) << reason;
  client_->OnError(FROM_HERE, reason);
}

bool VideoCaptureDeviceRSWin::CompareProfile(
    const VideoCaptureFormat& requested,
    const PXCCapture::Device::StreamProfile& lhs,
    const PXCCapture::Device::StreamProfile& rhs) {
  const int diff_height_lhs =
      std::abs(lhs.imageInfo.height - requested.frame_size.height());
  const int diff_height_rhs =
      std::abs(rhs.imageInfo.height - requested.frame_size.height());
  if (diff_height_lhs != diff_height_rhs)
    return diff_height_lhs < diff_height_rhs;

  const int diff_width_lhs =
      std::abs(lhs.imageInfo.width - requested.frame_size.width());
  const int diff_width_rhs =
      std::abs(rhs.imageInfo.width - requested.frame_size.width());
  if (diff_width_lhs != diff_width_rhs)
    return diff_width_lhs < diff_width_rhs;

  const float diff_fps_lhs =
      std::fabs(lhs.frameRate.max - requested.frame_rate);
  const float diff_fps_rhs =
      std::fabs(rhs.frameRate.max - requested.frame_rate);
  if (diff_fps_lhs != diff_fps_rhs)
    return diff_fps_lhs < diff_fps_rhs;

  // See the PXCImage::PixelFormat definition for formarts preference.
  return lhs.imageInfo.format < rhs.imageInfo.format;
}

const PXCCapture::Device::StreamProfile&
VideoCaptureDeviceRSWin::GetBestMatchedProfile(
    const VideoCaptureFormat& requested,
    const ProfileList& profiles) {
  DCHECK(!profiles.empty());
  const PXCCapture::Device::StreamProfile* best_match = &(*profiles.begin());
  for (const PXCCapture::Device::StreamProfile& profile : profiles) {
    if (CompareProfile(requested, profile, *best_match))
      best_match = &profile;
  }
  return *best_match;
}

}  // namespace media
