// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/config/gpu_info_collector.h"

#include <X11/Xlib.h>
#include <vector>

#include "base/command_line.h"
#include "base/debug/trace_event.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"
#include "library_loaders/libpci.h"
#include "third_party/libXNVCtrl/NVCtrl.h"
#include "third_party/libXNVCtrl/NVCtrlLib.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/gl_switches.h"

namespace gpu {

namespace {

// This checks if a system supports PCI bus.
// We check the existence of /sys/bus/pci or /sys/bug/pci_express.
bool IsPciSupported() {
  const base::FilePath pci_path("/sys/bus/pci/");
  const base::FilePath pcie_path("/sys/bus/pci_express/");
  return (base::PathExists(pci_path) ||
          base::PathExists(pcie_path));
}

// Scan /etc/ati/amdpcsdb.default for "ReleaseVersion".
// Return empty string on failing.
std::string CollectDriverVersionATI() {
  const base::FilePath::CharType kATIFileName[] =
      FILE_PATH_LITERAL("/etc/ati/amdpcsdb.default");
  base::FilePath ati_file_path(kATIFileName);
  if (!base::PathExists(ati_file_path))
    return std::string();
  std::string contents;
  if (!base::ReadFileToString(ati_file_path, &contents))
    return std::string();
  base::StringTokenizer t(contents, "\r\n");
  while (t.GetNext()) {
    std::string line = t.token();
    if (StartsWithASCII(line, "ReleaseVersion=", true)) {
      size_t begin = line.find_first_of("0123456789");
      if (begin != std::string::npos) {
        size_t end = line.find_first_not_of("0123456789.", begin);
        if (end == std::string::npos)
          return line.substr(begin);
        else
          return line.substr(begin, end - begin);
      }
    }
  }
  return std::string();
}

// Use NVCtrl extention to query NV driver version.
// Return empty string on failing.
std::string CollectDriverVersionNVidia() {
  Display* display = base::MessagePumpForUI::GetDefaultXDisplay();
  if (!display) {
    LOG(ERROR) << "XOpenDisplay failed.";
    return std::string();
  }
  int event_base = 0, error_base = 0;
  if (!XNVCTRLQueryExtension(display, &event_base, &error_base)) {
    VLOG(1) << "NVCtrl extension does not exist.";
    return std::string();
  }
  int screen_count = ScreenCount(display);
  for (int screen = 0; screen < screen_count; ++screen) {
    char* buffer = NULL;
    if (XNVCTRLIsNvScreen(display, screen) &&
        XNVCTRLQueryStringAttribute(display, screen, 0,
                                    NV_CTRL_STRING_NVIDIA_DRIVER_VERSION,
                                    &buffer)) {
      std::string driver_version(buffer);
      XFree(buffer);
      return driver_version;
    }
  }
  return std::string();
}

const uint32 kVendorIDIntel = 0x8086;
const uint32 kVendorIDNVidia = 0x10de;
const uint32 kVendorIDAMD = 0x1002;

bool CollectPCIVideoCardInfo(GPUInfo* gpu_info) {
  DCHECK(gpu_info);

  if (IsPciSupported() == false) {
    VLOG(1) << "PCI bus scanning is not supported";
    return false;
  }

  // TODO(zmo): be more flexible about library name.
  LibPciLoader libpci_loader;
  if (!libpci_loader.Load("libpci.so.3") &&
      !libpci_loader.Load("libpci.so")) {
    VLOG(1) << "Failed to locate libpci";
    return false;
  }

  pci_access* access = (libpci_loader.pci_alloc)();
  DCHECK(access != NULL);
  (libpci_loader.pci_init)(access);
  (libpci_loader.pci_scan_bus)(access);
  bool primary_gpu_identified = false;
  for (pci_dev* device = access->devices;
       device != NULL; device = device->next) {
    // Fill the IDs and class fields.
    (libpci_loader.pci_fill_info)(device, 33);
    // TODO(zmo): there might be other classes that qualify as display devices.
    if (device->device_class != 0x0300)  // Device class is DISPLAY_VGA.
      continue;

    GPUInfo::GPUDevice gpu;
    gpu.vendor_id = device->vendor_id;
    gpu.device_id = device->device_id;

    if (!primary_gpu_identified) {
      primary_gpu_identified = true;
      gpu_info->gpu = gpu;
    } else {
      // TODO(zmo): if there are multiple GPUs, we assume the non Intel
      // one is primary. Revisit this logic because we actually don't know
      // which GPU we are using at this point.
      if (gpu_info->gpu.vendor_id == kVendorIDIntel &&
          gpu.vendor_id != kVendorIDIntel) {
        gpu_info->secondary_gpus.push_back(gpu_info->gpu);
        gpu_info->gpu = gpu;
      } else {
        gpu_info->secondary_gpus.push_back(gpu);
      }
    }
  }

  // Detect Optimus or AMD Switchable GPU.
  if (gpu_info->secondary_gpus.size() == 1 &&
      gpu_info->secondary_gpus[0].vendor_id == kVendorIDIntel) {
    if (gpu_info->gpu.vendor_id == kVendorIDNVidia)
      gpu_info->optimus = true;
    if (gpu_info->gpu.vendor_id == kVendorIDAMD)
      gpu_info->amd_switchable = true;
  }

  (libpci_loader.pci_cleanup)(access);
  return (primary_gpu_identified);
}

}  // namespace anonymous

CollectInfoResult CollectContextGraphicsInfo(GPUInfo* gpu_info) {
  DCHECK(gpu_info);

  TRACE_EVENT0("gpu", "gpu_info_collector::CollectGraphicsInfo");

  if (CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kGpuNoContextLost)) {
    gpu_info->can_lose_context = false;
  } else {
#if defined(OS_CHROMEOS) || defined(OS_TIZEN_MOBILE)
    gpu_info->can_lose_context = false;
#else
    // TODO(zmo): need to consider the case where we are running on top
    // of desktop GL and GL_ARB_robustness extension is available.
    gpu_info->can_lose_context =
        (gfx::GetGLImplementation() == gfx::kGLImplementationEGLGLES2);
#endif
  }

  CollectInfoResult result = CollectGraphicsInfoGL(gpu_info);
  gpu_info->finalized = true;
  return result;
}

GpuIDResult CollectGpuID(uint32* vendor_id, uint32* device_id) {
  DCHECK(vendor_id && device_id);
  *vendor_id = 0;
  *device_id = 0;

  GPUInfo gpu_info;
  if (CollectPCIVideoCardInfo(&gpu_info)) {
    *vendor_id = gpu_info.gpu.vendor_id;
    *device_id = gpu_info.gpu.device_id;
    return kGpuIDSuccess;
  }
  return kGpuIDFailure;
}

CollectInfoResult CollectBasicGraphicsInfo(GPUInfo* gpu_info) {
  DCHECK(gpu_info);

  bool rt = CollectPCIVideoCardInfo(gpu_info);

  std::string driver_version;
  switch (gpu_info->gpu.vendor_id) {
    case kVendorIDAMD:
      driver_version = CollectDriverVersionATI();
      if (!driver_version.empty()) {
        gpu_info->driver_vendor = "ATI / AMD";
        gpu_info->driver_version = driver_version;
      }
      break;
    case kVendorIDNVidia:
      driver_version = CollectDriverVersionNVidia();
      if (!driver_version.empty()) {
        gpu_info->driver_vendor = "NVIDIA";
        gpu_info->driver_version = driver_version;
      }
      break;
    case kVendorIDIntel:
      // In dual-GPU cases, sometimes PCI scan only gives us the
      // integrated GPU (i.e., the Intel one).
      driver_version = CollectDriverVersionNVidia();
      if (!driver_version.empty()) {
        gpu_info->driver_vendor = "NVIDIA";
        gpu_info->driver_version = driver_version;
        // Machines with more than two GPUs are not handled.
        if (gpu_info->secondary_gpus.size() <= 1)
          gpu_info->optimus = true;
      }
      break;
  }

  return rt ? kCollectInfoSuccess : kCollectInfoNonFatalFailure;
}

CollectInfoResult CollectDriverInfoGL(GPUInfo* gpu_info) {
  DCHECK(gpu_info);

  std::string gl_version_string = gpu_info->gl_version_string;
  if (StartsWithASCII(gl_version_string, "OpenGL ES", true))
    gl_version_string = gl_version_string.substr(10);
  std::vector<std::string> pieces;
  base::SplitStringAlongWhitespace(gl_version_string, &pieces);
  // In linux, the gl version string might be in the format of
  //   GLVersion DriverVendor DriverVersion
  if (pieces.size() < 3)
    return kCollectInfoNonFatalFailure;

  std::string driver_version = pieces[2];
  size_t pos = driver_version.find_first_not_of("0123456789.");
  if (pos == 0)
    return kCollectInfoNonFatalFailure;
  if (pos != std::string::npos)
    driver_version = driver_version.substr(0, pos);

  gpu_info->driver_vendor = pieces[1];
  gpu_info->driver_version = driver_version;
  return kCollectInfoSuccess;
}

void MergeGPUInfo(GPUInfo* basic_gpu_info,
                  const GPUInfo& context_gpu_info) {
  MergeGPUInfoGL(basic_gpu_info, context_gpu_info);
}

}  // namespace gpu
