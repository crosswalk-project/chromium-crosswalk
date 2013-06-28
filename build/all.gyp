# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'All',
      'type': 'none',
      'xcode_create_dependents_test_runner': 1,
      'dependencies': [
        'some.gyp:*',
        '../base/base.gyp:*',
        '../chrome/chrome.gyp:*',
        '../content/content.gyp:*',
        '../crypto/crypto.gyp:*',
        '../efl_webview/efl_webview.gyp:*',
        '../media/media.gyp:*',
        '../net/net.gyp:*',
        '../sdch/sdch.gyp:*',
        '../sql/sql.gyp:*',
        '../sync/sync.gyp:*',
        '../testing/gmock.gyp:*',
        '../testing/gtest.gyp:*',
        '../third_party/bzip2/bzip2.gyp:*',
        '../third_party/icu/icu.gyp:*',
        '../third_party/libxml/libxml.gyp:*',
        '../third_party/sqlite/sqlite.gyp:*',
        '../third_party/zlib/zlib.gyp:*',
        '../ui/ui.gyp:*',
        '../url/url.gyp:*',
        '../webkit/support/webkit_support.gyp:*',
        'temp_gyp/googleurl.gyp:*',
      ],
      'conditions': [
        ['OS!="ios"', {
          'dependencies': [
            '../cc/cc_tests.gyp:*',
            '../components/components.gyp:*',
            '../device/device.gyp:*',
            '../gpu/gpu.gyp:*',
            '../gpu/tools/tools.gyp:*',
            '../ipc/ipc.gyp:*',
            '../jingle/jingle.gyp:*',
            '../ppapi/ppapi.gyp:*',
            '../ppapi/ppapi_internal.gyp:*',
            '../printing/printing.gyp:*',
            '../skia/skia.gyp:*',
            '../third_party/cacheinvalidation/cacheinvalidation.gyp:*',
            '../third_party/cld/cld.gyp:*',
            '../third_party/codesighs/codesighs.gyp:*',
            '../third_party/ffmpeg/ffmpeg.gyp:*',
            '../third_party/iccjpeg/iccjpeg.gyp:*',
            '../third_party/libpng/libpng.gyp:*',
            '../third_party/libusb/libusb.gyp:*',
            '../third_party/libwebp/libwebp.gyp:*',
            '../third_party/libxslt/libxslt.gyp:*',
            '../third_party/lzma_sdk/lzma_sdk.gyp:*',
            '../third_party/mesa/mesa.gyp:*',
            '../third_party/modp_b64/modp_b64.gyp:*',
            '../third_party/npapi/npapi.gyp:*',
            '../third_party/ots/ots.gyp:*',
            '../third_party/qcms/qcms.gyp:*',
            '../third_party/re2/re2.gyp:re2',
            '../third_party/WebKit/Source/WebKit/chromium/All.gyp:*',
            '../v8/tools/gyp/v8.gyp:*',
            '../webkit/compositor_bindings/compositor_bindings_tests.gyp:*',
            '../webkit/webkit.gyp:*',
            '<(libjpeg_gyp_path):*',
          ],
        }],
        ['os_posix==1 and OS!="android" and OS!="ios"', {
          'dependencies': [
            '../third_party/yasm/yasm.gyp:*#host',
          ],
        }],
        ['OS=="mac" or OS=="ios" or OS=="win"', {
          'dependencies': [
            '../third_party/nss/nss.gyp:*',
           ],
        }],
        ['OS=="win" or OS=="ios" or OS=="linux"', {
          'dependencies': [
            '../breakpad/breakpad.gyp:*',
           ],
        }],
        ['OS=="mac"', {
          'dependencies': [
            '../third_party/ocmock/ocmock.gyp:*',
          ],
        }],
        ['OS=="linux"', {
          'dependencies': [
            '../courgette/courgette.gyp:*',
            '../dbus/dbus.gyp:*',
            '../sandbox/sandbox.gyp:*',
          ],
          'conditions': [
            ['branding=="Chrome"', {
              'dependencies': [
                '../chrome/chrome.gyp:linux_packages_<(channel)',
              ],
            }],
            ['chromeos==0', {
              'dependencies': [
                '../third_party/cros_dbus_cplusplus/cros_dbus_cplusplus.gyp:*',
                '../third_party/libmtp/libmtp.gyp:*',
                '../third_party/mtpd/mtpd.gyp:*',
              ],
            }],
          ],
        }],
        ['use_x11==1', {
          'dependencies': [
            '../tools/xdisplaycheck/xdisplaycheck.gyp:*',
          ],
        }],
        ['toolkit_uses_gtk==1', {
          'dependencies': [
            '../tools/gtk_clipboard_dump/gtk_clipboard_dump.gyp:*',
          ],
        }],
        ['OS=="win"', {
          'conditions': [
            ['win_use_allocator_shim==1', {
              'dependencies': [
                '../base/allocator/allocator.gyp:*',
              ],
            }],
            # Don't enable dependencies that don't work on Win64.
            ['target_arch!="x64"', {
              'dependencies': [
                # TODO(jschuh) Enable Win64 Memory Watcher. crbug.com/176877
                '../tools/memory_watcher/memory_watcher.gyp:*',
                # TODO(jschuh) Enable Win64 Chrome Frame. crbug.com/176875
                '../chrome_frame/chrome_frame.gyp:*',
              ],
            }],
          ],
          'dependencies': [
            '../cloud_print/cloud_print.gyp:*',
            '../courgette/courgette.gyp:*',
            '../rlz/rlz.gyp:*',
            '../sandbox/sandbox.gyp:*',
            '../third_party/angle/src/build_angle.gyp:*',
            '../third_party/bsdiff/bsdiff.gyp:*',
            '../third_party/bspatch/bspatch.gyp:*',
            '../third_party/gles2_book/gles2_book.gyp:*',
          ],
        }, {
          'dependencies': [
            '../third_party/libevent/libevent.gyp:*',
          ],
        }],
        ['toolkit_views==1', {
          'dependencies': [
            '../ui/views/controls/webview/webview.gyp:*',
            '../ui/views/views.gyp:*',
          ],
        }],
        ['use_aura==1', {
          'dependencies': [
            '../ui/aura/aura.gyp:*',
            '../ui/oak/oak.gyp:*',
          ],
        }],
        ['use_ash==1', {
          'dependencies': [
            '../ash/ash.gyp:*',
          ],
        }],
        ['remoting==1', {
          'dependencies': [
            '../remoting/remoting.gyp:*',
          ],
        }],
        ['use_openssl==0', {
          'dependencies': [
            '../net/third_party/nss/ssl.gyp:*',
          ],
        }],
        ['enable_app_list==1', {
          'dependencies': [
            '../ui/app_list/app_list.gyp:*',
          ],
        }],
      ],
    }, # target_name: All
    {
      'target_name': 'All_syzygy',
      'type': 'none',
      'conditions': [
        ['OS=="win" and fastbuild==0 and target_arch=="ia32"', {
            'dependencies': [
              '../chrome/installer/mini_installer_syzygy.gyp:*',
            ],
          },
        ],
      ],
    }, # target_name: All_syzygy
    {
      'target_name': 'chromium_builder_tests',
      'type': 'none',
      'dependencies': [
        '../base/base.gyp:base_unittests',
        '../chrome/chrome.gyp:unit_tests',
        '../crypto/crypto.gyp:crypto_unittests',
        '../media/media.gyp:media_unittests',
        '../net/net.gyp:net_unittests',
        '../sql/sql.gyp:sql_unittests',
        '../ui/ui.gyp:ui_unittests',
        '../url/url.gyp:googleurl_unittests',
      ],
      'conditions': [
        ['OS!="ios"', {
          'dependencies': [
            '../cc/cc_tests.gyp:cc_unittests',
            '../chrome/chrome.gyp:browser_tests',
            '../chrome/chrome.gyp:chromedriver2_tests',
            '../chrome/chrome.gyp:chromedriver2_unittests',
            '../chrome/chrome.gyp:interactive_ui_tests',
            '../chrome/chrome.gyp:sync_integration_tests',
            '../cloud_print/cloud_print.gyp:cloud_print_unittests',
            '../components/components.gyp:components_unittests',
            '../content/content.gyp:content_browsertests',
            '../content/content.gyp:content_unittests',
            '../device/device.gyp:device_unittests',
            '../gpu/gpu.gyp:gpu_unittests',
            '../gpu/gles2_conform_support/gles2_conform_support.gyp:gles2_conform_support',
            '../ipc/ipc.gyp:ipc_tests',
            '../jingle/jingle.gyp:jingle_unittests',
            '../ppapi/ppapi_internal.gyp:ppapi_unittests',
            '../printing/printing.gyp:printing_unittests',
            '../remoting/remoting.gyp:remoting_unittests',
            '../sync/sync.gyp:sync_unit_tests',
            '../third_party/cacheinvalidation/cacheinvalidation.gyp:cacheinvalidation_unittests',
            '../third_party/libphonenumber/libphonenumber.gyp:libphonenumber_unittests',
            '../webkit/compositor_bindings/compositor_bindings_tests.gyp:webkit_compositor_bindings_unittests',
          ],
        }],
        ['OS=="win"', {
          'dependencies': [
            '../chrome/chrome.gyp:crash_service',
            '../chrome/chrome.gyp:installer_util_unittests',
            '../chrome/chrome.gyp:mini_installer_test',
            # mini_installer_tests depends on mini_installer. This should be
            # defined in installer.gyp.
            '../chrome/installer/mini_installer.gyp:mini_installer',
            '../chrome_frame/chrome_frame.gyp:npchrome_frame',
            '../courgette/courgette.gyp:courgette_unittests',
            '../sandbox/sandbox.gyp:sbox_integration_tests',
            '../sandbox/sandbox.gyp:sbox_unittests',
            '../sandbox/sandbox.gyp:sbox_validation_tests',
            '../webkit/webkit.gyp:pull_in_copy_TestNetscapePlugIn',
            '../ui/app_list/app_list.gyp:app_list_unittests',
            '../ui/views/views.gyp:views_unittests',
            '../webkit/webkit.gyp:test_shell_common',
          ],
          'conditions': [
            ['target_arch!="x64"', {
              'dependencies': [
                '../chrome_frame/chrome_frame.gyp:chrome_frame_net_tests',
                '../chrome_frame/chrome_frame.gyp:chrome_frame_perftests',
                '../chrome_frame/chrome_frame.gyp:chrome_frame_reliability_tests',
                '../chrome_frame/chrome_frame.gyp:chrome_frame_tests',
                '../chrome_frame/chrome_frame.gyp:chrome_frame_unittests',
              ]
            }, { # target_arch!="x64"
              'dependencies!': [
                '../chrome_frame/chrome_frame.gyp:npchrome_frame',
              ],
              'defines': [
                'OMIT_CHROME_FRAME',
              ],
            }], # target_arch=="x64"
            # remoting_host_installation uses lots of non-trivial GYP that tend
            # to break because of differences between ninja and msbuild. Make
            # sure this target is built by the builders on the main waterfall.
            # See http://crbug.com/180600.
            ['wix_exists == "True" and sas_dll_exists == "True"', {
              'dependencies': [
                '../remoting/remoting.gyp:remoting_host_installation',
              ],
            }],
          ],
        }],
        ['OS=="linux"', {
          'dependencies': [
            '../sandbox/sandbox.gyp:sandbox_linux_unittests',
            '../dbus/dbus.gyp:dbus_unittests',
          ],
        }],
        ['OS=="mac"', {
          'dependencies': [
            '../ui/app_list/app_list.gyp:app_list_unittests',
            '../ui/message_center/message_center.gyp:*',
          ],
        }],
        ['test_isolation_mode != "noop"', {
          'dependencies': [
            'chromium_swarm_tests',
          ],
        }],
      ],
    }, # target_name: chromium_builder_tests
    {
      'target_name': 'chromium_2010_builder_tests',
      'type': 'none',
      'dependencies': [
        'chromium_builder_tests',
      ],
    }, # target_name: chromium_2010_builder_tests
  ],
  'conditions': [
    ['OS!="ios"', {
      'targets': [
        {
          'target_name': 'chromium_builder_nacl_win_integration',
          'type': 'none',
          'dependencies': [
            'chromium_builder_qa', # needed for pyauto
            'chromium_builder_tests',
          ],
        }, # target_name: chromium_builder_nacl_win_integration
        {
          'target_name': 'chromium_builder_perf',
          'type': 'none',
          'dependencies': [
            'chromium_builder_qa', # needed for pyauto
            '../chrome/chrome.gyp:performance_browser_tests',
            '../chrome/chrome.gyp:performance_ui_tests',
            '../chrome/chrome.gyp:sync_performance_tests',
          ],
        }, # target_name: chromium_builder_perf
        {
          'target_name': 'chromium_gpu_builder',
          'type': 'none',
          'dependencies': [
            '../chrome/chrome.gyp:gpu_tests',
            '../chrome/chrome.gyp:performance_browser_tests',
            '../chrome/chrome.gyp:performance_ui_tests',
            '../content/content.gyp:content_browsertests',
            '../gpu/gpu.gyp:gl_tests',
          ],
          'conditions': [
            ['internal_gles2_conform_tests', {
              'dependencies': [
                '../gpu/gles2_conform_test/gles2_conform_test.gyp:gles2_conform_test',
              ],
            }], # internal_gles2_conform
          ],
        }, # target_name: chromium_gpu_builder
        {
          'target_name': 'chromium_gpu_debug_builder',
          'type': 'none',
          'dependencies': [
            '../chrome/chrome.gyp:gpu_tests',
            '../content/content.gyp:content_browsertests',
            '../gpu/gpu.gyp:gl_tests',
          ],
          'conditions': [
            ['internal_gles2_conform_tests', {
              'dependencies': [
                '../gpu/gles2_conform_test/gles2_conform_test.gyp:gles2_conform_test',
              ],
            }], # internal_gles2_conform
          ],
        }, # target_name: chromium_gpu_debug_builder
        {
          'target_name': 'chromium_builder_qa',
          'type': 'none',
          'dependencies': [
            '../chrome/chrome.gyp:chromedriver',
            '../chrome/chrome.gyp:chromedriver2',
            '../chrome/chrome.gyp:chromedriver2_server',
            '../chrome/chrome.gyp:chromedriver2_tests',
            '../chrome/chrome.gyp:chromedriver2_unittests',
            # Dependencies of pyauto_functional tests.
            '../remoting/remoting.gyp:remoting_webapp',
          ],
          'conditions': [
            # If you change this condition, make sure you also change it
            # in chrome_tests.gypi
            ['enable_automation==1 and (OS=="mac" or ((OS=="win" or os_posix==1) and target_arch==python_arch))', {
              'dependencies': [
                '../chrome/chrome.gyp:pyautolib',
              ],
            }],
            ['OS=="mac"', {
              'dependencies': [
                '../remoting/remoting.gyp:remoting_me2me_host_archive',
              ],
            }],
            ['OS=="win" and component != "shared_library" and wix_exists == "True" and sas_dll_exists == "True"', {
              'dependencies': [
                '../remoting/remoting.gyp:remoting_host_installation',
              ],
            }],
          ],
        }, # target_name: chromium_builder_qa
        {
          'target_name': 'chromium_builder_perf_av',
          'type': 'none',
          'dependencies': [
            'chromium_builder_qa',  # needed for perf pyauto tests
            '../content/content.gyp:content_shell',
            '../webkit/webkit.gyp:pull_in_DumpRenderTree',  # to run layout tests
          ],
        },  # target_name: chromium_builder_perf_av
        {
          # This target contains everything we need to run tests on the special
          # device-equipped WebRTC bots. We have device-requiring tests in
          # PyAuto, browser_tests and content_browsertests.
          'target_name': 'chromium_builder_webrtc',
          'type': 'none',
          'dependencies': [
            'chromium_builder_qa',  # needed for perf pyauto tests
            '../chrome/chrome.gyp:browser_tests',
            '../content/content.gyp:content_browsertests',
            '../content/content.gyp:content_unittests',
            '../third_party/libjingle/libjingle.gyp:peerconnection_server',
            '../third_party/webrtc/tools/tools.gyp:frame_analyzer',
            '../third_party/webrtc/tools/tools.gyp:rgba_to_i420_converter',
          ],
        },  # target_name: chromium_builder_webrtc
      ],  # targets
    }],
    ['OS=="mac"', {
      'targets': [
        {
          # Target to build everything plus the dmg.  We don't put the dmg
          # in the All target because developers really don't need it.
          'target_name': 'all_and_dmg',
          'type': 'none',
          'dependencies': [
            'All',
            '../chrome/chrome.gyp:build_app_dmg',
          ],
        },
        # These targets are here so the build bots can use them to build
        # subsets of a full tree for faster cycle times.
        {
          'target_name': 'chromium_builder_dbg',
          'type': 'none',
          'dependencies': [
            '../cc/cc_tests.gyp:cc_unittests',
            '../chrome/chrome.gyp:browser_tests',
            '../chrome/chrome.gyp:interactive_ui_tests',
            '../chrome/chrome.gyp:sync_integration_tests',
            '../chrome/chrome.gyp:unit_tests',
            '../cloud_print/cloud_print.gyp:cloud_print_unittests',
            '../components/components.gyp:components_unittests',
            '../content/content.gyp:content_browsertests',
            '../content/content.gyp:content_unittests',
            '../device/device.gyp:device_unittests',
            '../gpu/gpu.gyp:gpu_unittests',
            '../ipc/ipc.gyp:ipc_tests',
            '../jingle/jingle.gyp:jingle_unittests',
            '../media/media.gyp:media_unittests',
            '../ppapi/ppapi_internal.gyp:ppapi_unittests',
            '../printing/printing.gyp:printing_unittests',
            '../remoting/remoting.gyp:remoting_unittests',
            '../rlz/rlz.gyp:*',
            '../sql/sql.gyp:sql_unittests',
            '../sync/sync.gyp:sync_unit_tests',
            '../third_party/cacheinvalidation/cacheinvalidation.gyp:cacheinvalidation_unittests',
            '../third_party/libphonenumber/libphonenumber.gyp:libphonenumber_unittests',
            '../ui/ui.gyp:ui_unittests',
            '../url/url.gyp:googleurl_unittests',
            '../webkit/compositor_bindings/compositor_bindings_tests.gyp:webkit_compositor_bindings_unittests',
          ],
        },
        {
          'target_name': 'chromium_builder_rel',
          'type': 'none',
          'dependencies': [
            '../cc/cc_tests.gyp:cc_unittests',
            '../chrome/chrome.gyp:browser_tests',
            '../chrome/chrome.gyp:performance_browser_tests',
            '../chrome/chrome.gyp:performance_ui_tests',
            '../chrome/chrome.gyp:sync_integration_tests',
            '../chrome/chrome.gyp:unit_tests',
            '../cloud_print/cloud_print.gyp:cloud_print_unittests',
            '../components/components.gyp:components_unittests',
            '../content/content.gyp:content_browsertests',
            '../content/content.gyp:content_unittests',
            '../device/device.gyp:device_unittests',
            '../gpu/gpu.gyp:gpu_unittests',
            '../ipc/ipc.gyp:ipc_tests',
            '../jingle/jingle.gyp:jingle_unittests',
            '../media/media.gyp:media_unittests',
            '../ppapi/ppapi_internal.gyp:ppapi_unittests',
            '../printing/printing.gyp:printing_unittests',
            '../remoting/remoting.gyp:remoting_unittests',
            '../sql/sql.gyp:sql_unittests',
            '../sync/sync.gyp:sync_unit_tests',
            '../third_party/cacheinvalidation/cacheinvalidation.gyp:cacheinvalidation_unittests',
            '../third_party/libphonenumber/libphonenumber.gyp:libphonenumber_unittests',
            '../ui/ui.gyp:ui_unittests',
            '../url/url.gyp:googleurl_unittests',
            '../webkit/compositor_bindings/compositor_bindings_tests.gyp:webkit_compositor_bindings_unittests',
          ],
        },
        {
          'target_name': 'chromium_builder_dbg_tsan_mac',
          'type': 'none',
          'dependencies': [
            '../base/base.gyp:base_unittests',
            '../cloud_print/cloud_print.gyp:cloud_print_unittests',
            '../crypto/crypto.gyp:crypto_unittests',
            '../ipc/ipc.gyp:ipc_tests',
            '../jingle/jingle.gyp:jingle_unittests',
            '../media/media.gyp:media_unittests',
            '../net/net.gyp:net_unittests',
            '../printing/printing.gyp:printing_unittests',
            '../remoting/remoting.gyp:remoting_unittests',
            '../third_party/cacheinvalidation/cacheinvalidation.gyp:cacheinvalidation_unittests',
            '../third_party/libphonenumber/libphonenumber.gyp:libphonenumber_unittests',
            '../url/url.gyp:googleurl_unittests',
          ],
        },
        {
          'target_name': 'chromium_builder_asan_mac',
          'type': 'none',
          'dependencies': [
            '../chrome/chrome.gyp:chrome',
            '../net/net.gyp:dns_fuzz_stub',
            '../content/content.gyp:content_shell',
            '../webkit/webkit.gyp:pull_in_DumpRenderTree',
          ],
        },
        {
          'target_name': 'chromium_builder_dbg_valgrind_mac',
          'type': 'none',
          'dependencies': [
            '../base/base.gyp:base_unittests',
            '../chrome/chrome.gyp:unit_tests',
            '../cloud_print/cloud_print.gyp:cloud_print_unittests',
            '../components/components.gyp:components_unittests',
            '../content/content.gyp:content_unittests',
            '../crypto/crypto.gyp:crypto_unittests',
            '../device/device.gyp:device_unittests',
            '../ipc/ipc.gyp:ipc_tests',
            '../jingle/jingle.gyp:jingle_unittests',
            '../media/media.gyp:media_unittests',
            '../net/net.gyp:net_unittests',
            '../printing/printing.gyp:printing_unittests',
            '../remoting/remoting.gyp:remoting_unittests',
            '../sql/sql.gyp:sql_unittests',
            '../sync/sync.gyp:sync_unit_tests',
            '../third_party/cacheinvalidation/cacheinvalidation.gyp:cacheinvalidation_unittests',
            '../third_party/libphonenumber/libphonenumber.gyp:libphonenumber_unittests',
            '../ui/ui.gyp:ui_unittests',
            '../url/url.gyp:googleurl_unittests',
          ],
        },
      ],  # targets
    }], # OS="mac"
    ['OS=="win"', {
      'targets': [
        # These targets are here so the build bots can use them to build
        # subsets of a full tree for faster cycle times.
        {
          'target_name': 'chromium_builder',
          'type': 'none',
          'dependencies': [
            '../cc/cc_tests.gyp:cc_unittests',
            '../chrome/chrome.gyp:browser_tests',
            '../chrome/chrome.gyp:installer_util_unittests',
            '../chrome/chrome.gyp:interactive_ui_tests',
            '../chrome/chrome.gyp:mini_installer_test',
            '../chrome/chrome.gyp:performance_browser_tests',
            '../chrome/chrome.gyp:performance_ui_tests',
            '../chrome/chrome.gyp:sync_integration_tests',
            '../chrome/chrome.gyp:unit_tests',
            '../cloud_print/cloud_print.gyp:cloud_print_unittests',
            '../components/components.gyp:components_unittests',
            '../content/content.gyp:content_browsertests',
            '../content/content.gyp:content_unittests',
            # mini_installer_tests depends on mini_installer. This should be
            # defined in installer.gyp.
            '../chrome/installer/mini_installer.gyp:mini_installer',
            '../chrome_frame/chrome_frame.gyp:npchrome_frame',
            '../courgette/courgette.gyp:courgette_unittests',
            '../device/device.gyp:device_unittests',
            '../gpu/gpu.gyp:gpu_unittests',
            '../ipc/ipc.gyp:ipc_tests',
            '../jingle/jingle.gyp:jingle_unittests',
            '../media/media.gyp:media_unittests',
            '../ppapi/ppapi_internal.gyp:ppapi_unittests',
            '../printing/printing.gyp:printing_unittests',
            '../remoting/remoting.gyp:remoting_unittests',
            '../sql/sql.gyp:sql_unittests',
            '../sync/sync.gyp:sync_unit_tests',
            '../third_party/cacheinvalidation/cacheinvalidation.gyp:cacheinvalidation_unittests',
            '../third_party/libphonenumber/libphonenumber.gyp:libphonenumber_unittests',
            '../ui/ui.gyp:ui_unittests',
            '../ui/views/views.gyp:views_unittests',
            '../url/url.gyp:googleurl_unittests',
            '../webkit/compositor_bindings/compositor_bindings_tests.gyp:webkit_compositor_bindings_unittests',
            '../webkit/webkit.gyp:pull_in_copy_TestNetscapePlugIn',
          ],
          'conditions': [
             ['target_arch!="x64"', {
               'dependencies': [
                 '../chrome_frame/chrome_frame.gyp:chrome_frame_net_tests',
                 '../chrome_frame/chrome_frame.gyp:chrome_frame_perftests',
                 '../chrome_frame/chrome_frame.gyp:chrome_frame_reliability_tests',
                 '../chrome_frame/chrome_frame.gyp:chrome_frame_tests',
                 '../chrome_frame/chrome_frame.gyp:chrome_frame_unittests',
               ]
             }, { # target_arch!="x64"
               'dependencies!': [
                 '../chrome_frame/chrome_frame.gyp:npchrome_frame',
               ],
               'defines': [
                 'OMIT_CHROME_FRAME',
               ],
             }], # target_arch=="x64"
          ],
        },
        {
          'target_name': 'chromium_builder_win_cf',
          'type': 'none',
          'conditions': [
            ['target_arch!="x64"', {
              'dependencies': [
                '../chrome_frame/chrome_frame.gyp:chrome_frame_net_tests',
                '../chrome_frame/chrome_frame.gyp:chrome_frame_perftests',
                '../chrome_frame/chrome_frame.gyp:chrome_frame_reliability_tests',
                '../chrome_frame/chrome_frame.gyp:chrome_frame_tests',
                '../chrome_frame/chrome_frame.gyp:chrome_frame_unittests',
                '../chrome_frame/chrome_frame.gyp:npchrome_frame',
              ],
            }], # target_arch!="x64"
          ],
        },
        {
          'target_name': 'chromium_builder_dbg_tsan_win',
          'type': 'none',
          'dependencies': [
            '../base/base.gyp:base_unittests',
            '../cloud_print/cloud_print.gyp:cloud_print_unittests',
            '../components/components.gyp:components_unittests',
            '../content/content.gyp:content_unittests',
            '../crypto/crypto.gyp:crypto_unittests',
            '../ipc/ipc.gyp:ipc_tests',
            '../jingle/jingle.gyp:jingle_unittests',
            '../media/media.gyp:media_unittests',
            '../net/net.gyp:net_unittests',
            '../printing/printing.gyp:printing_unittests',
            '../remoting/remoting.gyp:remoting_unittests',
            '../sql/sql.gyp:sql_unittests',
            '../third_party/cacheinvalidation/cacheinvalidation.gyp:cacheinvalidation_unittests',
            '../third_party/libphonenumber/libphonenumber.gyp:libphonenumber_unittests',
            '../url/url.gyp:googleurl_unittests',
          ],
        },
        {
          'target_name': 'chromium_builder_dbg_drmemory_win',
          'type': 'none',
          'dependencies': [
            '../base/base.gyp:base_unittests',
            '../chrome/chrome.gyp:unit_tests',
            '../cloud_print/cloud_print.gyp:cloud_print_unittests',
            '../components/components.gyp:components_unittests',
            '../content/content.gyp:content_unittests',
            '../crypto/crypto.gyp:crypto_unittests',
            '../device/device.gyp:device_unittests',
            '../ipc/ipc.gyp:ipc_tests',
            '../jingle/jingle.gyp:jingle_unittests',
            '../media/media.gyp:media_unittests',
            '../net/net.gyp:net_unittests',
            '../printing/printing.gyp:printing_unittests',
            '../remoting/remoting.gyp:remoting_unittests',
            '../sql/sql.gyp:sql_unittests',
            '../third_party/cacheinvalidation/cacheinvalidation.gyp:cacheinvalidation_unittests',
            '../third_party/libphonenumber/libphonenumber.gyp:libphonenumber_unittests',
            '../url/url.gyp:googleurl_unittests',
          ],
        },
        {
          'target_name': 'webkit_builder_win',
          'type': 'none',
          'dependencies': [
            '../webkit/webkit.gyp:test_shell',
            '../webkit/webkit.gyp:pull_in_webkit_unit_tests',
            '../content/content.gyp:content_shell',
            '../webkit/webkit.gyp:pull_in_DumpRenderTree',
          ],
        },
      ],  # targets
      'conditions': [
        ['branding=="Chrome"', {
          'targets': [
            {
              'target_name': 'chrome_official_builder',
              'type': 'none',
              'dependencies': [
                '../chrome/chrome.gyp:chromedriver',
                '../chrome/chrome.gyp:crash_service',
                '../chrome/chrome.gyp:interactive_ui_tests',
                '../chrome/chrome.gyp:policy_templates',
                '../chrome/chrome.gyp:reliability_tests',
                '../chrome/chrome.gyp:automated_ui_tests',
                '../chrome/installer/mini_installer.gyp:mini_installer',
                '../chrome_frame/chrome_frame.gyp:npchrome_frame',
                '../courgette/courgette.gyp:courgette',
                '../cloud_print/cloud_print.gyp:cloud_print',
                '../remoting/remoting.gyp:remoting_webapp',
                '../third_party/widevine/cdm/widevine_cdm.gyp:widevinecdmadapter',
              ],
              'conditions': [
                # If you change this condition, make sure you also change it
                # in chrome_tests.gypi
                ['enable_automation==1 and (OS=="mac" or (os_posix==1 and target_arch==python_arch))', {
                  'dependencies': [
                    '../chrome/chrome.gyp:pyautolib',
                  ],
                }],
                ['internal_pdf', {
                  'dependencies': [
                    '../pdf/pdf.gyp:pdf',
                  ],
                }], # internal_pdf
                ['target_arch=="ia32"', {
                  'dependencies': [
                    '../chrome/chrome.gyp:crash_service_win64',
                    '../courgette/courgette.gyp:courgette64',
                  ],
                }],
                ['component != "shared_library" and wix_exists == "True" and \
                    sas_dll_exists == "True"', {
                  'dependencies': [
                    '../remoting/remoting.gyp:remoting_host_installation',
                  ],
                }], # component != "shared_library"
                ['target_arch=="x64"', {
                  'dependencies!': [
                    '../chrome_frame/chrome_frame.gyp:npchrome_frame',
                  ],
                  'defines': [
                    'OMIT_CHROME_FRAME',
                  ],
                }], # target_arch=="x64"
              ]
            },
          ], # targets
        }], # branding=="Chrome"
       ], # conditions
    }], # OS="win"
    ['use_aura==1', {
      'targets': [
        {
          'target_name': 'aura_builder',
          'type': 'none',
          'dependencies': [
            '../cc/cc_tests.gyp:cc_unittests',
            '../chrome/chrome.gyp:browser_tests',
            '../chrome/chrome.gyp:chrome',
            '../chrome/chrome.gyp:interactive_ui_tests',
            '../chrome/chrome.gyp:unit_tests',
            '../components/components.gyp:components_unittests',
            '../content/content.gyp:content_browsertests',
            '../content/content.gyp:content_unittests',
            '../device/device.gyp:device_unittests',
            '../ppapi/ppapi_internal.gyp:ppapi_unittests',
            '../remoting/remoting.gyp:remoting_unittests',
            '../ui/app_list/app_list.gyp:*',
            '../ui/aura/aura.gyp:*',
            '../ui/compositor/compositor.gyp:*',
            '../ui/message_center/message_center.gyp:*',
            '../ui/ui.gyp:ui_unittests',
            '../ui/views/views.gyp:views',
            '../ui/views/views.gyp:views_examples_with_content_exe',
            '../ui/views/views.gyp:views_unittests',
            '../ui/keyboard/keyboard.gyp:*',
            '../webkit/compositor_bindings/compositor_bindings_tests.gyp:webkit_compositor_bindings_unittests',
            '../webkit/webkit.gyp:pull_in_webkit_unit_tests',
          ],
          'conditions': [
            ['OS=="win"', {
              'dependencies': [
                '../chrome/chrome.gyp:crash_service',
                '../chrome_frame/chrome_frame.gyp:npchrome_frame',
              ],
            }],
            ['OS=="win" and target_arch!="x64"', {
              'dependencies': [
                '../chrome_frame/chrome_frame.gyp:chrome_frame_net_tests',
                '../chrome_frame/chrome_frame.gyp:chrome_frame_perftests',
                '../chrome_frame/chrome_frame.gyp:chrome_frame_reliability_tests',
                '../chrome_frame/chrome_frame.gyp:chrome_frame_tests',
                '../chrome_frame/chrome_frame.gyp:chrome_frame_unittests',
              ],
            }],
            ['OS=="win" and target_arch=="x64"', {
              'dependencies!': [
                '../chrome_frame/chrome_frame.gyp:npchrome_frame',
              ],
              'defines': [
                'OMIT_CHROME_FRAME',
              ],
            }],
            ['OS=="win" and target_arch=="ia32"', {
              'dependencies': [
                '../chrome/chrome.gyp:crash_service_win64',
              ],
            }],
            ['use_ash==1', {
              'dependencies': [
                '../ash/ash.gyp:ash_shell',
                '../ash/ash.gyp:ash_unittests',
              ],
            }],
            ['OS=="linux"', {
              # Tests that currently only work on Linux.
              'dependencies': [
                '../base/base.gyp:base_unittests',
                '../ipc/ipc.gyp:ipc_tests',
                '../sql/sql.gyp:sql_unittests',
                '../sync/sync.gyp:sync_unit_tests',
              ],
            }],
            ['OS=="mac"', {
              # Exclude dependencies that are not currently implemented.
              'dependencies!': [
                '../chrome/chrome.gyp:chrome',
                '../chrome/chrome.gyp:unit_tests',
                '../device/device.gyp:device_unittests',
                '../ui/views/views.gyp:views_unittests',
              ],
            }],
            ['chromeos==1', {
              'dependencies': [
                '../chromeos/chromeos.gyp:chromeos_unittests',
              ],
            }],
          ],
        },
      ],  # targets
    }], # "use_aura==1"
    ['test_isolation_mode != "noop"', {
      'targets': [
        {
          'target_name': 'chromium_swarm_tests',
          'type': 'none',
          'dependencies': [
            '../base/base.gyp:base_unittests_run',
            '../chrome/chrome.gyp:browser_tests_run',
            '../chrome/chrome.gyp:interactive_ui_tests_run',
            '../chrome/chrome.gyp:sync_integration_tests_run',
            '../chrome/chrome.gyp:unit_tests_run',
            '../net/net.gyp:net_unittests_run',
          ],
        }, # target_name: chromium_swarm_tests
      ],
    }],
  ], # conditions
}
