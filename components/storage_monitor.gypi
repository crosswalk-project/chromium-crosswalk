{
  'targets': [
    {
      # Build Chromium's StorageMonitor as a static library so we
      # don't need to reimplement the exact same functionality.
      # This is being worked upstream on:
      # https://codereview.chromium.org/152343005/
      'target_name': 'storage_monitor',
      'type': 'static_library',
      'includes' : [
        '../build/filename_rules.gypi',
      ],
      'defines': [
        'XWALK_STORAGE_MONITOR=1',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        '../chrome/browser/storage_monitor/image_capture_device.h',
        '../chrome/browser/storage_monitor/image_capture_device.mm',
        '../chrome/browser/storage_monitor/image_capture_device_manager.h',
        '../chrome/browser/storage_monitor/image_capture_device_manager.mm',
        '../chrome/browser/storage_monitor/media_storage_util.cc',
        '../chrome/browser/storage_monitor/media_storage_util.h',
        '../chrome/browser/storage_monitor/media_transfer_protocol_device_observer_linux.cc',
        '../chrome/browser/storage_monitor/media_transfer_protocol_device_observer_linux.h',
        '../chrome/browser/storage_monitor/mtab_watcher_linux.cc',
        '../chrome/browser/storage_monitor/mtab_watcher_linux.h',
        '../chrome/browser/storage_monitor/portable_device_watcher_win.cc',
        '../chrome/browser/storage_monitor/portable_device_watcher_win.h',
        '../chrome/browser/storage_monitor/removable_device_constants.cc',
        '../chrome/browser/storage_monitor/removable_device_constants.h',
        '../chrome/browser/storage_monitor/removable_storage_observer.h',
        '../chrome/browser/storage_monitor/storage_info.cc',
        '../chrome/browser/storage_monitor/storage_info.h',
        '../chrome/browser/storage_monitor/storage_monitor.cc',
        '../chrome/browser/storage_monitor/storage_monitor.h',
        '../chrome/browser/storage_monitor/storage_monitor_linux.cc',
        '../chrome/browser/storage_monitor/storage_monitor_linux.h',
        '../chrome/browser/storage_monitor/storage_monitor_mac.h',
        '../chrome/browser/storage_monitor/storage_monitor_mac.mm',
        '../chrome/browser/storage_monitor/storage_monitor_win.cc',
        '../chrome/browser/storage_monitor/storage_monitor_win.h',
        '../chrome/browser/storage_monitor/transient_device_ids.cc',
        '../chrome/browser/storage_monitor/transient_device_ids.h',
        '../chrome/browser/storage_monitor/udev_util_linux.cc',
        '../chrome/browser/storage_monitor/udev_util_linux.h',
        '../chrome/browser/storage_monitor/volume_mount_watcher_win.cc',
        '../chrome/browser/storage_monitor/volume_mount_watcher_win.h',
      ],
      'conditions': [
        ['OS=="linux"', {
          'dependencies': [
            '../build/linux/system.gyp:udev',
            '../dbus/dbus.gyp:dbus',
            '../device/media_transfer_protocol/media_transfer_protocol.gyp:device_media_transfer_protocol',
            '../device/media_transfer_protocol/media_transfer_protocol.gyp:mtp_file_entry_proto',
            '../device/media_transfer_protocol/media_transfer_protocol.gyp:mtp_storage_info_proto',
          ],
        }],
      ],
    },
  ],
}
