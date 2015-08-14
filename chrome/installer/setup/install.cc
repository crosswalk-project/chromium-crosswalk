// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/setup/install.h"

#include <windows.h>
#include <shlobj.h>
#include <time.h>

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/shortcut.h"
#include "base/win/windows_version.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/installer/setup/install_worker.h"
#include "chrome/installer/setup/setup_constants.h"
#include "chrome/installer/setup/setup_util.h"
#include "chrome/installer/setup/update_active_setup_version_work_item.h"
#include "chrome/installer/util/auto_launch_util.h"
#include "chrome/installer/util/beacons.h"
#include "chrome/installer/util/browser_distribution.h"
#include "chrome/installer/util/create_reg_key_work_item.h"
#include "chrome/installer/util/delete_after_reboot_helper.h"
#include "chrome/installer/util/google_update_constants.h"
#include "chrome/installer/util/helper.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/master_preferences.h"
#include "chrome/installer/util/master_preferences_constants.h"
#include "chrome/installer/util/set_reg_value_work_item.h"
#include "chrome/installer/util/shell_util.h"
#include "chrome/installer/util/util_constants.h"
#include "chrome/installer/util/work_item_list.h"


namespace {

void LogShortcutOperation(ShellUtil::ShortcutLocation location,
                          BrowserDistribution* dist,
                          const ShellUtil::ShortcutProperties& properties,
                          ShellUtil::ShortcutOperation operation,
                          bool failed) {
  // ShellUtil::SHELL_SHORTCUT_UPDATE_EXISTING should not be used at install and
  // thus this method does not handle logging a message for it.
  DCHECK(operation != ShellUtil::SHELL_SHORTCUT_UPDATE_EXISTING);
  std::string message;
  if (failed)
    message.append("Failed: ");
  message.append(
      (operation == ShellUtil::SHELL_SHORTCUT_CREATE_ALWAYS ||
       operation == ShellUtil::SHELL_SHORTCUT_CREATE_IF_NO_SYSTEM_LEVEL) ?
      "Creating " : "Overwriting ");
  if (failed && operation == ShellUtil::SHELL_SHORTCUT_REPLACE_EXISTING)
    message.append("(maybe the shortcut doesn't exist?) ");
  message.append((properties.level == ShellUtil::CURRENT_USER) ? "per-user " :
                                                                 "all-users ");
  switch (location) {
    case ShellUtil::SHORTCUT_LOCATION_DESKTOP:
      message.append("Desktop ");
      break;
    case ShellUtil::SHORTCUT_LOCATION_QUICK_LAUNCH:
      message.append("Quick Launch ");
      break;
    case ShellUtil::SHORTCUT_LOCATION_START_MENU_CHROME_DIR:
      message.append("Start menu/" +
                     base::UTF16ToUTF8(dist->GetStartMenuShortcutSubfolder(
                                     BrowserDistribution::SUBFOLDER_CHROME)) +
                      " ");
      break;
    case ShellUtil::SHORTCUT_LOCATION_START_MENU_CHROME_APPS_DIR:
      message.append("Start menu/" +
                     base::UTF16ToUTF8(dist->GetStartMenuShortcutSubfolder(
                                     BrowserDistribution::SUBFOLDER_APPS)) +
                     " ");
      break;
    default:
      NOTREACHED();
  }

  message.push_back('"');
  if (properties.has_shortcut_name())
    message.append(base::UTF16ToUTF8(properties.shortcut_name));
  else
    message.append(base::UTF16ToUTF8(dist->GetDisplayName()));
  message.push_back('"');

  message.append(" shortcut to ");
  message.append(base::UTF16ToUTF8(properties.target.value()));
  if (properties.has_arguments())
    message.append(base::UTF16ToUTF8(properties.arguments));

  if (properties.pin_to_taskbar &&
      base::win::GetVersion() >= base::win::VERSION_WIN7) {
    message.append(" and pinning to the taskbar.");
  } else {
    message.push_back('.');
  }

  if (failed)
    LOG(WARNING) << message;
  else
    VLOG(1) << message;
}

void ExecuteAndLogShortcutOperation(
    ShellUtil::ShortcutLocation location,
    BrowserDistribution* dist,
    const ShellUtil::ShortcutProperties& properties,
    ShellUtil::ShortcutOperation operation) {
  LogShortcutOperation(location, dist, properties, operation, false);
  if (!ShellUtil::CreateOrUpdateShortcut(location, dist, properties,
                                         operation)) {
    LogShortcutOperation(location, dist, properties, operation, true);
  }
}

void AddChromeToMediaPlayerList() {
  base::string16 reg_path(installer::kMediaPlayerRegPath);
  // registry paths can also be appended like file system path
  reg_path.push_back(base::FilePath::kSeparators[0]);
  reg_path.append(installer::kChromeExe);
  VLOG(1) << "Adding Chrome to Media player list at " << reg_path;
  scoped_ptr<WorkItem> work_item(WorkItem::CreateCreateRegKeyWorkItem(
      HKEY_LOCAL_MACHINE, reg_path, WorkItem::kWow64Default));

  // if the operation fails we log the error but still continue
  if (!work_item.get()->Do())
    LOG(ERROR) << "Could not add Chrome to media player inclusion list.";
}

// Copy master_preferences file provided to installer, in the same folder
// as chrome.exe so Chrome first run can find it. This function will be called
// only on the first install of Chrome.
void CopyPreferenceFileForFirstRun(
    const installer::InstallerState& installer_state,
    const base::FilePath& prefs_source_path) {
  base::FilePath prefs_dest_path(installer_state.target_path().AppendASCII(
      installer::kDefaultMasterPrefs));
  if (!base::CopyFile(prefs_source_path, prefs_dest_path)) {
    VLOG(1) << "Failed to copy master preferences from:"
            << prefs_source_path.value() << " gle: " << ::GetLastError();
  }
}

// This function installs a new version of Chrome to the specified location.
//
// setup_path: Path to the executable (setup.exe) as it will be copied
//           to Chrome install folder after install is complete
// archive_path: Path to the archive (chrome.7z) as it will be copied
//               to Chrome install folder after install is complete
// src_path: the path that contains a complete and unpacked Chrome package
//           to be installed.
// temp_path: the path of working directory used during installation. This path
//            does not need to exist.
// new_version: new Chrome version that needs to be installed
// current_version: returns the current active version (if any)
//
// This function makes best effort to do installation in a transactional
// manner. If failed it tries to rollback all changes on the file system
// and registry. For example, if package exists before calling the
// function, it rolls back all new file and directory changes under
// package. If package does not exist before calling the function
// (typical new install), the function creates package during install
// and removes the whole directory during rollback.
installer::InstallStatus InstallNewVersion(
    const installer::InstallationState& original_state,
    const installer::InstallerState& installer_state,
    const base::FilePath& setup_path,
    const base::FilePath& archive_path,
    const base::FilePath& src_path,
    const base::FilePath& temp_path,
    const Version& new_version,
    scoped_ptr<Version>* current_version) {
  DCHECK(current_version);

  installer_state.UpdateStage(installer::BUILDING);

  current_version->reset(installer_state.GetCurrentVersion(original_state));
  scoped_ptr<WorkItemList> install_list(WorkItem::CreateWorkItemList());

  AddInstallWorkItems(original_state,
                      installer_state,
                      setup_path,
                      archive_path,
                      src_path,
                      temp_path,
                      current_version->get(),
                      new_version,
                      install_list.get());

  base::FilePath new_chrome_exe(
      installer_state.target_path().Append(installer::kChromeNewExe));

  installer_state.UpdateStage(installer::EXECUTING);

  if (!install_list->Do()) {
    installer_state.UpdateStage(installer::ROLLINGBACK);
    installer::InstallStatus result =
        base::PathExists(new_chrome_exe) && current_version->get() &&
        new_version.Equals(*current_version->get()) ?
        installer::SAME_VERSION_REPAIR_FAILED :
        installer::INSTALL_FAILED;
    LOG(ERROR) << "Install failed, rolling back... result: " << result;
    install_list->Rollback();
    LOG(ERROR) << "Rollback complete. ";
    return result;
  }

  installer_state.UpdateStage(installer::REFRESHING_POLICY);

  installer::RefreshElevationPolicy();

  if (!current_version->get()) {
    VLOG(1) << "First install of version " << new_version.GetString();
    return installer::FIRST_INSTALL_SUCCESS;
  }

  if (new_version.Equals(**current_version)) {
    VLOG(1) << "Install repaired of version " << new_version.GetString();
    return installer::INSTALL_REPAIRED;
  }

  if (new_version.CompareTo(**current_version) > 0) {
    if (base::PathExists(new_chrome_exe)) {
      VLOG(1) << "Version updated to " << new_version.GetString()
              << " while running " << (*current_version)->GetString();
      return installer::IN_USE_UPDATED;
    }
    VLOG(1) << "Version updated to " << new_version.GetString();
    return installer::NEW_VERSION_UPDATED;
  }

  LOG(ERROR) << "Not sure how we got here while updating"
             << ", new version: " << new_version.GetString()
             << ", old version: " << (*current_version)->GetString();

  return installer::INSTALL_FAILED;
}

// Deletes the old "Uninstall Google Chrome" shortcut in the Start menu which
// was installed prior to Chrome 24.
void CleanupLegacyShortcuts(const installer::InstallerState& installer_state,
                            BrowserDistribution* dist,
                            const base::FilePath& chrome_exe) {
  ShellUtil::ShellChange shortcut_level = installer_state.system_install() ?
      ShellUtil::SYSTEM_LEVEL : ShellUtil::CURRENT_USER;
  base::FilePath uninstall_shortcut_path;
  ShellUtil::GetShortcutPath(ShellUtil::SHORTCUT_LOCATION_START_MENU_CHROME_DIR,
                             dist, shortcut_level, &uninstall_shortcut_path);
  uninstall_shortcut_path = uninstall_shortcut_path.Append(
      dist->GetUninstallLinkName() + installer::kLnkExt);
  base::DeleteFile(uninstall_shortcut_path, false);
}

}  // end namespace

namespace installer {

void EscapeXmlAttributeValueInSingleQuotes(base::string16* att_value) {
  base::ReplaceChars(*att_value, base::ASCIIToUTF16("&"),
                     base::ASCIIToUTF16("&amp;"), att_value);
  base::ReplaceChars(*att_value, base::ASCIIToUTF16("'"),
                     base::ASCIIToUTF16("&apos;"), att_value);
  base::ReplaceChars(*att_value, base::ASCIIToUTF16("<"),
                     base::ASCIIToUTF16("&lt;"), att_value);
}

bool CreateVisualElementsManifest(const base::FilePath& src_path,
                                  const Version& version) {
  // Construct the relative path to the versioned VisualElements directory.
  base::string16 elements_dir(base::ASCIIToUTF16(version.GetString()));
  elements_dir.push_back(base::FilePath::kSeparators[0]);
  elements_dir.append(installer::kVisualElements);

  // Some distributions of Chromium may not include visual elements. Only
  // proceed if this distribution does.
  if (!base::PathExists(src_path.Append(elements_dir))) {
    VLOG(1) << "No visual elements found, not writing "
            << installer::kVisualElementsManifest << " to " << src_path.value();
    return true;
  } else {
    // A printf-style format string for generating the visual elements
    // manifest. Required arguments, in order, are:
    //   - Localized display name for the product.
    //   - Relative path to the VisualElements directory, three times.
    static const char kManifestTemplate[] =
        "<Application>\r\n"
        "  <VisualElements\r\n"
        "      DisplayName='%ls'\r\n"
        "      Logo='%ls\\Logo.png'\r\n"
        "      SmallLogo='%ls\\SmallLogo.png'\r\n"
        "      ForegroundText='light'\r\n"
        "      BackgroundColor='#323232'>\r\n"
        "    <DefaultTile ShowName='allLogos'/>\r\n"
        "    <SplashScreen Image='%ls\\splash-620x300.png'/>\r\n"
        "  </VisualElements>\r\n"
        "</Application>";

    const base::string16 manifest_template(
        base::ASCIIToUTF16(kManifestTemplate));

    BrowserDistribution* dist = BrowserDistribution::GetSpecificDistribution(
        BrowserDistribution::CHROME_BROWSER);
    // TODO(grt): http://crbug.com/75152 Write a reference to a localized
    // resource for |display_name|.
    base::string16 display_name(dist->GetDisplayName());
    EscapeXmlAttributeValueInSingleQuotes(&display_name);

    // Fill the manifest with the desired values.
    base::string16 manifest16(base::StringPrintf(
        manifest_template.c_str(), display_name.c_str(), elements_dir.c_str(),
        elements_dir.c_str(), elements_dir.c_str()));

    // Write the manifest to |src_path|.
    const std::string manifest(base::UTF16ToUTF8(manifest16));
    int size = base::checked_cast<int>(manifest.size());
    if (base::WriteFile(
        src_path.Append(installer::kVisualElementsManifest),
            manifest.c_str(), size) == size) {
      VLOG(1) << "Successfully wrote " << installer::kVisualElementsManifest
              << " to " << src_path.value();
      return true;
    } else {
      PLOG(ERROR) << "Error writing " << installer::kVisualElementsManifest
                  << " to " << src_path.value();
      return false;
    }
  }
}

void CreateOrUpdateShortcuts(
    const base::FilePath& target,
    const installer::Product& product,
    const MasterPreferences& prefs,
    InstallShortcutLevel install_level,
    InstallShortcutOperation install_operation) {
  bool do_not_create_any_shortcuts = false;
  prefs.GetBool(master_preferences::kDoNotCreateAnyShortcuts,
                &do_not_create_any_shortcuts);
  if (do_not_create_any_shortcuts)
    return;

  // Extract shortcut preferences from |prefs|.
  bool do_not_create_desktop_shortcut = false;
  bool do_not_create_quick_launch_shortcut = false;
  bool do_not_create_taskbar_shortcut = false;
  bool alternate_desktop_shortcut = false;
  prefs.GetBool(master_preferences::kDoNotCreateDesktopShortcut,
                &do_not_create_desktop_shortcut);
  prefs.GetBool(master_preferences::kDoNotCreateQuickLaunchShortcut,
                &do_not_create_quick_launch_shortcut);
  prefs.GetBool(master_preferences::kDoNotCreateTaskbarShortcut,
                &do_not_create_taskbar_shortcut);
  prefs.GetBool(master_preferences::kAltShortcutText,
                &alternate_desktop_shortcut);

  BrowserDistribution* dist = product.distribution();

  // The default operation on update is to overwrite shortcuts with the
  // currently desired properties, but do so only for shortcuts that still
  // exist.
  ShellUtil::ShortcutOperation shortcut_operation;
  switch (install_operation) {
    case INSTALL_SHORTCUT_CREATE_ALL:
      shortcut_operation = ShellUtil::SHELL_SHORTCUT_CREATE_ALWAYS;
      break;
    case INSTALL_SHORTCUT_CREATE_EACH_IF_NO_SYSTEM_LEVEL:
      shortcut_operation = ShellUtil::SHELL_SHORTCUT_CREATE_IF_NO_SYSTEM_LEVEL;
      break;
    default:
      DCHECK(install_operation == INSTALL_SHORTCUT_REPLACE_EXISTING);
      shortcut_operation = ShellUtil::SHELL_SHORTCUT_REPLACE_EXISTING;
      break;
  }

  // Shortcuts are always installed per-user unless specified.
  ShellUtil::ShellChange shortcut_level = (install_level == ALL_USERS ?
      ShellUtil::SYSTEM_LEVEL : ShellUtil::CURRENT_USER);

  // |base_properties|: The basic properties to set on every shortcut installed
  // (to be refined on a per-shortcut basis).
  ShellUtil::ShortcutProperties base_properties(shortcut_level);
  product.AddDefaultShortcutProperties(target, &base_properties);

  if (!do_not_create_desktop_shortcut ||
      shortcut_operation == ShellUtil::SHELL_SHORTCUT_REPLACE_EXISTING) {
    ShellUtil::ShortcutProperties desktop_properties(base_properties);
    if (alternate_desktop_shortcut) {
      desktop_properties.set_shortcut_name(
          dist->GetShortcutName(
              BrowserDistribution::SHORTCUT_CHROME_ALTERNATE));
    }
    ExecuteAndLogShortcutOperation(
        ShellUtil::SHORTCUT_LOCATION_DESKTOP, dist, desktop_properties,
        shortcut_operation);

    // On update there is no harm in always trying to update the alternate
    // Desktop shortcut.
    if (!alternate_desktop_shortcut &&
        shortcut_operation == ShellUtil::SHELL_SHORTCUT_REPLACE_EXISTING) {
      desktop_properties.set_shortcut_name(
          dist->GetShortcutName(
              BrowserDistribution::SHORTCUT_CHROME_ALTERNATE));
      ExecuteAndLogShortcutOperation(
          ShellUtil::SHORTCUT_LOCATION_DESKTOP, dist, desktop_properties,
          shortcut_operation);
    }
  }

  if (!do_not_create_quick_launch_shortcut ||
      shortcut_operation == ShellUtil::SHELL_SHORTCUT_REPLACE_EXISTING) {
    // There is no such thing as an all-users Quick Launch shortcut, always
    // install the per-user shortcut.
    ShellUtil::ShortcutProperties quick_launch_properties(base_properties);
    quick_launch_properties.level = ShellUtil::CURRENT_USER;
    ExecuteAndLogShortcutOperation(
        ShellUtil::SHORTCUT_LOCATION_QUICK_LAUNCH, dist,
        quick_launch_properties, shortcut_operation);
  }

  ShellUtil::ShortcutProperties start_menu_properties(base_properties);
  // IMPORTANT: Only the default (no arguments and default browserappid) browser
  // shortcut in the Start menu (Start screen on Win8+) should be considered for
  // dual mode.
  if (InstallUtil::ShouldInstallMetroProperties())
    start_menu_properties.set_dual_mode(true);
  if (!do_not_create_taskbar_shortcut &&
      (shortcut_operation == ShellUtil::SHELL_SHORTCUT_CREATE_ALWAYS ||
       shortcut_operation ==
           ShellUtil::SHELL_SHORTCUT_CREATE_IF_NO_SYSTEM_LEVEL)) {
    start_menu_properties.set_pin_to_taskbar(true);
  }
  ExecuteAndLogShortcutOperation(
      ShellUtil::SHORTCUT_LOCATION_START_MENU_CHROME_DIR, dist,
      start_menu_properties, shortcut_operation);
}

void RegisterChromeOnMachine(const installer::InstallerState& installer_state,
                             const installer::Product& product,
                             bool make_chrome_default) {
  DCHECK(product.is_chrome());

  // Try to add Chrome to Media Player shim inclusion list. We don't do any
  // error checking here because this operation will fail if user doesn't
  // have admin rights and we want to ignore the error.
  AddChromeToMediaPlayerList();

  // Make Chrome the default browser if desired when possible. Otherwise, only
  // register it with Windows.
  BrowserDistribution* dist = product.distribution();
  const base::FilePath chrome_exe(
      installer_state.target_path().Append(installer::kChromeExe));
  VLOG(1) << "Registering Chrome as browser: " << chrome_exe.value();
  if (make_chrome_default && ShellUtil::CanMakeChromeDefaultUnattended()) {
    int level = ShellUtil::CURRENT_USER;
    if (installer_state.system_install())
      level = level | ShellUtil::SYSTEM_LEVEL;
    ShellUtil::MakeChromeDefault(dist, level, chrome_exe, true);
  } else {
    ShellUtil::RegisterChromeBrowser(dist, chrome_exe, base::string16(), false);
  }
}

InstallStatus InstallOrUpdateProduct(
    const installer::InstallationState& original_state,
    const installer::InstallerState& installer_state,
    const base::FilePath& setup_path,
    const base::FilePath& archive_path,
    const base::FilePath& install_temp_path,
    const base::FilePath& src_path,
    const base::FilePath& prefs_path,
    const MasterPreferences& prefs,
    const Version& new_version) {
  DCHECK(!installer_state.products().empty());

  // TODO(robertshield): Removing the pending on-reboot moves should be done
  // elsewhere.
  // Remove any scheduled MOVEFILE_DELAY_UNTIL_REBOOT entries in the target of
  // this installation. These may have been added during a previous uninstall of
  // the same version.
  LOG_IF(ERROR, !RemoveFromMovesPendingReboot(installer_state.target_path()))
      << "Error accessing pending moves value.";

  // Create VisualElementManifest.xml in |src_path| (if required) so that it
  // looks as if it had been extracted from the archive when calling
  // InstallNewVersion() below.
  installer_state.UpdateStage(installer::CREATING_VISUAL_MANIFEST);
  CreateVisualElementsManifest(src_path, new_version);

  scoped_ptr<Version> existing_version;
  InstallStatus result = InstallNewVersion(original_state, installer_state,
      setup_path, archive_path, src_path, install_temp_path, new_version,
      &existing_version);

  // TODO(robertshield): Everything below this line should instead be captured
  // by WorkItems.
  if (!InstallUtil::GetInstallReturnCode(result)) {
    installer_state.UpdateStage(installer::UPDATING_CHANNELS);

    // Update the modifiers on the channel values for the product(s) being
    // installed and for the binaries in case of multi-install.
    installer_state.UpdateChannels();

    installer_state.UpdateStage(installer::COPYING_PREFERENCES_FILE);

    if (result == FIRST_INSTALL_SUCCESS && !prefs_path.empty())
      CopyPreferenceFileForFirstRun(installer_state, prefs_path);

    installer_state.UpdateStage(installer::CREATING_SHORTCUTS);

    const installer::Product* chrome_product =
        installer_state.FindProduct(BrowserDistribution::CHROME_BROWSER);
    // Creates shortcuts for Chrome.
    if (chrome_product) {
      BrowserDistribution* chrome_dist = chrome_product->distribution();
      const base::FilePath chrome_exe(
          installer_state.target_path().Append(kChromeExe));
      CleanupLegacyShortcuts(installer_state, chrome_dist, chrome_exe);

      // Install per-user shortcuts on user-level installs and all-users
      // shortcuts on system-level installs. Note that Active Setup will take
      // care of installing missing per-user shortcuts on system-level install
      // (i.e., quick launch, taskbar pin, and possibly deleted all-users
      // shortcuts).
      InstallShortcutLevel install_level = installer_state.system_install() ?
          ALL_USERS : CURRENT_USER;

      InstallShortcutOperation install_operation =
          INSTALL_SHORTCUT_REPLACE_EXISTING;
      if (result == installer::FIRST_INSTALL_SUCCESS ||
          result == installer::INSTALL_REPAIRED ||
          !original_state.GetProductState(installer_state.system_install(),
                                          chrome_dist->GetType())) {
        // Always create the shortcuts on a new install, a repair install, and
        // when the Chrome product is being added to the current install.
        install_operation = INSTALL_SHORTCUT_CREATE_ALL;
      }

      CreateOrUpdateShortcuts(chrome_exe, *chrome_product, prefs, install_level,
                              install_operation);
    }

    if (chrome_product) {
      // Register Chrome and, if requested, make Chrome the default browser.
      installer_state.UpdateStage(installer::REGISTERING_CHROME);

      bool make_chrome_default = false;
      prefs.GetBool(master_preferences::kMakeChromeDefault,
                    &make_chrome_default);

      // If this is not the user's first Chrome install, but they have chosen
      // Chrome to become their default browser on the download page, we must
      // force it here because the master_preferences file will not get copied
      // into the build.
      bool force_chrome_default_for_user = false;
      if (result == NEW_VERSION_UPDATED ||
          result == INSTALL_REPAIRED) {
        prefs.GetBool(master_preferences::kMakeChromeDefaultForUser,
                      &force_chrome_default_for_user);
      }

      RegisterChromeOnMachine(installer_state, *chrome_product,
          make_chrome_default || force_chrome_default_for_user);

      // Configure auto-launch.
      if (result == FIRST_INSTALL_SUCCESS) {
        installer_state.UpdateStage(installer::CONFIGURE_AUTO_LAUNCH);

        // Add auto-launch key if specified in master_preferences.
        bool auto_launch_chrome = false;
        prefs.GetBool(
            installer::master_preferences::kAutoLaunchChrome,
            &auto_launch_chrome);
        if (auto_launch_chrome) {
          auto_launch_util::EnableForegroundStartAtLogin(
              base::ASCIIToUTF16(chrome::kInitialProfile),
              installer_state.target_path());
        }
      }

      if (!installer_state.system_install()) {
        DCHECK_EQ(chrome_product->distribution(),
                  BrowserDistribution::GetDistribution());
        UpdateDefaultBrowserBeaconForPath(
            installer_state.target_path().Append(installer::kChromeExe));
      }
    }

    installer_state.UpdateStage(installer::REMOVING_OLD_VERSIONS);

    installer_state.RemoveOldVersionDirectories(
        new_version,
        existing_version.get(),
        install_temp_path);
  }

  return result;
}

void HandleOsUpgradeForBrowser(const installer::InstallerState& installer_state,
                               const installer::Product& chrome) {
  DCHECK(chrome.is_chrome());
  // Upon upgrading to Windows 8, we need to fix Chrome shortcuts and register
  // Chrome, so that Metro Chrome would work if Chrome is the default browser.
  if (base::win::GetVersion() >= base::win::VERSION_WIN8) {
    VLOG(1) << "Updating and registering shortcuts.";
    // Read master_preferences copied beside chrome.exe at install.
    MasterPreferences prefs(
        installer_state.target_path().AppendASCII(kDefaultMasterPrefs));

    // Unfortunately, if this is a system-level install, we can't update the
    // shortcuts of each individual user (this only matters if this is an OS
    // upgrade from XP/Vista to Win7+ as some properties are only set on
    // shortcuts as of Win7).
    // At least attempt to update potentially existing all-users shortcuts.
    InstallShortcutLevel level = installer_state.system_install() ?
        ALL_USERS : CURRENT_USER;
    base::FilePath chrome_exe(installer_state.target_path().Append(kChromeExe));
    CreateOrUpdateShortcuts(chrome_exe, chrome, prefs, level,
                            INSTALL_SHORTCUT_CREATE_EACH_IF_NO_SYSTEM_LEVEL);
    RegisterChromeOnMachine(installer_state, chrome, false);

    UpdateOsUpgradeBeacon(installer_state.system_install(),
                          BrowserDistribution::GetDistribution());

    // Update the per-user default browser beacon. For user-level installs this
    // can be done directly; whereas it requires triggering Active Setup for
    // each user's subsequent login on system-level installs.
    if (!installer_state.system_install()) {
      UpdateDefaultBrowserBeaconForPath(chrome_exe);
    } else {
      UpdateActiveSetupVersionWorkItem active_setup_work_item(
          InstallUtil::GetActiveSetupPath(chrome.distribution()),
          UpdateActiveSetupVersionWorkItem::
              UPDATE_AND_BUMP_OS_UPGRADES_COMPONENT);
      if (active_setup_work_item.Do())
        VLOG(1) << "Bumped Active Setup Version on-os-upgrade.";
      else
        LOG(ERROR) << "Failed to bump Active Setup Version on-os-upgrade.";
    }
  }
}

// NOTE: Should the work done here, on Active Setup, change: kActiveSetupVersion
// in install_worker.cc needs to be increased for Active Setup to invoke this
// again for all users of this install.
void HandleActiveSetupForBrowser(const base::FilePath& installation_root,
                                 const installer::Product& chrome,
                                 bool force) {
  DCHECK(chrome.is_chrome());

  // If the shortcuts are not being forcefully created we may want to forcefully
  // create them anyways if this Active Setup trigger is in response to an OS
  // update.
  force = force || installer::UpdateLastOSUpgradeHandledByActiveSetup(
                       chrome.distribution());

  // Only create shortcuts on Active Setup if the first run sentinel is not
  // present for this user (as some shortcuts used to be installed on first
  // run and this could otherwise re-install shortcuts for users that have
  // already deleted them in the past).
  // Decide whether to create the shortcuts or simply replace existing
  // shortcuts; if the decision is to create them, only shortcuts whose matching
  // all-users shortcut isn't present on the system will be created.
  InstallShortcutOperation install_operation =
      (!force && InstallUtil::IsFirstRunSentinelPresent() ?
           INSTALL_SHORTCUT_REPLACE_EXISTING :
           INSTALL_SHORTCUT_CREATE_EACH_IF_NO_SYSTEM_LEVEL);

  // Read master_preferences copied beside chrome.exe at install.
  MasterPreferences prefs(installation_root.AppendASCII(kDefaultMasterPrefs));
  base::FilePath chrome_exe(installation_root.Append(kChromeExe));
  CreateOrUpdateShortcuts(
      chrome_exe, chrome, prefs, CURRENT_USER, install_operation);

  UpdateDefaultBrowserBeaconForPath(chrome_exe);
}

}  // namespace installer
