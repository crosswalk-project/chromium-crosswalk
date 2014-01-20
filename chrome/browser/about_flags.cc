// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/about_flags.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <utility>

#include "ash/ash_switches.h"
#include "base/command_line.h"
#include "base/memory/singleton.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "cc/base/switches.h"
#include "chrome/browser/flags_storage.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/common/chrome_switches.h"
#include "components/nacl/common/nacl_switches.h"
#include "content/public/browser/user_metrics.h"
#include "extensions/common/switches.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "grit/google_chrome_strings.h"
#include "media/base/media_switches.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_base_switches.h"
#include "ui/compositor/compositor_switches.h"
#include "ui/events/event_switches.h"
#include "ui/gfx/switches.h"
#include "ui/gl/gl_switches.h"
#include "ui/keyboard/keyboard_switches.h"
#include "ui/message_center/message_center_switches.h"
#include "ui/surface/surface_switches.h"

#if defined(USE_ASH)
#include "ash/ash_switches.h"
#endif

#if defined(OS_CHROMEOS)
#include "chromeos/chromeos_switches.h"
#include "third_party/cros_system_api/switches/chrome_switches.h"
#endif

using content::UserMetricsAction;

namespace about_flags {

// Macros to simplify specifying the type.
#define SINGLE_VALUE_TYPE_AND_VALUE(command_line_switch, switch_value) \
    Experiment::SINGLE_VALUE, \
    command_line_switch, switch_value, NULL, NULL, NULL, 0
#define SINGLE_VALUE_TYPE(command_line_switch) \
    SINGLE_VALUE_TYPE_AND_VALUE(command_line_switch, "")
#define ENABLE_DISABLE_VALUE_TYPE_AND_VALUE(enable_switch, enable_value, \
                                            disable_switch, disable_value) \
    Experiment::ENABLE_DISABLE_VALUE, enable_switch, enable_value, \
    disable_switch, disable_value, NULL, 3
#define ENABLE_DISABLE_VALUE_TYPE(enable_switch, disable_switch) \
    ENABLE_DISABLE_VALUE_TYPE_AND_VALUE(enable_switch, "", disable_switch, "")
#define MULTI_VALUE_TYPE(choices) \
    Experiment::MULTI_VALUE, NULL, NULL, NULL, NULL, choices, arraysize(choices)

namespace {

const unsigned kOsAll = kOsMac | kOsWin | kOsLinux | kOsCrOS | kOsAndroid;
const unsigned kOsDesktop = kOsMac | kOsWin | kOsLinux | kOsCrOS;

// Adds a |StringValue| to |list| for each platform where |bitmask| indicates
// whether the experiment is available on that platform.
void AddOsStrings(unsigned bitmask, ListValue* list) {
  struct {
    unsigned bit;
    const char* const name;
  } kBitsToOs[] = {
    {kOsMac, "Mac"},
    {kOsWin, "Windows"},
    {kOsLinux, "Linux"},
    {kOsCrOS, "Chrome OS"},
    {kOsAndroid, "Android"},
    {kOsCrOSOwnerOnly, "Chrome OS (owner only)"},
  };
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(kBitsToOs); ++i)
    if (bitmask & kBitsToOs[i].bit)
      list->Append(new StringValue(kBitsToOs[i].name));
}

// Convert switch constants to proper CommandLine::StringType strings.
CommandLine::StringType GetSwitchString(const std::string& flag) {
  CommandLine cmd_line(CommandLine::NO_PROGRAM);
  cmd_line.AppendSwitch(flag);
  DCHECK(cmd_line.argv().size() == 2);
  return cmd_line.argv()[1];
}

// Scoops flags from a command line.
std::set<CommandLine::StringType> ExtractFlagsFromCommandLine(
    const CommandLine& cmdline) {
  std::set<CommandLine::StringType> flags;
  // First do the ones between --flag-switches-begin and --flag-switches-end.
  CommandLine::StringVector::const_iterator first =
      std::find(cmdline.argv().begin(), cmdline.argv().end(),
                GetSwitchString(switches::kFlagSwitchesBegin));
  CommandLine::StringVector::const_iterator last =
      std::find(cmdline.argv().begin(), cmdline.argv().end(),
                GetSwitchString(switches::kFlagSwitchesEnd));
  if (first != cmdline.argv().end() && last != cmdline.argv().end())
    flags.insert(first + 1, last);
#if defined(OS_CHROMEOS)
  // Then add those between --policy-switches-begin and --policy-switches-end.
  first = std::find(cmdline.argv().begin(), cmdline.argv().end(),
                    GetSwitchString(chromeos::switches::kPolicySwitchesBegin));
  last = std::find(cmdline.argv().begin(), cmdline.argv().end(),
                   GetSwitchString(chromeos::switches::kPolicySwitchesEnd));
  if (first != cmdline.argv().end() && last != cmdline.argv().end())
    flags.insert(first + 1, last);
#endif
  return flags;
}

const Experiment::Choice kEnableCompositingForFixedPositionChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED,
    switches::kEnableCompositingForFixedPosition, ""},
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    switches::kDisableCompositingForFixedPosition, ""},
  { IDS_FLAGS_COMPOSITING_FOR_FIXED_POSITION_HIGH_DPI,
    switches::kEnableHighDpiCompositingForFixedPosition, ""}
};

const Experiment::Choice kEnableCompositingForTransitionChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED,
    switches::kEnableCompositingForTransition, ""},
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    switches::kDisableCompositingForTransition, ""},
};

const Experiment::Choice kEnableAcceleratedFixedRootBackgroundChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED,
    switches::kEnableAcceleratedFixedRootBackground, ""},
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    switches::kDisableAcceleratedFixedRootBackground, ""},
};

const Experiment::Choice kGDIPresentChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_FLAGS_PRESENT_WITH_GDI_FIRST_SHOW,
    switches::kDoFirstShowPresentWithGDI, ""},
  { IDS_FLAGS_PRESENT_WITH_GDI_ALL_SHOW,
    switches::kDoAllShowPresentWithGDI, ""}
};

const Experiment::Choice kTouchEventsChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_AUTOMATIC, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED,
    switches::kTouchEvents,
    switches::kTouchEventsEnabled },
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    switches::kTouchEvents,
    switches::kTouchEventsDisabled }
};

const Experiment::Choice kTouchOptimizedUIChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_AUTOMATIC, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED,
    switches::kTouchOptimizedUI,
    switches::kTouchOptimizedUIEnabled },
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    switches::kTouchOptimizedUI,
    switches::kTouchOptimizedUIDisabled }
};

const Experiment::Choice kNaClDebugMaskChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  // Secure shell can be used on ChromeOS for forwarding the TCP port opened by
  // debug stub to a remote machine. Since secure shell uses NaCl, we provide
  // an option to switch off its debugging.
  { IDS_NACL_DEBUG_MASK_CHOICE_EXCLUDE_UTILS,
      switches::kNaClDebugMask, "!*://*/*ssh_client.nmf" },
  { IDS_NACL_DEBUG_MASK_CHOICE_INCLUDE_DEBUG,
      switches::kNaClDebugMask, "*://*/*debug.nmf" }
};

#if defined(OS_CHROMEOS)

const Experiment::Choice kChromeCaptivePortalDetectionChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_FLAGS_CHROME_CAPTIVE_PORTAL_DETECTOR,
    chromeos::switches::kEnableChromeCaptivePortalDetector, "" },
  { IDS_FLAGS_SHILL_CAPTIVE_PORTAL_DETECTOR,
    chromeos::switches::kDisableChromeCaptivePortalDetector, "" }
};

#endif

const Experiment::Choice kImplSidePaintingChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED,
    cc::switches::kEnableImplSidePainting, ""},
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    cc::switches::kDisableImplSidePainting, ""}
};

const Experiment::Choice kDeadlineSchedulingChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED,
    switches::kEnableDeadlineScheduling, ""},
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    switches::kDisableDeadlineScheduling, ""}
};

const Experiment::Choice kUIDeadlineSchedulingChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED,
    switches::kUIEnableDeadlineScheduling, ""},
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    switches::kUIDisableDeadlineScheduling, ""}
};

const Experiment::Choice kLCDTextChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED, cc::switches::kEnableLCDText, ""},
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED, cc::switches::kDisableLCDText, ""}
};

const Experiment::Choice kDelegatedRendererChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED,
    switches::kEnableDelegatedRenderer, ""},
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    switches::kDisableDelegatedRenderer, ""}
};

const Experiment::Choice kMaxTilesForInterestAreaChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_FLAGS_MAX_TILES_FOR_INTEREST_AREA_SHORT,
    cc::switches::kMaxTilesForInterestArea, "64"},
  { IDS_FLAGS_MAX_TILES_FOR_INTEREST_AREA_TALL,
    cc::switches::kMaxTilesForInterestArea, "128"},
  { IDS_FLAGS_MAX_TILES_FOR_INTEREST_AREA_GRANDE,
    cc::switches::kMaxTilesForInterestArea, "256"},
  { IDS_FLAGS_MAX_TILES_FOR_INTEREST_AREA_VENTI,
    cc::switches::kMaxTilesForInterestArea, "512"}
};

const Experiment::Choice kDefaultTileWidthChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_FLAGS_DEFAULT_TILE_WIDTH_SHORT,
    switches::kDefaultTileWidth, "128"},
  { IDS_FLAGS_DEFAULT_TILE_WIDTH_TALL,
    switches::kDefaultTileWidth, "256"},
  { IDS_FLAGS_DEFAULT_TILE_WIDTH_GRANDE,
    switches::kDefaultTileWidth, "512"},
  { IDS_FLAGS_DEFAULT_TILE_WIDTH_VENTI,
    switches::kDefaultTileWidth, "1024"}
};

#if defined(USE_ASH)
const Experiment::Choice kAshOverviewDelayChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_FLAGS_OVERVIEW_DELAY_INSTANT,
    ash::switches::kAshOverviewDelayOnAltTab, "0" },
  { IDS_FLAGS_OVERVIEW_DELAY_SHORT,
    ash::switches::kAshOverviewDelayOnAltTab, "100" },
  { IDS_FLAGS_OVERVIEW_DELAY_LONG,
    ash::switches::kAshOverviewDelayOnAltTab, "500" },
  { IDS_FLAGS_OVERVIEW_DELAY_NEVER,
    ash::switches::kAshOverviewDelayOnAltTab, "10000" },
};
#endif

const Experiment::Choice kDefaultTileHeightChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_FLAGS_DEFAULT_TILE_HEIGHT_SHORT,
    switches::kDefaultTileHeight, "128"},
  { IDS_FLAGS_DEFAULT_TILE_HEIGHT_TALL,
    switches::kDefaultTileHeight, "256"},
  { IDS_FLAGS_DEFAULT_TILE_HEIGHT_GRANDE,
    switches::kDefaultTileHeight, "512"},
  { IDS_FLAGS_DEFAULT_TILE_HEIGHT_VENTI,
    switches::kDefaultTileHeight, "1024"}
};

const Experiment::Choice kSimpleCacheBackendChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    switches::kUseSimpleCacheBackend, "off" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED,
    switches::kUseSimpleCacheBackend, "on"}
};

#if defined(USE_AURA)
const Experiment::Choice kTabCaptureUpscaleQualityChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_FLAGS_TAB_CAPTURE_SCALE_QUALITY_FAST,
    switches::kTabCaptureUpscaleQuality, "fast" },
  { IDS_FLAGS_TAB_CAPTURE_SCALE_QUALITY_GOOD,
    switches::kTabCaptureUpscaleQuality, "good" },
  { IDS_FLAGS_TAB_CAPTURE_SCALE_QUALITY_BEST,
    switches::kTabCaptureUpscaleQuality, "best" },
};

const Experiment::Choice kTabCaptureDownscaleQualityChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_FLAGS_TAB_CAPTURE_SCALE_QUALITY_FAST,
    switches::kTabCaptureDownscaleQuality, "fast" },
  { IDS_FLAGS_TAB_CAPTURE_SCALE_QUALITY_GOOD,
    switches::kTabCaptureDownscaleQuality, "good" },
  { IDS_FLAGS_TAB_CAPTURE_SCALE_QUALITY_BEST,
    switches::kTabCaptureDownscaleQuality, "best" },
};
#endif

const Experiment::Choice kMapImageChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED,
    cc::switches::kEnableMapImage, ""},
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    cc::switches::kDisableMapImage, ""}
};

// RECORDING USER METRICS FOR FLAGS:
// -----------------------------------------------------------------------------
// The first line of the experiment is the internal name. If you'd like to
// gather statistics about the usage of your flag, you should append a marker
// comment to the end of the feature name, like so:
//   "my-special-feature",  // FLAGS:RECORD_UMA
//
// After doing that, run //chrome/tools/extract_actions.py (see instructions at
// the top of that file for details) to update the chromeactions.txt file, which
// will enable UMA to record your feature flag.
//
// After your feature has shipped under a flag, you can locate the metrics under
// the action name AboutFlags_internal-action-name. Actions are recorded once
// per startup, so you should divide this number by AboutFlags_StartupTick to
// get a sense of usage. Note that this will not be the same as number of users
// with a given feature enabled because users can quit and relaunch the
// application multiple times over a given time interval. The dashboard also
// shows you how many (metrics reporting) users have enabled the flag over the
// last seven days. However, note that this is not the same as the number of
// users who have the flag enabled, since enabling the flag happens once,
// whereas running with the flag enabled happens until the user flips the flag
// again.

// To add a new experiment add to the end of kExperiments. There are two
// distinct types of experiments:
// . SINGLE_VALUE: experiment is either on or off. Use the SINGLE_VALUE_TYPE
//   macro for this type supplying the command line to the macro.
// . MULTI_VALUE: a list of choices, the first of which should correspond to a
//   deactivated state for this lab (i.e. no command line option).  To specify
//   this type of experiment use the macro MULTI_VALUE_TYPE supplying it the
//   array of choices.
// See the documentation of Experiment for details on the fields.
//
// When adding a new choice, add it to the end of the list.
const Experiment kExperiments[] = {
  {
    "expose-for-tabs",  // FLAGS:RECORD_UMA
    IDS_FLAGS_TABPOSE_NAME,
    IDS_FLAGS_TABPOSE_DESCRIPTION,
    kOsMac,
#if defined(OS_MACOSX)
    // The switch exists only on OS X.
    SINGLE_VALUE_TYPE(switches::kEnableExposeForTabs)
#else
    SINGLE_VALUE_TYPE("")
#endif
  },
  {
    "conflicting-modules-check",  // FLAGS:RECORD_UMA
    IDS_FLAGS_CONFLICTS_CHECK_NAME,
    IDS_FLAGS_CONFLICTS_CHECK_DESCRIPTION,
    kOsWin,
    SINGLE_VALUE_TYPE(switches::kConflictingModulesCheck)
  },
  {
    "ignore-gpu-blacklist",
    IDS_FLAGS_IGNORE_GPU_BLACKLIST_NAME,
    IDS_FLAGS_IGNORE_GPU_BLACKLIST_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kIgnoreGpuBlacklist)
  },
  {
    "force-compositing-mode-2",
    IDS_FLAGS_FORCE_COMPOSITING_MODE_NAME,
    IDS_FLAGS_FORCE_COMPOSITING_MODE_DESCRIPTION,
    kOsMac | kOsWin | kOsLinux,
    ENABLE_DISABLE_VALUE_TYPE(switches::kForceCompositingMode,
                              switches::kDisableForceCompositingMode)
  },
  {
    "threaded-compositing-mode",
    IDS_FLAGS_THREADED_COMPOSITING_MODE_NAME,
    IDS_FLAGS_THREADED_COMPOSITING_MODE_DESCRIPTION,
    kOsMac | kOsWin | kOsLinux,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableThreadedCompositing,
                              switches::kDisableThreadedCompositing)
  },
  {
    "force-accelerated-composited-scrolling",
     IDS_FLAGS_FORCE_ACCELERATED_OVERFLOW_SCROLL_MODE_NAME,
     IDS_FLAGS_FORCE_ACCELERATED_OVERFLOW_SCROLL_MODE_DESCRIPTION,
     kOsAll,
     ENABLE_DISABLE_VALUE_TYPE(switches::kEnableAcceleratedOverflowScroll,
                               switches::kDisableAcceleratedOverflowScroll)
  },
  {
    "force-universal-accelerated-composited-scrolling",
     IDS_FLAGS_FORCE_UNIVERSAL_ACCELERATED_OVERFLOW_SCROLL_MODE_NAME,
     IDS_FLAGS_FORCE_UNIVERSAL_ACCELERATED_OVERFLOW_SCROLL_MODE_DESCRIPTION,
     kOsAll,
     ENABLE_DISABLE_VALUE_TYPE(
         switches::kEnableUniversalAcceleratedOverflowScroll,
         switches::kDisableUniversalAcceleratedOverflowScroll)
  },
  {
    "present-with-GDI",
    IDS_FLAGS_PRESENT_WITH_GDI_NAME,
    IDS_FLAGS_PRESENT_WITH_GDI_DESCRIPTION,
    kOsWin,
    MULTI_VALUE_TYPE(kGDIPresentChoices)
  },
  {
    "enable-experimental-canvas-features",
    IDS_FLAGS_ENABLE_EXPERIMENTAL_CANVAS_FEATURES_NAME,
    IDS_FLAGS_ENABLE_EXPERIMENTAL_CANVAS_FEATURES_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableExperimentalCanvasFeatures)
  },
  {
    "disable-accelerated-2d-canvas",
    IDS_FLAGS_DISABLE_ACCELERATED_2D_CANVAS_NAME,
    IDS_FLAGS_DISABLE_ACCELERATED_2D_CANVAS_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kDisableAccelerated2dCanvas)
  },
  {
    "disable-threaded-animation",
    IDS_FLAGS_DISABLE_THREADED_ANIMATION_NAME,
    IDS_FLAGS_DISABLE_THREADED_ANIMATION_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(cc::switches::kDisableThreadedAnimation)
  },
  {
    "composited-layer-borders",
    IDS_FLAGS_COMPOSITED_LAYER_BORDERS,
    IDS_FLAGS_COMPOSITED_LAYER_BORDERS_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(cc::switches::kShowCompositedLayerBorders)
  },
  {
    "show-fps-counter",
    IDS_FLAGS_SHOW_FPS_COUNTER,
    IDS_FLAGS_SHOW_FPS_COUNTER_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(cc::switches::kShowFPSCounter)
  },
  {
    "accelerated-filters",
    IDS_FLAGS_ACCELERATED_FILTERS,
    IDS_FLAGS_ACCELERATED_FILTERS_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableAcceleratedFilters)
  },
  {
    "disable-webgl",
    IDS_FLAGS_DISABLE_WEBGL_NAME,
    IDS_FLAGS_DISABLE_WEBGL_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kDisableExperimentalWebGL)
  },
  {
    "enable-d3d11",
    IDS_FLAGS_ENABLE_D3D11_NAME,
    IDS_FLAGS_ENABLE_D3D11_DESCRIPTION,
    kOsWin,
    SINGLE_VALUE_TYPE(switches::kEnableD3D11)
  },
  {
    "disable-webrtc",
    IDS_FLAGS_DISABLE_WEBRTC_NAME,
    IDS_FLAGS_DISABLE_WEBRTC_DESCRIPTION,
    kOsAndroid,
#if defined(OS_ANDROID)
    SINGLE_VALUE_TYPE(switches::kDisableWebRTC)
#else
    SINGLE_VALUE_TYPE("")
#endif
  },
#if defined(ENABLE_WEBRTC)
  {
    "disable-device-enumeration",
    IDS_FLAGS_DISABLE_DEVICE_ENUMERATION_NAME,
    IDS_FLAGS_DISABLE_DEVICE_ENUMERATION_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kDisableDeviceEnumeration)
  },
  {
    "disable-sctp-data-channels",
    IDS_FLAGS_DISABLE_SCTP_DATA_CHANNELS_NAME,
    IDS_FLAGS_DISABLE_SCTP_DATA_CHANNELS_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kDisableSCTPDataChannels)
  },
  {
    "disable-webrtc-hw-decoding",
    IDS_FLAGS_DISABLE_WEBRTC_HW_DECODING_NAME,
    IDS_FLAGS_DISABLE_WEBRTC_HW_DECODING_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kDisableWebRtcHWDecoding)
  },
  {
    "disable-webrtc-hw-encoding",
    IDS_FLAGS_DISABLE_WEBRTC_HW_ENCODING_NAME,
    IDS_FLAGS_DISABLE_WEBRTC_HW_ENCODING_DESCRIPTION,
    kOsAndroid | kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kDisableWebRtcHWEncoding)
  },
#endif
#if defined(OS_ANDROID)
  {
    "disable-webaudio",
    IDS_FLAGS_DISABLE_WEBAUDIO_NAME,
    IDS_FLAGS_DISABLE_WEBAUDIO_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kDisableWebAudio)
  },
#endif
  {
    "fixed-position-creates-stacking-context",
    IDS_FLAGS_FIXED_POSITION_CREATES_STACKING_CONTEXT_NAME,
    IDS_FLAGS_FIXED_POSITION_CREATES_STACKING_CONTEXT_DESCRIPTION,
    kOsAll,
    ENABLE_DISABLE_VALUE_TYPE(
        switches::kEnableFixedPositionCreatesStackingContext,
        switches::kDisableFixedPositionCreatesStackingContext)
  },
  {
    "enable-compositing-for-fixed-position",
    IDS_FLAGS_COMPOSITING_FOR_FIXED_POSITION_NAME,
    IDS_FLAGS_COMPOSITING_FOR_FIXED_POSITION_DESCRIPTION,
    kOsAll,
    MULTI_VALUE_TYPE(kEnableCompositingForFixedPositionChoices)
  },
  {
    "enable-compositing-for-transition",
    IDS_FLAGS_COMPOSITING_FOR_TRANSITION_NAME,
    IDS_FLAGS_COMPOSITING_FOR_TRANSITION_DESCRIPTION,
    kOsAll,
    MULTI_VALUE_TYPE(kEnableCompositingForTransitionChoices)
  },
  {
    "enable-accelerated-fixed-root-background",
    IDS_FLAGS_ACCELERATED_FIXED_ROOT_BACKGROUND_NAME,
    IDS_FLAGS_ACCELERATED_FIXED_ROOT_BACKGROUND_DESCRIPTION,
    kOsAll,
    MULTI_VALUE_TYPE(kEnableAcceleratedFixedRootBackgroundChoices)
  },
  // TODO(bbudge): When NaCl is on by default, remove this flag entry.
  {
    "enable-nacl",  // FLAGS:RECORD_UMA
    IDS_FLAGS_ENABLE_NACL_NAME,
    IDS_FLAGS_ENABLE_NACL_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kEnableNaCl)
  },
  {
    "enable-nacl-debug",  // FLAGS:RECORD_UMA
    IDS_FLAGS_ENABLE_NACL_DEBUG_NAME,
    IDS_FLAGS_ENABLE_NACL_DEBUG_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kEnableNaClDebug)
  },
  {
    "disable-pnacl",  // FLAGS:RECORD_UMA
    IDS_FLAGS_DISABLE_PNACL_NAME,
    IDS_FLAGS_DISABLE_PNACL_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kDisablePnacl)
  },
  {
    "nacl-debug-mask",  // FLAGS:RECORD_UMA
    IDS_FLAGS_NACL_DEBUG_MASK_NAME,
    IDS_FLAGS_NACL_DEBUG_MASK_DESCRIPTION,
    kOsDesktop,
    MULTI_VALUE_TYPE(kNaClDebugMaskChoices)
  },
  {
    "extension-apis",  // FLAGS:RECORD_UMA
    IDS_FLAGS_EXPERIMENTAL_EXTENSION_APIS_NAME,
    IDS_FLAGS_EXPERIMENTAL_EXTENSION_APIS_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(extensions::switches::kEnableExperimentalExtensionApis)
  },
  {
    "extensions-on-chrome-urls",
    IDS_FLAGS_EXTENSIONS_ON_CHROME_URLS_NAME,
    IDS_FLAGS_EXTENSIONS_ON_CHROME_URLS_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(extensions::switches::kExtensionsOnChromeURLs)
  },
  {
    "enable-fast-unload",
    IDS_FLAGS_ENABLE_FAST_UNLOAD_NAME,
    IDS_FLAGS_ENABLE_FAST_UNLOAD_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableFastUnload)
  },
  {
    "enable-adview",
    IDS_FLAGS_ENABLE_ADVIEW_NAME,
    IDS_FLAGS_ENABLE_ADVIEW_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kEnableAdview)
  },
  {
    "enable-adview-src-attribute",
    IDS_FLAGS_ENABLE_ADVIEW_SRC_ATTRIBUTE_NAME,
    IDS_FLAGS_ENABLE_ADVIEW_SRC_ATTRIBUTE_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kEnableAdviewSrcAttribute)
  },
  {
    "enable-app-window-controls",
    IDS_FLAGS_ENABLE_APP_WINDOW_CONTROLS_NAME,
    IDS_FLAGS_ENABLE_APP_WINDOW_CONTROLS_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kEnableAppWindowControls)
  },
  {
    "script-badges",
    IDS_FLAGS_SCRIPT_BADGES_NAME,
    IDS_FLAGS_SCRIPT_BADGES_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kScriptBadges)
  },
  {
    "script-bubble",
    IDS_FLAGS_SCRIPT_BUBBLE_NAME,
    IDS_FLAGS_SCRIPT_BUBBLE_DESCRIPTION,
    kOsDesktop,
    ENABLE_DISABLE_VALUE_TYPE_AND_VALUE(switches::kScriptBubble, "1",
                                        switches::kScriptBubble, "0")
  },
  {
    "apps-new-install-bubble",
    IDS_FLAGS_APPS_NEW_INSTALL_BUBBLE_NAME,
    IDS_FLAGS_APPS_NEW_INSTALL_BUBBLE_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kAppsNewInstallBubble)
  },
  {
    "disable-hyperlink-auditing",
    IDS_FLAGS_DISABLE_HYPERLINK_AUDITING_NAME,
    IDS_FLAGS_DISABLE_HYPERLINK_AUDITING_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kNoPings)
  },
  {
    "tab-groups-context-menu",
    IDS_FLAGS_TAB_GROUPS_CONTEXT_MENU_NAME,
    IDS_FLAGS_TAB_GROUPS_CONTEXT_MENU_DESCRIPTION,
    kOsWin,
    SINGLE_VALUE_TYPE(switches::kEnableTabGroupsContextMenu)
  },
  {
    "enable-instant-extended-api",
    IDS_FLAGS_ENABLE_INSTANT_EXTENDED_API,
    IDS_FLAGS_ENABLE_INSTANT_EXTENDED_API_DESCRIPTION,
    kOsMac | kOsWin | kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableInstantExtendedAPI,
                              switches::kDisableInstantExtendedAPI)
  },
  {
    "use-cacheable-new-tab-page",
    IDS_FLAGS_ENABLE_INSTANT_EXTENDED_CACHEABLE_NTP,
    IDS_FLAGS_ENABLE_INSTANT_EXTENDED_CACHEABLE_NTP_DESCRIPTION,
    kOsMac | kOsWin | kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kUseCacheableNewTabPage)
  },
  {
    "enable-local-first-load-ntp",
    IDS_FLAGS_ENABLE_LOCAL_FIRST_LOAD_NTP,
    IDS_FLAGS_ENABLE_LOCAL_FIRST_LOAD_NTP_DESCRIPTION,
    kOsMac | kOsWin | kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableLocalFirstLoadNTP,
                              switches::kDisableLocalFirstLoadNTP)
  },
#if defined(OS_ANDROID)
  {
    "enable-new-ntp",
    IDS_FLAGS_ENABLE_NEW_NTP,
    IDS_FLAGS_ENABLE_NEW_NTP_DESCRIPTION,
    kOsAndroid,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableNewNTP,
                              switches::kDisableNewNTP)
  },
#endif
  {
    "show-autofill-type-predictions",
    IDS_FLAGS_SHOW_AUTOFILL_TYPE_PREDICTIONS_NAME,
    IDS_FLAGS_SHOW_AUTOFILL_TYPE_PREDICTIONS_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(autofill::switches::kShowAutofillTypePredictions)
  },
  {
    "enable-gesture-tap-highlight",
    IDS_FLAGS_ENABLE_GESTURE_TAP_HIGHLIGHTING_NAME,
    IDS_FLAGS_ENABLE_GESTURE_TAP_HIGHLIGHTING_DESCRIPTION,
    kOsLinux | kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableGestureTapHighlight,
                              switches::kDisableGestureTapHighlight)
  },
  {
    "enable-smooth-scrolling",  // FLAGS:RECORD_UMA
    IDS_FLAGS_ENABLE_SMOOTH_SCROLLING_NAME,
    IDS_FLAGS_ENABLE_SMOOTH_SCROLLING_DESCRIPTION,
    // Can't expose the switch unless the code is compiled in.
    // On by default for the Mac (different implementation in WebKit).
    kOsWin | kOsLinux,
    SINGLE_VALUE_TYPE(switches::kEnableSmoothScrolling)
  },
  {
    "enable-overlay-scrollbars",
    IDS_FLAGS_ENABLE_OVERLAY_SCROLLBARS_NAME,
    IDS_FLAGS_ENABLE_OVERLAY_SCROLLBARS_DESCRIPTION,
    // Uses the system preference on Mac (a different implementation).
    // On Android, this is always enabled.
    kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kEnableOverlayScrollbars)
  },
  {
    "enable-panels",
    IDS_FLAGS_ENABLE_PANELS_NAME,
    IDS_FLAGS_ENABLE_PANELS_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kEnablePanels)
  },
  {
    // See http://crbug.com/120416 for how to remove this flag.
    "save-page-as-mhtml",  // FLAGS:RECORD_UMA
    IDS_FLAGS_SAVE_PAGE_AS_MHTML_NAME,
    IDS_FLAGS_SAVE_PAGE_AS_MHTML_DESCRIPTION,
    kOsMac | kOsWin | kOsLinux,
    SINGLE_VALUE_TYPE(switches::kSavePageAsMHTML)
  },
  {
    "enable-autologin",
    IDS_FLAGS_ENABLE_AUTOLOGIN_NAME,
    IDS_FLAGS_ENABLE_AUTOLOGIN_DESCRIPTION,
    kOsMac | kOsWin | kOsLinux,
    SINGLE_VALUE_TYPE(switches::kEnableAutologin)
  },
  {
    "enable-quic",
    IDS_FLAGS_ENABLE_QUIC_NAME,
    IDS_FLAGS_ENABLE_QUIC_DESCRIPTION,
    kOsAll,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableQuic,
                              switches::kDisableQuic)
  },
  {
    "enable-quic-https",
    IDS_FLAGS_ENABLE_QUIC_HTTPS_NAME,
    IDS_FLAGS_ENABLE_QUIC_HTTPS_DESCRIPTION,
    kOsAll,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableQuicHttps,
                              switches::kDisableQuicHttps)
  },
  {
    "enable-spdy4a2",
    IDS_FLAGS_ENABLE_SPDY4A2_NAME,
    IDS_FLAGS_ENABLE_SPDY4A2_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableSpdy4a2)
  },
  {
    "enable-http2-draft-04",
    IDS_FLAGS_ENABLE_HTTP2_DRAFT_04_NAME,
    IDS_FLAGS_ENABLE_HTTP2_DRAFT_04_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableHttp2Draft04)
  },
  {
    "enable-async-dns",
    IDS_FLAGS_ENABLE_ASYNC_DNS_NAME,
    IDS_FLAGS_ENABLE_ASYNC_DNS_DESCRIPTION,
    kOsWin | kOsMac | kOsLinux | kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableAsyncDns,
                              switches::kDisableAsyncDns)
  },
  {
    "disable-webkit-media-source",
    IDS_FLAGS_DISABLE_WEBKIT_MEDIA_SOURCE_NAME,
    IDS_FLAGS_DISABLE_WEBKIT_MEDIA_SOURCE_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kDisableWebKitMediaSource)
  },
  {
    "disable-unprefixed-media-source",
    IDS_FLAGS_DISABLE_UNPREFIXED_MEDIA_SOURCE_NAME,
    IDS_FLAGS_DISABLE_UNPREFIXED_MEDIA_SOURCE_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kDisableUnprefixedMediaSource)
  },
  {
    "enable-encrypted-media",
    IDS_FLAGS_ENABLE_ENCRYPTED_MEDIA_NAME,
    IDS_FLAGS_ENABLE_ENCRYPTED_MEDIA_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableEncryptedMedia)
  },
  {
    "disable-prefixed-encrypted-media",
    IDS_FLAGS_DISABLE_PREFIXED_ENCRYPTED_MEDIA_NAME,
    IDS_FLAGS_DISABLE_PREFIXED_ENCRYPTED_MEDIA_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kDisablePrefixedEncryptedMedia)
  },
#if defined(OS_ANDROID)
  {
    "disable-infobar-for-protected-media-identifier",
    IDS_FLAGS_DISABLE_INFOBAR_FOR_PROTECTED_MEDIA_IDENTIFIER_NAME,
    IDS_FLAGS_DISABLE_INFOBAR_FOR_PROTECTED_MEDIA_IDENTIFIER_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kDisableInfobarForProtectedMediaIdentifier)
  },
  {
    "mediadrm-enable-non-compositing",
    IDS_FLAGS_MEDIADRM_ENABLE_NON_COMPOSITING_NAME,
    IDS_FLAGS_MEDIADRM_ENABLE_NON_COMPOSITING_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kMediaDrmEnableNonCompositing)
  },
#endif  // defined(OS_ANDROID)
  {
    "enable-opus-playback",
    IDS_FLAGS_ENABLE_OPUS_PLAYBACK_NAME,
    IDS_FLAGS_ENABLE_OPUS_PLAYBACK_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kEnableOpusPlayback)
  },
 {
    "disable-vp8-alpha-playback",
    IDS_FLAGS_DISABLE_VP8_ALPHA_PLAYBACK_NAME,
    IDS_FLAGS_DISABLE_VP8_ALPHA_PLAYBACK_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kDisableVp8AlphaPlayback)
  },
  {
    "enable-managed-users",
    IDS_FLAGS_ENABLE_LOCALLY_MANAGED_USERS_NAME,
    IDS_FLAGS_ENABLE_LOCALLY_MANAGED_USERS_DESCRIPTION,
    kOsMac | kOsWin | kOsLinux | kOsCrOSOwnerOnly,
    SINGLE_VALUE_TYPE(switches::kEnableManagedUsers)
  },
  {
    "per-tile-painting",
    IDS_FLAGS_PER_TILE_PAINTING_NAME,
    IDS_FLAGS_PER_TILE_PAINTING_DESCRIPTION,
    kOsMac | kOsLinux | kOsCrOS,
    SINGLE_VALUE_TYPE(cc::switches::kEnablePerTilePainting)
  },
  {
    "enable-javascript-harmony",
    IDS_FLAGS_ENABLE_JAVASCRIPT_HARMONY_NAME,
    IDS_FLAGS_ENABLE_JAVASCRIPT_HARMONY_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE_AND_VALUE(switches::kJavaScriptFlags, "--harmony")
  },
  {
    "enable-tab-browser-dragging",
    IDS_FLAGS_ENABLE_TAB_BROWSER_DRAGGING_NAME,
    IDS_FLAGS_ENABLE_TAB_BROWSER_DRAGGING_DESCRIPTION,
    kOsWin,
    SINGLE_VALUE_TYPE(switches::kTabBrowserDragging)
  },
  {
    "disable-restore-session-state",
    IDS_FLAGS_DISABLE_RESTORE_SESSION_STATE_NAME,
    IDS_FLAGS_DISABLE_RESTORE_SESSION_STATE_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kDisableRestoreSessionState)
  },
  {
    "disable-software-rasterizer",
    IDS_FLAGS_DISABLE_SOFTWARE_RASTERIZER_NAME,
    IDS_FLAGS_DISABLE_SOFTWARE_RASTERIZER_DESCRIPTION,
#if defined(ENABLE_SWIFTSHADER)
    kOsAll,
#else
    0,
#endif
    SINGLE_VALUE_TYPE(switches::kDisableSoftwareRasterizer)
  },
  {
    "enable-experimental-web-platform-features",
    IDS_FLAGS_EXPERIMENTAL_WEB_PLATFORM_FEATURES_NAME,
    IDS_FLAGS_EXPERIMENTAL_WEB_PLATFORM_FEATURES_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableExperimentalWebPlatformFeatures)
  },
  {
    "disable-ntp-other-sessions-menu",
    IDS_FLAGS_NTP_OTHER_SESSIONS_MENU_NAME,
    IDS_FLAGS_NTP_OTHER_SESSIONS_MENU_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kDisableNTPOtherSessionsMenu)
  },
#if defined(USE_ASH)
  {
    "enable-ash-oak",
    IDS_FLAGS_ENABLE_ASH_OAK_NAME,
    IDS_FLAGS_ENABLE_ASH_OAK_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(ash::switches::kAshEnableOak),
  },
#endif
  {
    "enable-devtools-experiments",
    IDS_FLAGS_ENABLE_DEVTOOLS_EXPERIMENTS_NAME,
    IDS_FLAGS_ENABLE_DEVTOOLS_EXPERIMENTS_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kEnableDevToolsExperiments)
  },
  {
    "silent-debugger-extension-api",
    IDS_FLAGS_SILENT_DEBUGGER_EXTENSION_API_NAME,
    IDS_FLAGS_SILENT_DEBUGGER_EXTENSION_API_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kSilentDebuggerExtensionAPI)
  },
  {
    "enable-suggestions-ntp",
    IDS_FLAGS_NTP_SUGGESTIONS_PAGE_NAME,
    IDS_FLAGS_NTP_SUGGESTIONS_PAGE_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kEnableSuggestionsTabPage)
  },
  {
    "spellcheck-autocorrect",
    IDS_FLAGS_SPELLCHECK_AUTOCORRECT,
    IDS_FLAGS_SPELLCHECK_AUTOCORRECT_DESCRIPTION,
    kOsWin | kOsLinux | kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kEnableSpellingAutoCorrect)
  },
  {
    "enable-scroll-prediction",
    IDS_FLAGS_ENABLE_SCROLL_PREDICTION_NAME,
    IDS_FLAGS_ENABLE_SCROLL_PREDICTION_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kEnableScrollPrediction)
  },
  {
    "touch-events",
    IDS_TOUCH_EVENTS_NAME,
    IDS_TOUCH_EVENTS_DESCRIPTION,
    kOsDesktop,
    MULTI_VALUE_TYPE(kTouchEventsChoices)
  },
  {
    "touch-optimized-ui",
    IDS_FLAGS_TOUCH_OPTIMIZED_UI_NAME,
    IDS_FLAGS_TOUCH_OPTIMIZED_UI_DESCRIPTION,
    kOsWin,
    MULTI_VALUE_TYPE(kTouchOptimizedUIChoices)
  },
  {
    "disable-touch-adjustment",
    IDS_DISABLE_TOUCH_ADJUSTMENT_NAME,
    IDS_DISABLE_TOUCH_ADJUSTMENT_DESCRIPTION,
    kOsWin | kOsLinux | kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kDisableTouchAdjustment)
  },
#if defined(OS_CHROMEOS)
  {
      "ash-use-alternate-shelf",
      IDS_FLAGS_ALTERNATE_SHELF_LAYOUT_NAME,
      IDS_FLAGS_ALTERNATE_SHELF_LAYOUT_DESCRIPTION,
      kOsCrOS,
      ENABLE_DISABLE_VALUE_TYPE(ash::switches::kAshUseAlternateShelfLayout,
                                ash::switches::kAshDisableAlternateShelfLayout)
  },
  {
      "ash-disable-drag-off-shelf",
      IDS_FLAGS_DRAG_OFF_SHELF_NAME,
      IDS_FLAGS_DRAG_OFF_SHELF_DESCRIPTION,
      kOsCrOS,
      SINGLE_VALUE_TYPE(ash::switches::kAshDisableDragOffShelf)
  },
  {
    "enable-background-loader",
    IDS_ENABLE_BACKLOADER_NAME,
    IDS_ENABLE_BACKLOADER_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kEnableBackgroundLoader)
  },
  {
    "ash-enable-docked-windows",
    IDS_FLAGS_DOCKED_WINDOWS_NAME,
    IDS_FLAGS_DOCKED_WINDOWS_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(ash::switches::kAshEnableDockedWindows)
  },
#endif
  {
    "enable-download-resumption",
    IDS_FLAGS_ENABLE_DOWNLOAD_RESUMPTION_NAME,
    IDS_FLAGS_ENABLE_DOWNLOAD_RESUMPTION_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kEnableDownloadResumption)
  },
  {
    "allow-nacl-socket-api",
    IDS_FLAGS_ALLOW_NACL_SOCKET_API_NAME,
    IDS_FLAGS_ALLOW_NACL_SOCKET_API_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE_AND_VALUE(switches::kAllowNaClSocketAPI, "*")
  },
  {
    "stacked-tab-strip",
    IDS_FLAGS_STACKED_TAB_STRIP_NAME,
    IDS_FLAGS_STACKED_TAB_STRIP_DESCRIPTION,
    kOsWin,
    SINGLE_VALUE_TYPE(switches::kEnableStackedTabStrip)
  },
  {
    "force-device-scale-factor",
    IDS_FLAGS_FORCE_HIGH_DPI_NAME,
    IDS_FLAGS_FORCE_HIGH_DPI_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE_AND_VALUE(switches::kForceDeviceScaleFactor, "2")
  },
#if defined(OS_CHROMEOS)
  {
    "allow-touchpad-three-finger-click",
    IDS_FLAGS_ALLOW_TOUCHPAD_THREE_FINGER_CLICK_NAME,
    IDS_FLAGS_ALLOW_TOUCHPAD_THREE_FINGER_CLICK_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kEnableTouchpadThreeFingerClick)
  },
#endif
#if defined(USE_ASH)
  {
    "show-launcher-alignment-menu",
    IDS_FLAGS_SHOW_SHELF_ALIGNMENT_MENU_NAME,
    IDS_FLAGS_SHOW_SHELF_ALIGNMENT_MENU_DESCRIPTION,
    kOsAll,
    ENABLE_DISABLE_VALUE_TYPE(
        ash::switches::kShowShelfAlignmentMenu,
        ash::switches::kHideShelfAlignmentMenu)
  },
  {
    "disable-minimize-on-second-launcher-item-click",
    IDS_FLAGS_DISABLE_MINIMIZE_ON_SECOND_LAUNCHER_ITEM_CLICK_NAME,
    IDS_FLAGS_DISABLE_MINIMIZE_ON_SECOND_LAUNCHER_ITEM_CLICK_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kDisableMinimizeOnSecondLauncherItemClick)
  },
  {
    "disable-overview-mode",
    IDS_FLAGS_DISABLE_OVERVIEW_MODE_NAME,
    IDS_FLAGS_DISABLE_OVERVIEW_MODE_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(ash::switches::kAshDisableOverviewMode)
  },
  {
    "overview-delay-on-alt-tab",
    IDS_FLAGS_OVERVIEW_DELAY_NAME,
    IDS_FLAGS_OVERVIEW_DELAY_DESCRIPTION,
    kOsCrOS,
    MULTI_VALUE_TYPE(kAshOverviewDelayChoices)
  },
  {
    "show-touch-hud",
    IDS_FLAGS_SHOW_TOUCH_HUD_NAME,
    IDS_FLAGS_SHOW_TOUCH_HUD_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(ash::switches::kAshTouchHud)
  },
  {
    "enable-pinch",
    IDS_FLAGS_ENABLE_PINCH_SCALE_NAME,
    IDS_FLAGS_ENABLE_PINCH_SCALE_DESCRIPTION,
    kOsLinux | kOsWin | kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnablePinch, switches::kDisablePinch),
  },
  {
    "enable-pinch-virtual-viewport",
    IDS_FLAGS_ENABLE_PINCH_VIRTUAL_VIEWPORT_NAME,
    IDS_FLAGS_ENABLE_PINCH_VIRTUAL_VIEWPORT_DESCRIPTION,
    kOsLinux | kOsWin | kOsCrOS,
    SINGLE_VALUE_TYPE(cc::switches::kEnablePinchVirtualViewport),
  },
#endif  // defined(USE_ASH)
#if defined(OS_CHROMEOS)
  {
    "disable-boot-animation",
    IDS_FLAGS_DISABLE_BOOT_ANIMATION,
    IDS_FLAGS_DISABLE_BOOT_ANIMATION_DESCRIPTION,
    kOsCrOSOwnerOnly,
    SINGLE_VALUE_TYPE(chromeos::switches::kDisableBootAnimation),
  },
  {
    "captive-portal-detector",
    IDS_FLAGS_CAPTIVE_PORTAL_DETECTOR_NAME,
    IDS_FLAGS_CAPTIVE_PORTAL_DETECTOR_DESCRIPTION,
    kOsCrOSOwnerOnly,
    MULTI_VALUE_TYPE(kChromeCaptivePortalDetectionChoices),
  },
  {
    "file-manager-show-checkboxes",
    IDS_FLAGS_FILE_MANAGER_SHOW_CHECKBOXES_NAME,
    IDS_FLAGS_FILE_MANAGER_SHOW_CHECKBOXES_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kFileManagerShowCheckboxes)
  },
  {
    "file-manager-enable-webstore-integration",
    IDS_FLAGS_FILE_MANAGER_ENABLE_WEBSTORE_INTEGRATION,
    IDS_FLAGS_FILE_MANAGER_ENABLE_WEBSTORE_INTEGRATION_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kFileManagerEnableWebstoreIntegration)
  },
  {
    "disable-quickoffice-component-app",
    IDS_FLAGS_DISABLE_QUICKOFFICE_COMPONENT_APP_NAME,
    IDS_FLAGS_DISABLE_QUICKOFFICE_COMPONENT_APP_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kDisableQuickofficeComponentApp),
  },
  {
    "enable-saml-signin",
    IDS_FLAGS_ENABLE_SAML_SIGNIN_NAME,
    IDS_FLAGS_ENABLE_SAML_SIGNIN_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kEnableSamlSignin),
  },
  {
    "enable-multi-profiles",
    IDS_FLAGS_ENABLE_MULTI_PROFILES_NAME,
    IDS_FLAGS_ENABLE_MULTI_PROFILES_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kMultiProfiles),
  },
#endif  // defined(OS_CHROMEOS)
  {
    "views-textfield",
    IDS_FLAGS_VIEWS_TEXTFIELD_NAME,
    IDS_FLAGS_VIEWS_TEXTFIELD_DESCRIPTION,
    kOsWin,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableViewsTextfield,
                              switches::kDisableViewsTextfield),
  },
  { "disable-accelerated-video-decode",
    IDS_FLAGS_DISABLE_ACCELERATED_VIDEO_DECODE_NAME,
    IDS_FLAGS_DISABLE_ACCELERATED_VIDEO_DECODE_DESCRIPTION,
    kOsWin | kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kDisableAcceleratedVideoDecode),
  },
  {
    "enable-contacts",
    IDS_FLAGS_ENABLE_CONTACTS_NAME,
    IDS_FLAGS_ENABLE_CONTACTS_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kEnableContacts)
  },
#if defined(USE_ASH)
  {
    "ash-debug-shortcuts",
    IDS_FLAGS_DEBUG_SHORTCUTS_NAME,
    IDS_FLAGS_DEBUG_SHORTCUTS_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(ash::switches::kAshDebugShortcuts),
  },
  { "ash-enable-advanced-gestures",
    IDS_FLAGS_ENABLE_ADVANCED_GESTURES_NAME,
    IDS_FLAGS_ENABLE_ADVANCED_GESTURES_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(ash::switches::kAshEnableAdvancedGestures),
  },
  { "ash-alternate-caption-button",
    IDS_FLAGS_ASH_FRAME_CAPTION_BUTTON_STYLE_NAME,
    IDS_FLAGS_ASH_FRAME_CAPTION_BUTTON_STYLE_DESCRIPTION,
    kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE(
        ash::switches::kAshEnableAlternateFrameCaptionButtonStyle,
        ash::switches::kAshDisableAlternateFrameCaptionButtonStyle),
  },
  {
    "ash-disable-auto-maximizing",
    IDS_FLAGS_ASH_AUTO_MAXIMIZING_NAME,
    IDS_FLAGS_ASH_AUTO_MAXIMIZING_DESCRIPTION,
    kOsWin | kOsLinux | kOsCrOS,
    SINGLE_VALUE_TYPE(ash::switches::kAshDisableAutoMaximizing)
  },
  { "ash-disable-drag-and-drop-applist-to-launcher",
    IDS_FLAGS_DND_APPLIST_TO_LAUNCHER_NAME,
    IDS_FLAGS_DND_APPLIST_TO_LAUNCHER_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(ash::switches::kAshDisableDragAndDropAppListToLauncher),
  },
#if defined(OS_LINUX)
  { "ash-enable-memory-monitor",
      IDS_FLAGS_ENABLE_MEMORY_MONITOR_NAME,
      IDS_FLAGS_ENABLE_MEMORY_MONITOR_DESCRIPTION,
      kOsCrOS,
      SINGLE_VALUE_TYPE(ash::switches::kAshEnableMemoryMonitor),
  },
#endif
#if defined(OS_CHROMEOS)
  { "ash-enable-full-multi-profile-mode",
      IDS_FLAGS_ENABLE_FULL_MULTI_PROFILE_MODE,
      IDS_FLAGS_ENABLE_FULL_MULTI_PROFILE_MODE_DESCRIPTION,
      kOsCrOS,
      SINGLE_VALUE_TYPE(ash::switches::kAshEnableFullMultiProfileMode),
  },
#endif
#endif
#if defined(OS_CHROMEOS)
  {
    "ash-audio-device-menu",
    IDS_FLAGS_ASH_AUDIO_DEVICE_MENU_NAME,
    IDS_FLAGS_ASH_AUDIO_DEVICE_MENU_DESCRIPTION,
    kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE(ash::switches::kAshEnableAudioDeviceMenu,
                              ash::switches::kAshDisableAudioDeviceMenu)
  },
  {
    "enable-carrier-switching",
    IDS_FLAGS_ENABLE_CARRIER_SWITCHING,
    IDS_FLAGS_ENABLE_CARRIER_SWITCHING_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kEnableCarrierSwitching)
  },
  {
    "enable-request-tablet-site",
    IDS_FLAGS_ENABLE_REQUEST_TABLET_SITE_NAME,
    IDS_FLAGS_ENABLE_REQUEST_TABLET_SITE_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kEnableRequestTabletSite)
  },
#endif
  {
    "debug-packed-apps",
    IDS_FLAGS_DEBUG_PACKED_APP_NAME,
    IDS_FLAGS_DEBUG_PACKED_APP_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kDebugPackedApps)
  },
  {
    "enable-password-generation",
    IDS_FLAGS_ENABLE_PASSWORD_GENERATION_NAME,
    IDS_FLAGS_ENABLE_PASSWORD_GENERATION_DESCRIPTION,
    kOsDesktop,
    ENABLE_DISABLE_VALUE_TYPE(autofill::switches::kEnablePasswordGeneration,
                              autofill::switches::kDisablePasswordGeneration)
  },
  {
    "enable-password-manager-reauthentication",
    IDS_FLAGS_PASSWORD_MANAGER_REAUTHENTICATION_NAME,
    IDS_FLAGS_PASSWORD_MANAGER_REAUTHENTICATION_DESCRIPTION,
    kOsMac,
    SINGLE_VALUE_TYPE(switches::kEnablePasswordManagerReauthentication)
  },
  {
    "enable-people-search",
    IDS_FLAGS_ENABLE_PEOPLE_SEARCH_NAME,
    IDS_FLAGS_ENABLE_PEOPLE_SEARCH_DESCRIPTION,
    kOsMac | kOsWin | kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kEnablePeopleSearch)
  },
  {
    "password-autofill-public-suffix-domain-matching",
    IDS_FLAGS_PASSWORD_AUTOFILL_PUBLIC_SUFFIX_DOMAIN_MATCHING_NAME,
    IDS_FLAGS_PASSWORD_AUTOFILL_PUBLIC_SUFFIX_DOMAIN_MATCHING_DESCRIPTION,
    kOsAndroid,
    ENABLE_DISABLE_VALUE_TYPE(
        switches::kEnablePasswordAutofillPublicSuffixDomainMatching,
        switches::kDisablePasswordAutofillPublicSuffixDomainMatching)
  },
  {
    "enable-deferred-image-decoding",
    IDS_FLAGS_ENABLE_DEFERRED_IMAGE_DECODING_NAME,
    IDS_FLAGS_ENABLE_DEFERRED_IMAGE_DECODING_DESCRIPTION,
    kOsMac | kOsLinux | kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kEnableDeferredImageDecoding)
  },
  {
    "performance-monitor-gathering",
    IDS_FLAGS_PERFORMANCE_MONITOR_GATHERING_NAME,
    IDS_FLAGS_PERFORMANCE_MONITOR_GATHERING_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kPerformanceMonitorGathering)
  },
  {
    "enable-experimental-form-filling",
    IDS_FLAGS_ENABLE_EXPERIMENTAL_FORM_FILLING_NAME,
    IDS_FLAGS_ENABLE_EXPERIMENTAL_FORM_FILLING_DESCRIPTION,
    kOsWin | kOsCrOS,
    SINGLE_VALUE_TYPE(autofill::switches::kEnableExperimentalFormFilling)
  },
  {
    "wallet-service-use-sandbox",
    IDS_FLAGS_WALLET_SERVICE_USE_SANDBOX_NAME,
    IDS_FLAGS_WALLET_SERVICE_USE_SANDBOX_DESCRIPTION,
    kOsCrOS | kOsWin | kOsMac,
    ENABLE_DISABLE_VALUE_TYPE_AND_VALUE(
        autofill::switches::kWalletServiceUseSandbox, "1",
        autofill::switches::kWalletServiceUseSandbox, "0")
  },
  {
    "enable-interactive-autocomplete",
    IDS_FLAGS_ENABLE_INTERACTIVE_AUTOCOMPLETE_NAME,
    IDS_FLAGS_ENABLE_INTERACTIVE_AUTOCOMPLETE_DESCRIPTION,
    kOsWin | kOsCrOS | kOsAndroid | kOsMac,
    ENABLE_DISABLE_VALUE_TYPE(
        autofill::switches::kEnableInteractiveAutocomplete,
        autofill::switches::kDisableInteractiveAutocomplete)
  },
#if defined(USE_AURA)
  {
    "overscroll-history-navigation",
    IDS_FLAGS_OVERSCROLL_HISTORY_NAVIGATION_NAME,
    IDS_FLAGS_OVERSCROLL_HISTORY_NAVIGATION_DESCRIPTION,
    kOsAll,
    ENABLE_DISABLE_VALUE_TYPE_AND_VALUE(
        switches::kOverscrollHistoryNavigation, "1",
        switches::kOverscrollHistoryNavigation, "0")
  },
#endif
  {
    "scroll-end-effect",
    IDS_FLAGS_SCROLL_END_EFFECT_NAME,
    IDS_FLAGS_SCROLL_END_EFFECT_DESCRIPTION,
    kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE_AND_VALUE(
        switches::kScrollEndEffect, "1",
        switches::kScrollEndEffect, "0")
  },
  {
    "enable-touch-side-bezels",
    IDS_FLAGS_ENABLE_TOUCH_SIDE_BEZELS_NAME,
    IDS_FLAGS_ENABLE_TOUCH_SIDE_BEZELS_DESCRIPTION,
    kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE_AND_VALUE(
        switches::kTouchSideBezels, "1",
        switches::kTouchSideBezels, "0")
  },
  {
    "enable-touch-drag-drop",
    IDS_FLAGS_ENABLE_TOUCH_DRAG_DROP_NAME,
    IDS_FLAGS_ENABLE_TOUCH_DRAG_DROP_DESCRIPTION,
    kOsWin | kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableTouchDragDrop,
                              switches::kDisableTouchDragDrop)
  },
  {
    "enable-touch-editing",
    IDS_FLAGS_ENABLE_TOUCH_EDITING_NAME,
    IDS_FLAGS_ENABLE_TOUCH_EDITING_DESCRIPTION,
    kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableTouchEditing,
                              switches::kDisableTouchEditing)
  },
  {
    "enable-rich-notifications",
    IDS_FLAGS_ENABLE_RICH_NOTIFICATIONS_NAME,
    IDS_FLAGS_ENABLE_RICH_NOTIFICATIONS_DESCRIPTION,
    kOsWin | kOsMac,
    ENABLE_DISABLE_VALUE_TYPE(
        message_center::switches::kEnableRichNotifications,
        message_center::switches::kDisableRichNotifications)
  },
  {
    "enable-sync-synced-notifications",
    IDS_FLAGS_ENABLE_SYNCED_NOTIFICATIONS_NAME,
    IDS_FLAGS_ENABLE_SYNCED_NOTIFICATIONS_DESCRIPTION,
    kOsDesktop,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableSyncSyncedNotifications,
                              switches::kDisableSyncSyncedNotifications)
  },
  {
    "disable-full-history-sync",
    IDS_FLAGS_FULL_HISTORY_SYNC_NAME,
    IDS_FLAGS_FULL_HISTORY_SYNC_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kHistoryDisableFullHistorySync)
  },
  {
    "enable-usermedia-screen-capture",
    IDS_FLAGS_ENABLE_SCREEN_CAPTURE_NAME,
    IDS_FLAGS_ENABLE_SCREEN_CAPTURE_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kEnableUserMediaScreenCapturing)
  },
  {
    "enable-apps-devtool-app",
    IDS_FLAGS_ENABLE_APPS_DEVTOOL_APP_NAME,
    IDS_FLAGS_ENABLE_APPS_DEVTOOL_APP_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kAppsDevtool)
  },
  {
    "impl-side-painting",
    IDS_FLAGS_IMPL_SIDE_PAINTING_NAME,
    IDS_FLAGS_IMPL_SIDE_PAINTING_DESCRIPTION,
    kOsAndroid | kOsLinux | kOsCrOS,
    MULTI_VALUE_TYPE(kImplSidePaintingChoices)
  },
  {
    "deadline-scheduling",
    IDS_FLAGS_DEADLINE_SCHEDULING_NAME,
    IDS_FLAGS_DEADLINE_SCHEDULING_DESCRIPTION,
    kOsMac | kOsWin | kOsLinux | kOsCrOS | kOsAndroid,
    MULTI_VALUE_TYPE(kDeadlineSchedulingChoices)
  },
  {
    "ui-deadline-scheduling",
    IDS_FLAGS_UI_DEADLINE_SCHEDULING_NAME,
    IDS_FLAGS_UI_DEADLINE_SCHEDULING_DESCRIPTION,
#ifdef USE_AURA
    kOsWin | kOsLinux |
#endif
    kOsCrOS,
    MULTI_VALUE_TYPE(kUIDeadlineSchedulingChoices)
  },
  {
    "lcd-text-aa",
    IDS_FLAGS_LCD_TEXT_NAME,
    IDS_FLAGS_LCD_TEXT_DESCRIPTION,
    kOsDesktop,
    MULTI_VALUE_TYPE(kLCDTextChoices)
  },
  {
    "delegated-renderer",
    IDS_FLAGS_DELEGATED_RENDERER_NAME,
    IDS_FLAGS_DELEGATED_RENDERER_DESCRIPTION,
#ifdef USE_AURA
    kOsWin | kOsLinux |
#endif
    kOsAndroid | kOsCrOS,
    MULTI_VALUE_TYPE(kDelegatedRendererChoices)
  },
  {
    "enable-websocket-experimental-implementation",
    IDS_FLAGS_ENABLE_EXPERIMENTAL_WEBSOCKET_NAME,
    IDS_FLAGS_ENABLE_EXPERIMENTAL_WEBSOCKET_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableExperimentalWebSocket)
  },
  {
    "max-tiles-for-interest-area",
    IDS_FLAGS_MAX_TILES_FOR_INTEREST_AREA_NAME,
    IDS_FLAGS_MAX_TILES_FOR_INTEREST_AREA_DESCRIPTION,
    kOsAndroid | kOsLinux | kOsCrOS,
    MULTI_VALUE_TYPE(kMaxTilesForInterestAreaChoices)
  },
  {
    "enable-offline-mode",
    IDS_FLAGS_ENABLE_OFFLINE_MODE_NAME,
    IDS_FLAGS_ENABLE_OFFLINE_MODE_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableOfflineCacheAccess)
  },
  {
    "default-tile-width",
    IDS_FLAGS_DEFAULT_TILE_WIDTH_NAME,
    IDS_FLAGS_DEFAULT_TILE_WIDTH_DESCRIPTION,
    kOsAll,
    MULTI_VALUE_TYPE(kDefaultTileWidthChoices)
  },
  {
    "default-tile-height",
    IDS_FLAGS_DEFAULT_TILE_HEIGHT_NAME,
    IDS_FLAGS_DEFAULT_TILE_HEIGHT_DESCRIPTION,
    kOsAll,
    MULTI_VALUE_TYPE(kDefaultTileHeightChoices)
  },
  // TODO(sky): ifdef needed until focus sorted out in DesktopNativeWidgetAura.
#if !defined(USE_AURA)
  {
    "track-active-visit-time",
    IDS_FLAGS_TRACK_ACTIVE_VISIT_TIME_NAME,
    IDS_FLAGS_TRACK_ACTIVE_VISIT_TIME_DESCRIPTION,
    kOsWin,
    SINGLE_VALUE_TYPE(switches::kTrackActiveVisitTime)
  },
#endif
#if defined(OS_ANDROID)
  {
    "disable-gesture-requirement-for-media-playback",
    IDS_FLAGS_DISABLE_GESTURE_REQUIREMENT_FOR_MEDIA_PLAYBACK_NAME,
    IDS_FLAGS_DISABLE_GESTURE_REQUIREMENT_FOR_MEDIA_PLAYBACK_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kDisableGestureRequirementForMediaPlayback)
  },
#endif
#if defined(ENABLE_GOOGLE_NOW)
  {
    "enable-google-now",
    IDS_FLAGS_ENABLE_GOOGLE_NOW_INTEGRATION_NAME,
    IDS_FLAGS_ENABLE_GOOGLE_NOW_INTEGRATION_DESCRIPTION,
    kOsWin | kOsCrOS | kOsMac,
    ENABLE_DISABLE_VALUE_TYPE(
        switches::kEnableGoogleNowIntegration,
        switches::kDisableGoogleNowIntegration)
  },
#endif
#if defined(OS_CHROMEOS)
  {
    "enable-virtual-keyboard",
    IDS_FLAGS_ENABLE_VIRTUAL_KEYBOARD_NAME,
    IDS_FLAGS_ENABLE_VIRTUAL_KEYBOARD_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(keyboard::switches::kEnableVirtualKeyboard)
  },
  {
    "enable-swipe-selection",
    IDS_FLAGS_ENABLE_SWIPE_SELECTION_NAME,
    IDS_FLAGS_ENABLE_SWIPE_SELECTION_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(keyboard::switches::kEnableSwipeSelection)
  },
#endif
  {
    "enable-simple-cache-backend",
    IDS_FLAGS_ENABLE_SIMPLE_CACHE_BACKEND_NAME,
    IDS_FLAGS_ENABLE_SIMPLE_CACHE_BACKEND_DESCRIPTION,
    kOsAll,
    MULTI_VALUE_TYPE(kSimpleCacheBackendChoices)
  },
  {
    "enable-tcp-fast-open",
    IDS_FLAGS_ENABLE_TCP_FAST_OPEN_NAME,
    IDS_FLAGS_ENABLE_TCP_FAST_OPEN_DESCRIPTION,
    kOsLinux | kOsCrOS | kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kEnableTcpFastOpen)
  },
  {
    "apps-use-native-frame",
    IDS_FLAGS_ENABLE_NATIVE_FRAMES_FOR_APPS_NAME,
    IDS_FLAGS_ENABLE_NATIVE_FRAMES_FOR_APPS_DESCRIPTION,
    kOsMac | kOsWin,
    SINGLE_VALUE_TYPE(switches::kAppsUseNativeFrame)
  },
  {
    "enable-syncfs-directory-operation",
    IDS_FLAGS_ENABLE_SYNC_DIRECTORY_OPERATION_NAME,
    IDS_FLAGS_ENABLE_SYNC_DIRECTORY_OPERATION_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kSyncfsEnableDirectoryOperation),
  },
#if defined(ENABLE_MDNS)
  {
    "disable-device-discovery",
    IDS_FLAGS_DISABLE_DEVICE_DISCOVERY_NAME,
    IDS_FLAGS_DISABLE_DEVICE_DISCOVERY_DESCRIPTION,
    kOsWin | kOsLinux | kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kDisableDeviceDiscovery)
  },
  {
    "device-discovery-notifications",
    IDS_FLAGS_DEVICE_DISCOVERY_NOTIFICATIONS_NAME,
    IDS_FLAGS_DEVICE_DISCOVERY_NOTIFICATIONS_DESCRIPTION,
    kOsWin | kOsLinux | kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableDeviceDiscoveryNotifications,
                              switches::kDisableDeviceDiscoveryNotifications)
  },
  {
    "enable-privet-local-printing",
    IDS_FLAGS_ENABLE_PRIVET_LOCAL_PRINTING_NAME,
    IDS_FLAGS_ENABLE_PRIVET_LOCAL_PRINTING_DESCRIPTION,
    kOsWin | kOsLinux | kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kEnablePrivetLocalPrinting)
  },
#endif  // ENABLE_MDNS
#if defined(OS_MACOSX)
  {
    "enable-app-shims",
    IDS_FLAGS_ENABLE_APP_SHIMS_NAME,
    IDS_FLAGS_ENABLE_APP_SHIMS_DESCRIPTION,
    kOsMac,
    SINGLE_VALUE_TYPE(switches::kEnableAppShims)
  },
  {
    "enable-simplified-fullscreen",
    IDS_FLAGS_ENABLE_SIMPLIFIED_FULLSCREEN_NAME,
    IDS_FLAGS_ENABLE_SIMPLIFIED_FULLSCREEN_DESCRIPTION,
    kOsMac,
    SINGLE_VALUE_TYPE(switches::kEnableSimplifiedFullscreen)
  },
#endif
#if defined(OS_CHROMEOS) || defined(OS_WIN)
  {
    "omnibox-auto-completion-for-ime",
    IDS_FLAGS_ENABLE_OMNIBOX_AUTO_COMPLETION_FOR_IME_NAME,
    IDS_FLAGS_ENABLE_OMNIBOX_AUTO_COMPLETION_FOR_IME_DESCRIPTION,
    kOsCrOS | kOsWin,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableOmniboxAutoCompletionForIme,
                              switches::kDisableOmniboxAutoCompletionForIme)
  },
#endif
#if defined(USE_AURA)
  {
    "tab-capture-upscale-quality",
    IDS_FLAGS_TAB_CAPTURE_UPSCALE_QUALITY_NAME,
    IDS_FLAGS_TAB_CAPTURE_UPSCALE_QUALITY_DESCRIPTION,
    kOsAll,
    MULTI_VALUE_TYPE(kTabCaptureUpscaleQualityChoices)
  },
  {
    "tab-capture-downscale-quality",
    IDS_FLAGS_TAB_CAPTURE_DOWNSCALE_QUALITY_NAME,
    IDS_FLAGS_TAB_CAPTURE_DOWNSCALE_QUALITY_DESCRIPTION,
    kOsAll,
    MULTI_VALUE_TYPE(kTabCaptureDownscaleQualityChoices)
  },
#endif
  {
    "enable-spelling-service-feedback",
    IDS_FLAGS_ENABLE_SPELLING_SERVICE_FEEDBACK_NAME,
    IDS_FLAGS_ENABLE_SPELLING_SERVICE_FEEDBACK_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableSpellingServiceFeedback)
  },
  {
    "enable-webgl-draft-extensions",
    IDS_FLAGS_ENABLE_WEBGL_DRAFT_EXTENSIONS_NAME,
    IDS_FLAGS_ENABLE_WEBGL_DRAFT_EXTENSIONS_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableWebGLDraftExtensions)
  },
  {
    "enable-html-imports",
    IDS_FLAGS_ENABLE_HTML_IMPORTS_NAME,
    IDS_FLAGS_ENABLE_HTML_IMPORTS_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableHTMLImports)
  },
  {
    "high-dpi-support",
    IDS_FLAGS_HIDPI_NAME,
    IDS_FLAGS_HIDPI_DESCRIPTION,
    kOsWin,
    ENABLE_DISABLE_VALUE_TYPE_AND_VALUE(switches::kHighDPISupport, "1",
                                        switches::kHighDPISupport, "0")
  },
#if defined(OS_CHROMEOS)
  {
    "enable-quickoffice-viewing",
    IDS_FLAGS_ENABLE_QUICKOFFICE_DESKTOP_VIEWING_NAME,
    IDS_FLAGS_ENABLE_QUICKOFFICE_DESKTOP_VIEWING_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kEnableQuickofficeViewing),
  },
  {
    "enable-sticky-keys",
    IDS_FLAGS_ENABLE_STICKY_KEYS_NAME,
    IDS_FLAGS_ENABLE_STICKY_KEYS_DESCRIPTION,
    kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableStickyKeys,
                              switches::kDisableStickyKeys)
  },
#endif
  {
    "enable-web-midi",
    IDS_FLAGS_ENABLE_WEB_MIDI_NAME,
    IDS_FLAGS_ENABLE_WEB_MIDI_DESCRIPTION,
    kOsMac,
    SINGLE_VALUE_TYPE(switches::kEnableWebMIDI)
  },
  {
    "enable-new-profile-management",
    IDS_FLAGS_ENABLE_NEW_PROFILE_MANAGEMENT_NAME,
    IDS_FLAGS_ENABLE_NEW_PROFILE_MANAGEMENT_DESCRIPTION,
    kOsWin,
    SINGLE_VALUE_TYPE(switches::kNewProfileManagement)
  },
  {
    "enable-inline-signin",
    IDS_FLAGS_ENABLE_INLINE_SIGNIN_NAME,
    IDS_FLAGS_ENABLE_INLINE_SIGNIN_DESCRIPTION,
    kOsMac | kOsWin | kOsLinux,
    SINGLE_VALUE_TYPE(switches::kEnableInlineSignin)
  },
  {
    "enable-google-profile-info",
    IDS_FLAGS_ENABLE_GOOGLE_PROFILE_INFO_NAME,
    IDS_FLAGS_ENABLE_GOOGLE_PROFILE_INFO_DESCRIPTION,
    kOsMac | kOsWin | kOsLinux,
    SINGLE_VALUE_TYPE(switches::kGoogleProfileInfo)
  },
  {
    "disable-app-launcher",
    IDS_FLAGS_RESET_APP_LIST_INSTALL_STATE_NAME,
    IDS_FLAGS_RESET_APP_LIST_INSTALL_STATE_DESCRIPTION,
    kOsMac | kOsWin,
    SINGLE_VALUE_TYPE(switches::kResetAppListInstallState)
  },
#if defined(ENABLE_APP_LIST)
  {
    "enable-app-launcher-start-page",
    IDS_FLAGS_ENABLE_APP_LIST_START_PAGE_NAME,
    IDS_FLAGS_ENABLE_APP_LIST_START_PAGE_DESCRIPTION,
    kOsWin | kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kShowAppListStartPage)
  },
#endif
#if defined(OS_CHROMEOS)
  {
    "disable-user-image-sync",
    IDS_FLAGS_DISABLE_USER_IMAGE_SYNC_NAME,
    IDS_FLAGS_DISABLE_USER_IMAGE_SYNC_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kDisableUserImageSync)
  },
#endif
#if defined(OS_ANDROID)
  {
    "enable-accessibility-tab-switcher",
    IDS_FLAGS_ENABLE_ACCESSIBILITY_TAB_SWITCHER_NAME,
    IDS_FLAGS_ENABLE_ACCESSIBILITY_TAB_SWITCHER_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kEnableAccessibilityTabSwitcher)
  },
#endif
  {
    "disable-batched-shutdown",
    IDS_FLAGS_DISABLE_BATCHED_SHUTDOWN_NAME,
    IDS_FLAGS_DISABLE_BATCHED_SHUTDOWN_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kDisableBatchedShutdown)
  },
  {
    "map-image",
    IDS_FLAGS_MAP_IMAGE_NAME,
    IDS_FLAGS_MAP_IMAGE_DESCRIPTION,
    kOsAll,
    MULTI_VALUE_TYPE(kMapImageChoices)
  },
  {
    "enable-add-to-homescreen",
    IDS_FLAGS_ENABLE_ADD_TO_HOMESCREEN_NAME,
    IDS_FLAGS_ENABLE_ADD_TO_HOMESCREEN_DESCRIPTION,
    kOsAndroid,
#if defined(OS_ANDROID)
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableAddToHomescreen,
                              switches::kDisableAddToHomescreen),
#else
    SINGLE_VALUE_TYPE("")
#endif
  },
#if defined(OS_CHROMEOS)
  {
    "enable-first-run-ui",
    IDS_FLAGS_ENABLE_FIRST_RUN_UI_NAME,
    IDS_FLAGS_ENABLE_FIRST_RUN_UI_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kEnableFirstRunUI)
  },
#endif
  {
    "disable-compositor-touch-hit-testing",
    IDS_FLAGS_DISABLE_COMPOSITOR_TOUCH_HIT_TESTING_NAME,
    IDS_FLAGS_DISABLE_COMPOSITOR_TOUCH_HIT_TESTING_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(cc::switches::kDisableCompositorTouchHitTesting),
  },
  {
    "enable-accelerated-scrollable-frames",
     IDS_FLAGS_ENABLE_ACCELERATED_SCROLLABLE_FRAMES_NAME,
     IDS_FLAGS_ENABLE_ACCELERATED_SCROLLABLE_FRAMES_DESCRIPTION,
     kOsAll,
     ENABLE_DISABLE_VALUE_TYPE(switches::kEnableAcceleratedScrollableFrames,
                               switches::kDisableAcceleratedScrollableFrames)
  },
  {
    "enable-composited-scrolling-for-frames",
     IDS_FLAGS_ENABLE_COMPOSITED_SCROLLING_FOR_FRAMES_NAME,
     IDS_FLAGS_ENABLE_COMPOSITED_SCROLLING_FOR_FRAMES_DESCRIPTION,
     kOsAll,
     ENABLE_DISABLE_VALUE_TYPE(switches::kEnableCompositedScrollingForFrames,
                               switches::kDisableCompositedScrollingForFrames)
  },
  {
    "enable-streamlined-hosted-apps",
    IDS_FLAGS_ENABLE_STREAMLINED_HOSTED_APPS_NAME,
    IDS_FLAGS_ENABLE_STREAMLINED_HOSTED_APPS_DESCRIPTION,
    kOsWin | kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kEnableStreamlinedHostedApps)
  },
  {
    "enable-ephemeral-apps",
    IDS_FLAGS_ENABLE_EPHEMERAL_APPS_NAME,
    IDS_FLAGS_ENABLE_EPHEMERAL_APPS_DESCRIPTION,
    kOsWin | kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kEnableEphemeralApps)
  },
  {
    "enable-service-worker",
    IDS_FLAGS_ENABLE_SERVICE_WORKER_NAME,
    IDS_FLAGS_ENABLE_SERVICE_WORKER_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableServiceWorker)
  },
#if defined(OS_ANDROID)
  {
    "disable-click-delay",
    IDS_FLAGS_DISABLE_CLICK_DELAY_NAME,
    IDS_FLAGS_DISABLE_CLICK_DELAY_DESCRIPTION,
    kOsAndroid,
    // Java-only switch: CommandLine.DISABLE_CLICK_DELAY
    SINGLE_VALUE_TYPE("disable-click-delay")
  },
#endif
#if defined(OS_CHROMEOS)
  {
    "enable-ime-mode-indicator",
    IDS_FLAGS_ENABLE_IME_MODE_INDICATOR,
    IDS_FLAGS_ENABLE_IME_MODE_INDICATOR_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kEnableIMEModeIndicator)
  },
#endif
  {
    "enable-translate-new-ux",
    IDS_FLAGS_ENABLE_TRANSLATE_NEW_UX_NAME,
    IDS_FLAGS_ENABLE_TRANSLATE_NEW_UX_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kEnableTranslateNewUX)
  },
};

const Experiment* experiments = kExperiments;
size_t num_experiments = arraysize(kExperiments);

// Stores and encapsulates the little state that about:flags has.
class FlagsState {
 public:
  FlagsState() : needs_restart_(false) {}
  void ConvertFlagsToSwitches(FlagsStorage* flags_storage,
                              CommandLine* command_line,
                              SentinelsMode sentinels);
  bool IsRestartNeededToCommitChanges();
  void SetExperimentEnabled(
      FlagsStorage* flags_storage,
      const std::string& internal_name,
      bool enable);
  void RemoveFlagsSwitches(
      std::map<std::string, CommandLine::StringType>* switch_list);
  void ResetAllFlags(FlagsStorage* flags_storage);
  void reset();

  // Returns the singleton instance of this class
  static FlagsState* GetInstance() {
    return Singleton<FlagsState>::get();
  }

 private:
  bool needs_restart_;
  std::map<std::string, std::string> flags_switches_;

  DISALLOW_COPY_AND_ASSIGN(FlagsState);
};

// Adds the internal names for the specified experiment to |names|.
void AddInternalName(const Experiment& e, std::set<std::string>* names) {
  if (e.type == Experiment::SINGLE_VALUE) {
    names->insert(e.internal_name);
  } else {
    DCHECK(e.type == Experiment::MULTI_VALUE ||
           e.type == Experiment::ENABLE_DISABLE_VALUE);
    for (int i = 0; i < e.num_choices; ++i)
      names->insert(e.NameForChoice(i));
  }
}

// Confirms that an experiment is valid, used in a DCHECK in
// SanitizeList below.
bool ValidateExperiment(const Experiment& e) {
  switch (e.type) {
    case Experiment::SINGLE_VALUE:
      DCHECK_EQ(0, e.num_choices);
      DCHECK(!e.choices);
      break;
    case Experiment::MULTI_VALUE:
      DCHECK_GT(e.num_choices, 0);
      DCHECK(e.choices);
      DCHECK(e.choices[0].command_line_switch);
      DCHECK_EQ('\0', e.choices[0].command_line_switch[0]);
      break;
    case Experiment::ENABLE_DISABLE_VALUE:
      DCHECK_EQ(3, e.num_choices);
      DCHECK(!e.choices);
      DCHECK(e.command_line_switch);
      DCHECK(e.command_line_value);
      DCHECK(e.disable_command_line_switch);
      DCHECK(e.disable_command_line_value);
      break;
    default:
      NOTREACHED();
  }
  return true;
}

// Removes all experiments from prefs::kEnabledLabsExperiments that are
// unknown, to prevent this list to become very long as experiments are added
// and removed.
void SanitizeList(FlagsStorage* flags_storage) {
  std::set<std::string> known_experiments;
  for (size_t i = 0; i < num_experiments; ++i) {
    DCHECK(ValidateExperiment(experiments[i]));
    AddInternalName(experiments[i], &known_experiments);
  }

  std::set<std::string> enabled_experiments = flags_storage->GetFlags();

  std::set<std::string> new_enabled_experiments;
  std::set_intersection(
      known_experiments.begin(), known_experiments.end(),
      enabled_experiments.begin(), enabled_experiments.end(),
      std::inserter(new_enabled_experiments, new_enabled_experiments.begin()));

  if (new_enabled_experiments != enabled_experiments)
    flags_storage->SetFlags(new_enabled_experiments);
}

void GetSanitizedEnabledFlags(
    FlagsStorage* flags_storage, std::set<std::string>* result) {
  SanitizeList(flags_storage);
  *result = flags_storage->GetFlags();
}

// Variant of GetSanitizedEnabledFlags that also removes any flags that aren't
// enabled on the current platform.
void GetSanitizedEnabledFlagsForCurrentPlatform(
    FlagsStorage* flags_storage, std::set<std::string>* result) {
  GetSanitizedEnabledFlags(flags_storage, result);

  // Filter out any experiments that aren't enabled on the current platform.  We
  // don't remove these from prefs else syncing to a platform with a different
  // set of experiments would be lossy.
  std::set<std::string> platform_experiments;
  int current_platform = GetCurrentPlatform();
  for (size_t i = 0; i < num_experiments; ++i) {
    if (experiments[i].supported_platforms & current_platform)
      AddInternalName(experiments[i], &platform_experiments);
#if defined(OS_CHROMEOS)
    if (experiments[i].supported_platforms & kOsCrOSOwnerOnly)
      AddInternalName(experiments[i], &platform_experiments);
#endif
  }

  std::set<std::string> new_enabled_experiments;
  std::set_intersection(
      platform_experiments.begin(), platform_experiments.end(),
      result->begin(), result->end(),
      std::inserter(new_enabled_experiments, new_enabled_experiments.begin()));

  result->swap(new_enabled_experiments);
}

// Returns the Value representing the choice data in the specified experiment.
Value* CreateChoiceData(const Experiment& experiment,
                        const std::set<std::string>& enabled_experiments) {
  DCHECK(experiment.type == Experiment::MULTI_VALUE ||
         experiment.type == Experiment::ENABLE_DISABLE_VALUE);
  ListValue* result = new ListValue;
  for (int i = 0; i < experiment.num_choices; ++i) {
    DictionaryValue* value = new DictionaryValue;
    const std::string name = experiment.NameForChoice(i);
    value->SetString("internal_name", name);
    value->SetString("description", experiment.DescriptionForChoice(i));
    value->SetBoolean("selected", enabled_experiments.count(name) > 0);
    result->Append(value);
  }
  return result;
}

}  // namespace

std::string Experiment::NameForChoice(int index) const {
  DCHECK(type == Experiment::MULTI_VALUE ||
         type == Experiment::ENABLE_DISABLE_VALUE);
  DCHECK_LT(index, num_choices);
  return std::string(internal_name) + testing::kMultiSeparator +
         base::IntToString(index);
}

string16 Experiment::DescriptionForChoice(int index) const {
  DCHECK(type == Experiment::MULTI_VALUE ||
         type == Experiment::ENABLE_DISABLE_VALUE);
  DCHECK_LT(index, num_choices);
  int description_id;
  if (type == Experiment::ENABLE_DISABLE_VALUE) {
    const int kEnableDisableDescriptionIds[] = {
      IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT,
      IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED,
      IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    };
    description_id = kEnableDisableDescriptionIds[index];
  } else {
    description_id = choices[index].description_id;
  }
  return l10n_util::GetStringUTF16(description_id);
}

void ConvertFlagsToSwitches(FlagsStorage* flags_storage,
                            CommandLine* command_line,
                            SentinelsMode sentinels) {
  FlagsState::GetInstance()->ConvertFlagsToSwitches(flags_storage,
                                                    command_line,
                                                    sentinels);
}

bool AreSwitchesIdenticalToCurrentCommandLine(
    const CommandLine& new_cmdline, const CommandLine& active_cmdline) {
  std::set<CommandLine::StringType> new_flags =
      ExtractFlagsFromCommandLine(new_cmdline);
  std::set<CommandLine::StringType> active_flags =
      ExtractFlagsFromCommandLine(active_cmdline);

  // Needed because std::equal doesn't check if the 2nd set is empty.
  if (new_flags.size() != active_flags.size())
    return false;

  return std::equal(new_flags.begin(), new_flags.end(), active_flags.begin());
}

void GetFlagsExperimentsData(FlagsStorage* flags_storage,
                             FlagAccess access,
                             base::ListValue* supported_experiments,
                             base::ListValue* unsupported_experiments) {
  std::set<std::string> enabled_experiments;
  GetSanitizedEnabledFlags(flags_storage, &enabled_experiments);

  int current_platform = GetCurrentPlatform();

  for (size_t i = 0; i < num_experiments; ++i) {
    const Experiment& experiment = experiments[i];

    DictionaryValue* data = new DictionaryValue();
    data->SetString("internal_name", experiment.internal_name);
    data->SetString("name",
                    l10n_util::GetStringUTF16(experiment.visible_name_id));
    data->SetString("description",
                    l10n_util::GetStringUTF16(
                        experiment.visible_description_id));

    ListValue* supported_platforms = new ListValue();
    AddOsStrings(experiment.supported_platforms, supported_platforms);
    data->Set("supported_platforms", supported_platforms);

    switch (experiment.type) {
      case Experiment::SINGLE_VALUE:
        data->SetBoolean(
            "enabled",
            enabled_experiments.count(experiment.internal_name) > 0);
        break;
      case Experiment::MULTI_VALUE:
      case Experiment::ENABLE_DISABLE_VALUE:
        data->Set("choices", CreateChoiceData(experiment, enabled_experiments));
        break;
      default:
        NOTREACHED();
    }

    bool supported = (experiment.supported_platforms & current_platform) != 0;
#if defined(OS_CHROMEOS)
    if (access == kOwnerAccessToFlags &&
        (experiment.supported_platforms & kOsCrOSOwnerOnly) != 0) {
      supported = true;
    }
#endif
    if (supported)
      supported_experiments->Append(data);
    else
      unsupported_experiments->Append(data);
  }
}

bool IsRestartNeededToCommitChanges() {
  return FlagsState::GetInstance()->IsRestartNeededToCommitChanges();
}

void SetExperimentEnabled(FlagsStorage* flags_storage,
                          const std::string& internal_name,
                          bool enable) {
  FlagsState::GetInstance()->SetExperimentEnabled(flags_storage,
                                                  internal_name, enable);
}

void RemoveFlagsSwitches(
    std::map<std::string, CommandLine::StringType>* switch_list) {
  FlagsState::GetInstance()->RemoveFlagsSwitches(switch_list);
}

void ResetAllFlags(FlagsStorage* flags_storage) {
  FlagsState::GetInstance()->ResetAllFlags(flags_storage);
}

int GetCurrentPlatform() {
#if defined(OS_MACOSX)
  return kOsMac;
#elif defined(OS_WIN)
  return kOsWin;
#elif defined(OS_CHROMEOS)  // Needs to be before the OS_LINUX check.
  return kOsCrOS;
#elif defined(OS_LINUX) || defined(OS_OPENBSD)
  return kOsLinux;
#elif defined(OS_ANDROID)
  return kOsAndroid;
#else
#error Unknown platform
#endif
}

void RecordUMAStatistics(FlagsStorage* flags_storage) {
  std::set<std::string> flags = flags_storage->GetFlags();
  for (std::set<std::string>::iterator it = flags.begin(); it != flags.end();
       ++it) {
    std::string action("AboutFlags_");
    action += *it;
    content::RecordComputedAction(action);
  }
  // Since flag metrics are recorded every startup, add a tick so that the
  // stats can be made meaningful.
  if (flags.size())
    content::RecordAction(UserMetricsAction("AboutFlags_StartupTick"));
  content::RecordAction(UserMetricsAction("StartupTick"));
}

//////////////////////////////////////////////////////////////////////////////
// FlagsState implementation.

namespace {

typedef std::map<std::string, std::pair<std::string, std::string> >
    NameToSwitchAndValueMap;

void SetFlagToSwitchMapping(const std::string& key,
                            const std::string& switch_name,
                            const std::string& switch_value,
                            NameToSwitchAndValueMap* name_to_switch_map) {
  DCHECK(name_to_switch_map->end() == name_to_switch_map->find(key));
  (*name_to_switch_map)[key] = std::make_pair(switch_name, switch_value);
}

void FlagsState::ConvertFlagsToSwitches(FlagsStorage* flags_storage,
                                        CommandLine* command_line,
                                        SentinelsMode sentinels) {
  if (command_line->HasSwitch(switches::kNoExperiments))
    return;

  std::set<std::string> enabled_experiments;

  GetSanitizedEnabledFlagsForCurrentPlatform(flags_storage,
                                             &enabled_experiments);

  NameToSwitchAndValueMap name_to_switch_map;
  for (size_t i = 0; i < num_experiments; ++i) {
    const Experiment& e = experiments[i];
    if (e.type == Experiment::SINGLE_VALUE) {
      SetFlagToSwitchMapping(e.internal_name, e.command_line_switch,
                             e.command_line_value, &name_to_switch_map);
    } else if (e.type == Experiment::MULTI_VALUE) {
      for (int j = 0; j < e.num_choices; ++j) {
        SetFlagToSwitchMapping(e.NameForChoice(j),
                               e.choices[j].command_line_switch,
                               e.choices[j].command_line_value,
                               &name_to_switch_map);
      }
    } else {
      DCHECK_EQ(e.type, Experiment::ENABLE_DISABLE_VALUE);
      SetFlagToSwitchMapping(e.NameForChoice(0), std::string(), std::string(),
                             &name_to_switch_map);
      SetFlagToSwitchMapping(e.NameForChoice(1), e.command_line_switch,
                             e.command_line_value, &name_to_switch_map);
      SetFlagToSwitchMapping(e.NameForChoice(2), e.disable_command_line_switch,
                             e.disable_command_line_value, &name_to_switch_map);
    }
  }

  if (sentinels == kAddSentinels) {
    command_line->AppendSwitch(switches::kFlagSwitchesBegin);
    flags_switches_.insert(
        std::pair<std::string, std::string>(switches::kFlagSwitchesBegin,
                                            std::string()));
  }
  for (std::set<std::string>::iterator it = enabled_experiments.begin();
       it != enabled_experiments.end();
       ++it) {
    const std::string& experiment_name = *it;
    NameToSwitchAndValueMap::const_iterator name_to_switch_it =
        name_to_switch_map.find(experiment_name);
    if (name_to_switch_it == name_to_switch_map.end()) {
      NOTREACHED();
      continue;
    }

    const std::pair<std::string, std::string>&
        switch_and_value_pair = name_to_switch_it->second;

    CHECK(!switch_and_value_pair.first.empty());
    command_line->AppendSwitchASCII(switch_and_value_pair.first,
                                    switch_and_value_pair.second);
    flags_switches_[switch_and_value_pair.first] = switch_and_value_pair.second;
  }
  if (sentinels == kAddSentinels) {
    command_line->AppendSwitch(switches::kFlagSwitchesEnd);
    flags_switches_.insert(
        std::pair<std::string, std::string>(switches::kFlagSwitchesEnd,
                                            std::string()));
  }
}

bool FlagsState::IsRestartNeededToCommitChanges() {
  return needs_restart_;
}

void FlagsState::SetExperimentEnabled(FlagsStorage* flags_storage,
                                      const std::string& internal_name,
                                      bool enable) {
  size_t at_index = internal_name.find(testing::kMultiSeparator);
  if (at_index != std::string::npos) {
    DCHECK(enable);
    // We're being asked to enable a multi-choice experiment. Disable the
    // currently selected choice.
    DCHECK_NE(at_index, 0u);
    const std::string experiment_name = internal_name.substr(0, at_index);
    SetExperimentEnabled(flags_storage, experiment_name, false);

    // And enable the new choice, if it is not the default first choice.
    if (internal_name != experiment_name + "@0") {
      std::set<std::string> enabled_experiments;
      GetSanitizedEnabledFlags(flags_storage, &enabled_experiments);
      needs_restart_ |= enabled_experiments.insert(internal_name).second;
      flags_storage->SetFlags(enabled_experiments);
    }
    return;
  }

  std::set<std::string> enabled_experiments;
  GetSanitizedEnabledFlags(flags_storage, &enabled_experiments);

  const Experiment* e = NULL;
  for (size_t i = 0; i < num_experiments; ++i) {
    if (experiments[i].internal_name == internal_name) {
      e = experiments + i;
      break;
    }
  }
  DCHECK(e);

  if (e->type == Experiment::SINGLE_VALUE) {
    if (enable)
      needs_restart_ |= enabled_experiments.insert(internal_name).second;
    else
      needs_restart_ |= (enabled_experiments.erase(internal_name) > 0);
  } else {
    if (enable) {
      // Enable the first choice.
      needs_restart_ |= enabled_experiments.insert(e->NameForChoice(0)).second;
    } else {
      // Find the currently enabled choice and disable it.
      for (int i = 0; i < e->num_choices; ++i) {
        std::string choice_name = e->NameForChoice(i);
        if (enabled_experiments.find(choice_name) !=
            enabled_experiments.end()) {
          needs_restart_ = true;
          enabled_experiments.erase(choice_name);
          // Continue on just in case there's a bug and more than one
          // experiment for this choice was enabled.
        }
      }
    }
  }

  flags_storage->SetFlags(enabled_experiments);
}

void FlagsState::RemoveFlagsSwitches(
    std::map<std::string, CommandLine::StringType>* switch_list) {
  for (std::map<std::string, std::string>::const_iterator
           it = flags_switches_.begin(); it != flags_switches_.end(); ++it) {
    switch_list->erase(it->first);
  }
}

void FlagsState::ResetAllFlags(FlagsStorage* flags_storage) {
  needs_restart_ = true;

  std::set<std::string> no_experiments;
  flags_storage->SetFlags(no_experiments);
}

void FlagsState::reset() {
  needs_restart_ = false;
  flags_switches_.clear();
}

}  // namespace

namespace testing {

// WARNING: '@' is also used in the html file. If you update this constant you
// also need to update the html file.
const char kMultiSeparator[] = "@";

void ClearState() {
  FlagsState::GetInstance()->reset();
}

void SetExperiments(const Experiment* e, size_t count) {
  if (!e) {
    experiments = kExperiments;
    num_experiments = arraysize(kExperiments);
  } else {
    experiments = e;
    num_experiments = count;
  }
}

const Experiment* GetExperiments(size_t* count) {
  *count = num_experiments;
  return experiments;
}

}  // namespace testing

}  // namespace about_flags
