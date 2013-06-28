# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'printing',
      'type': '<(component)',
      'dependencies': [
        '../base/base.gyp:base',
        '../base/base.gyp:base_i18n',
        '../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '../build/temp_gyp/googleurl.gyp:googleurl',
        '../skia/skia.gyp:skia',
        '../third_party/icu/icu.gyp:icui18n',
        '../third_party/icu/icu.gyp:icuuc',
        '../ui/ui.gyp:ui',
      ],
      'defines': [
        'PRINTING_IMPLEMENTATION',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'backend/print_backend.cc',
        'backend/print_backend.h',
        'backend/print_backend_consts.cc',
        'backend/print_backend_consts.h',
        'backend/print_backend_dummy.cc',
        'backend/printing_info_win.cc',
        'backend/printing_info_win.h',
        'emf_win.cc',
        'emf_win.h',
        'image.cc',
        'image_linux.cc',
        'image_mac.cc',
        'image_win.cc',
        'image.h',
        'metafile.h',
        'metafile_impl.h',
        'metafile_skia_wrapper.h',
        'metafile_skia_wrapper.cc',
        'page_number.cc',
        'page_number.h',
        'page_range.cc',
        'page_range.h',
        'page_setup.cc',
        'page_setup.h',
        'page_size_margins.cc',
        'page_size_margins.h',
        'pdf_metafile_cg_mac.cc',
        'pdf_metafile_cg_mac.h',
        'pdf_metafile_skia.h',
        'pdf_metafile_skia.cc',
        'print_destination_interface.h',
        'print_destination_none.cc',
        'print_destination_win.cc',
        'printed_document_gtk.cc',
        'printed_document.cc',
        'printed_document.h',
        'printed_document_mac.cc',
        'printed_document_win.cc',
        'printed_page.cc',
        'printed_page.h',
        'printed_pages_source.h',
        'printing_context.cc',
        'printing_context.h',
        'print_dialog_gtk_interface.h',
        'print_job_constants.cc',
        'print_job_constants.h',
        'print_settings.cc',
        'print_settings.h',
        'print_settings_initializer.cc',
        'print_settings_initializer.h',
        'print_settings_initializer_gtk.cc',
        'print_settings_initializer_gtk.h',
        'print_settings_initializer_mac.cc',
        'print_settings_initializer_mac.h',
        'print_settings_initializer_win.cc',
        'print_settings_initializer_win.h',
        'units.cc',
        'units.h',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '..',
        ],
      },
      'conditions': [
        ['enable_printing!=1', {
          'sources/': [
            ['exclude', '.'],
          ],
        }],
        ['toolkit_uses_gtk == 0',{
            'sources/': [['exclude', '_cairo\\.cc$']]
        }],
        ['OS!="mac"', {'sources/': [['exclude', '_mac\\.(cc|mm?)$']]}],
        ['OS!="win"', {'sources/': [['exclude', '_win\\.cc$']]
          }, {  # else: OS=="win"
            'sources/': [['exclude', '_posix\\.cc$']]
        }],
        ['toolkit_uses_gtk == 1', {
          'dependencies': [
            # For FT_Init_FreeType and friends.
            '../build/linux/system.gyp:freetype2',
            '../build/linux/system.gyp:gtk',
            '../build/linux/system.gyp:gtkprint',
          ],
        }],
        # Mac-Aura does not support printing.
        ['OS=="mac" and use_aura==1',{
          'sources!': [
            'printed_document_mac.cc',
            'printing_context_mac.mm',
            'printing_context_mac.h',
          ],
        }],
        ['OS=="mac" and use_aura==0',{
          'sources': [
            'printing_context_mac.mm',
            'printing_context_mac.h',
          ],
        }],
        ['OS=="win"', {
          'dependencies': [
            '../win8/win8.gyp:win8_util',
          ],
          'conditions': [
            ['use_aura==0', {
              'sources': [
                'printing_context_win.cc',
                'printing_context_win.h',
              ],
          }]],
          'defines': [
            # PRINT_BACKEND_AVAILABLE disables the default dummy implementation
            # of the print backend and enables a custom implementation instead.
            'PRINT_BACKEND_AVAILABLE',
          ],
          'sources': [
            'backend/win_helper.cc',
            'backend/win_helper.h',
            'backend/print_backend_win.cc',
          ],
          'sources!': [
            'print_destination_none.cc',
          ],
        }],
        ['chromeos==1 or use_aura==1',{
          'sources': [
            'printing_context_no_system_dialog.cc',
            'printing_context_no_system_dialog.h',
          ],
        }],
        ['use_cups==1', {
          'dependencies': [
            'cups',
          ],
          'variables': {
            'cups_version': '<!(cups-config --api-version)',
          },
          'conditions': [
            ['OS!="mac"', {
              'dependencies': [
                '../build/linux/system.gyp:libgcrypt',
              ],
            }],
            ['cups_version=="1.6"', {
              'cflags': [
                # CUPS 1.6 deprecated the PPD APIs, but we will stay with this
                # API for now as supported Linux and Mac OS'es are still using
                # older versions of CUPS. More info: crbug.com/226176
                '-Wno-deprecated-declarations',
              ],
            }],
          ],
          'defines': [
            # PRINT_BACKEND_AVAILABLE disables the default dummy implementation
            # of the print backend and enables a custom implementation instead.
            'PRINT_BACKEND_AVAILABLE',
          ],
          'sources': [
            'backend/cups_helper.cc',
            'backend/cups_helper.h',
            'backend/print_backend_cups.cc',
          ],
        }],
        ['OS=="linux" and chromeos==1', {
          'defines': [
            # PRINT_BACKEND_AVAILABLE disables the default dummy implementation
            # of the print backend and enables a custom implementation instead.
            'PRINT_BACKEND_AVAILABLE',
          ],
          'sources': [
            'backend/print_backend_chromeos.cc',
          ],
        }],
        ['toolkit_uses_gtk==1 and chromeos==0', {
          'sources': [
            'printing_context_gtk.cc',
            'printing_context_gtk.h',
          ],
        }],
        ['toolkit_uses_efl==1 and chromeos==0', {
          'sources': [
            'printing_context_efl.cc',
          ],
        }],
      ],
    },
    {
      'target_name': 'printing_unittests',
      'type': 'executable',
      'dependencies': [
        'printing',
        '../testing/gtest.gyp:gtest',
        '../base/base.gyp:test_support_base',
        '../ui/ui.gyp:ui',
      ],
      'sources': [
        'backend/print_backend_unittest.cc',
        'emf_win_unittest.cc',
        'printing_test.h',
        'page_number_unittest.cc',
        'page_range_unittest.cc',
        'page_setup_unittest.cc',
        'pdf_metafile_cg_mac_unittest.cc',
        'printed_page_unittest.cc',
        'run_all_unittests.cc',
        'units_unittest.cc',
      ],
      'conditions': [
        ['enable_printing!=1', {
          'sources/': [
            ['exclude', '.'],
            ['include', 'run_all_unittests.cc'],
          ],
        }],
        ['toolkit_uses_gtk == 0', {'sources/': [['exclude', '_gtk_unittest\\.cc$']]}],
        ['OS!="mac"', {'sources/': [['exclude', '_mac_unittest\\.(cc|mm?)$']]}],
        ['OS!="win"', {'sources/': [['exclude', '_win_unittest\\.cc$']]}],
        ['OS=="win" and use_aura == 0', {
          'sources': [
            'printing_context_win_unittest.cc',
          ]
        }],
        ['use_cups==1', {
          'defines': [
            'USE_CUPS',
          ],
          'sources': [
            'backend/cups_helper_unittest.cc',
          ],
        }],
        ['toolkit_uses_gtk == 1', {
          'dependencies': [
            '../build/linux/system.gyp:gtk',
          ],
        }],
        [ 'os_posix == 1 and OS != "mac" and OS != "android" and OS != "ios"', {
          'conditions': [
            ['linux_use_tcmalloc == 1', {
              'dependencies': [
                '../base/allocator/allocator.gyp:allocator',
              ],
            }],
          ],
        }],
      ],
      # TODO(jschuh): crbug.com/167187 fix size_t to int truncations.
      'msvs_disabled_warnings': [ 4267, ],
    },
    {
      'target_name': 'cups',
      'type': 'none',
      'conditions': [
        ['use_cups==1', {
          'direct_dependent_settings': {
            'defines': [
              'USE_CUPS',
            ],
            'conditions': [
              ['OS=="mac"', {
                'link_settings': {
                  'libraries': [
                    '$(SDKROOT)/usr/lib/libcups.dylib',
                  ]
                },
              }, {
                'link_settings': {
                  'libraries': [
                    '<!@(python cups_config_helper.py --libs)',
                  ],
                },
              }],
              [ 'os_bsd==1', {
                'cflags': [
                  '<!@(python cups_config_helper.py --cflags)',
                ],
              }],
            ],
          },
        }],
      ],
    },
  ],
}
