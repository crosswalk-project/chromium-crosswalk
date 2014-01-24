// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/gpu/compositor_util.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "build/build_config.h"
#include "cc/base/switches.h"
#include "content/browser/gpu/gpu_data_manager_impl.h"
#include "content/public/common/content_switches.h"
#include "gpu/config/gpu_feature_type.h"

namespace content {

namespace {

struct GpuFeatureInfo {
  std::string name;
  uint32 blocked;
  bool disabled;
  std::string disabled_description;
  bool fallback_to_software;
};

// Determine if accelerated-2d-canvas is supported, which depends on whether
// lose_context could happen.
bool SupportsAccelerated2dCanvas() {
  if (GpuDataManagerImpl::GetInstance()->GetGPUInfo().can_lose_context)
    return false;
  return true;
}

#if defined(OS_CHROMEOS)
const size_t kNumFeatures = 14;
#else
const size_t kNumFeatures = 13;
#endif
const GpuFeatureInfo GetGpuFeatureInfo(size_t index) {
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  GpuDataManagerImpl* manager = GpuDataManagerImpl::GetInstance();

  const GpuFeatureInfo kGpuFeatureInfo[] = {
      {
          "2d_canvas",
          manager->IsFeatureBlacklisted(
              gpu::GPU_FEATURE_TYPE_ACCELERATED_2D_CANVAS),
          command_line.HasSwitch(switches::kDisableAccelerated2dCanvas) ||
          !SupportsAccelerated2dCanvas(),
          "Accelerated 2D canvas is unavailable: either disabled at the command"
          " line or not supported by the current system.",
          true
      },
      {
          "compositing",
          manager->IsFeatureBlacklisted(
              gpu::GPU_FEATURE_TYPE_ACCELERATED_COMPOSITING),
          command_line.HasSwitch(switches::kDisableAcceleratedCompositing),
          "Accelerated compositing has been disabled, either via about:flags or"
          " command line. This adversely affects performance of all hardware"
          " accelerated features.",
          true
      },
      {
          "3d_css",
          manager->IsFeatureBlacklisted(
              gpu::GPU_FEATURE_TYPE_ACCELERATED_COMPOSITING) ||
          manager->IsFeatureBlacklisted(gpu::GPU_FEATURE_TYPE_3D_CSS),
          command_line.HasSwitch(switches::kDisableAcceleratedLayers),
          "Accelerated layers have been disabled at the command line.",
          false
      },
      {
          "css_animation",
          manager->IsFeatureBlacklisted(
              gpu::GPU_FEATURE_TYPE_ACCELERATED_COMPOSITING) ||
          manager->IsFeatureBlacklisted(gpu::GPU_FEATURE_TYPE_3D_CSS),
          command_line.HasSwitch(cc::switches::kDisableThreadedAnimation) ||
          command_line.HasSwitch(switches::kDisableAcceleratedCompositing) ||
          command_line.HasSwitch(switches::kDisableAcceleratedLayers),
          "Accelerated CSS animation has been disabled at the command line.",
          true
      },
      {
          "webgl",
          manager->IsFeatureBlacklisted(gpu::GPU_FEATURE_TYPE_WEBGL),
          command_line.HasSwitch(switches::kDisableExperimentalWebGL),
          "WebGL has been disabled, either via about:flags or command line.",
          false
      },
      {
          "multisampling",
          manager->IsFeatureBlacklisted(gpu::GPU_FEATURE_TYPE_MULTISAMPLING),
          command_line.HasSwitch(switches::kDisableGLMultisampling),
          "Multisampling has been disabled, either via about:flags or command"
          " line.",
          false
      },
      {
          "flash_3d",
          manager->IsFeatureBlacklisted(gpu::GPU_FEATURE_TYPE_FLASH3D),
          command_line.HasSwitch(switches::kDisableFlash3d),
          "Using 3d in flash has been disabled, either via about:flags or"
          " command line.",
          false
      },
      {
          "flash_stage3d",
          manager->IsFeatureBlacklisted(gpu::GPU_FEATURE_TYPE_FLASH_STAGE3D),
          command_line.HasSwitch(switches::kDisableFlashStage3d),
          "Using Stage3d in Flash has been disabled, either via about:flags or"
          " command line.",
          false
      },
      {
          "flash_stage3d_baseline",
          manager->IsFeatureBlacklisted(
              gpu::GPU_FEATURE_TYPE_FLASH_STAGE3D_BASELINE) ||
          manager->IsFeatureBlacklisted(gpu::GPU_FEATURE_TYPE_FLASH_STAGE3D),
          command_line.HasSwitch(switches::kDisableFlashStage3d),
          "Using Stage3d Baseline profile in Flash has been disabled, either"
          " via about:flags or command line.",
          false
      },
      {
          "texture_sharing",
          manager->IsFeatureBlacklisted(gpu::GPU_FEATURE_TYPE_TEXTURE_SHARING),
          command_line.HasSwitch(switches::kDisableImageTransportSurface),
          "Sharing textures between processes has been disabled, either via"
          " about:flags or command line.",
          false
      },
      {
          "video_decode",
          manager->IsFeatureBlacklisted(
              gpu::GPU_FEATURE_TYPE_ACCELERATED_VIDEO_DECODE),
          command_line.HasSwitch(switches::kDisableAcceleratedVideoDecode),
          "Accelerated video decode has been disabled, either via about:flags"
          " or command line.",
          true
      },
#if defined(ENABLE_WEBRTC)
      {
          "video_encode",
          manager->IsFeatureBlacklisted(
              gpu::GPU_FEATURE_TYPE_ACCELERATED_VIDEO_ENCODE),
          command_line.HasSwitch(switches::kDisableWebRtcHWEncoding),
          "Accelerated video encode has been disabled, either via about:flags"
          " or command line.",
          true
      },
#endif
      {
          "video",
          manager->IsFeatureBlacklisted(
              gpu::GPU_FEATURE_TYPE_ACCELERATED_VIDEO),
          command_line.HasSwitch(switches::kDisableAcceleratedVideo) ||
          command_line.HasSwitch(switches::kDisableAcceleratedCompositing),
          "Accelerated video presentation has been disabled, either via"
          " about:flags or command line.",
          true
      },
#if defined(OS_CHROMEOS)
      {
          "panel_fitting",
          manager->IsFeatureBlacklisted(gpu::GPU_FEATURE_TYPE_PANEL_FITTING),
          command_line.HasSwitch(switches::kDisablePanelFitting),
          "Panel fitting has been disabled, either via about:flags or command"
          " line.",
          false
      },
#endif
      {
          "force_compositing_mode",
          manager->IsFeatureBlacklisted(
              gpu::GPU_FEATURE_TYPE_FORCE_COMPOSITING_MODE) &&
          !IsForceCompositingModeEnabled(),
          !IsForceCompositingModeEnabled() &&
          !manager->IsFeatureBlacklisted(
              gpu::GPU_FEATURE_TYPE_FORCE_COMPOSITING_MODE),
          "Force compositing mode is off, either disabled at the command"
          " line or not supported by the current system.",
          false
      },
  };
  return kGpuFeatureInfo[index];
}

bool CanDoAcceleratedCompositing() {
  const GpuDataManagerImpl* manager = GpuDataManagerImpl::GetInstance();

  // Don't use force compositing mode if gpu access has been blocked or
  // accelerated compositing is blacklisted.
  if (!manager->GpuAccessAllowed(NULL) ||
      manager->IsFeatureBlacklisted(
          gpu::GPU_FEATURE_TYPE_ACCELERATED_COMPOSITING))
    return false;

  // Check for SwiftShader.
  if (manager->ShouldUseSwiftShader())
    return false;

  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kDisableAcceleratedCompositing))
    return false;

  return true;
}

bool IsForceCompositingModeBlacklisted() {
  return GpuDataManagerImpl::GetInstance()->IsFeatureBlacklisted(
      gpu::GPU_FEATURE_TYPE_FORCE_COMPOSITING_MODE);
}

}  // namespace

bool IsThreadedCompositingEnabled() {
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();

  // Command line switches take precedence over blacklist.
  if (command_line.HasSwitch(switches::kDisableForceCompositingMode) ||
      command_line.HasSwitch(switches::kDisableThreadedCompositing)) {
    return false;
  } else if (command_line.HasSwitch(switches::kEnableThreadedCompositing)) {
    return true;
  }

#if defined(USE_AURA)
  // We always want threaded compositing on Aura.
  return true;
#endif

  if (!CanDoAcceleratedCompositing() || IsForceCompositingModeBlacklisted())
    return false;

#if defined(OS_MACOSX) || defined(OS_WIN)
  // Windows Vista+ has been shipping with TCM enabled at 100% since M24 and
  // Mac OSX 10.8+ since M28. The blacklist check above takes care of returning
  // false before this hits on unsupported Win/Mac versions.
  return true;
#endif

  return false;
}

bool IsForceCompositingModeEnabled() {
  // Force compositing mode is a subset of threaded compositing mode.
  if (IsThreadedCompositingEnabled())
    return true;

  const CommandLine& command_line = *CommandLine::ForCurrentProcess();

  // Command line switches take precedence over blacklisting.
  if (command_line.HasSwitch(switches::kDisableForceCompositingMode))
    return false;
  else if (command_line.HasSwitch(switches::kForceCompositingMode))
    return true;

  if (!CanDoAcceleratedCompositing() || IsForceCompositingModeBlacklisted())
    return false;

#if defined(OS_MACOSX) || defined(OS_WIN)
  // Windows Vista+ has been shipping with TCM enabled at 100% since M24 and
  // Mac OSX 10.8+ since M28. The blacklist check above takes care of returning
  // false before this hits on unsupported Win/Mac versions.
  return true;
#endif

  return false;
}

bool IsDelegatedRendererEnabled() {
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  bool enabled = false;

#if defined(USE_AURA)
  // Enable on Aura.
  enabled = true;
#endif

  // Flags override.
  enabled |= command_line.HasSwitch(switches::kEnableDelegatedRenderer);
  enabled &= !command_line.HasSwitch(switches::kDisableDelegatedRenderer);

  // Needs compositing, and thread.
  if (enabled &&
      (!IsForceCompositingModeEnabled() || !IsThreadedCompositingEnabled())) {
    enabled = false;
    LOG(ERROR) << "Disabling delegated-rendering because it needs "
               << "force-compositing-mode and threaded-compositing.";
  }

  return enabled;
}

bool IsDeadlineSchedulingEnabled() {
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();

  // Default to enabled.
  bool enabled = true;

  // Flags override.
  enabled |= command_line.HasSwitch(switches::kEnableDeadlineScheduling);
  enabled &= !command_line.HasSwitch(switches::kDisableDeadlineScheduling);

  return enabled;
}

base::Value* GetFeatureStatus() {
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  GpuDataManagerImpl* manager = GpuDataManagerImpl::GetInstance();
  std::string gpu_access_blocked_reason;
  bool gpu_access_blocked =
      !manager->GpuAccessAllowed(&gpu_access_blocked_reason);

  base::DictionaryValue* feature_status_dict = new base::DictionaryValue();

  for (size_t i = 0; i < kNumFeatures; ++i) {
    const GpuFeatureInfo gpu_feature_info = GetGpuFeatureInfo(i);
    // force_compositing_mode status is part of the compositing status.
    if (gpu_feature_info.name == "force_compositing_mode")
      continue;

    std::string status;
    if (gpu_feature_info.disabled) {
      status = "disabled";
      if (gpu_feature_info.name == "css_animation") {
        status += "_software_animated";
      } else if (gpu_feature_info.name == "raster") {
        if (cc::switches::IsImplSidePaintingEnabled())
          status += "_software_multithreaded";
        else
          status += "_software";
      } else {
        if (gpu_feature_info.fallback_to_software)
          status += "_software";
        else
          status += "_off";
      }
    } else if (manager->ShouldUseSwiftShader()) {
      status = "unavailable_software";
    } else if (gpu_feature_info.blocked ||
               gpu_access_blocked) {
      status = "unavailable";
      if (gpu_feature_info.fallback_to_software)
        status += "_software";
      else
        status += "_off";
    } else {
      status = "enabled";
      if (gpu_feature_info.name == "webgl" &&
          (command_line.HasSwitch(switches::kDisableAcceleratedCompositing) ||
           manager->IsFeatureBlacklisted(
               gpu::GPU_FEATURE_TYPE_ACCELERATED_COMPOSITING)))
        status += "_readback";
      bool has_thread = IsThreadedCompositingEnabled();
      if (gpu_feature_info.name == "compositing") {
        bool force_compositing = IsForceCompositingModeEnabled();
        if (force_compositing)
          status += "_force";
        if (has_thread)
          status += "_threaded";
      }
      if (gpu_feature_info.name == "css_animation") {
        if (has_thread)
          status = "accelerated_threaded";
        else
          status = "accelerated";
      }
    }
    // TODO(reveman): Remove this when crbug.com/223286 has been fixed.
    if (gpu_feature_info.name == "raster" &&
        cc::switches::IsImplSidePaintingEnabled()) {
      status = "disabled_software_multithreaded";
    }
    feature_status_dict->SetString(
        gpu_feature_info.name.c_str(), status.c_str());
  }
  gpu::GpuSwitchingOption gpu_switching_option =
      manager->GetGpuSwitchingOption();
  if (gpu_switching_option != gpu::GPU_SWITCHING_OPTION_UNKNOWN) {
    std::string gpu_switching;
    switch (gpu_switching_option) {
    case gpu::GPU_SWITCHING_OPTION_AUTOMATIC:
        gpu_switching = "gpu_switching_automatic";
        break;
    case gpu::GPU_SWITCHING_OPTION_FORCE_DISCRETE:
        gpu_switching = "gpu_switching_force_discrete";
        break;
    case gpu::GPU_SWITCHING_OPTION_FORCE_INTEGRATED:
        gpu_switching = "gpu_switching_force_integrated";
        break;
      default:
        break;
    }
    feature_status_dict->SetString("gpu_switching", gpu_switching.c_str());
  }
  return feature_status_dict;
}

base::Value* GetProblems() {
  GpuDataManagerImpl* manager = GpuDataManagerImpl::GetInstance();
  std::string gpu_access_blocked_reason;
  bool gpu_access_blocked =
      !manager->GpuAccessAllowed(&gpu_access_blocked_reason);

  base::ListValue* problem_list = new base::ListValue();
  manager->GetBlacklistReasons(problem_list);

  if (gpu_access_blocked) {
    base::DictionaryValue* problem = new base::DictionaryValue();
    problem->SetString("description",
        "GPU process was unable to boot: " + gpu_access_blocked_reason);
    problem->Set("crBugs", new base::ListValue());
    problem->Set("webkitBugs", new base::ListValue());
    problem_list->Insert(0, problem);
  }

  for (size_t i = 0; i < kNumFeatures; ++i) {
    const GpuFeatureInfo gpu_feature_info = GetGpuFeatureInfo(i);
    if (gpu_feature_info.disabled) {
      base::DictionaryValue* problem = new base::DictionaryValue();
      problem->SetString(
          "description", gpu_feature_info.disabled_description);
      problem->Set("crBugs", new base::ListValue());
      problem->Set("webkitBugs", new base::ListValue());
      problem_list->Append(problem);
    }
  }
  return problem_list;
}

base::Value* GetDriverBugWorkarounds() {
  base::ListValue* workaround_list = new base::ListValue();
  GpuDataManagerImpl::GetInstance()->GetDriverBugWorkarounds(workaround_list);
  return workaround_list;
}

}  // namespace content
