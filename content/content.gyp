# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,  # Use higher warning level.
    'chromium_enable_vtune_jit_for_v8%': 0,  # enable the vtune support for V8 engine.
    'directxsdk_exists': '<!pymod_do_main(dir_exists ../third_party/directxsdk)',
  },
  'target_defaults': {
    'defines': ['CONTENT_IMPLEMENTATION'],
    'conditions': [
      # TODO(jschuh): Remove this after crbug.com/173851 gets fixed.
      ['OS=="win" and target_arch=="x64"', {
        'msvs_settings': {
          'VCCLCompilerTool': {
            'AdditionalOptions': ['/bigobj'],
          },
        },
      }],
    ],
  },
  'conditions': [
    ['OS != "ios"', {
      'includes': [
        '../build/win_precompile.gypi',
        'content_resources.gypi',
      ],
    }],
    ['OS == "win"', {
      'targets': [
        {
          # GN: //content:content_startup_helper_win
          'target_name': 'content_startup_helper_win',
          'type': 'static_library',
          'include_dirs': [
            '..',
          ],
          'dependencies': [
            '../base/base.gyp:base',
            '../base/base.gyp:base_i18n',
            '../sandbox/sandbox.gyp:sandbox',
          ],
          'sources': [
            'app/startup_helper_win.cc',
            'public/app/startup_helper_win.h',
          ],
        }
      ],
    }],
    # In component mode, we build all of content as a single DLL.
    # However, in the static mode, we need to build content as multiple
    # targets in order to prevent dependencies from getting introduced
    # upstream unnecessarily (e.g., content_renderer depends on allocator
    # and chrome_exe depends on content_common but we don't want
    # chrome_exe to have to depend on allocator).
    ['component=="static_library"', {
      'target_defines': [
        'COMPILE_CONTENT_STATICALLY',
      ],
      'targets': [
        {
          # GN version: //content
          'target_name': 'content',
          'type': 'none',
          'dependencies': [
            'content_browser',
            'content_common',
          ],
          'export_dependent_settings': [
            'content_common',
          ],
          'conditions': [
            ['OS != "ios"', {
              'dependencies': [
                'content_child',
                'content_gpu',
                'content_plugin',
                'content_ppapi_plugin',
                'content_renderer',
                'content_utility',
              ],
            }],
          ],
        },
        {
          # GN version: //content/app:browser
          'target_name': 'content_app_browser',
          'type': 'static_library',
          'variables': { 'enable_wexit_time_destructors': 1, },
          'includes': [
            'content_app.gypi',
          ],
          'dependencies': [
            'content_common',
          ],
          'export_dependent_settings': [
            'content_common',
          ],
          'conditions': [
            ['chrome_multiple_dll', {
              'defines': [
                'CHROME_MULTIPLE_DLL_BROWSER',
              ],
            }],
          ],
        },
        {
          # GN version: //content/app:child
          'target_name': 'content_app_child',
          'type': 'static_library',
          'variables': { 'enable_wexit_time_destructors': 1, },
          'includes': [
            'content_app.gypi',
          ],
          'dependencies': [
            'content_common',
          ],
          'export_dependent_settings': [
            'content_common',
          ],
          'conditions': [
            ['chrome_multiple_dll', {
              'defines': [
                'CHROME_MULTIPLE_DLL_CHILD',
              ],
            }],
          ],
        },
        {
          # GN version: //content/app:both
          'target_name': 'content_app_both',
          'type': 'static_library',
          'variables': { 'enable_wexit_time_destructors': 1, },
          'includes': [
            'content_app.gypi',
          ],
          'dependencies': [
            'content_common',
          ],
          'export_dependent_settings': [
            'content_common',
          ],
        },
        {
          # GN version: //content/browser and //content/public/browser
          'target_name': 'content_browser',
          'type': 'static_library',
          'variables': { 'enable_wexit_time_destructors': 1, },
          'includes': [
            'content_browser.gypi',
            # Disable LTO due to ELF section name out of range
            # crbug.com/422251
            '../build/android/disable_lto.gypi',
          ],
          'dependencies': [
            'content_common',
          ],
          'export_dependent_settings': [
            'content_common',
          ],
          'conditions': [
            ['java_bridge==1', {
              'dependencies': [
                'content_child',
              ]
            }],
            ['OS=="android"', {
              'dependencies': [
                'content_gpu',
                'content_utility',
              ],
            }],
            ['OS != "ios"', {
              'dependencies': [
                'content_resources',
              ],
            }],
          ],
        },
        {
          # GN version: //content/common and //content/public/common
          'target_name': 'content_common',
          'type': 'static_library',
          'variables': { 'enable_wexit_time_destructors': 1, },
          'includes': [
            'content_common.gypi',
          ],
          'conditions': [
            ['OS != "ios"', {
              'dependencies': [
                'content_resources',
              ],
            }],
          ],
          # Disable c4267 warnings until we fix size_t to int truncations.
          'msvs_disabled_warnings': [ 4267, ],
        },
      ],
      'conditions': [
        ['OS != "ios"', {
          'targets': [
            {
              # GN version: //content/child and //content/public/child
              'target_name': 'content_child',
              'type': 'static_library',
              'variables': { 'enable_wexit_time_destructors': 1, },
              'includes': [
                'content_child.gypi',
              ],
              'dependencies': [
                'content_resources',
              ],
              # Disable c4267 warnings until we fix size_t to int truncations.
              'msvs_disabled_warnings': [ 4267, ],
            },
            {
              # GN version: //content/gpu
              'target_name': 'content_gpu',
              'type': 'static_library',
              'variables': { 'enable_wexit_time_destructors': 1, },
              'includes': [
                'content_gpu.gypi',
              ],
              'dependencies': [
                'content_child',
                'content_common',
              ],
            },
            {
              # GN version: //content/plugin and //content/public/plugin
              'target_name': 'content_plugin',
              'type': 'static_library',
              'variables': { 'enable_wexit_time_destructors': 1, },
              'includes': [
                'content_plugin.gypi',
              ],
              'dependencies': [
                'content_child',
                'content_common',
              ],
            },
            {
              # GN version: //content/ppapi_plugin
              'target_name': 'content_ppapi_plugin',
              'type': 'static_library',
              'variables': { 'enable_wexit_time_destructors': 1, },
              'includes': [
                'content_ppapi_plugin.gypi',
              ],
              # Disable c4267 warnings until we fix size_t to int truncations.
              'msvs_disabled_warnings': [ 4267, ],
            },
            {
              # GN version: //content/renderer and //content/public/renderer
              'target_name': 'content_renderer',
              'type': 'static_library',
              'variables': { 'enable_wexit_time_destructors': 1, },
              'includes': [
                'content_renderer.gypi',
              ],
              'dependencies': [
                'content_child',
                'content_common',
                'content_resources',
              ],
              'export_dependent_settings': [
                'content_common',
              ],
              'conditions': [
                ['chromium_enable_vtune_jit_for_v8==1', {
                  'dependencies': [
                    '../v8/src/third_party/vtune/v8vtune.gyp:v8_vtune',
                  ],
                }],
              ],
            },
            {
              # GN version: //content/utility and //content/public/utility
              'target_name': 'content_utility',
              'type': 'static_library',
              'variables': { 'enable_wexit_time_destructors': 1, },
              'includes': [
                'content_utility.gypi',
              ],
              'dependencies': [
                'content_child',
                'content_common',
              ],
            },
          ],
        }],
      ],
    },
    {  # component != static_library
      'targets': [
        {
          # GN version: //content
          'target_name': 'content',
          'type': 'shared_library',
          'variables': { 'enable_wexit_time_destructors': 1, },
          'dependencies': [
            'content_resources',
          ],
          'conditions': [
            ['chromium_enable_vtune_jit_for_v8==1', {
              'dependencies': [
                '../v8/src/third_party/vtune/v8vtune.gyp:v8_vtune',
              ],
            }],
          ],
          'includes': [
            'content_app.gypi',
            'content_browser.gypi',
            'content_child.gypi',
            'content_common.gypi',
            'content_gpu.gypi',
            'content_plugin.gypi',
            'content_ppapi_plugin.gypi',
            'content_renderer.gypi',
            'content_utility.gypi',
          ],
          'msvs_settings': {
            'VCLinkerTool': {
              'conditions': [
                ['incremental_chrome_dll==1', {
                  'UseLibraryDependencyInputs': "true",
                }],
              ],
            },
          },
        },
        {
          # GN version: //content/app:browser
          'target_name': 'content_app_browser',
          'type': 'none',
          'dependencies': ['content', 'content_browser'],
        },
        {
          # GN version: //content/app:child
          'target_name': 'content_app_child',
          'type': 'none',
          'dependencies': ['content', 'content_child'],
        },
        {
          # GN version: //content/app:both
          'target_name': 'content_app_both',
          'type': 'none',
          'dependencies': ['content'],
          'export_dependent_settings': ['content'],
        },
        {
          # GN version: //content/browser and //content/public/browser
          'target_name': 'content_browser',
          'type': 'none',
          'dependencies': ['content'],
          'export_dependent_settings': ['content'],
        },
        {
          # GN version: //content/common and //content/public/common
          'target_name': 'content_common',
          'type': 'none',
          'dependencies': ['content', 'content_resources'],
          # Disable c4267 warnings until we fix size_t to int truncations.
          'msvs_disabled_warnings': [ 4267, ],
          'export_dependent_settings': ['content'],
        },
        {
          # GN Version: //content/child
          'target_name': 'content_child',
          'type': 'none',
          'dependencies': ['content'],
        },
        {
          # GN version: //content/gpu
          'target_name': 'content_gpu',
          'type': 'none',
          'dependencies': ['content'],
        },
        {
          # GN version: //content/plugin
          'target_name': 'content_plugin',
          'type': 'none',
          'dependencies': ['content'],
        },
        {
          # GN version: //content/ppapi_plugin
          'target_name': 'content_ppapi_plugin',
          'type': 'none',
          'dependencies': ['content'],
          # Disable c4267 warnings until we fix size_t to int truncations.
          'msvs_disabled_warnings': [ 4267, ],
        },
        {
          # GN version: //content/renderer and //content/public/renderer
          'target_name': 'content_renderer',
          'type': 'none',
          'dependencies': ['content'],
          'export_dependent_settings': ['content'],
        },
        {
          # GN version: //content/utility
          'target_name': 'content_utility',
          'type': 'none',
          'dependencies': ['content'],
          'export_dependent_settings': ['content'],
        },
      ],
    }],
    ['OS == "android"', {
      'targets': [
        {
          'target_name': 'common_aidl',
          'type': 'none',
          'variables': {
            'aidl_interface_file': 'public/android/java/src/org/chromium/content/common/common.aidl',
            'aidl_import_include': 'public/android/java/src',
          },
          'sources': [
            'public/android/java/src/org/chromium/content/common/IChildProcessCallback.aidl',
            'public/android/java/src/org/chromium/content/common/IChildProcessService.aidl',
          ],
          'includes': [ '../build/java_aidl.gypi' ],
        },
        {
          'target_name': 'content_java',
          'type': 'none',
          'dependencies': [
            '../base/base.gyp:base',
            '../device/battery/battery.gyp:device_battery_java',
            '../device/bluetooth/bluetooth.gyp:device_bluetooth_java',
            '../device/vibration/vibration.gyp:device_vibration_java',
            '../media/media.gyp:media_java',
            '../mojo/mojo_base.gyp:mojo_application_bindings',
            '../mojo/mojo_base.gyp:mojo_system_java',
            '../net/net.gyp:net',
            '../third_party/mojo/mojo_public.gyp:mojo_bindings_java',
            '../ui/android/ui_android.gyp:ui_java',
            '../ui/touch_selection/ui_touch_selection.gyp:selection_event_type_java',
            '../ui/touch_selection/ui_touch_selection.gyp:touch_handle_orientation_java',
            '../third_party/WebKit/public/blink_headers.gyp:blink_headers_java',
            'common_aidl',
            'console_message_level_java',
            'content_common',
            'content_strings_grd',
            'content_gamepad_mapping',
            'gesture_event_type_java',
            'invalidate_types_java',
            'navigation_controller_java',
            'popup_item_type_java',
            'result_codes_java',
            'readback_response_java',
            'speech_recognition_error_java',
            'top_controls_state_java',
            'screen_orientation_values_java',
          ],
          # TODO(sgurun) remove this when M is public. crbug/512264.
          'conditions': [
            ['android_sdk_version != "M"', {
              'dependencies': [
                '../third_party/android_tools/android_tools.gyp:preview_java_sources',
              ],
            }],
          ],
          'variables': {
            'java_in_dir': '../content/public/android/java',
            'has_java_resources': 1,
            'R_package': 'org.chromium.content',
            'R_package_relpath': 'org/chromium/content',
          },
          'includes': [ '../build/java.gypi' ],
        },
        {
          'target_name': 'console_message_level_java',
          'type': 'none',
          'variables': {
            'source_file': 'public/common/console_message_level.h',
          },
          'includes': [ '../build/android/java_cpp_enum.gypi' ],
        },
        {
          'target_name': 'content_strings_grd',
          # The android_webview/Android.mk file depends on this target directly.
          'android_unmangled_name': 1,
          'type': 'none',
          'variables': {
            'grd_file': '../content/public/android/java/strings/android_content_strings.grd',
          },
          'includes': [
            '../build/java_strings_grd.gypi',
          ],
        },
        {
          'target_name': 'content_gamepad_mapping',
          'type': 'none',
          'variables': {
            'source_file': 'browser/gamepad/gamepad_standard_mappings.h',
          },
          'includes': [ '../build/android/java_cpp_enum.gypi' ],
        },
        {
          'target_name': 'gesture_event_type_java',
          'type': 'none',
          'variables': {
            'source_file': 'browser/android/gesture_event_type.h',
          },
          'includes': [ '../build/android/java_cpp_enum.gypi' ],
        },
        {
          'target_name': 'invalidate_types_java',
          'type': 'none',
          'variables': {
            'source_file': 'public/browser/invalidate_type.h',
          },
          'includes': [ '../build/android/java_cpp_enum.gypi' ],
        },
        {
          'target_name': 'navigation_controller_java',
          'type': 'none',
          'variables': {
            'source_file': 'public/browser/navigation_controller.h',
          },
          'includes': [ '../build/android/java_cpp_enum.gypi' ],
        },
        {
          'target_name': 'popup_item_type_java',
          'type': 'none',
          'variables': {
            'source_file': 'browser/android/content_view_core_impl.cc',
          },
          'includes': [ '../build/android/java_cpp_enum.gypi' ],
        },
        {
          'target_name': 'readback_response_java',
          'type': 'none',
          'variables': {
            'source_file': 'public/browser/readback_types.h',
          },
          'includes': [ '../build/android/java_cpp_enum.gypi' ],
        },
        {
          'target_name': 'result_codes_java',
          'type': 'none',
          'variables': {
            'source_file': 'public/common/result_codes.h',
          },
          'includes': [ '../build/android/java_cpp_enum.gypi' ],
        },
        {
          'target_name': 'speech_recognition_error_java',
          'type': 'none',
          'variables': {
            'source_file': 'public/common/speech_recognition_error.h',
          },
          'includes': [ '../build/android/java_cpp_enum.gypi' ],
        },
        {
          'target_name': 'top_controls_state_java',
          'type': 'none',
          'variables': {
            'source_file': 'public/common/top_controls_state.h',
          },
          'includes': [ '../build/android/java_cpp_enum.gypi' ],
        },
        {
          'target_name': 'screen_orientation_values_java',
          'type': 'none',
          'variables': {
            'source_file': 'public/common/screen_orientation_values.h',
          },
          'includes': [ '../build/android/java_cpp_enum.gypi' ],
        },
        {
          'target_name': 'java_set_jni_headers',
          'type': 'none',
          'variables': {
            'jni_gen_package': 'content',
            'input_java_class': 'java/util/HashSet.class',
          },
          'includes': [ '../build/jar_file_jni_generator.gypi' ],
        },
        {
          'target_name': 'motionevent_jni_headers',
          'type': 'none',
          'variables': {
             'jni_gen_package': 'content',
             'input_java_class': 'android/view/MotionEvent.class',
           },
          'includes': [ '../build/jar_file_jni_generator.gypi' ],
        },
        {
          'target_name': 'content_jni_headers',
          'type': 'none',
          'dependencies': [
            'java_set_jni_headers',
            'motionevent_jni_headers'
          ],
          'includes': [ 'content_jni.gypi' ],
        },
        {
          'target_name': 'content_icudata',
          'type': 'none',
          'conditions': [
            ['icu_use_data_file_flag==1', {
              'copies': [
                {
                  'destination': '<(PRODUCT_DIR)/content_shell/assets',
                  'files': [
                    '<(PRODUCT_DIR)/icudtl.dat',
                  ],
                },
              ],
            }],
          ],
        },
        {
          'target_name': 'content_v8_external_data',
          'type': 'none',
          'conditions': [
            ['v8_use_external_startup_data==1', {
              'copies': [
                {
                  'destination': '<(PRODUCT_DIR)/content_shell/assets',
                  'files': [
                    '<(PRODUCT_DIR)/natives_blob.bin',
                    '<(PRODUCT_DIR)/snapshot_blob.bin',
                  ],
                },
              ],
            }],
          ],
        },
      ],
    }],  # OS == "android"
  ],
}
