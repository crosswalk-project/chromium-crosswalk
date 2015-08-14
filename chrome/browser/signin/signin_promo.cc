// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/signin_promo.h"

#include "base/prefs/pref_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/signin/gaia_auth_extension_loader.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/google/google_brand.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_info_cache.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/signin/account_tracker_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/ui/webui/options/core_options_handler.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "chrome/common/net/url_util.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "components/google/core/browser/google_util.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/signin/core/common/profile_management_switches.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "google_apis/gaia/gaia_urls.h"
#include "net/base/escape.h"
#include "net/base/network_change_notifier.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#endif

using content::WebContents;

namespace {

// The maximum number of times we want to show the sign in promo at startup.
const int kSignInPromoShowAtStartupMaximum = 10;

// Forces the web based signin flow when set.
bool g_force_web_based_signin_flow = false;

// Checks we want to show the sign in promo for the given brand.
bool AllowPromoAtStartupForCurrentBrand() {
  std::string brand;
  google_brand::GetBrand(&brand);

  if (brand.empty())
    return true;

  if (google_brand::IsInternetCafeBrandCode(brand))
    return false;

  // Enable for both organic and distribution.
  return true;
}

// Returns true if a user has seen the sign in promo at startup previously.
bool HasShownPromoAtStartup(Profile* profile) {
  return profile->GetPrefs()->HasPrefPath(prefs::kSignInPromoStartupCount);
}

// Returns true if the user has previously skipped the sign in promo.
bool HasUserSkippedPromo(Profile* profile) {
  return profile->GetPrefs()->GetBoolean(prefs::kSignInPromoUserSkipped);
}

}  // namespace

namespace signin {

bool ShouldShowPromo(Profile* profile) {
#if defined(OS_CHROMEOS)
  // There's no need to show the sign in promo on cros since cros users are
  // already logged in.
  return false;
#else

  // Don't bother if we don't have any kind of network connection.
  if (net::NetworkChangeNotifier::IsOffline())
    return false;

  // Don't show for supervised profiles.
  if (profile->IsSupervised())
    return false;

  // Display the signin promo if the user is not signed in.
  SigninManager* signin = SigninManagerFactory::GetForProfile(
      profile->GetOriginalProfile());
  return !signin->AuthInProgress() && signin->IsSigninAllowed() &&
      !signin->IsAuthenticated();
#endif
}

bool ShouldShowPromoAtStartup(Profile* profile, bool is_new_profile) {
  DCHECK(profile);

  // Don't show if the profile is an incognito.
  if (profile->IsOffTheRecord())
    return false;

  if (!ShouldShowPromo(profile))
    return false;

  if (!is_new_profile) {
    if (!HasShownPromoAtStartup(profile))
      return false;
  }

#if defined(OS_WIN)
  // Do not show the promo on first run on Win10 and newer.
  if (is_new_profile && base::win::GetVersion() >= base::win::VERSION_WIN10)
    return false;
#endif

  if (HasUserSkippedPromo(profile))
    return false;

  // For Chinese users skip the sign in promo.
  if (g_browser_process->GetApplicationLocale() == "zh-CN")
    return false;

  PrefService* prefs = profile->GetPrefs();
  int show_count = prefs->GetInteger(prefs::kSignInPromoStartupCount);
  if (show_count >= kSignInPromoShowAtStartupMaximum)
    return false;

  // This pref can be set in the master preferences file to allow or disallow
  // showing the sign in promo at startup.
  if (prefs->HasPrefPath(prefs::kSignInPromoShowOnFirstRunAllowed))
    return prefs->GetBoolean(prefs::kSignInPromoShowOnFirstRunAllowed);

  // For now don't show the promo for some brands.
  if (!AllowPromoAtStartupForCurrentBrand())
    return false;

  // Default to show the promo for Google Chrome builds.
#if defined(GOOGLE_CHROME_BUILD)
  return true;
#else
  return false;
#endif
}

void DidShowPromoAtStartup(Profile* profile) {
  int show_count = profile->GetPrefs()->GetInteger(
      prefs::kSignInPromoStartupCount);
  show_count++;
  profile->GetPrefs()->SetInteger(prefs::kSignInPromoStartupCount, show_count);
}

void SetUserSkippedPromo(Profile* profile) {
  profile->GetPrefs()->SetBoolean(prefs::kSignInPromoUserSkipped, true);
}

GURL GetLandingURL(const char* option, int value) {
  std::string url = base::StringPrintf("%s/success.html?%s=%d",
                                       extensions::kGaiaAuthExtensionOrigin,
                                       option,
                                       value);
  return GURL(url);
}

GURL GetPromoURL(signin_metrics::Source source, bool auto_close) {
  return GetPromoURL(source, auto_close, false /* is_constrained */);
}

GURL GetPromoURL(signin_metrics::Source source,
                 bool auto_close,
                 bool is_constrained) {
  DCHECK_NE(signin_metrics::SOURCE_UNKNOWN, source);

  std::string url(chrome::kChromeUIChromeSigninURL);
  base::StringAppendF(&url, "?%s=%d", kSignInPromoQueryKeySource, source);
  if (auto_close)
    base::StringAppendF(&url, "&%s=1", kSignInPromoQueryKeyAutoClose);
  if (is_constrained)
    base::StringAppendF(&url, "&%s=1", kSignInPromoQueryKeyConstrained);
  return GURL(url);
}

GURL GetReauthURL(Profile* profile, const std::string& account_id) {
  AccountTrackerService::AccountInfo info =
      AccountTrackerServiceFactory::GetForProfile(profile)->
          GetAccountInfo(account_id);

  signin_metrics::Source source = switches::IsNewAvatarMenu() ?
      signin_metrics::SOURCE_REAUTH : signin_metrics::SOURCE_SETTINGS;

  GURL url = signin::GetPromoURL(
      source, true /* auto_close */,
      switches::IsNewAvatarMenu() /* is_constrained */);
  url = net::AppendQueryParameter(url, "email", info.email);
  url = net::AppendQueryParameter(url, "validateEmail", "1");
  return net::AppendQueryParameter(url, "readOnlyEmail", "1");
}

GURL GetNextPageURLForPromoURL(const GURL& url) {
  std::string value;
  if (net::GetValueForKeyInQuery(url, kSignInPromoQueryKeyContinue, &value)) {
    GURL continue_url = GURL(value);
    if (continue_url.is_valid())
      return continue_url;
  }

  return GURL();
}

GURL GetSigninPartitionURL() {
  return GURL("chrome-guest://chrome-signin/?");
}

signin_metrics::Source GetSourceForPromoURL(const GURL& url) {
  std::string value;
  if (net::GetValueForKeyInQuery(url, kSignInPromoQueryKeySource, &value)) {
    int source = 0;
    if (base::StringToInt(value, &source) &&
        source >= signin_metrics::SOURCE_START_PAGE &&
        source < signin_metrics::SOURCE_UNKNOWN) {
      return static_cast<signin_metrics::Source>(source);
    }
  }
  return signin_metrics::SOURCE_UNKNOWN;
}

bool IsAutoCloseEnabledInURL(const GURL& url) {
  std::string value;
  if (net::GetValueForKeyInQuery(url, kSignInPromoQueryKeyAutoClose, &value)) {
    int enabled = 0;
    if (base::StringToInt(value, &enabled) && enabled == 1)
      return true;
  }
  return false;
}

bool ShouldShowAccountManagement(const GURL& url) {
  std::string value;
  if (net::GetValueForKeyInQuery(
          url, kSignInPromoQueryKeyShowAccountManagement, &value)) {
    int enabled = 0;
    if (base::StringToInt(value, &enabled) && enabled == 1)
      return true;
  }
  return false;
}

void ForceWebBasedSigninFlowForTesting(bool force) {
  g_force_web_based_signin_flow = force;
}

void RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterIntegerPref(prefs::kSignInPromoStartupCount, 0);
  registry->RegisterBooleanPref(prefs::kSignInPromoUserSkipped, false);
  registry->RegisterBooleanPref(prefs::kSignInPromoShowOnFirstRunAllowed, true);
  registry->RegisterBooleanPref(prefs::kSignInPromoShowNTPBubble, false);
}

}  // namespace signin
