// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/bookmarks/enhanced_bookmarks_features.h"

#include "base/command_line.h"
#include "base/prefs/pref_service.h"
#include "chrome/common/chrome_switches.h"
#include "components/variations/variations_associated_data.h"

#if !defined(OS_ANDROID) && !defined(OS_IOS)
#include "extensions/common/features/feature.h"
#include "extensions/common/features/feature_provider.h"
#endif  // !defined(OS_ANDROID) && !defined(OS_IOS)

#if defined(OS_ANDROID)
#include "base/android/build_info.h"
#endif  // defined(OS_ANDROID)

namespace {

const char kFieldTrialName[] = "EnhancedBookmarks";

bool GetBookmarksExperimentExtensionID(std::string* extension_id) {
  *extension_id = variations::GetVariationParamValue(
      kFieldTrialName, "id");
  if (extension_id->empty())
    return false;

#if defined(OS_IOS) || defined(OS_ANDROID)
  return true;
#else
  const extensions::FeatureProvider* feature_provider =
      extensions::FeatureProvider::GetPermissionFeatures();
  extensions::Feature* feature = feature_provider->GetFeature("metricsPrivate");
  return feature && feature->IsIdInWhitelist(*extension_id);
#endif  // defined(OS_IOS) || defined(OS_ANDROID)
}

}  // namespace

bool IsEnhancedBookmarksEnabled() {
  std::string extension_id;
  return IsEnhancedBookmarksEnabled(&extension_id);
}

bool IsEnhancedBookmarksEnabled(std::string* extension_id) {
  // kEnhancedBookmarksExperiment flag could have values "", "1" and "0".
  // "0" - user opted out. "1" is only possible on mobile as desktop needs a
  // extension id that would not be available by just using the flag.

#if defined(OS_ANDROID) || defined(OS_IOS)
  // Tests use command line flag to force enhanced bookmark to be on.
  bool opt_in = base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
                    switches::kEnhancedBookmarksExperiment) == "1";
  if (opt_in)
    return true;
#endif  // defined(OS_ANDROID) || defined(OS_IOS)

  bool opt_out = base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
                     switches::kEnhancedBookmarksExperiment) == "0";
#if defined(OS_ANDROID)
  opt_out |= base::android::BuildInfo::GetInstance()->sdk_int() <
                 base::android::SdkVersion::SDK_VERSION_ICE_CREAM_SANDWICH_MR1;
#endif  // defined(OS_ANDROID)

  if (opt_out)
    return false;

  return GetBookmarksExperimentExtensionID(extension_id);
}

bool IsEnableDomDistillerSet() {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableDomDistiller)) {
    return true;
  }
  if (variations::GetVariationParamValue(kFieldTrialName,
                                         "enable-dom-distiller") == "1")
    return true;

  return false;
}

bool IsEnableSyncArticlesSet() {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableSyncArticles)) {
    return true;
  }
  if (variations::GetVariationParamValue(kFieldTrialName,
                                         "enable-sync-articles") == "1")
    return true;

  return false;
}
