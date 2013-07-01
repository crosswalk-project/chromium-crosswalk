{
  'variables': {
    'efl_webview_product_name': 'EFL WebView',
    # TODO: define efl webview version format.
    'cameo_version': '0.28.0.1',
    'conditions': [
      ['OS=="linux"', {
       'use_custom_freetype%': 1,
      }, {
       'use_custom_freetype%': 0,
      }],
    ], # conditions
  },
  'targets': [
    {
      'target_name': 'efl_webview',
      'type': 'shared_library',
      'defines': ['CONTENT_IMPLEMENTATION'],
      'variables': {
        'chromium_code': 1,
      },
      'dependencies': [
        '../base/base.gyp:base',
        '../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '../build/temp_gyp/googleurl.gyp:googleurl',
        '../content/content.gyp:content_app',
        '../content/content.gyp:content_browser',
        '../content/content.gyp:content_common',
        '../content/content.gyp:content_gpu',
        '../content/content.gyp:content_plugin',
        '../content/content.gyp:content_ppapi_plugin',
        '../content/content.gyp:content_renderer',
        '../content/content.gyp:content_utility',
        '../content/content.gyp:content_worker',
        '../content/content_resources.gyp:content_resources',
        '../ipc/ipc.gyp:ipc',
        '../media/media.gyp:media',
        '../net/net.gyp:net',
        '../net/net.gyp:net_resources',
        '../skia/skia.gyp:skia',
        '../third_party/WebKit/Source/WebKit/chromium/WebKit.gyp:webkit',
        '../ui/gl/gl.gyp:gl',
        '../ui/ui.gyp:ui',
        '../v8/tools/gyp/v8.gyp:v8',
        '../webkit/support/webkit_support.gyp:webkit_resources',
        '../webkit/support/webkit_support.gyp:webkit_support',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'lib/dummy.cc'
      ],
      'conditions': [
        ['OS=="linux"', {
          'dependencies': [
            '../build/linux/system.gyp:fontconfig',
          ],
        }],  # OS=="linux"
        ['os_posix==1 and linux_use_tcmalloc==1', {
          'dependencies': [
            # This is needed by content/app/content_main_runner.cc
            '../base/allocator/allocator.gyp:allocator',
          ],
        }],  # os_posix==1 and linux_use_tcmalloc==1
        ['use_custom_freetype==1', {
          'dependencies': [
             '../third_party/freetype2/freetype2.gyp:freetype2',
          ],
        }],  # use_custom_freetype==1
        ['toolkit_uses_gtk == 1', {
          'dependencies': [
            '<(DEPTH)/build/linux/system.gyp:gtk',
          ],
        }],  # toolkit_uses_gtk
        ['toolkit_uses_efl == 1', {
          'dependencies': [
            '../build/linux/system.gyp:efl',
          ],
        }],
      ],
    },
    {
      'target_name': 'efl_webview_example',
      'type': 'executable',
      'defines!': ['CONTENT_IMPLEMENTATION'],
      'dependencies': [
        'efl_webview',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'examples/main.cc',
      ],
      'conditions': [
        ['toolkit_uses_efl == 1', {
          'dependencies': [
            '../build/linux/system.gyp:efl',
          ],
        }],
      ],
    },
    {
      'target_name': 'efl_process',
      'type': 'executable',
      'defines': ['EFL_PROCESS=efl_process'],
      'defines': ['CONTENT_IMPLEMENTATION'],
      'dependencies': [
        'efl_webview',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'process/main.cc',
      ],
      'conditions': [
        ['toolkit_uses_efl == 1', {
          'dependencies': [
            '../build/linux/system.gyp:efl',
          ],
        }],
      ],
    },
  ],
}
