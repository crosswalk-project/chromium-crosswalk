// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/about_flags.h"

#include <iterator>
#include <map>
#include <set>
#include <utility>

#include "base/command_line.h"
#include "base/memory/singleton.h"
#include "base/metrics/sparse_histogram.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "cc/base/switches.h"
#include "chrome/browser/flags_storage.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/google_chrome_strings.h"
#include "components/autofill/core/common/autofill_switches.h"
#include "components/cloud_devices/common/cloud_devices_switches.h"
#include "components/dom_distiller/core/dom_distiller_switches.h"
#include "components/metrics/metrics_hashes.h"
#include "components/nacl/common/nacl_switches.h"
#include "components/omnibox/omnibox_switches.h"
#include "components/plugins/common/plugins_switches.h"
#include "components/proximity_auth/switches.h"
#include "components/search/search_switches.h"
#include "content/public/browser/user_metrics.h"
#include "media/base/media_switches.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_base_switches.h"
#include "ui/display/display_switches.h"
#include "ui/events/event_switches.h"
#include "ui/gfx/switches.h"
#include "ui/gl/gl_switches.h"
#include "ui/keyboard/keyboard_switches.h"
#include "ui/native_theme/native_theme_switches.h"
#include "ui/views/views_switches.h"

#if defined(OS_ANDROID)
#include "chrome/common/chrome_version_info.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_switches.h"
#else
#include "ui/message_center/message_center_switches.h"
#endif

#if defined(USE_ASH)
#include "ash/ash_switches.h"
#endif

#if defined(OS_CHROMEOS)
#include "chromeos/chromeos_switches.h"
#include "third_party/cros_system_api/switches/chrome_switches.h"
#endif

#if defined(ENABLE_APP_LIST)
#include "ui/app_list/app_list_switches.h"
#endif

#if defined(ENABLE_EXTENSIONS)
#include "extensions/common/switches.h"
#endif

#if defined(USE_OZONE)
#include "ui/ozone/public/ozone_switches.h"
#endif

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

// Enumeration of OSs.
enum {
  kOsMac = 1 << 0,
  kOsWin = 1 << 1,
  kOsLinux = 1 << 2,
  kOsCrOS = 1 << 3,
  kOsAndroid = 1 << 4,
  kOsCrOSOwnerOnly = 1 << 5
};

const unsigned kOsAll = kOsMac | kOsWin | kOsLinux | kOsCrOS | kOsAndroid;
const unsigned kOsDesktop = kOsMac | kOsWin | kOsLinux | kOsCrOS;

// Adds a |StringValue| to |list| for each platform where |bitmask| indicates
// whether the experiment is available on that platform.
void AddOsStrings(unsigned bitmask, base::ListValue* list) {
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
  for (size_t i = 0; i < arraysize(kBitsToOs); ++i) {
    if (bitmask & kBitsToOs[i].bit)
      list->Append(new base::StringValue(kBitsToOs[i].name));
  }
}

// Convert switch constants to proper CommandLine::StringType strings.
base::CommandLine::StringType GetSwitchString(const std::string& flag) {
  base::CommandLine cmd_line(base::CommandLine::NO_PROGRAM);
  cmd_line.AppendSwitch(flag);
  DCHECK_EQ(2U, cmd_line.argv().size());
  return cmd_line.argv()[1];
}

// Scoops flags from a command line.
std::set<base::CommandLine::StringType> ExtractFlagsFromCommandLine(
    const base::CommandLine& cmdline) {
  std::set<base::CommandLine::StringType> flags;
  // First do the ones between --flag-switches-begin and --flag-switches-end.
  base::CommandLine::StringVector::const_iterator first =
      std::find(cmdline.argv().begin(), cmdline.argv().end(),
                GetSwitchString(switches::kFlagSwitchesBegin));
  base::CommandLine::StringVector::const_iterator last =
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

const Experiment::Choice kTouchEventsChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_AUTOMATIC, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED,
    switches::kTouchEvents,
    switches::kTouchEventsEnabled },
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    switches::kTouchEvents,
    switches::kTouchEventsDisabled }
};

#if defined(USE_AURA)
const Experiment::Choice kOverscrollHistoryNavigationChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    switches::kOverscrollHistoryNavigation,
    "0" },
  { IDS_OVERSCROLL_HISTORY_NAVIGATION_SIMPLE_UI,
    switches::kOverscrollHistoryNavigation,
    "2" }
};
#endif

const Experiment::Choice kTouchTextSelectionStrategyChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_TOUCH_SELECTION_STRATEGY_CHARACTER,
    switches::kTouchTextSelectionStrategy,
    "character" },
  { IDS_TOUCH_SELECTION_STRATEGY_DIRECTION,
    switches::kTouchTextSelectionStrategy,
    "direction" }
};

#if !defined(DISABLE_NACL)
const Experiment::Choice kNaClDebugMaskChoices[] = {
  // Secure shell can be used on ChromeOS for forwarding the TCP port opened by
  // debug stub to a remote machine. Since secure shell uses NaCl, we usually
  // want to avoid debugging that. The PNaCl translator is also a NaCl module,
  // so by default we want to avoid debugging that.
  // NOTE: As the default value must be the empty string, the mask excluding
  // the PNaCl translator and secure shell is substituted elsewhere.
  { IDS_NACL_DEBUG_MASK_CHOICE_EXCLUDE_UTILS_PNACL, "", "" },
  { IDS_NACL_DEBUG_MASK_CHOICE_DEBUG_ALL, switches::kNaClDebugMask, "*://*" },
  { IDS_NACL_DEBUG_MASK_CHOICE_INCLUDE_DEBUG,
      switches::kNaClDebugMask, "*://*/*debug.nmf" }
};
#endif

const Experiment::Choice kMarkNonSecureAsChoices[] = {
    { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
    { IDS_MARK_NON_SECURE_AS_NEUTRAL,
        switches::kMarkNonSecureAs, switches::kMarkNonSecureAsNeutral},
    { IDS_MARK_NON_SECURE_AS_NON_SECURE,
        switches::kMarkNonSecureAs, switches::kMarkNonSecureAsNonSecure},
    { IDS_MARK_NON_SECURE_AS_DUBIOUS,
        switches::kMarkNonSecureAs, switches::kMarkNonSecureAsDubious}
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

const Experiment::Choice kShowSavedCopyChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_FLAGS_ENABLE_SHOW_SAVED_COPY_PRIMARY,
    switches::kShowSavedCopy, switches::kEnableShowSavedCopyPrimary },
  { IDS_FLAGS_ENABLE_SHOW_SAVED_COPY_SECONDARY,
    switches::kShowSavedCopy, switches::kEnableShowSavedCopySecondary },
  { IDS_FLAGS_DISABLE_SHOW_SAVED_COPY,
    switches::kShowSavedCopy, switches::kDisableShowSavedCopy }
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

#if defined(OS_ANDROID)
const Experiment::Choice kZeroSuggestExperimentsChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_FLAGS_ZERO_SUGGEST_MOST_VISITED,
    switches::kEnableZeroSuggestMostVisited, ""},
  { IDS_FLAGS_ZERO_SUGGEST_MOST_VISITED_WITHOUT_SERP,
    switches::kEnableZeroSuggestMostVisitedWithoutSerp, ""},
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    switches::kDisableZeroSuggest, ""}
};

const Experiment::Choice kReaderModeHeuristicsChoices[] = {
    { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", ""},
    { IDS_FLAGS_READER_MODE_HEURISTICS_MARKUP,
      switches::kReaderModeHeuristics,
      switches::reader_mode_heuristics::kOGArticle },
    { IDS_FLAGS_READER_MODE_HEURISTICS_ADABOOST,
      switches::kReaderModeHeuristics,
      switches::reader_mode_heuristics::kAdaBoost },
    { IDS_FLAGS_READER_MODE_HEURISTICS_ALWAYS_ON,
      switches::kReaderModeHeuristics,
      switches::reader_mode_heuristics::kAlwaysTrue },
    { IDS_FLAGS_READER_MODE_HEURISTICS_ALWAYS_OFF,
      switches::kReaderModeHeuristics,
      switches::reader_mode_heuristics::kNone },
};
#endif

const Experiment::Choice kNumRasterThreadsChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_FLAGS_NUM_RASTER_THREADS_ONE, switches::kNumRasterThreads, "1" },
  { IDS_FLAGS_NUM_RASTER_THREADS_TWO, switches::kNumRasterThreads, "2" },
  { IDS_FLAGS_NUM_RASTER_THREADS_THREE, switches::kNumRasterThreads, "3" },
  { IDS_FLAGS_NUM_RASTER_THREADS_FOUR, switches::kNumRasterThreads, "4" }
};

const Experiment::Choice kGpuRasterizationMSAASampleCountChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT,
    "",
    "" },
  { IDS_FLAGS_GPU_RASTERIZATION_MSAA_SAMPLE_COUNT_ZERO,
    switches::kGpuRasterizationMSAASampleCount, "0" },
  { IDS_FLAGS_GPU_RASTERIZATION_MSAA_SAMPLE_COUNT_TWO,
    switches::kGpuRasterizationMSAASampleCount, "2" },
  { IDS_FLAGS_GPU_RASTERIZATION_MSAA_SAMPLE_COUNT_FOUR,
    switches::kGpuRasterizationMSAASampleCount, "4" },
  { IDS_FLAGS_GPU_RASTERIZATION_MSAA_SAMPLE_COUNT_EIGHT,
    switches::kGpuRasterizationMSAASampleCount, "8" },
  { IDS_FLAGS_GPU_RASTERIZATION_MSAA_SAMPLE_COUNT_SIXTEEN,
    switches::kGpuRasterizationMSAASampleCount, "16" },
};

const Experiment::Choice kEnableGpuRasterizationChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED,
    switches::kEnableGpuRasterization, "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    switches::kDisableGpuRasterization, "" },
  { IDS_FLAGS_FORCE_GPU_RASTERIZATION,
    switches::kForceGpuRasterization, "" },
};

#if defined(OS_CHROMEOS)
const Experiment::Choice kMemoryPressureThresholdChoices[] = {
    { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
    { IDS_FLAGS_CONSERVATIVE_THRESHOLDS,
      chromeos::switches::kMemoryPressureThresholds,
      chromeos::switches::kConservativeThreshold },
    { IDS_FLAGS_AGGRESSIVE_CACHE_DISCARD_THRESHOLDS,
      chromeos::switches::kMemoryPressureThresholds,
      chromeos::switches::kAggressiveCacheDiscardThreshold },
    { IDS_FLAGS_AGGRESSIVE_TAB_DISCARD_THRESHOLDS,
      chromeos::switches::kMemoryPressureThresholds,
      chromeos::switches::kAggressiveTabDiscardThreshold },
    { IDS_FLAGS_AGGRESSIVE_THRESHOLDS,
      chromeos::switches::kMemoryPressureThresholds,
      chromeos::switches::kAggressiveThreshold },
};
#endif

const Experiment::Choice kExtensionContentVerificationChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_FLAGS_EXTENSION_CONTENT_VERIFICATION_BOOTSTRAP,
    switches::kExtensionContentVerification,
    switches::kExtensionContentVerificationBootstrap },
  { IDS_FLAGS_EXTENSION_CONTENT_VERIFICATION_ENFORCE,
    switches::kExtensionContentVerification,
    switches::kExtensionContentVerificationEnforce },
  { IDS_FLAGS_EXTENSION_CONTENT_VERIFICATION_ENFORCE_STRICT,
    switches::kExtensionContentVerification,
    switches::kExtensionContentVerificationEnforceStrict },
};

// Note that the value is specified in seconds (where 0 is equivalent to
// disabled).
const Experiment::Choice kRememberCertificateErrorDecisionsChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    switches::kRememberCertErrorDecisions,
    "-1" },
  { IDS_REMEMBER_CERTIFICATE_ERROR_DECISION_CHOICE_ONE_DAY,
    switches::kRememberCertErrorDecisions,
    "86400" },
  { IDS_REMEMBER_CERTIFICATE_ERROR_DECISION_CHOICE_THREE_DAYS,
    switches::kRememberCertErrorDecisions,
    "259200" },
  { IDS_REMEMBER_CERTIFICATE_ERROR_DECISION_CHOICE_ONE_WEEK,
    switches::kRememberCertErrorDecisions,
    "604800" },
  { IDS_REMEMBER_CERTIFICATE_ERROR_DECISION_CHOICE_ONE_MONTH,
    switches::kRememberCertErrorDecisions,
    "2592000" },
  { IDS_REMEMBER_CERTIFICATE_ERROR_DECISION_CHOICE_THREE_MONTHS,
    switches::kRememberCertErrorDecisions,
    "7776000" },
};

const Experiment::Choice kAutofillSyncCredentialChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", ""},
  { IDS_ALLOW_AUTOFILL_SYNC_CREDENTIAL,
    password_manager::switches::kAllowAutofillSyncCredential, ""},
  { IDS_DISALLOW_AUTOFILL_SYNC_CREDENTIAL_FOR_REAUTH,
    password_manager::switches::kDisallowAutofillSyncCredentialForReauth, ""},
  { IDS_DISALLOW_AUTOFILL_SYNC_CREDENTIAL,
    password_manager::switches::kDisallowAutofillSyncCredential, ""},
};

const Experiment::Choice kFillOnAccountSelectChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    autofill::switches::kDisableFillOnAccountSelect, "" },
  { IDS_FLAGS_FILL_ON_ACCOUNT_SELECT_ENABLE_HIGHLIGHTING,
    autofill::switches::kEnableFillOnAccountSelect, "" },
  { IDS_FLAGS_FILL_ON_ACCOUNT_SELECT_ENABLE_NO_HIGHLIGHTING,
    autofill::switches::kEnableFillOnAccountSelectNoHighlighting, "" },
};

#if defined(USE_ASH)
const Experiment::Choice kAshScreenRotationAnimationChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    ash::switches::kAshEnableScreenRotationAnimation,
    "none" },
  { IDS_ASH_SCREEN_ROTATION_ANIMATION_PARTIAL_ROTATION,
    ash::switches::kAshEnableScreenRotationAnimation,
    "partial-rotation" },
  { IDS_ASH_SCREEN_ROTATION_ANIMATION_FULL_ROTATION,
    ash::switches::kAshEnableScreenRotationAnimation,
    "full-rotation" }
};
#endif

#if defined(OS_CHROMEOS)
const Experiment::Choice kDataSaverPromptChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    chromeos::switches::kDisableDataSaverPrompt, "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED,
    chromeos::switches::kEnableDataSaverPrompt, "" },
  { IDS_FLAGS_DATASAVER_PROMPT_DEMO_MODE,
    chromeos::switches::kEnableDataSaverPrompt,
    chromeos::switches::kDataSaverPromptDemoMode },
};

const Experiment::Choice kFloatingVirtualKeyboardChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    keyboard::switches::kFloatingVirtualKeyboard,
    keyboard::switches::kFloatingVirtualKeyboardDisabled},
  { IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED,
    keyboard::switches::kFloatingVirtualKeyboard,
    keyboard::switches::kFloatingVirtualKeyboardEnabled},
};

const Experiment::Choice kGestureTypingChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    keyboard::switches::kGestureTyping,
    keyboard::switches::kGestureTypingDisabled},
  { IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED,
    keyboard::switches::kGestureTyping,
    keyboard::switches::kGestureTypingEnabled},
};

const Experiment::Choice kGestureEditingChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    keyboard::switches::kGestureEditing,
    keyboard::switches::kGestureEditingDisabled},
  { IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED,
    keyboard::switches::kGestureEditing,
    keyboard::switches::kGestureEditingEnabled},
};
#endif

const Experiment::Choice kSupervisedUserSafeSitesChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED,
    switches::kSupervisedUserSafeSites,
    "enabled" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    switches::kSupervisedUserSafeSites,
    "disabled" },
  { IDS_SUPERVISED_USER_SAFESITES_BLACKLIST_ONLY,
    switches::kSupervisedUserSafeSites,
    "blacklist-only" },
  { IDS_SUPERVISED_USER_SAFESITES_ONLINE_CHECK_ONLY,
    switches::kSupervisedUserSafeSites,
    "online-check-only" }
};

// RECORDING USER METRICS FOR FLAGS:
// -----------------------------------------------------------------------------
// The first line of the experiment is the internal name. If you'd like to
// gather statistics about the usage of your flag, you should append a marker
// comment to the end of the feature name, like so:
//   "my-special-feature",  // FLAGS:RECORD_UMA
//
// After doing that, run
//   tools/metrics/actions/extract_actions.py
// to add the metric to actions.xml (which will enable UMA to record your
// feature flag), then update the <owner>s and <description> sections. Make sure
// to include the actions.xml file when you upload your code for review!
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
// Command-line switches must have entries in enum "LoginCustomFlags" in
// histograms.xml. See note in histograms.xml and don't forget to run
// AboutFlagsHistogramTest unit test to calculate and verify checksum.
//
// When adding a new choice, add it to the end of the list.
const Experiment kExperiments[] = {
  {
    "ignore-gpu-blacklist",
    IDS_FLAGS_IGNORE_GPU_BLACKLIST_NAME,
    IDS_FLAGS_IGNORE_GPU_BLACKLIST_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kIgnoreGpuBlacklist)
  },
#if defined(OS_WIN)
  {
    "disable-direct-write",
    IDS_FLAGS_DISABLE_DIRECT_WRITE_NAME,
    IDS_FLAGS_DISABLE_DIRECT_WRITE_DESCRIPTION,
    kOsWin,
    SINGLE_VALUE_TYPE(switches::kDisableDirectWrite)
  },
#endif
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
    "enable-display-list-2d-canvas",
    IDS_FLAGS_ENABLE_DISPLAY_LIST_2D_CANVAS_NAME,
    IDS_FLAGS_ENABLE_DISPLAY_LIST_2D_CANVAS_DESCRIPTION,
    kOsAll,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableDisplayList2dCanvas,
                              switches::kDisableDisplayList2dCanvas)
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
    "disable-webgl",
    IDS_FLAGS_DISABLE_WEBGL_NAME,
    IDS_FLAGS_DISABLE_WEBGL_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kDisableExperimentalWebGL)
  },
#if defined(OS_WIN) || defined(OS_MACOSX)
  {
    "enable-npapi",
    IDS_FLAGS_ENABLE_NPAPI_NAME,
    IDS_FLAGS_ENABLE_NPAPI_DESCRIPTION,
    kOsWin | kOsMac,
    SINGLE_VALUE_TYPE(switches::kEnableNpapi)
  },
#endif
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
    "disable-webrtc-hw-decoding",
    IDS_FLAGS_DISABLE_WEBRTC_HW_DECODING_NAME,
    IDS_FLAGS_DISABLE_WEBRTC_HW_DECODING_DESCRIPTION,
    kOsAndroid | kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kDisableWebRtcHWDecoding)
  },
  {
    "disable-webrtc-hw-encoding",
    IDS_FLAGS_DISABLE_WEBRTC_HW_ENCODING_NAME,
    IDS_FLAGS_DISABLE_WEBRTC_HW_ENCODING_DESCRIPTION,
    kOsAndroid | kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kDisableWebRtcHWEncoding)
  },
  {
    "enable-webrtc-stun-origin",
    IDS_FLAGS_ENABLE_WEBRTC_STUN_ORIGIN_NAME,
    IDS_FLAGS_ENABLE_WEBRTC_STUN_ORIGIN_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableWebRtcStunOrigin)
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
  // Native client is compiled out when DISABLE_NACL is defined.
#if !defined(DISABLE_NACL)
  {
    "enable-nacl",  // FLAGS:RECORD_UMA
    IDS_FLAGS_ENABLE_NACL_NAME,
    IDS_FLAGS_ENABLE_NACL_DESCRIPTION,
    kOsAll,
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
    "nacl-debug-mask",  // FLAGS:RECORD_UMA
    IDS_FLAGS_NACL_DEBUG_MASK_NAME,
    IDS_FLAGS_NACL_DEBUG_MASK_DESCRIPTION,
    kOsDesktop,
    MULTI_VALUE_TYPE(kNaClDebugMaskChoices)
  },
#endif
#if defined(ENABLE_EXTENSIONS)
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
#endif
  {
    "enable-fast-unload",
    IDS_FLAGS_ENABLE_FAST_UNLOAD_NAME,
    IDS_FLAGS_ENABLE_FAST_UNLOAD_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableFastUnload)
  },
#if defined(ENABLE_EXTENSIONS)
  {
    "enable-app-window-controls",
    IDS_FLAGS_ENABLE_APP_WINDOW_CONTROLS_NAME,
    IDS_FLAGS_ENABLE_APP_WINDOW_CONTROLS_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(extensions::switches::kEnableAppWindowControls)
  },
#endif
  {
    "disable-hyperlink-auditing",
    IDS_FLAGS_DISABLE_HYPERLINK_AUDITING_NAME,
    IDS_FLAGS_DISABLE_HYPERLINK_AUDITING_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kNoPings)
  },
#if defined(OS_ANDROID)
  {
    "contextual-search",
    IDS_FLAGS_ENABLE_CONTEXTUAL_SEARCH,
    IDS_FLAGS_ENABLE_CONTEXTUAL_SEARCH_DESCRIPTION,
    kOsAndroid,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableContextualSearch,
                              switches::kDisableContextualSearch)
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
    "enable-smooth-scrolling",  // FLAGS:RECORD_UMA
    IDS_FLAGS_ENABLE_SMOOTH_SCROLLING_NAME,
    IDS_FLAGS_ENABLE_SMOOTH_SCROLLING_DESCRIPTION,
    // Can't expose the switch unless the code is compiled in.
    // On by default for the Mac (different implementation in WebKit).
    kOsLinux,
    SINGLE_VALUE_TYPE(switches::kEnableSmoothScrolling)
  },
#if defined(USE_AURA) || defined(OS_LINUX)
  {
    "overlay-scrollbars",
    IDS_FLAGS_ENABLE_OVERLAY_SCROLLBARS_NAME,
    IDS_FLAGS_ENABLE_OVERLAY_SCROLLBARS_DESCRIPTION,
    // Uses the system preference on Mac (a different implementation).
    // On Android, this is always enabled.
    kOsLinux | kOsCrOS | kOsWin,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableOverlayScrollbar,
                              switches::kDisableOverlayScrollbar)
  },
#endif
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
    "enable-quic",
    IDS_FLAGS_ENABLE_QUIC_NAME,
    IDS_FLAGS_ENABLE_QUIC_DESCRIPTION,
    kOsAll,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableQuic,
                              switches::kDisableQuic)
  },
  {
    "disable-media-source",
    IDS_FLAGS_DISABLE_MEDIA_SOURCE_NAME,
    IDS_FLAGS_DISABLE_MEDIA_SOURCE_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kDisableMediaSource)
  },
  {
    "disable-encrypted-media",
    IDS_FLAGS_DISABLE_ENCRYPTED_MEDIA_NAME,
    IDS_FLAGS_DISABLE_ENCRYPTED_MEDIA_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kDisableEncryptedMedia)
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
#endif  // defined(OS_ANDROID)
  {
    "disable-javascript-harmony-shipping",
    IDS_FLAGS_DISABLE_JAVASCRIPT_HARMONY_SHIPPING_NAME,
    IDS_FLAGS_DISABLE_JAVASCRIPT_HARMONY_SHIPPING_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kDisableJavaScriptHarmonyShipping)
  },
  {
    "enable-javascript-harmony",
    IDS_FLAGS_ENABLE_JAVASCRIPT_HARMONY_NAME,
    IDS_FLAGS_ENABLE_JAVASCRIPT_HARMONY_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kJavaScriptHarmony)
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
    "enable-gpu-rasterization",
    IDS_FLAGS_ENABLE_GPU_RASTERIZATION_NAME,
    IDS_FLAGS_ENABLE_GPU_RASTERIZATION_DESCRIPTION,
    kOsAll,
    MULTI_VALUE_TYPE(kEnableGpuRasterizationChoices)
  },
  {
    "gpu-rasterization-msaa-sample-count",
    IDS_FLAGS_GPU_RASTERIZATION_MSAA_SAMPLE_COUNT_NAME,
    IDS_FLAGS_GPU_RASTERIZATION_MSAA_SAMPLE_COUNT_DESCRIPTION,
    kOsAll,
    MULTI_VALUE_TYPE(kGpuRasterizationMSAASampleCountChoices)
  },
  {
    "enable-slimming-paint",
    IDS_FLAGS_ENABLE_SLIMMING_PAINT_NAME,
    IDS_FLAGS_ENABLE_SLIMMING_PAINT_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableSlimmingPaint)
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
#if defined(ENABLE_SPELLCHECK)
  {
    "spellcheck-autocorrect",
    IDS_FLAGS_SPELLCHECK_AUTOCORRECT,
    IDS_FLAGS_SPELLCHECK_AUTOCORRECT_DESCRIPTION,
    kOsWin | kOsLinux | kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kEnableSpellingAutoCorrect)
  },
#endif
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
    "disable-touch-adjustment",
    IDS_DISABLE_TOUCH_ADJUSTMENT_NAME,
    IDS_DISABLE_TOUCH_ADJUSTMENT_DESCRIPTION,
    kOsWin | kOsLinux | kOsCrOS | kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kDisableTouchAdjustment)
  },
#if defined(OS_CHROMEOS)
  {
    "network-portal-notification",
    IDS_FLAGS_NETWORK_PORTAL_NOTIFICATION_NAME,
    IDS_FLAGS_NETWORK_PORTAL_NOTIFICATION_DESCRIPTION,
    kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE(
        chromeos::switches::kEnableNetworkPortalNotification,
        chromeos::switches::kDisableNetworkPortalNotification)
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
    "enable-download-notification",
    IDS_FLAGS_ENABLE_DOWNLOAD_NOTIFICATION_NAME,
    IDS_FLAGS_ENABLE_DOWNLOAD_NOTIFICATION_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kEnableDownloadNotification)
  },
#if defined(ENABLE_PLUGINS)
  {
    "allow-nacl-socket-api",
    IDS_FLAGS_ALLOW_NACL_SOCKET_API_NAME,
    IDS_FLAGS_ALLOW_NACL_SOCKET_API_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE_AND_VALUE(switches::kAllowNaClSocketAPI, "*")
  },
#endif
#if defined(OS_CHROMEOS)
  {
    "allow-touchpad-three-finger-click",
    IDS_FLAGS_ALLOW_TOUCHPAD_THREE_FINGER_CLICK_NAME,
    IDS_FLAGS_ALLOW_TOUCHPAD_THREE_FINGER_CLICK_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kEnableTouchpadThreeFingerClick)
  },
  {
    "ash-enable-unified-desktop",
    IDS_FLAGS_ASH_ENABLE_UNIFIED_DESKTOP_NAME,
    IDS_FLAGS_ASH_ENABLE_UNIFIED_DESKTOP_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(ash::switches::kAshEnableUnifiedDesktop)
  },
  {
    "disable-easy-unlock",
    IDS_FLAGS_DISABLE_EASY_UNLOCK_NAME,
    IDS_FLAGS_DISABLE_EASY_UNLOCK_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(proximity_auth::switches::kDisableEasyUnlock)
  },
  {
    "enable-easy-unlock-proximity-detection",
    IDS_FLAGS_ENABLE_EASY_UNLOCK_PROXIMITY_DETECTION_NAME,
    IDS_FLAGS_ENABLE_EASY_UNLOCK_PROXIMITY_DETECTION_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(proximity_auth::switches::kEnableProximityDetection)
  },
  {
    "enable-easy-unlock-bluetooth-low-energy-detection",
    IDS_FLAGS_ENABLE_EASY_UNLOCK_BLUETOOTH_LOW_ENERGY_DISCOVERY_NAME,
    IDS_FLAGS_ENABLE_EASY_UNLOCK_BLUETOOTH_LOW_ENERGY_DISCOVERY_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(
        proximity_auth::switches::kEnableBluetoothLowEnergyDiscovery)
  },
#endif
#if defined(USE_ASH)
  {
    "disable-minimize-on-second-launcher-item-click",
    IDS_FLAGS_DISABLE_MINIMIZE_ON_SECOND_LAUNCHER_ITEM_CLICK_NAME,
    IDS_FLAGS_DISABLE_MINIMIZE_ON_SECOND_LAUNCHER_ITEM_CLICK_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kDisableMinimizeOnSecondLauncherItemClick)
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
#endif  // defined(USE_ASH)
  {
    "enable-pinch-virtual-viewport",
    IDS_FLAGS_ENABLE_PINCH_VIRTUAL_VIEWPORT_NAME,
    IDS_FLAGS_ENABLE_PINCH_VIRTUAL_VIEWPORT_DESCRIPTION,
    kOsLinux | kOsWin | kOsCrOS | kOsAndroid,
    ENABLE_DISABLE_VALUE_TYPE(
        cc::switches::kEnablePinchVirtualViewport,
        cc::switches::kDisablePinchVirtualViewport),
  },
  {
    "enable-viewport-meta",
    IDS_FLAGS_ENABLE_VIEWPORT_META_NAME,
    IDS_FLAGS_ENABLE_VIEWPORT_META_DESCRIPTION,
    kOsLinux | kOsWin | kOsCrOS | kOsMac,
    SINGLE_VALUE_TYPE(switches::kEnableViewportMeta),
  },
#if defined(OS_CHROMEOS)
  {
    "disable-boot-animation",
    IDS_FLAGS_DISABLE_BOOT_ANIMATION,
    IDS_FLAGS_DISABLE_BOOT_ANIMATION_DESCRIPTION,
    kOsCrOSOwnerOnly,
    SINGLE_VALUE_TYPE(chromeos::switches::kDisableBootAnimation),
  },
  {
    "enable-video-player-chromecast-support",
    IDS_FLAGS_ENABLE_VIDEO_PLAYER_CHROMECAST_SUPPORT_NAME,
    IDS_FLAGS_ENABLE_VIDEO_PLAYER_CHROMECAST_SUPPORT_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kEnableVideoPlayerChromecastSupport)
  },
  {
    "disable-office-editing-component-app",
    IDS_FLAGS_DISABLE_OFFICE_EDITING_COMPONENT_APP_NAME,
    IDS_FLAGS_DISABLE_OFFICE_EDITING_COMPONENT_APP_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kDisableOfficeEditingComponentApp),
  },
  {
    "disable-display-color-calibration",
    IDS_FLAGS_DISABLE_DISPLAY_COLOR_CALIBRATION_NAME,
    IDS_FLAGS_DISABLE_DISPLAY_COLOR_CALIBRATION_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(ui::switches::kDisableDisplayColorCalibration),
  },
  {
    "ash-disable-screen-orientation-lock",
    IDS_FLAGS_ASH_DISABLE_SCREEN_ORIENTATION_LOCK_NAME,
    IDS_FLAGS_ASH_DISABLE_SCREEN_ORIENTATION_LOCK_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(ash::switches::kAshDisableScreenOrientationLock),
  },
#endif  // defined(OS_CHROMEOS)
  { "disable-accelerated-video-decode",
    IDS_FLAGS_DISABLE_ACCELERATED_VIDEO_DECODE_NAME,
    IDS_FLAGS_DISABLE_ACCELERATED_VIDEO_DECODE_DESCRIPTION,
    kOsMac | kOsWin | kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kDisableAcceleratedVideoDecode),
  },
#if defined(USE_ASH)
  {
    "ash-debug-shortcuts",
    IDS_FLAGS_DEBUG_SHORTCUTS_NAME,
    IDS_FLAGS_DEBUG_SHORTCUTS_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(ash::switches::kAshDebugShortcuts),
  },
  {
    "ash-disable-maximize-mode-window-backdrop",
    IDS_FLAGS_ASH_DISABLE_MAXIMIZE_MODE_WINDOW_BACKDROP_NAME,
    IDS_FLAGS_ASH_DISABLE_MAXIMIZE_MODE_WINDOW_BACKDROP_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(ash::switches::kAshDisableMaximizeModeWindowBackdrop),
  },
  { "ash-enable-touch-view-testing",
    IDS_FLAGS_ASH_ENABLE_TOUCH_VIEW_TESTING_NAME,
    IDS_FLAGS_ASH_ENABLE_TOUCH_VIEW_TESTING_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(ash::switches::kAshEnableTouchViewTesting),
  },
  {
    "disable-touch-feedback",
    IDS_FLAGS_DISABLE_TOUCH_FEEDBACK_NAME,
    IDS_FLAGS_DISABLE_TOUCH_FEEDBACK_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kDisableTouchFeedback),
  },
  { "ash-enable-mirrored-screen",
    IDS_FLAGS_ASH_ENABLE_MIRRORED_SCREEN_NAME,
    IDS_FLAGS_ASH_ENABLE_MIRRORED_SCREEN_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(ash::switches::kAshEnableMirroredScreen),
  },
  {
    "ash-enable-screen-rotation-animations",
    IDS_FLAGS_ASH_ENABLE_SCREEN_ROTATION_ANIMATION_NAME,
    IDS_FLAGS_ASH_ENABLE_SCREEN_ROTATION_ANIMATION_DESCRIPTION,
    kOsCrOS,
    MULTI_VALUE_TYPE(kAshScreenRotationAnimationChoices)
  },
#endif  // defined(USE_ASH)
#if defined(OS_CHROMEOS)
  {
    "disable-cloud-import",
    IDS_FLAGS_DISABLE_CLOUD_IMPORT,
    IDS_FLAGS_DISABLE_CLOUD_IMPORT_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kDisableCloudImport)
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
    kOsWin | kOsLinux | kOsCrOS | kOsMac | kOsAndroid,
    ENABLE_DISABLE_VALUE_TYPE(autofill::switches::kEnablePasswordGeneration,
                              autofill::switches::kDisablePasswordGeneration)
  },
  {
    "enable-automatic-password-saving",
    IDS_FLAGS_ENABLE_AUTOMATIC_PASSWORD_SAVING_NAME,
    IDS_FLAGS_ENABLE_AUTOMATIC_PASSWORD_SAVING_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(
        password_manager::switches::kEnableAutomaticPasswordSaving)
  },
  {
    "password-manager-reauthentication",
    IDS_FLAGS_PASSWORD_MANAGER_REAUTHENTICATION_NAME,
    IDS_FLAGS_PASSWORD_MANAGER_REAUTHENTICATION_DESCRIPTION,
    kOsMac | kOsWin,
    SINGLE_VALUE_TYPE(switches::kDisablePasswordManagerReauthentication)
  },
  {
    "enable-password-link",
    IDS_FLAGS_PASSWORD_MANAGER_LINK_NAME,
    IDS_FLAGS_PASSWORD_MANAGER_LINK_DESCRIPTION,
    kOsAndroid | kOsDesktop,
    ENABLE_DISABLE_VALUE_TYPE(
        password_manager::switches::kEnablePasswordLink,
        password_manager::switches::kDisablePasswordLink)
  },
  {
     "enable-password-save-in-page-navigation",
     IDS_FLAGS_ENABLE_SAVE_PASSOWRD_ON_IN_PAGE_NAVIGATION_NAME,
     IDS_FLAGS_ENABLE_SAVE_PASSOWRD_ON_IN_PAGE_NAVIGATION_DESCRIPTION,
     kOsAll,
     SINGLE_VALUE_TYPE(
             autofill::switches::kEnablePasswordSaveOnInPageNavigation)
  },
  {
    "enable-affiliation-based-matching",
    IDS_FLAGS_ENABLE_AFFILIATION_BASED_MATCHING_NAME,
    IDS_FLAGS_ENABLE_AFFILIATION_BASED_MATCHING_DESCRIPTION,
    kOsWin | kOsLinux | kOsCrOS | kOsMac | kOsAndroid,
    ENABLE_DISABLE_VALUE_TYPE(
        password_manager::switches::kEnableAffiliationBasedMatching,
        password_manager::switches::kDisableAffiliationBasedMatching)
  },
  {
    "enable-deferred-image-decoding",
    IDS_FLAGS_ENABLE_DEFERRED_IMAGE_DECODING_NAME,
    IDS_FLAGS_ENABLE_DEFERRED_IMAGE_DECODING_DESCRIPTION,
    kOsMac | kOsLinux | kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kEnableDeferredImageDecoding)
  },
  {
    "wallet-service-use-sandbox",
    IDS_FLAGS_WALLET_SERVICE_USE_SANDBOX_NAME,
    IDS_FLAGS_WALLET_SERVICE_USE_SANDBOX_DESCRIPTION,
    kOsAndroid | kOsDesktop,
    ENABLE_DISABLE_VALUE_TYPE_AND_VALUE(
        autofill::switches::kWalletServiceUseSandbox, "1",
        autofill::switches::kWalletServiceUseSandbox, "0")
  },
#if defined(USE_AURA)
  {
    "overscroll-history-navigation",
    IDS_FLAGS_OVERSCROLL_HISTORY_NAVIGATION_NAME,
    IDS_FLAGS_OVERSCROLL_HISTORY_NAVIGATION_DESCRIPTION,
    kOsAll,
    MULTI_VALUE_TYPE(kOverscrollHistoryNavigationChoices)
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
    "enable-icon-ntp",
    IDS_FLAGS_ENABLE_ICON_NTP_NAME,
    IDS_FLAGS_ENABLE_ICON_NTP_DESCRIPTION,
    kOsAndroid | kOsMac | kOsWin | kOsLinux | kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableIconNtp,
                              switches::kDisableIconNtp)
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
    kOsCrOS | kOsWin | kOsLinux,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableTouchEditing,
                              switches::kDisableTouchEditing)
  },
  {
      "touch-selection-strategy",
      IDS_FLAGS_TOUCH_SELECTION_STRATEGY_NAME,
      IDS_FLAGS_TOUCH_SELECTION_STRATEGY_DESCRIPTION,
      kOsAndroid,   // TODO(mfomitchev): Add CrOS/Win/Linux support soon.
      MULTI_VALUE_TYPE(kTouchTextSelectionStrategyChoices)
  },
  {
    "enable-stale-while-revalidate",
    IDS_FLAGS_ENABLE_STALE_WHILE_REVALIDATE_NAME,
    IDS_FLAGS_ENABLE_STALE_WHILE_REVALIDATE_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableStaleWhileRevalidate)
  },
  {
    "enable-suggestions-service",
    IDS_FLAGS_ENABLE_SUGGESTIONS_SERVICE_NAME,
    IDS_FLAGS_ENABLE_SUGGESTIONS_SERVICE_DESCRIPTION,
    kOsAndroid | kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableSuggestionsService,
                              switches::kDisableSuggestionsService)
  },
  {
    "enable-supervised-user-managed-bookmarks-folder",
    IDS_FLAGS_ENABLE_SUPERVISED_USER_MANAGED_BOOKMARKS_FOLDER_NAME,
    IDS_FLAGS_ENABLE_SUPERVISED_USER_MANAGED_BOOKMARKS_FOLDER_DESCRIPTION,
    kOsAndroid | kOsMac | kOsWin | kOsLinux | kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kEnableSupervisedUserManagedBookmarksFolder)
  },
#if defined(ENABLE_APP_LIST)
  {
    "enable-sync-app-list",
    IDS_FLAGS_ENABLE_SYNC_APP_LIST_NAME,
    IDS_FLAGS_ENABLE_SYNC_APP_LIST_DESCRIPTION,
    kOsDesktop,
    ENABLE_DISABLE_VALUE_TYPE(app_list::switches::kEnableSyncAppList,
                              app_list::switches::kDisableSyncAppList)
  },
#endif
#if defined(OS_MACOSX)
  {
    "enable-avfoundation",
    IDS_FLAGS_ENABLE_AVFOUNDATION_NAME,
    IDS_FLAGS_ENABLE_AVFOUNDATION_DESCRIPTION,
    kOsMac,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableAVFoundation,
                              switches::kForceQTKit)
  },
#endif
  {
    "lcd-text-aa",
    IDS_FLAGS_LCD_TEXT_NAME,
    IDS_FLAGS_LCD_TEXT_DESCRIPTION,
    kOsDesktop,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableLCDText,
                              switches::kDisableLCDText)
  },
#if defined(OS_ANDROID) || defined(OS_MACOSX)
  {
    "delegated-renderer",
    IDS_FLAGS_DELEGATED_RENDERER_NAME,
    IDS_FLAGS_DELEGATED_RENDERER_DESCRIPTION,
    kOsAndroid,  // TODO(ccameron) Add mac support soon.
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableDelegatedRenderer,
                              switches::kDisableDelegatedRenderer)
  },
#endif
  {
    "max-tiles-for-interest-area",
    IDS_FLAGS_MAX_TILES_FOR_INTEREST_AREA_NAME,
    IDS_FLAGS_MAX_TILES_FOR_INTEREST_AREA_DESCRIPTION,
    kOsAll,
    MULTI_VALUE_TYPE(kMaxTilesForInterestAreaChoices)
  },
  {
    "enable-offer-store-unmasked-wallet-cards",
    IDS_FLAGS_ENABLE_OFFER_STORE_UNMASKED_WALLET_CARDS,
    IDS_FLAGS_ENABLE_OFFER_STORE_UNMASKED_WALLET_CARDS_DESCRIPTION,
    kOsAll,
    ENABLE_DISABLE_VALUE_TYPE(
        autofill::switches::kEnableOfferStoreUnmaskedWalletCards,
        autofill::switches::kDisableOfferStoreUnmaskedWalletCards)
  },
  {
    "enable-offline-auto-reload",
    IDS_FLAGS_ENABLE_OFFLINE_AUTO_RELOAD_NAME,
    IDS_FLAGS_ENABLE_OFFLINE_AUTO_RELOAD_DESCRIPTION,
    kOsAll,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableOfflineAutoReload,
                              switches::kDisableOfflineAutoReload)
  },
  {
    "enable-offline-auto-reload-visible-only",
    IDS_FLAGS_ENABLE_OFFLINE_AUTO_RELOAD_VISIBLE_ONLY_NAME,
    IDS_FLAGS_ENABLE_OFFLINE_AUTO_RELOAD_VISIBLE_ONLY_DESCRIPTION,
    kOsAll,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableOfflineAutoReloadVisibleOnly,
                              switches::kDisableOfflineAutoReloadVisibleOnly)
  },
  {
    "show-saved-copy",
    IDS_FLAGS_SHOW_SAVED_COPY_NAME,
    IDS_FLAGS_SHOW_SAVED_COPY_DESCRIPTION,
    kOsAll,
    MULTI_VALUE_TYPE(kShowSavedCopyChoices)
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
#if defined(OS_ANDROID)
  {
    "disable-gesture-requirement-for-media-playback",
    IDS_FLAGS_DISABLE_GESTURE_REQUIREMENT_FOR_MEDIA_PLAYBACK_NAME,
    IDS_FLAGS_DISABLE_GESTURE_REQUIREMENT_FOR_MEDIA_PLAYBACK_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kDisableGestureRequirementForMediaPlayback)
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
    "enable-virtual-keyboard-overscroll",
    IDS_FLAGS_ENABLE_VIRTUAL_KEYBOARD_OVERSCROLL_NAME,
    IDS_FLAGS_ENABLE_VIRTUAL_KEYBOARD_OVERSCROLL_DESCRIPTION,
    kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE(
        keyboard::switches::kEnableVirtualKeyboardOverscroll,
        keyboard::switches::kDisableVirtualKeyboardOverscroll)
  },
  {
    "enable-swipe-selection",
    IDS_FLAGS_ENABLE_SWIPE_SELECTION_NAME,
    IDS_FLAGS_ENABLE_SWIPE_SELECTION_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(keyboard::switches::kEnableSwipeSelection)
  },
  {
    "enable-input-view",
    IDS_FLAGS_ENABLE_INPUT_VIEW_NAME,
    IDS_FLAGS_ENABLE_INPUT_VIEW_DESCRIPTION,
    kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE(keyboard::switches::kEnableInputView,
                              keyboard::switches::kDisableInputView)
  },
  {
    "disable-new-korean-ime",
    IDS_FLAGS_DISABLE_NEW_KOREAN_IME_NAME,
    IDS_FLAGS_DISABLE_NEW_KOREAN_IME_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kDisableNewKoreanIme)
  },
  {
    "disable-new-md-input-view",
    IDS_FLAGS_DISABLE_NEW_MD_INPUT_VIEW_NAME,
    IDS_FLAGS_DISABLE_NEW_MD_INPUT_VIEW_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(keyboard::switches::kDisableNewMDInputView)
  },
  {
    "enable-physical-keyboard-autocorrect",
    IDS_FLAGS_ENABLE_PHYSICAL_KEYBOARD_AUTOCORRECT_NAME,
    IDS_FLAGS_ENABLE_PHYSICAL_KEYBOARD_AUTOCORRECT_DESCRIPTION,
    kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE(
        chromeos::switches::kEnablePhysicalKeyboardAutocorrect,
        chromeos::switches::kDisablePhysicalKeyboardAutocorrect)
  },
  {
    "disable-voice-input",
    IDS_FLAGS_DISABLE_VOICE_INPUT_NAME,
    IDS_FLAGS_DISABLE_VOICE_INPUT_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(keyboard::switches::kDisableVoiceInput)
  },
  {
    "enable-experimental-input-view-features",
    IDS_FLAGS_ENABLE_EXPERIMENTAL_INPUT_VIEW_FEATURES_NAME,
    IDS_FLAGS_ENABLE_EXPERIMENTAL_INPUT_VIEW_FEATURES_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(keyboard::switches::kEnableExperimentalInputViewFeatures)
  },
  {
    "floating-virtual-keyboard",
    IDS_FLAGS_FLOATING_VIRTUAL_KEYBOARD_NAME,
    IDS_FLAGS_FLOATING_VIRTUAL_KEYBOARD_DESCRIPTION,
    kOsCrOS,
    MULTI_VALUE_TYPE(kFloatingVirtualKeyboardChoices)
  },
  {
    "gesture-typing",
    IDS_FLAGS_GESTURE_TYPING_NAME,
    IDS_FLAGS_GESTURE_TYPING_DESCRIPTION,
    kOsCrOS,
    MULTI_VALUE_TYPE(kGestureTypingChoices)
  },
  {
    "gesture-editing",
    IDS_FLAGS_GESTURE_EDITING_NAME,
    IDS_FLAGS_GESTURE_EDITING_DESCRIPTION,
    kOsCrOS,
    MULTI_VALUE_TYPE(kGestureEditingChoices)
  },
  {
    "disable-smart-virtual-keyboard",
    IDS_FLAGS_DISABLE_SMART_VIRTUAL_KEYBOARD_NAME,
    IDS_FLAGS_DISABLE_SMART_VIRTUAL_KEYBOARD_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(keyboard::switches::kDisableSmartVirtualKeyboard)
  },
#endif
  {
    "enable-simple-cache-backend",
    IDS_FLAGS_ENABLE_SIMPLE_CACHE_BACKEND_NAME,
    IDS_FLAGS_ENABLE_SIMPLE_CACHE_BACKEND_DESCRIPTION,
    kOsWin | kOsMac | kOsLinux | kOsCrOS,
    MULTI_VALUE_TYPE(kSimpleCacheBackendChoices)
  },
  {
    "enable-tcp-fast-open",
    IDS_FLAGS_ENABLE_TCP_FAST_OPEN_NAME,
    IDS_FLAGS_ENABLE_TCP_FAST_OPEN_DESCRIPTION,
    kOsLinux | kOsCrOS | kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kEnableTcpFastOpen)
  },
#if defined(ENABLE_SERVICE_DISCOVERY)
  {
    "device-discovery-notifications",
    IDS_FLAGS_DEVICE_DISCOVERY_NOTIFICATIONS_NAME,
    IDS_FLAGS_DEVICE_DISCOVERY_NOTIFICATIONS_DESCRIPTION,
    kOsDesktop,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableDeviceDiscoveryNotifications,
                              switches::kDisableDeviceDiscoveryNotifications)
  },
  {
    "enable-print-preview-register-promos",
    IDS_FLAGS_ENABLE_PRINT_PREVIEW_REGISTER_PROMOS_NAME,
    IDS_FLAGS_ENABLE_PRINT_PREVIEW_REGISTER_PROMOS_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kEnablePrintPreviewRegisterPromos)
  },
#endif  // ENABLE_SERVICE_DISCOVERY
#if defined(OS_WIN)
  {
    "enable-cloud-print-xps",
    IDS_FLAGS_ENABLE_CLOUD_PRINT_XPS_NAME,
    IDS_FLAGS_ENABLE_CLOUD_PRINT_XPS_DESCRIPTION,
    kOsWin,
    SINGLE_VALUE_TYPE(switches::kEnableCloudPrintXps)
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
#if defined(TOOLKIT_VIEWS)
  {
    "disable-hide-inactive-stacked-tab-close-buttons",
    IDS_FLAGS_DISABLE_HIDE_INACTIVE_STACKED_TAB_CLOSE_BUTTONS_NAME,
    IDS_FLAGS_DISABLE_HIDE_INACTIVE_STACKED_TAB_CLOSE_BUTTONS_DESCRIPTION,
    kOsCrOS | kOsWin | kOsLinux,
    SINGLE_VALUE_TYPE(switches::kDisableHideInactiveStackedTabCloseButtons)
  },
#endif
#if defined(ENABLE_SPELLCHECK)
  {
    "enable-spelling-feedback-field-trial",
    IDS_FLAGS_ENABLE_SPELLING_FEEDBACK_FIELD_TRIAL_NAME,
    IDS_FLAGS_ENABLE_SPELLING_FEEDBACK_FIELD_TRIAL_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableSpellingFeedbackFieldTrial)
  },
#endif
  {
    "enable-webgl-draft-extensions",
    IDS_FLAGS_ENABLE_WEBGL_DRAFT_EXTENSIONS_NAME,
    IDS_FLAGS_ENABLE_WEBGL_DRAFT_EXTENSIONS_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableWebGLDraftExtensions)
  },
  {
    "enable-new-profile-management",
    IDS_FLAGS_ENABLE_NEW_PROFILE_MANAGEMENT_NAME,
    IDS_FLAGS_ENABLE_NEW_PROFILE_MANAGEMENT_DESCRIPTION,
    kOsAndroid | kOsMac | kOsWin | kOsLinux | kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableNewProfileManagement,
                              switches::kDisableNewProfileManagement)
  },
  {
    "enable-account-consistency",
    IDS_FLAGS_ENABLE_ACCOUNT_CONSISTENCY_NAME,
    IDS_FLAGS_ENABLE_ACCOUNT_CONSISTENCY_DESCRIPTION,
    kOsAndroid | kOsMac | kOsWin | kOsLinux | kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableAccountConsistency,
                              switches::kDisableAccountConsistency)
  },
  {
    "enable-iframe-based-signin",
    IDS_FLAGS_ENABLE_IFRAME_BASED_SIGNIN_NAME,
    IDS_FLAGS_ENABLE_IFRAME_BASED_SIGNIN_DESCRIPTION,
    kOsMac | kOsWin | kOsLinux,
    SINGLE_VALUE_TYPE(switches::kEnableIframeBasedSignin)
  },
  {
    "enable-google-profile-info",
    IDS_FLAGS_ENABLE_GOOGLE_PROFILE_INFO_NAME,
    IDS_FLAGS_ENABLE_GOOGLE_PROFILE_INFO_DESCRIPTION,
    kOsMac | kOsWin | kOsLinux,
    SINGLE_VALUE_TYPE(switches::kGoogleProfileInfo)
  },
  {
    "reset-app-list-install-state",
    IDS_FLAGS_RESET_APP_LIST_INSTALL_STATE_NAME,
    IDS_FLAGS_RESET_APP_LIST_INSTALL_STATE_DESCRIPTION,
    kOsMac | kOsWin | kOsLinux,
    SINGLE_VALUE_TYPE(switches::kResetAppListInstallState)
  },
#if defined(ENABLE_APP_LIST)
#if defined(OS_LINUX)
  {
    // This is compiled out on non-Linux platforms because otherwise it would be
    // visible on Win/Mac/CrOS but not on Linux GTK, which would be confusing.
    // TODO(mgiuca): Remove the #if when Aura is the default on Linux.
    "enable-app-list",
    IDS_FLAGS_ENABLE_APP_LIST_NAME,
    IDS_FLAGS_ENABLE_APP_LIST_DESCRIPTION,
    kOsLinux,
    SINGLE_VALUE_TYPE(switches::kEnableAppList)
  },
#endif
#if defined(ENABLE_EXTENSIONS)
  {
    "enable-surface-worker",
    IDS_FLAGS_ENABLE_SURFACE_WORKER_NAME,
    IDS_FLAGS_ENABLE_SURFACE_WORKER_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(extensions::switches::kEnableSurfaceWorker)
  },
#endif
  {
    "disable-drive-apps-in-app-list",
    IDS_FLAGS_DISABLE_DRIVE_APPS_IN_APP_LIST_NAME,
    IDS_FLAGS_DISABLE_DRIVE_APPS_IN_APP_LIST_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(app_list::switches::kDisableDriveAppsInAppList)
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
  {
    // TODO(dmazzoni): remove this flag when native android accessibility
    // ships in the stable channel. http://crbug.com/356775
    "enable-accessibility-script-injection",
    IDS_FLAGS_ENABLE_ACCESSIBILITY_SCRIPT_INJECTION_NAME,
    IDS_FLAGS_ENABLE_ACCESSIBILITY_SCRIPT_INJECTION_DESCRIPTION,
    kOsAndroid,
    // Java-only switch: ContentSwitches.ENABLE_ACCESSIBILITY_SCRIPT_INJECTION.
    SINGLE_VALUE_TYPE("enable-accessibility-script-injection")
  },
#endif
  {
    "enable-one-copy",
    IDS_FLAGS_ONE_COPY_NAME,
    IDS_FLAGS_ONE_COPY_DESCRIPTION,
    kOsAll,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableOneCopy,
                              switches::kDisableOneCopy)
  },
  {
    "enable-zero-copy",
    IDS_FLAGS_ZERO_COPY_NAME,
    IDS_FLAGS_ZERO_COPY_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableZeroCopy)
  },
#if defined(OS_CHROMEOS)
  {
    "enable-first-run-ui-transitions",
    IDS_FLAGS_ENABLE_FIRST_RUN_UI_TRANSITIONS_NAME,
    IDS_FLAGS_ENABLE_FIRST_RUN_UI_TRANSITIONS_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kEnableFirstRunUITransitions)
  },
#endif
  {
    "disable-new-bookmark-apps",
    IDS_FLAGS_NEW_BOOKMARK_APPS_NAME,
    IDS_FLAGS_NEW_BOOKMARK_APPS_DESCRIPTION,
    kOsWin | kOsCrOS | kOsLinux | kOsMac,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableNewBookmarkApps,
                              switches::kDisableNewBookmarkApps)
  },
#if defined(OS_MACOSX)
  {
    "disable-hosted-app-shim-creation",
    IDS_FLAGS_DISABLE_HOSTED_APP_SHIM_CREATION_NAME,
    IDS_FLAGS_DISABLE_HOSTED_APP_SHIM_CREATION_DESCRIPTION,
    kOsMac,
    SINGLE_VALUE_TYPE(switches::kDisableHostedAppShimCreation)
  },
  {
    "enable-hosted-app-quit-notification",
    IDS_FLAGS_ENABLE_HOSTED_APP_QUIT_NOTIFICATION_NAME,
    IDS_FLAGS_ENABLE_HOSTED_APP_QUIT_NOTIFICATION_DESCRIPTION,
    kOsMac,
    SINGLE_VALUE_TYPE(switches::kHostedAppQuitNotification)
  },
#endif
  {
    "enable-ephemeral-apps-in-webstore",
    IDS_FLAGS_ENABLE_EPHEMERAL_APPS_IN_WEBSTORE_NAME,
    IDS_FLAGS_ENABLE_EPHEMERAL_APPS_IN_WEBSTORE_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableEphemeralAppsInWebstore)
  },
  {
    "enable-linkable-ephemeral-apps",
    IDS_FLAGS_ENABLE_LINKABLE_EPHEMERAL_APPS_NAME,
    IDS_FLAGS_ENABLE_LINKABLE_EPHEMERAL_APPS_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableLinkableEphemeralApps)
  },
  {
    "enable-service-worker-sync",
    IDS_FLAGS_ENABLE_SERVICE_WORKER_SYNC_NAME,
    IDS_FLAGS_ENABLE_SERVICE_WORKER_SYNC_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableServiceWorkerSync)
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
  {
    "disable-pull-to-refresh-effect",
    IDS_FLAGS_DISABLE_PULL_TO_REFRESH_EFFECT_NAME,
    IDS_FLAGS_DISABLE_PULL_TO_REFRESH_EFFECT_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kDisablePullToRefreshEffect)
  },
#endif
#if defined(OS_MACOSX)
  {
    "enable-translate-new-ux",
    IDS_FLAGS_ENABLE_TRANSLATE_NEW_UX_NAME,
    IDS_FLAGS_ENABLE_TRANSLATE_NEW_UX_DESCRIPTION,
    kOsMac,
    SINGLE_VALUE_TYPE(switches::kEnableTranslateNewUX)
  },
#endif
#if defined(TOOLKIT_VIEWS)
  {
    "disable-views-rect-based-targeting",  // FLAGS:RECORD_UMA
    IDS_FLAGS_DISABLE_VIEWS_RECT_BASED_TARGETING_NAME,
    IDS_FLAGS_DISABLE_VIEWS_RECT_BASED_TARGETING_DESCRIPTION,
    kOsCrOS | kOsWin | kOsLinux,
    SINGLE_VALUE_TYPE(views::switches::kDisableViewsRectBasedTargeting)
  },
  {
    "enable-link-disambiguation-popup",
    IDS_FLAGS_ENABLE_LINK_DISAMBIGUATION_POPUP_NAME,
    IDS_FLAGS_ENABLE_LINK_DISAMBIGUATION_POPUP_DESCRIPTION,
    kOsCrOS | kOsWin,
    SINGLE_VALUE_TYPE(switches::kEnableLinkDisambiguationPopup)
  },
#endif
#if defined(ENABLE_EXTENSIONS)
  {
    "enable-apps-show-on-first-paint",
    IDS_FLAGS_ENABLE_APPS_SHOW_ON_FIRST_PAINT_NAME,
    IDS_FLAGS_ENABLE_APPS_SHOW_ON_FIRST_PAINT_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(extensions::switches::kEnableAppsShowOnFirstPaint)
  },
#endif
  {
    "enhanced-bookmarks-experiment",
    IDS_FLAGS_ENABLE_ENHANCED_BOOKMARKS_NAME,
    IDS_FLAGS_ENABLE_ENHANCED_BOOKMARKS_DESCRIPTION,
    kOsDesktop | kOsAndroid,
    ENABLE_DISABLE_VALUE_TYPE_AND_VALUE(
        switches::kEnhancedBookmarksExperiment, "1",
        switches::kEnhancedBookmarksExperiment, "0")
  },
#if defined(OS_ANDROID)
  {
    "enable-hosted-mode",
    IDS_FLAGS_ENABLE_HOSTED_MODE_NAME,
    IDS_FLAGS_ENABLE_HOSTED_MODE_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kEnableHostedMode)
  },
  {
    "enable-zero-suggest-experiment",
    IDS_FLAGS_ZERO_SUGGEST_EXPERIMENT_NAME,
    IDS_FLAGS_ZERO_SUGGEST_EXPERIMENT_DESCRIPTION,
    kOsAndroid,
    MULTI_VALUE_TYPE(kZeroSuggestExperimentsChoices)
  },
  {
    "reader-mode-heuristics",
    IDS_FLAGS_READER_MODE_HEURISTICS_NAME,
    IDS_FLAGS_READER_MODE_HEURISTICS_DESCRIPTION,
    kOsAndroid,
    MULTI_VALUE_TYPE(kReaderModeHeuristicsChoices)
  },
  {
    "enable-reader-mode-toolbar-icon",
    IDS_FLAGS_READER_MODE_EXPERIMENT_NAME,
    IDS_FLAGS_READER_MODE_EXPERIMENT_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kEnableReaderModeToolbarIcon)
  },
  {
    "enable-dom-distiller-button-animation",
    IDS_FLAGS_READER_MODE_BUTTON_ANIMATION,
    IDS_FLAGS_READER_MODE_BUTTON_ANIMATION_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kEnableDomDistillerButtonAnimation)
  },
#endif
  {
    "num-raster-threads",
    IDS_FLAGS_NUM_RASTER_THREADS_NAME,
    IDS_FLAGS_NUM_RASTER_THREADS_DESCRIPTION,
    kOsAll,
    MULTI_VALUE_TYPE(kNumRasterThreadsChoices)
  },
  {
    "enable-single-click-autofill",
    IDS_FLAGS_ENABLE_SINGLE_CLICK_AUTOFILL_NAME,
    IDS_FLAGS_ENABLE_SINGLE_CLICK_AUTOFILL_DESCRIPTION,
    kOsCrOS | kOsMac | kOsWin | kOsLinux | kOsAndroid,
    ENABLE_DISABLE_VALUE_TYPE(
        autofill::switches::kEnableSingleClickAutofill,
        autofill::switches::kDisableSingleClickAutofill)
  },
  {
    "enable-permissions-bubbles",
    IDS_FLAGS_ENABLE_PERMISSIONS_BUBBLES_NAME,
    IDS_FLAGS_ENABLE_PERMISSIONS_BUBBLES_DESCRIPTION,
    kOsCrOS | kOsMac | kOsWin | kOsLinux,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnablePermissionsBubbles,
                              switches::kDisablePermissionsBubbles)
  },
  {
    "enable-site-engagement-service",
    IDS_FLAGS_ENABLE_SITE_ENGAGEMENT_SERVICE_NAME,
    IDS_FLAGS_ENABLE_SITE_ENGAGEMENT_SERVICE_DESCRIPTION,
    kOsAll,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableSiteEngagementService,
                              switches::kDisableSiteEngagementService)
  },
  {
    "enable-session-crashed-bubble",
    IDS_FLAGS_ENABLE_SESSION_CRASHED_BUBBLE_NAME,
    IDS_FLAGS_ENABLE_SESSION_CRASHED_BUBBLE_DESCRIPTION,
    kOsWin | kOsLinux,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableSessionCrashedBubble,
                              switches::kDisableSessionCrashedBubble)
  },
  {
    "enable-pdf-material-ui",
    IDS_FLAGS_PDF_MATERIAL_UI_NAME,
    IDS_FLAGS_PDF_MATERIAL_UI_DESCRIPTION,
    kOsDesktop,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnablePdfMaterialUI,
                              switches::kDisablePdfMaterialUI)
  },
  {
    "disable-cast-streaming-hw-encoding",
    IDS_FLAGS_DISABLE_CAST_STREAMING_HW_ENCODING_NAME,
    IDS_FLAGS_DISABLE_CAST_STREAMING_HW_ENCODING_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kDisableCastStreamingHWEncoding)
  },
#if defined(OS_ANDROID)
  {
    "disable-cast",
    IDS_FLAGS_DISABLE_CAST_NAME,
    IDS_FLAGS_DISABLE_CAST_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kDisableCast)
  },
  {
    "prefetch-search-results",
    IDS_FLAGS_PREFETCH_SEARCH_RESULTS_NAME,
    IDS_FLAGS_PREFETCH_SEARCH_RESULTS_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kPrefetchSearchResults)
  },
#endif
#if defined(ENABLE_APP_LIST)
  {
    "enable-experimental-app-list",
    IDS_FLAGS_ENABLE_EXPERIMENTAL_APP_LIST_NAME,
    IDS_FLAGS_ENABLE_EXPERIMENTAL_APP_LIST_DESCRIPTION,
    kOsWin | kOsLinux | kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE(app_list::switches::kEnableExperimentalAppList,
                              app_list::switches::kDisableExperimentalAppList)
  },
  {
    "enable-centered-app-list",
    IDS_FLAGS_ENABLE_CENTERED_APP_LIST_NAME,
    IDS_FLAGS_ENABLE_CENTERED_APP_LIST_DESCRIPTION,
    kOsWin | kOsLinux | kOsCrOS,
    SINGLE_VALUE_TYPE(app_list::switches::kEnableCenteredAppList)
  },
  {
    "enable-new-app-list-mixer",
    IDS_FLAGS_ENABLE_NEW_APP_LIST_MIXER_NAME,
    IDS_FLAGS_ENABLE_NEW_APP_LIST_MIXER_DESCRIPTION,
    kOsWin | kOsLinux | kOsCrOS | kOsMac,
    ENABLE_DISABLE_VALUE_TYPE(app_list::switches::kEnableNewAppListMixer,
                              app_list::switches::kDisableNewAppListMixer)
  },
#endif
  {
    "disable-threaded-scrolling",
    IDS_FLAGS_DISABLE_THREADED_SCROLLING_NAME,
    IDS_FLAGS_DISABLE_THREADED_SCROLLING_DESCRIPTION,
    kOsWin | kOsLinux | kOsCrOS | kOsAndroid | kOsMac,
    SINGLE_VALUE_TYPE(switches::kDisableThreadedScrolling)
  },
  {
    "bleeding-edge-renderer-mode",
    IDS_FLAGS_BLEEDING_RENDERER_NAME,
    IDS_FLAGS_BLEEDING_RENDERER_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kEnableBleedingEdgeRenderingFastPaths)
  },
  {
    "enable-settings-window",
    IDS_FLAGS_ENABLE_SETTINGS_WINDOW_NAME,
    IDS_FLAGS_ENABLE_SETTINGS_WINDOW_DESCRIPTION,
    kOsDesktop,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableSettingsWindow,
                              switches::kDisableSettingsWindow)
  },
#if defined(OS_ANDROID)
  {
    "enable-instant-search-clicks",
    IDS_FLAGS_ENABLE_INSTANT_SEARCH_CLICKS_NAME,
    IDS_FLAGS_ENABLE_INSTANT_SEARCH_CLICKS_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kEnableInstantSearchClicks)
  },
#endif
  {
    "enable-save-password-bubble",
    IDS_FLAGS_ENABLE_SAVE_PASSWORD_BUBBLE_NAME,
    IDS_FLAGS_ENABLE_SAVE_PASSWORD_BUBBLE_DESCRIPTION,
    kOsWin | kOsLinux | kOsCrOS | kOsMac,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableSavePasswordBubble,
                              switches::kDisableSavePasswordBubble)
  },
  {
    "enable-apps-file-associations",
    IDS_FLAGS_ENABLE_APPS_FILE_ASSOCIATIONS_NAME,
    IDS_FLAGS_ENABLE_APPS_FILE_ASSOCIATIONS_DESCRIPTION,
    kOsMac,
    SINGLE_VALUE_TYPE(switches::kEnableAppsFileAssociations)
  },
#if defined(OS_ANDROID)
  {
    "enable-embeddedsearch-api",
    IDS_FLAGS_ENABLE_EMBEDDEDSEARCH_API_NAME,
    IDS_FLAGS_ENABLE_EMBEDDEDSEARCH_API_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kEnableEmbeddedSearchAPI)
  },
#endif
  {
    "distance-field-text",
    IDS_FLAGS_DISTANCE_FIELD_TEXT_NAME,
    IDS_FLAGS_DISTANCE_FIELD_TEXT_DESCRIPTION,
    kOsAll,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableDistanceFieldText,
                              switches::kDisableDistanceFieldText)
  },
  {
    "extension-content-verification",
    IDS_FLAGS_EXTENSION_CONTENT_VERIFICATION_NAME,
    IDS_FLAGS_EXTENSION_CONTENT_VERIFICATION_DESCRIPTION,
    kOsDesktop,
    MULTI_VALUE_TYPE(kExtensionContentVerificationChoices)
  },
#if defined(USE_AURA)
  {
    "text-input-focus-manager",
    IDS_FLAGS_TEXT_INPUT_FOCUS_MANAGER_NAME,
    IDS_FLAGS_TEXT_INPUT_FOCUS_MANAGER_DESCRIPTION,
    kOsCrOS | kOsLinux | kOsWin,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableTextInputFocusManager,
                              switches::kDisableTextInputFocusManager)
  },
#endif
#if defined(ENABLE_EXTENSIONS)
  {
    "extension-active-script-permission",
    IDS_FLAGS_USER_CONSENT_FOR_EXTENSION_SCRIPTS_NAME,
    IDS_FLAGS_USER_CONSENT_FOR_EXTENSION_SCRIPTS_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(extensions::switches::kEnableScriptsRequireAction)
  },
#endif
  {
    "answers-in-suggest",
    IDS_FLAGS_ENABLE_ANSWERS_IN_SUGGEST_NAME,
    IDS_FLAGS_ENABLE_ANSWERS_IN_SUGGEST_DESCRIPTION,
    kOsAndroid | kOsDesktop,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableAnswersInSuggest,
                              switches::kDisableAnswersInSuggest)
  },
#if defined(OS_ANDROID)
  {
    "enable-data-reduction-proxy-dev",
    IDS_FLAGS_ENABLE_DATA_REDUCTION_PROXY_DEV_NAME,
    IDS_FLAGS_ENABLE_DATA_REDUCTION_PROXY_DEV_DESCRIPTION,
    kOsAndroid,
    ENABLE_DISABLE_VALUE_TYPE(
        data_reduction_proxy::switches::kEnableDataReductionProxyDev,
        data_reduction_proxy::switches::kDisableDataReductionProxyDev)
  },
  {
    "enable-data-reduction-proxy-alt",
    IDS_FLAGS_ENABLE_DATA_REDUCTION_PROXY_ALTERNATIVE_NAME,
    IDS_FLAGS_ENABLE_DATA_REDUCTION_PROXY_ALTERNATIVE_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE(
        data_reduction_proxy::switches::kEnableDataReductionProxyAlt)
  },
#endif
  {
    "enable-hotword-hardware",
    IDS_FLAGS_ENABLE_EXPERIMENTAL_HOTWORD_HARDWARE_NAME,
    IDS_FLAGS_ENABLE_EXPERIMENTAL_HOTWORD_HARDWARE_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kEnableExperimentalHotwordHardware)
  },
#if defined(ENABLE_EXTENSIONS)
  {
    "enable-embedded-extension-options",
    IDS_FLAGS_ENABLE_EMBEDDED_EXTENSION_OPTIONS_NAME,
    IDS_FLAGS_ENABLE_EMBEDDED_EXTENSION_OPTIONS_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(extensions::switches::kEnableEmbeddedExtensionOptions)
  },
#endif
#if defined(USE_ASH)
  {
    "enable-web-app-frame",
    IDS_FLAGS_ENABLE_WEB_APP_FRAME_NAME,
    IDS_FLAGS_ENABLE_WEB_APP_FRAME_DESCRIPTION,
    kOsWin | kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kEnableWebAppFrame)
  },
#endif
  {
    "enable-website-settings-manager",
    IDS_FLAGS_ENABLE_WEBSITE_SETTINGS_NAME,
    IDS_FLAGS_ENABLE_WEBSITE_SETTINGS_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kEnableWebsiteSettingsManager)
  },
  {
    "remember-cert-error-decisions",
    IDS_FLAGS_REMEMBER_CERTIFICATE_ERROR_DECISIONS_NAME,
    IDS_FLAGS_REMEMBER_CERTIFICATE_ERROR_DECISIONS_DESCRIPTION,
    kOsAll,
    MULTI_VALUE_TYPE(kRememberCertificateErrorDecisionsChoices)
  },
  {
    "enable-drop-sync-credential",
    IDS_FLAGS_ENABLE_DROP_SYNC_CREDENTIAL_NAME,
    IDS_FLAGS_ENABLE_DROP_SYNC_CREDENTIAL_DESCRIPTION,
    kOsAll,
    ENABLE_DISABLE_VALUE_TYPE(
        password_manager::switches::kEnableDropSyncCredential,
        password_manager::switches::kDisableDropSyncCredential)
  },
#if defined(ENABLE_EXTENSIONS)
  {
    "enable-extension-action-redesign",
    IDS_FLAGS_ENABLE_EXTENSION_ACTION_REDESIGN_NAME,
    IDS_FLAGS_ENABLE_EXTENSION_ACTION_REDESIGN_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(extensions::switches::kEnableExtensionActionRedesign)
  },
#endif
  {
    "autofill-sync-credential",
    IDS_FLAGS_AUTOFILL_SYNC_CREDENTIAL_NAME,
    IDS_FLAGS_AUTOFILL_SYNC_CREDENTIAL_DESCRIPTION,
    kOsAll,
    MULTI_VALUE_TYPE(kAutofillSyncCredentialChoices)
  },
#if !defined(OS_ANDROID)
  {
    "enable-message-center-always-scroll-up-upon-notification-removal",
    IDS_FLAGS_ENABLE_MESSAGE_CENTER_ALWAYS_SCROLL_UP_UPON_REMOVAL_NAME,
    IDS_FLAGS_ENABLE_MESSAGE_CENTER_ALWAYS_SCROLL_UP_UPON_REMOVAL_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(
        switches::kEnableMessageCenterAlwaysScrollUpUponNotificationRemoval)
  },
  {
    "enable-md-settings",
    IDS_FLAGS_ENABLE_MATERIAL_DESIGN_SETTINGS_NAME,
    IDS_FLAGS_ENABLE_MATERIAL_DESIGN_SETTINGS_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(
        switches::kEnableMaterialDesignSettings)
  },
#endif
#if defined(OS_CHROMEOS)
  {
    "disable-memory-pressure-chromeos",
    IDS_FLAGS_DISABLE_MEMORY_PRESSURE_NAME,
    IDS_FLAGS_DISABLE_MEMORY_PRESSURE_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kDisableMemoryPressureSystemChromeOS)
  },
  {
    "memory-pressure-thresholds",
    IDS_FLAGS_MEMORY_PRESSURE_THRESHOLD_NAME,
    IDS_FLAGS_MEMORY_PRESSURE_THRESHOLD_DESCRIPTION,
    kOsCrOS,
    MULTI_VALUE_TYPE(kMemoryPressureThresholdChoices)
  },
  {
    "wake-on-packets",
    IDS_FLAGS_WAKE_ON_PACKETS_NAME,
    IDS_FLAGS_WAKE_ON_PACKETS_DESCRIPTION,
    kOsCrOSOwnerOnly,
    SINGLE_VALUE_TYPE(chromeos::switches::kWakeOnPackets)
  },
#endif  // OS_CHROMEOS
  {
    "enable-tab-audio-muting",
    IDS_FLAGS_ENABLE_TAB_AUDIO_MUTING_NAME,
    IDS_FLAGS_ENABLE_TAB_AUDIO_MUTING_DESCRIPTION,
    kOsDesktop,
    SINGLE_VALUE_TYPE(switches::kEnableTabAudioMuting)
  },
  {
    "enable-credential-manager-api",
    IDS_FLAGS_CREDENTIAL_MANAGER_API_NAME,
    IDS_FLAGS_CREDENTIAL_MANAGER_API_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableCredentialManagerAPI)
  },
  {
    "reduced-referrer-granularity",
    IDS_FLAGS_REDUCED_REFERRER_GRANULARITY_NAME,
    IDS_FLAGS_REDUCED_REFERRER_GRANULARITY_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kReducedReferrerGranularity)
  },
#if defined(ENABLE_PLUGINS)
  {
    "enable-plugin-power-saver",
    IDS_FLAGS_ENABLE_PLUGIN_POWER_SAVER_NAME,
    IDS_FLAGS_ENABLE_PLUGIN_POWER_SAVER_DESCRIPTION,
    kOsDesktop,
    ENABLE_DISABLE_VALUE_TYPE(plugins::switches::kEnablePluginPowerSaver,
                              plugins::switches::kDisablePluginPowerSaver)
  },
#endif
#if defined(OS_CHROMEOS)
  {
    "disable-new-zip-unpacker",
    IDS_FLAGS_DISABLE_NEW_ZIP_UNPACKER_NAME,
    IDS_FLAGS_DISABLE_NEW_ZIP_UNPACKER_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kDisableNewZIPUnpacker)
  },
#endif  // defined(OS_CHROMEOS)
  {
    "enable-credit-card-scan",
    IDS_FLAGS_ENABLE_CREDIT_CARD_SCAN_NAME,
    IDS_FLAGS_ENABLE_CREDIT_CARD_SCAN_DESCRIPTION,
    kOsAndroid,
    ENABLE_DISABLE_VALUE_TYPE(autofill::switches::kEnableCreditCardScan,
                              autofill::switches::kDisableCreditCardScan)
  },
#if defined(OS_CHROMEOS)
  {
    "disable-captive-portal-bypass-proxy",
    IDS_FLAGS_DISABLE_CAPTIVE_PORTAL_BYPASS_PROXY_NAME,
    IDS_FLAGS_DISABLE_CAPTIVE_PORTAL_BYPASS_PROXY_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kDisableCaptivePortalBypassProxy)
  },
#endif  // defined(OS_CHROMEOS)
#if defined(OS_ANDROID)
  {
    "enable-seccomp-filter-sandbox",
    IDS_FLAGS_ENABLE_SECCOMP_FILTER_SANDBOX_ANDROID_NAME,
    IDS_FLAGS_ENABLE_SECCOMP_FILTER_SANDBOX_ANDROID_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kEnableSeccompFilterSandbox)
  },
#endif
  {
    "enable-touch-hover",
    IDS_FLAGS_ENABLE_TOUCH_HOVER_NAME,
    IDS_FLAGS_ENABLE_TOUCH_HOVER_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE("enable-touch-hover")
  },
  {
    "enable-fill-on-account-select",
    IDS_FILL_ON_ACCOUNT_SELECT_NAME,
    IDS_FILL_ON_ACCOUNT_SELECT_DESCRIPTION,
    kOsAll,
    MULTI_VALUE_TYPE(kFillOnAccountSelectChoices)
  },
#if defined(OS_CHROMEOS)
  {
    "enable-wifi-credential-sync",  // FLAGS:RECORD_UMA
    IDS_FLAGS_ENABLE_WIFI_CREDENTIAL_SYNC_NAME,
    IDS_FLAGS_ENABLE_WIFI_CREDENTIAL_SYNC_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kEnableWifiCredentialSync)
  },
  {
    "enable-potentially-annoying-security-features",
    IDS_FLAGS_EXPERIMENTAL_SECURITY_FEATURES_NAME,
    IDS_FLAGS_EXPERIMENTAL_SECURITY_FEATURES_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnablePotentiallyAnnoyingSecurityFeatures)
  },
#endif
  {
    "disable-delay-agnostic-aec",
    IDS_FLAGS_DISABLE_DELAY_AGNOSTIC_AEC_NAME,
    IDS_FLAGS_DISABLE_DELAY_AGNOSTIC_AEC_DESCRIPTION,
    kOsWin | kOsLinux | kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kDisableDelayAgnosticAec)
  },
  {
    "enable-delay-agnostic-aec",
    IDS_FLAGS_ENABLE_DELAY_AGNOSTIC_AEC_NAME,
    IDS_FLAGS_ENABLE_DELAY_AGNOSTIC_AEC_DESCRIPTION,
    kOsMac,
    SINGLE_VALUE_TYPE(switches::kEnableDelayAgnosticAec)
  },
  {
    "mark-non-secure-as",  // FLAGS:RECORD_UMA
     IDS_MARK_NON_SECURE_AS_NAME,
     IDS_MARK_NON_SECURE_AS_DESCRIPTION,
     kOsAll,
     MULTI_VALUE_TYPE(kMarkNonSecureAsChoices)
  },
  {
    "enable-site-per-process",
    IDS_FLAGS_ENABLE_SITE_PER_PROCESS_NAME,
    IDS_FLAGS_ENABLE_SITE_PER_PROCESS_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kSitePerProcess)
  },
#if defined(OS_MACOSX)
  {
    "enable-harfbuzz-rendertext",
    IDS_FLAGS_ENABLE_HARFBUZZ_RENDERTEXT_NAME,
    IDS_FLAGS_ENABLE_HARFBUZZ_RENDERTEXT_DESCRIPTION,
    kOsMac,
    SINGLE_VALUE_TYPE(switches::kEnableHarfBuzzRenderText)
  },
#endif  // defined(OS_MACOSX)
#if defined(OS_CHROMEOS)
  {
    "disable-timezone-tracking",
    IDS_FLAGS_DISABLE_RESOLVE_TIMEZONE_BY_GEOLOCATION_NAME,
    IDS_FLAGS_DISABLE_RESOLVE_TIMEZONE_BY_GEOLOCATION_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kDisableTimeZoneTrackingOption)
  },
  {
    "disable-webview-signin-flow",
    IDS_FLAGS_DISABLE_WEBVIEW_SIGNIN_FLOW_NAME,
    IDS_FLAGS_DISABLE_WEBVIEW_SIGNIN_FLOW_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kDisableWebviewSigninFlow)
  },
#endif  // defined(OS_CHROMEOS)
  {
    "enable-data-reduction-proxy-lo-fi",
    IDS_FLAGS_ENABLE_DATA_REDUCTION_PROXY_LO_FI_NAME,
    IDS_FLAGS_ENABLE_DATA_REDUCTION_PROXY_LO_FI_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(data_reduction_proxy::switches::
                      kEnableDataReductionProxyLoFi)
  },
  {
    "clear-data-reduction-proxy-data-savings",
    IDS_FLAGS_DATA_REDUCTION_PROXY_RESET_SAVINGS_NAME,
    IDS_FLAGS_DATA_REDUCTION_PROXY_RESET_SAVINGS_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(data_reduction_proxy::switches::
                      kClearDataReductionProxyDataSavings)
  },
#if defined(ENABLE_DATA_REDUCTION_PROXY_DEBUGGING)
  {
    "enable-data-reduction-proxy-bypass-warnings",
    IDS_FLAGS_ENABLE_DATA_REDUCTION_PROXY_BYPASS_WARNING_NAME,
    IDS_FLAGS_ENABLE_DATA_REDUCTION_PROXY_BYPASS_WARNING_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE(data_reduction_proxy::switches::
                      kEnableDataReductionProxyBypassWarning)
  },
#endif
  {
    "allow-insecure-localhost",
    IDS_ALLOW_INSECURE_LOCALHOST,
    IDS_ALLOW_INSECURE_LOCALHOST_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kAllowInsecureLocalhost)
  },
  {
    "bypass-app-banner-engagement-checks",
    IDS_FLAGS_BYPASS_APP_BANNER_ENGAGEMENT_CHECKS_NAME,
    IDS_FLAGS_BYPASS_APP_BANNER_ENGAGEMENT_CHECKS_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kBypassAppBannerEngagementChecks)
  },
  {
    "use-sync-sandbox",
    IDS_FLAGS_SYNC_SANDBOX_NAME,
    IDS_FLAGS_SYNC_SANDBOX_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE_AND_VALUE(
        switches::kSyncServiceURL,
        "https://chrome-sync.sandbox.google.com/chrome-sync/alpha")
  },
  {
    "enable-child-account-detection",
    IDS_FLAGS_CHILD_ACCOUNT_DETECTION_NAME,
    IDS_FLAGS_CHILD_ACCOUNT_DETECTION_DESCRIPTION,
    kOsAndroid | kOsMac | kOsWin | kOsLinux | kOsCrOS,
    ENABLE_DISABLE_VALUE_TYPE(switches::kEnableChildAccountDetection,
                              switches::kDisableChildAccountDetection)
  },
#if defined(OS_CHROMEOS) && defined(USE_OZONE)
  {
    "ozone-test-single-overlay-support",
    IDS_FLAGS_OZONE_TEST_SINGLE_HARDWARE_OVERLAY,
    IDS_FLAGS_OZONE_TEST_SINGLE_HARDWARE_OVERLAY_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(switches::kOzoneTestSingleOverlaySupport)
  },
#endif  // defined(OS_CHROMEOS) && defined(USE_OZONE)
  {
    "v8-pac-mojo-out-of-process",
    IDS_FLAGS_V8_PAC_MOJO_OUT_OF_PROCESS_NAME,
    IDS_FLAGS_V8_PAC_MOJO_OUT_OF_PROCESS_DESCRIPTION,
    kOsDesktop,
    ENABLE_DISABLE_VALUE_TYPE(switches::kV8PacMojoOutOfProcess,
                              switches::kDisableOutOfProcessPac)
  },
#if defined(ENABLE_MEDIA_ROUTER)
  {
    "enable-media-router",
    IDS_FLAGS_ENABLE_MEDIA_ROUTER_NAME,
    IDS_FLAGS_ENABLE_MEDIA_ROUTER_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableMediaRouter)
  },
#endif  // defined(ENABLE_MEDIA_ROUTER)
// Since kEnableLauncherSearchProviderApi is not available when app list is
// disabled, flag guard enable-launcher-search-provider-api.
#if defined(ENABLE_APP_LIST)
  {
    "enable-launcher-search-provider-api",
    IDS_FLAGS_ENABLE_LAUNCHER_SEARCH_PROVIDER_API,
    IDS_FLAGS_ENABLE_LAUNCHER_SEARCH_PROVIDER_API_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(app_list::switches::kEnableLauncherSearchProviderApi)
  },
#endif  // defined(ENABLE_APP_LIST)
#if defined(OS_CHROMEOS)
  {
    "disable-mtp-write-support",
    IDS_FLAG_DISABLE_MTP_WRITE_SUPPORT_NAME,
    IDS_FLAG_DISABLE_MTP_WRITE_SUPPORT_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kDisableMtpWriteSupport)
  },
#endif  // defined(OS_CHROMEOS)
#if defined(OS_CHROMEOS)
  {
    "enable-datasaver-prompt",
    IDS_FLAGS_DATASAVER_PROMPT_NAME,
    IDS_FLAGS_DATASAVER_PROMPT_DESCRIPTION,
    kOsCrOS,
    MULTI_VALUE_TYPE(kDataSaverPromptChoices)
  },
#endif  // defined(OS_CHROMEOS)
  {
    "supervised-user-safesites",
    IDS_FLAGS_SUPERVISED_USER_SAFESITES_NAME,
    IDS_FLAGS_SUPERVISED_USER_SAFESITES_DESCRIPTION,
    kOsAndroid | kOsMac | kOsWin | kOsLinux | kOsCrOS,
    MULTI_VALUE_TYPE(kSupervisedUserSafeSitesChoices)
  },
#if defined(OS_ANDROID)
  {
    "enable-autofill-keyboard-accessory-view",
    IDS_FLAGS_AUTOFILL_ACCESSORY_VIEW_NAME,
    IDS_FLAGS_AUTOFILL_ACCESSORY_VIEW_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE(autofill::switches::kEnableAccessorySuggestionView)
  },
#endif  // defined(OS_ANDROID)
  {
    "disable-new-video-renderer",
    IDS_FLAGS_DISABLE_NEW_VIDEO_RENDERER_NAME,
    IDS_FLAGS_DISABLE_NEW_VIDEO_RENDERER_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kDisableNewVideoRenderer)
  },
#if defined(OS_CHROMEOS)
  {
    "enable-printer-app-search",
    IDS_FLAGS_PRINTER_PROVIDER_SEARCH_APP_NAME,
    IDS_FLAGS_PRINTER_PROVIDER_SEARCH_APP_DESCRIPTION,
    kOsCrOS,
    SINGLE_VALUE_TYPE(chromeos::switches::kEnablePrinterAppSearch)
  },
#endif  // OS_CHROMEOS
  // Temporary flag to ease the transition to standard-compliant scrollTop
  // behavior.  Will be removed shortly after http://crbug.com/157855 ships.
  {
    "scroll-top-left-interop",
    IDS_FLAGS_SCROLL_TOP_LEFT_INTEROP_NAME,
    IDS_FLAGS_SCROLL_TOP_LEFT_INTEROP_DESCRIPTION,
    kOsAll,
    ENABLE_DISABLE_VALUE_TYPE_AND_VALUE(
        switches::kEnableBlinkFeatures, "ScrollTopLeftInterop",
        switches::kDisableBlinkFeatures, "ScrollTopLeftInterop")
  },
#if defined(OS_WIN)
  {
    "try-supported-channel-layouts",
    IDS_FLAGS_TRY_SUPPORTED_CHANNEL_LAYOUTS_NAME,
    IDS_FLAGS_TRY_SUPPORTED_CHANNEL_LAYOUTS_DESCRIPTION,
    kOsWin,
    SINGLE_VALUE_TYPE(switches::kTrySupportedChannelLayouts)
  },
#endif
  // NOTE: Adding new command-line switches requires adding corresponding
  // entries to enum "LoginCustomFlags" in histograms.xml. See note in
  // histograms.xml and don't forget to run AboutFlagsHistogramTest unit test.
};

const Experiment* experiments = kExperiments;
size_t num_experiments = arraysize(kExperiments);

// Stores and encapsulates the little state that about:flags has.
class FlagsState {
 public:
  FlagsState() : needs_restart_(false) {}
  void ConvertFlagsToSwitches(FlagsStorage* flags_storage,
                              base::CommandLine* command_line,
                              SentinelsMode sentinels);
  bool IsRestartNeededToCommitChanges();
  void SetExperimentEnabled(
      FlagsStorage* flags_storage,
      const std::string& internal_name,
      bool enable);
  void RemoveFlagsSwitches(
      std::map<std::string, base::CommandLine::StringType>* switch_list);
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

  std::set<std::string> new_enabled_experiments =
      base::STLSetIntersection<std::set<std::string> >(
          known_experiments, enabled_experiments);

  if (new_enabled_experiments != enabled_experiments)
    flags_storage->SetFlags(new_enabled_experiments);
}

void GetSanitizedEnabledFlags(
    FlagsStorage* flags_storage, std::set<std::string>* result) {
  SanitizeList(flags_storage);
  *result = flags_storage->GetFlags();
}

bool SkipConditionalExperiment(const Experiment& experiment,
                               FlagsStorage* flags_storage) {
#if defined(OS_ANDROID) || defined(ENABLE_DATA_REDUCTION_PROXY_DEBUGGING)
  chrome::VersionInfo::Channel channel = chrome::VersionInfo::GetChannel();
#endif

#if defined(OS_ANDROID)
  // enable-data-reduction-proxy-dev is only available for the Dev/Beta channel.
  if (!strcmp("enable-data-reduction-proxy-dev", experiment.internal_name) &&
      channel != chrome::VersionInfo::CHANNEL_BETA &&
      channel != chrome::VersionInfo::CHANNEL_DEV) {
    return true;
  }
  // enable-data-reduction-proxy-alt is only available for the Dev channel.
  if (!strcmp("enable-data-reduction-proxy-alt", experiment.internal_name) &&
      channel != chrome::VersionInfo::CHANNEL_DEV) {
    return true;
  }
  // enable-data-reduction-proxy-lo-fi is only available for Chromium builds and
  // the Canary/Dev channel.
  if (!strcmp("enable-data-reduction-proxy-lo-fi", experiment.internal_name) &&
      channel != chrome::VersionInfo::CHANNEL_DEV &&
      channel != chrome::VersionInfo::CHANNEL_CANARY &&
      channel != chrome::VersionInfo::CHANNEL_UNKNOWN) {
    return true;
  }
#endif

#if defined(ENABLE_DATA_REDUCTION_PROXY_DEBUGGING)
  // enable-data-reduction-proxy-bypass-warning is only available for Chromium
  // builds and Canary/Dev channel.
  if (!strcmp("enable-data-reduction-proxy-bypass-warnings",
              experiment.internal_name) &&
      channel != chrome::VersionInfo::CHANNEL_UNKNOWN &&
      channel != chrome::VersionInfo::CHANNEL_CANARY &&
      channel != chrome::VersionInfo::CHANNEL_DEV) {
    return true;
  }
#endif

  return false;
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

  std::set<std::string> new_enabled_experiments =
      base::STLSetIntersection<std::set<std::string> >(
          platform_experiments, *result);

  result->swap(new_enabled_experiments);
}

// Returns the Value representing the choice data in the specified experiment.
base::Value* CreateChoiceData(
    const Experiment& experiment,
    const std::set<std::string>& enabled_experiments) {
  DCHECK(experiment.type == Experiment::MULTI_VALUE ||
         experiment.type == Experiment::ENABLE_DISABLE_VALUE);
  base::ListValue* result = new base::ListValue;
  for (int i = 0; i < experiment.num_choices; ++i) {
    base::DictionaryValue* value = new base::DictionaryValue;
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

base::string16 Experiment::DescriptionForChoice(int index) const {
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
                            base::CommandLine* command_line,
                            SentinelsMode sentinels) {
  FlagsState::GetInstance()->ConvertFlagsToSwitches(flags_storage,
                                                    command_line,
                                                    sentinels);
}

bool AreSwitchesIdenticalToCurrentCommandLine(
    const base::CommandLine& new_cmdline,
    const base::CommandLine& active_cmdline,
    std::set<base::CommandLine::StringType>* out_difference) {
  std::set<base::CommandLine::StringType> new_flags =
      ExtractFlagsFromCommandLine(new_cmdline);
  std::set<base::CommandLine::StringType> active_flags =
      ExtractFlagsFromCommandLine(active_cmdline);

  bool result = false;
  // Needed because std::equal doesn't check if the 2nd set is empty.
  if (new_flags.size() == active_flags.size()) {
    result =
        std::equal(new_flags.begin(), new_flags.end(), active_flags.begin());
  }

  if (out_difference && !result) {
    std::set_symmetric_difference(
        new_flags.begin(),
        new_flags.end(),
        active_flags.begin(),
        active_flags.end(),
        std::inserter(*out_difference, out_difference->begin()));
  }

  return result;
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
    if (SkipConditionalExperiment(experiment, flags_storage))
      continue;

    base::DictionaryValue* data = new base::DictionaryValue();
    data->SetString("internal_name", experiment.internal_name);
    data->SetString("name",
                    l10n_util::GetStringUTF16(experiment.visible_name_id));
    data->SetString("description",
                    l10n_util::GetStringUTF16(
                        experiment.visible_description_id));

    base::ListValue* supported_platforms = new base::ListValue();
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
    std::map<std::string, base::CommandLine::StringType>* switch_list) {
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
  for (const std::string& flag : flags) {
    std::string action("AboutFlags_");
    action += flag;
    content::RecordComputedAction(action);
  }
  // Since flag metrics are recorded every startup, add a tick so that the
  // stats can be made meaningful.
  if (flags.size())
    content::RecordAction(base::UserMetricsAction("AboutFlags_StartupTick"));
  content::RecordAction(base::UserMetricsAction("StartupTick"));
}

base::HistogramBase::Sample GetSwitchUMAId(const std::string& switch_name) {
  return static_cast<base::HistogramBase::Sample>(
      metrics::HashMetricName(switch_name));
}

void ReportCustomFlags(const std::string& uma_histogram_hame,
                       const std::set<std::string>& command_line_difference) {
  for (const std::string& flag : command_line_difference) {
    int uma_id = about_flags::testing::kBadSwitchFormatHistogramId;
    if (StartsWithASCII(flag, "--", true /* case_sensitive */)) {
      // Skip '--' before switch name.
      std::string switch_name(flag.substr(2));

      // Kill value, if any.
      const size_t value_pos = switch_name.find('=');
      if (value_pos != std::string::npos)
        switch_name.resize(value_pos);

      uma_id = GetSwitchUMAId(switch_name);
    } else {
      NOTREACHED() << "ReportCustomFlags(): flag '" << flag
                   << "' has incorrect format.";
    }
    DVLOG(1) << "ReportCustomFlags(): histogram='" << uma_histogram_hame
             << "' '" << flag << "', uma_id=" << uma_id;

    // Sparse histogram macro does not cache the histogram, so it's safe
    // to use macro with non-static histogram name here.
    UMA_HISTOGRAM_SPARSE_SLOWLY(uma_histogram_hame, uma_id);
  }
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
                                        base::CommandLine* command_line,
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
  for (const std::string& experiment_name : enabled_experiments) {
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
    std::map<std::string, base::CommandLine::StringType>* switch_list) {
  for (const auto& entry : flags_switches_)
    switch_list->erase(entry.first);
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

const base::HistogramBase::Sample kBadSwitchFormatHistogramId = 0;

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
