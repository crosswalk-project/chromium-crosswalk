vars = {
  'angle_revision':
    '5576734a46f80bc8c5265cac9fb1af774b658d89',
  'boringssl_revision':
    'd7421ebf6cae07051caf657016f160585b64f8a6',
  'buildspec_platforms':
    'all',
  'buildtools_revision':
    '3ba3ca22ec610fe95683f6bfdeea9d90c768abd7',
  'chromium_git':
    'https://chromium.googlesource.com',
  'deqp_revision':
    '194294e69d44eac48bc1fb063bd607189650aa5e',
  'deqp_url':
    'https://android.googlesource.com/platform/external/deqp',
  'google_toolbox_for_mac_revision':
    '401878398253074c515c03cb3a3f8bb0cc8da6e9',
  'googlecode_url':
    'http://%s.googlecode.com/svn',
  'lighttpd_revision':
    '9dfa55d15937a688a92cbf2b7a8621b0927d06eb',
  'lss_revision':
    '4fc942258fe5509549333b9487ec018e3c8c5b10',
  'nacl_revision':
    '546ef11ffcbedf8c33bfa12643408c1182b6839e',
  'nss_revision':
    'a676aa04ff18b30983cac03dede5861b3db62c9d',
  'openmax_dl_revision':
    '37b900ccc45cfab71faf4fd332df15369766b94c',
  'pdfium_revision':
    '4274ea205ca2219f6a4cfc54025284c1aa3b14b8',
  'sfntly_revision':
    '130f832eddf98467e6578b548cb74ce17d04a26d',
  'skia_revision':
    'c94cd7cc01b655b7f4289537962c36a4ee8dd63e',
  'swarming_revision':
    '8fce79620b04bbe5415ace1103db27505bdc4c06',
  'v8_revision':
    '10449d46aa20f10f39598627bf07f70def597029'
}

allowed_hosts = [
  'android.googlesource.com',
  'boringssl.googlesource.com',
  'chromium.googlesource.com',
  'pdfium.googlesource.com'
]

deps = {
  'src/breakpad/src':
    (Var("chromium_git")) + '/breakpad/breakpad/src.git@615a6b0e2b376e3ae946972a52ef897bf6daaff3',
  'src/buildtools':
    (Var("chromium_git")) + '/chromium/buildtools.git@3ba3ca22ec610fe95683f6bfdeea9d90c768abd7',
  'src/chrome/test/data/perf/canvas_bench':
    (Var("chromium_git")) + '/chromium/canvas_bench.git@a7b40ea5ae0239517d78845a5fc9b12976bfc732',
  'src/chrome/test/data/perf/frame_rate/content':
    (Var("chromium_git")) + '/chromium/frame_rate/content.git@c10272c88463efeef6bb19c9ec07c42bc8fe22b9',
  'src/media/cdm/api':
    (Var("chromium_git")) + '/chromium/cdm.git@a4773c3cec827c3a880e8a2c22e43a287ced0d20',
  'src/native_client':
    (Var("chromium_git")) + '/native_client/src/native_client.git@546ef11ffcbedf8c33bfa12643408c1182b6839e',
  'src/sdch/open-vcdiff':
    (Var("chromium_git")) + '/external/github.com/google/open-vcdiff.git@21d7d0b9c3d0c3ccbdb221c85ae889373f0a2a58',
  'src/testing/gmock':
    (Var("chromium_git")) + '/external/googlemock.git@0421b6f358139f02e102c9c332ce19a33faf75be',
  'src/testing/gtest':
    (Var("chromium_git")) + '/external/github.com/google/googletest.git@6f8a66431cb592dad629028a50b3dd418a408c87',
  'src/third_party/angle':
    (Var("chromium_git")) + '/angle/angle.git@5576734a46f80bc8c5265cac9fb1af774b658d89',
  'src/third_party/bidichecker':
    (Var("chromium_git")) + '/external/bidichecker/lib.git@97f2aa645b74c28c57eca56992235c79850fa9e0',
  'src/third_party/boringssl/src':
    'https://boringssl.googlesource.com/boringssl.git@d7421ebf6cae07051caf657016f160585b64f8a6',
  'src/third_party/catapult':
    (Var("chromium_git")) + '/external/github.com/catapult-project/catapult.git@704c40202baaeb9484af0622aedf41c4a48e0df2',
  'src/third_party/cld_2/src':
    (Var("chromium_git")) + '/external/github.com/CLD2Owners/cld2.git@84b58a5d7690ebf05a91406f371ce00c3daf31c0',
  'src/third_party/colorama/src':
    (Var("chromium_git")) + '/external/colorama.git@799604a1041e9b3bc5d2789ecbd7e8db2e18e6b8',
  'src/third_party/crashpad/crashpad':
    (Var("chromium_git")) + '/crashpad/crashpad.git@97b0f86d0ccb095391ca64b3948f0d6c02975ac1',
  'src/third_party/dom_distiller_js/dist':
    (Var("chromium_git")) + '/external/github.com/chromium/dom-distiller-dist.git@e21fe06cb71327ec62431f823e783d7b02f97b26',
  'src/third_party/ffmpeg':
    (Var("chromium_git")) + '/chromium/third_party/ffmpeg.git@c6fe5d0f501ea3321689f662370a73c186373b57',
  'src/third_party/flac':
    (Var("chromium_git")) + '/chromium/deps/flac.git@2c4b86af352b23498315c016dc207e3fb2733fc0',
  'src/third_party/hunspell_dictionaries':
    (Var("chromium_git")) + '/chromium/deps/hunspell_dictionaries.git@c106afdcec5d3de2622e19f1b3294c47bbd8bd72',
  'src/third_party/icu':
    (Var("chromium_git")) + '/chromium/deps/icu.git@42c58d4e49f2250039f0e98d43e0b76e8f5ca024',
  'src/third_party/jsoncpp/source':
    (Var("chromium_git")) + '/external/github.com/open-source-parsers/jsoncpp.git@f572e8e42e22cfcf5ab0aea26574f408943edfa4',
  'src/third_party/leveldatabase/src':
    (Var("chromium_git")) + '/external/leveldb.git@1a9648e1f55be762042263df5dc7cb6388702e25',
  'src/third_party/libaddressinput/src':
    (Var("chromium_git")) + '/external/libaddressinput.git@5eeeb797e79fa01503fcdcbebdc50036fac023ef',
  'src/third_party/libexif/sources':
    (Var("chromium_git")) + '/chromium/deps/libexif/sources.git@ed98343daabd7b4497f97fda972e132e6877c48a',
  'src/third_party/libjingle/source/talk':
    (Var("chromium_git")) + '/external/webrtc/trunk/talk.git@87af2765fbc041187d9b91637a6cc373a42acf07',
  'src/third_party/libjpeg_turbo':
    (Var("chromium_git")) + '/chromium/deps/libjpeg_turbo.git@e4e75037f29745f1546b6ebf5cf532e841c04c2c',
  'src/third_party/libphonenumber/src/phonenumbers':
    (Var("chromium_git")) + '/external/libphonenumber/cpp/src/phonenumbers.git@0d6e3e50e17c94262ad1ca3b7d52b11223084bca',
  'src/third_party/libphonenumber/src/resources':
    (Var("chromium_git")) + '/external/libphonenumber/resources.git@b6dfdc7952571ff7ee72643cd88c988cbe966396',
  'src/third_party/libphonenumber/src/test':
    (Var("chromium_git")) + '/external/libphonenumber/cpp/test.git@f351a7e007f9c9995494499120bbc361ca808a16',
  'src/third_party/libsrtp':
    (Var("chromium_git")) + '/chromium/deps/libsrtp.git@502e81a238a864cfce774d4de3893810629bf227',
  'src/third_party/libvpx_new/source/libvpx':
    (Var("chromium_git")) + '/webm/libvpx.git@0941ff72a00732cea6750477edfe649348e699de',
  'src/third_party/libwebm/source':
    (Var("chromium_git")) + '/webm/libwebm.git@75a6d2da8b63e0c446ec0ce1ac942c2962d959d7',
  'src/third_party/libyuv':
    (Var("chromium_git")) + '/libyuv/libyuv.git@6100f50f136829b9769baf79306b31e0665ec16a',
  'src/third_party/mesa/src':
    (Var("chromium_git")) + '/chromium/deps/mesa.git@a3dc7d623fbb83a6246adf3d363922ad3909c14c',
  'src/third_party/openh264/src':
    (Var("chromium_git")) + '/external/github.com/cisco/openh264@b37cda248234162033e3e11b0335f3131cdfe488',
  'src/third_party/openmax_dl':
    (Var("chromium_git")) + '/external/webrtc/deps/third_party/openmax.git@37b900ccc45cfab71faf4fd332df15369766b94c',
  'src/third_party/opus/src':
    (Var("chromium_git")) + '/chromium/deps/opus.git@cae696156f1e60006e39821e79a1811ae1933c69',
  'src/third_party/pdfium':
    'https://pdfium.googlesource.com/pdfium.git@e6d8b1ad419408bad268ea7b22ab0034240be4f3',
  'src/third_party/py_trace_event/src':
    (Var("chromium_git")) + '/external/py_trace_event.git@dd463ea9e2c430de2b9e53dea57a77b4c3ac9b30',
  'src/third_party/pyftpdlib/src':
    (Var("chromium_git")) + '/external/pyftpdlib.git@2be6d65e31c7ee6320d059f581f05ae8d89d7e45',
  'src/third_party/pywebsocket/src':
    (Var("chromium_git")) + '/external/github.com/google/pywebsocket.git@09d842794c14ca064f8a4223b313ac2c84bd33c8',
  'src/third_party/safe_browsing/testing':
    (Var("chromium_git")) + '/external/google-safe-browsing/testing.git@9d7e8064f3ca2e45891470c9b5b1dce54af6a9d6',
  'src/third_party/scons-2.0.1':
    (Var("chromium_git")) + '/native_client/src/third_party/scons-2.0.1.git@1c1550e17fc26355d08627fbdec13d8291227067',
  'src/third_party/sfntly/src':
    (Var("chromium_git")) + '/external/github.com/googlei18n/sfntly.git@130f832eddf98467e6578b548cb74ce17d04a26d',
  'src/third_party/skia':
    (Var("chromium_git")) + '/skia.git@47f2cbd7676d37c28d30f9f34476820adb32d1a3',
  'src/third_party/smhasher/src':
    (Var("chromium_git")) + '/external/smhasher.git@e87738e57558e0ec472b2fc3a643b838e5b6e88f',
  'src/third_party/snappy/src':
    (Var("chromium_git")) + '/external/snappy.git@762bb32f0c9d2f31ba4958c7c0933d22e80c20bf',
  'src/third_party/usrsctp/usrsctplib':
    (Var("chromium_git")) + '/external/usrsctplib.git@36444a999739e9e408f8f587cb4c3ffeef2e50ac',
  'src/third_party/webdriver/pylib':
    (Var("chromium_git")) + '/external/selenium/py.git@5fd78261a75fe08d27ca4835fb6c5ce4b42275bd',
  'src/third_party/webgl/src':
    (Var("chromium_git")) + '/external/khronosgroup/webgl.git@421bc6095d1065bb6057e33fbab737b35d39930d',
  'src/third_party/webpagereplay':
    (Var("chromium_git")) + '/external/github.com/chromium/web-page-replay.git@7564939bdf6482d57b9bd5e9c931679f96d8cf75',
  'src/third_party/webrtc':
    (Var("chromium_git")) + '/external/webrtc/trunk/webrtc.git@5908e8d1598c14351761c4b416b22e1b2c339ad6',
  'src/third_party/yasm/source/patched-yasm':
    (Var("chromium_git")) + '/chromium/deps/yasm/patched-yasm.git@4671120cd8558ce62ee8672ebf3eb6f5216f909b',
  'src/tools/gyp':
    (Var("chromium_git")) + '/external/gyp.git@33b351b2eda8facc464c89213fc8cd9919666c05',
  'src/tools/page_cycler/acid3':
    (Var("chromium_git")) + '/chromium/deps/acid3.git@6be0a66a1ebd7ebc5abc1b2f405a945f6d871521',
  'src/tools/swarming_client':
    (Var("chromium_git")) + '/external/swarming.client.git@8fce79620b04bbe5415ace1103db27505bdc4c06',
  'src/v8':
    (Var("chromium_git")) + '/v8/v8.git@d3de228428dd15d5fc50e5988611e23b83a2abc5'
}

deps_os = {
  'android': {
    'src/third_party/android_protobuf/src':
      (Var("chromium_git")) + '/external/android_protobuf.git@999188d0dc72e97f7fe08bb756958a2cf090f4e7',
    'src/third_party/android_tools':
      (Var("chromium_git")) + '/android_tools.git@54492f99c84cab0826a8e656efeb33a1b1bf5a04',
    'src/third_party/apache-mime4j':
      (Var("chromium_git")) + '/chromium/deps/apache-mime4j.git@28cb1108bff4b6cf0a2e86ff58b3d025934ebe3a',
    'src/third_party/appurify-python/src':
      (Var("chromium_git")) + '/external/github.com/appurify/appurify-python.git@ee7abd5c5ae3106f72b2a0b9d2cb55094688e867',
    'src/third_party/cardboard-java/src':
      (Var("chromium_git")) + '/external/github.com/googlesamples/cardboard-java.git@e36ee57e72bbd057ddb53b127954177b50e18df7',
    'src/third_party/custom_tabs_client/src':
      (Var("chromium_git")) + '/external/github.com/GoogleChrome/custom-tabs-client.git@41ded9d4b5dbaf7b79e291ee8a222f5efbc52190',
    'src/third_party/elfutils/src':
      (Var("chromium_git")) + '/external/elfutils.git@249673729a7e5dbd5de4f3760bdcaa3d23d154d7',
    'src/third_party/errorprone/lib':
      (Var("chromium_git")) + '/chromium/third_party/errorprone.git@0eea83b66343133b9c76b7d3288c30321818ebcf',
    'src/third_party/findbugs':
      (Var("chromium_git")) + '/chromium/deps/findbugs.git@57f05238d3ac77ea0a194813d3065dd780c6e566',
    'src/third_party/freetype-android/src':
      (Var("chromium_git")) + '/chromium/src/third_party/freetype2.git@66cf29b1bc869b44863b0de2115bd017dfcce849',
    'src/third_party/httpcomponents-client':
      (Var("chromium_git")) + '/chromium/deps/httpcomponents-client.git@285c4dafc5de0e853fa845dce5773e223219601c',
    'src/third_party/httpcomponents-core':
      (Var("chromium_git")) + '/chromium/deps/httpcomponents-core.git@9f7180a96f8fa5cab23f793c14b413356d419e62',
    'src/third_party/jarjar':
      (Var("chromium_git")) + '/chromium/deps/jarjar.git@2e1ead4c68c450e0b77fe49e3f9137842b8b6920',
    'src/third_party/jsr-305/src':
      (Var("chromium_git")) + '/external/jsr-305.git@642c508235471f7220af6d5df2d3210e3bfc0919',
    'src/third_party/junit/src':
      (Var("chromium_git")) + '/external/junit.git@45a44647e7306262162e1346b750c3209019f2e1',
    'src/third_party/lss':
      (Var("chromium_git")) + '/external/linux-syscall-support/lss.git@4fc942258fe5509549333b9487ec018e3c8c5b10',
    'src/third_party/mockito/src':
      (Var("chromium_git")) + '/external/mockito/mockito.git@4d987dcd923b81525c42b1333e6c4e07440776c3',
    'src/third_party/requests/src':
      (Var("chromium_git")) + '/external/github.com/kennethreitz/requests.git@f172b30356d821d180fa4ecfa3e71c7274a32de4',
    'src/third_party/robolectric/lib':
      (Var("chromium_git")) + '/chromium/third_party/robolectric.git@6b63c99a8b6967acdb42cbed0adb067c80efc810',
    'src/third_party/ub-uiautomator/lib':
      (Var("chromium_git")) + '/chromium/third_party/ub-uiautomator.git@00270549ce3161ae72ceb24712618ea28b4f9434'
  },
  'ios': {
    'src/chrome/test/data/perf/canvas_bench': None,
    'src/chrome/test/data/perf/frame_rate/content': None,
    'src/ios/third_party/gcdwebserver/src':
      (Var("chromium_git")) + '/external/github.com/swisspol/GCDWebServer.git@3d5fd0b8281a7224c057deb2d17709b5bea64836',
    'src/native_client': None,
    'src/third_party/class-dump/src':
      (Var("chromium_git")) + '/external/github.com/nygard/class-dump.git@978d177ca6f0d2e5e34acf3e8dadc63e3140ebbc',
    'src/third_party/ffmpeg': None,
    'src/third_party/google_toolbox_for_mac/src':
      (Var("chromium_git")) + '/external/github.com/google/google-toolbox-for-mac.git@401878398253074c515c03cb3a3f8bb0cc8da6e9',
    'src/third_party/hunspell_dictionaries': None,
    'src/third_party/nss':
      (Var("chromium_git")) + '/chromium/deps/nss.git@a676aa04ff18b30983cac03dede5861b3db62c9d',
    'src/third_party/webgl': None
  },
  'mac': {
    'src/chrome/installer/mac/third_party/xz/xz':
      (Var("chromium_git")) + '/chromium/deps/xz.git@eecaf55632ca72e90eb2641376bce7cdbc7284f7',
    'src/chrome/tools/test/reference_build/chrome_mac':
      (Var("chromium_git")) + '/chromium/reference_builds/chrome_mac.git@8dc181329e7c5255f83b4b85dc2f71498a237955',
    'src/third_party/google_toolbox_for_mac/src':
      (Var("chromium_git")) + '/external/github.com/google/google-toolbox-for-mac.git@401878398253074c515c03cb3a3f8bb0cc8da6e9',
    'src/third_party/lighttpd':
      (Var("chromium_git")) + '/chromium/deps/lighttpd.git@9dfa55d15937a688a92cbf2b7a8621b0927d06eb',
    'src/third_party/nss':
      (Var("chromium_git")) + '/chromium/deps/nss.git@a676aa04ff18b30983cac03dede5861b3db62c9d',
    'src/third_party/pdfsqueeze':
      (Var("chromium_git")) + '/external/pdfsqueeze.git@5936b871e6a087b7e50d4cbcb122378d8a07499f'
  },
  'unix': {
    'src/chrome/tools/test/reference_build/chrome_linux':
      (Var("chromium_git")) + '/chromium/reference_builds/chrome_linux64.git@033d053a528e820e1de3e2db766678d862a86b36',
    'src/third_party/chromite':
      (Var("chromium_git")) + '/chromiumos/chromite.git@e19f83ba227bf1ec0077f5d3a816a415f1dd88d0',
    'src/third_party/cros_system_api':
      (Var("chromium_git")) + '/chromiumos/platform/system_api.git@ed198c46f73e67d9a494c3c0623e3ba16e4da5e8',
    'src/third_party/deqp/src':
      'https://android.googlesource.com/platform/external/deqp@194294e69d44eac48bc1fb063bd607189650aa5e',
    'src/third_party/fontconfig/src':
      (Var("chromium_git")) + '/external/fontconfig.git@f16c3118e25546c1b749f9823c51827a60aeb5c1',
    'src/third_party/freetype2/src':
      (Var("chromium_git")) + '/chromium/src/third_party/freetype2.git@fc1532a7c4c592f24a4c1a0261d2845524ca5cff',
    'src/third_party/liblouis/src':
      (Var("chromium_git")) + '/external/liblouis-github.git@5f9c03f2a3478561deb6ae4798175094be8a26c2',
    'src/third_party/lss':
      (Var("chromium_git")) + '/external/linux-syscall-support/lss.git@4fc942258fe5509549333b9487ec018e3c8c5b10',
    'src/third_party/minigbm/src':
      (Var("chromium_git")) + '/chromiumos/platform/minigbm.git@f9d2ab79a15a1bb6a1307f3b608964c81c27791b',
    'src/third_party/pyelftools':
      (Var("chromium_git")) + '/chromiumos/third_party/pyelftools.git@bdc1d380acd88d4bfaf47265008091483b0d614e',
    'src/third_party/xdg-utils':
      (Var("chromium_git")) + '/chromium/deps/xdg-utils.git@d80274d5869b17b8c9067a1022e4416ee7ed5e0d'
  },
  'win': {
    'src/chrome/tools/test/reference_build/chrome_win':
      (Var("chromium_git")) + '/chromium/reference_builds/chrome_win.git@f8a3a845dfc845df6b14280f04f86a61959357ef',
    'src/third_party/bison':
      (Var("chromium_git")) + '/chromium/deps/bison.git@083c9a45e4affdd5464ee2b224c2df649c6e26c3',
    'src/third_party/cygwin':
      (Var("chromium_git")) + '/chromium/deps/cygwin.git@c89e446b273697fadf3a10ff1007a97c0b7de6df',
    'src/third_party/deqp/src':
      'https://android.googlesource.com/platform/external/deqp@194294e69d44eac48bc1fb063bd607189650aa5e',
    'src/third_party/gnu_binutils':
      (Var("chromium_git")) + '/native_client/deps/third_party/gnu_binutils.git@f4003433b61b25666565690caf3d7a7a1a4ec436',
    'src/third_party/gperf':
      (Var("chromium_git")) + '/chromium/deps/gperf.git@d892d79f64f9449770443fb06da49b5a1e5d33c1',
    'src/third_party/lighttpd':
      (Var("chromium_git")) + '/chromium/deps/lighttpd.git@9dfa55d15937a688a92cbf2b7a8621b0927d06eb',
    'src/third_party/mingw-w64/mingw/bin':
      (Var("chromium_git")) + '/native_client/deps/third_party/mingw-w64/mingw/bin.git@3cc8b140b883a9fe4986d12cfd46c16a093d3527',
    'src/third_party/nacl_sdk_binaries':
      (Var("chromium_git")) + '/chromium/deps/nacl_sdk_binaries.git@759dfca03bdc774da7ecbf974f6e2b84f43699a5',
    'src/third_party/nss':
      (Var("chromium_git")) + '/chromium/deps/nss.git@a676aa04ff18b30983cac03dede5861b3db62c9d',
    'src/third_party/pefile':
      (Var("chromium_git")) + '/external/pefile.git@72c6ae42396cb913bcab63c15585dc3b5c3f92f1',
    'src/third_party/perl':
      (Var("chromium_git")) + '/chromium/deps/perl.git@ac0d98b5cee6c024b0cffeb4f8f45b6fc5ccdb78',
    'src/third_party/psyco_win32':
      (Var("chromium_git")) + '/chromium/deps/psyco_win32.git@f5af9f6910ee5a8075bbaeed0591469f1661d868',
    'src/third_party/yasm/binaries':
      (Var("chromium_git")) + '/chromium/deps/yasm/binaries.git@52f9b3f4b0aa06da24ef8b123058bb61ee468881'
  }
}

hooks = [
  {
    'action': [
      'python',
      'src/build/landmines.py'
    ],
    'pattern':
      '.',
    'name':
      'landmines'
  },
  {
    'action': [
      'python',
      'src/build/download_nacl_toolchains.py',
      '--mode',
      'nacl_core_sdk',
      'sync',
      '--extract'
    ],
    'pattern':
      '.',
    'name':
      'nacltools'
  },
  {
    'action': [
      'python',
      'src/build/android/play_services/update.py',
      'download'
    ],
    'pattern':
      '.',
    'name':
      'sdkextras'
  },
  {
    'action': [
      'python',
      'src/build/linux/sysroot_scripts/install-sysroot.py',
      '--running-as-hook'
    ],
    'pattern':
      '.',
    'name':
      'sysroot'
  },
  {
    'action': [
      'python',
      'src/build/vs_toolchain.py',
      'update'
    ],
    'pattern':
      '.',
    'name':
      'win_toolchain'
  },
  {
    'action': [
      'python',
      'src/third_party/binutils/download.py'
    ],
    'pattern':
      'src/third_party/binutils',
    'name':
      'binutils'
  },
  {
    'action': [
      'python',
      'src/tools/clang/scripts/update.py',
      '--if-needed'
    ],
    'pattern':
      '.',
    'name':
      'clang'
  },
  {
    'action': [
      'python',
      'src/build/util/lastchange.py',
      '-o',
      'src/build/util/LASTCHANGE'
    ],
    'pattern':
      '.',
    'name':
      'lastchange'
  },
  {
    'action': [
      'python',
      'src/build/util/lastchange.py',
      '--git-hash-only',
      '-s',
      'src/third_party/WebKit',
      '-o',
      'src/build/util/LASTCHANGE.blink'
    ],
    'pattern':
      '.',
    'name':
      'lastchange_blink'
  },
  {
    'action': [
      'python',
      'src/ios/build/util/canary_version.py',
      '-o',
      'src/ios/build/util/CANARY_VERSION'
    ],
    'pattern':
      '.',
    'name':
      'ios_canary_version'
  },
  {
    'action': [
      'download_from_google_storage',
      '--no_resume',
      '--platform=win32',
      '--no_auth',
      '--bucket',
      'chromium-gn',
      '-s',
      'src/buildtools/win/gn.exe.sha1'
    ],
    'pattern':
      '.',
    'name':
      'gn_win'
  },
  {
    'action': [
      'download_from_google_storage',
      '--no_resume',
      '--platform=darwin',
      '--no_auth',
      '--bucket',
      'chromium-gn',
      '-s',
      'src/buildtools/mac/gn.sha1'
    ],
    'pattern':
      '.',
    'name':
      'gn_mac'
  },
  {
    'action': [
      'download_from_google_storage',
      '--no_resume',
      '--platform=linux*',
      '--no_auth',
      '--bucket',
      'chromium-gn',
      '-s',
      'src/buildtools/linux64/gn.sha1'
    ],
    'pattern':
      '.',
    'name':
      'gn_linux64'
  },
  {
    'action': [
      'download_from_google_storage',
      '--no_resume',
      '--platform=win32',
      '--no_auth',
      '--bucket',
      'chromium-clang-format',
      '-s',
      'src/buildtools/win/clang-format.exe.sha1'
    ],
    'pattern':
      '.',
    'name':
      'clang_format_win'
  },
  {
    'action': [
      'download_from_google_storage',
      '--no_resume',
      '--platform=darwin',
      '--no_auth',
      '--bucket',
      'chromium-clang-format',
      '-s',
      'src/buildtools/mac/clang-format.sha1'
    ],
    'pattern':
      '.',
    'name':
      'clang_format_mac'
  },
  {
    'action': [
      'download_from_google_storage',
      '--no_resume',
      '--platform=linux*',
      '--no_auth',
      '--bucket',
      'chromium-clang-format',
      '-s',
      'src/buildtools/linux64/clang-format.sha1'
    ],
    'pattern':
      '.',
    'name':
      'clang_format_linux'
  },
  {
    'action': [
      'download_from_google_storage',
      '--no_resume',
      '--platform=darwin',
      '--no_auth',
      '--bucket',
      'chromium-libcpp',
      '-s',
      'src/third_party/libc++-static/libc++.a.sha1'
    ],
    'pattern':
      '.',
    'name':
      'libcpp_mac'
  },
  {
    'action': [
      'download_from_google_storage',
      '--no_resume',
      '--platform=win32',
      '--no_auth',
      '--bucket',
      'chromium-luci',
      '-d',
      'src/tools/luci-go/win64'
    ],
    'pattern':
      '.',
    'name':
      'luci-go_win'
  },
  {
    'action': [
      'download_from_google_storage',
      '--no_resume',
      '--platform=darwin',
      '--no_auth',
      '--bucket',
      'chromium-luci',
      '-d',
      'src/tools/luci-go/mac64'
    ],
    'pattern':
      '.',
    'name':
      'luci-go_mac'
  },
  {
    'action': [
      'download_from_google_storage',
      '--no_resume',
      '--platform=linux*',
      '--no_auth',
      '--bucket',
      'chromium-luci',
      '-d',
      'src/tools/luci-go/linux64'
    ],
    'pattern':
      '.',
    'name':
      'luci-go_linux'
  },
  {
    'action': [
      'download_from_google_storage',
      '--no_resume',
      '--platform=linux*',
      '--no_auth',
      '--bucket',
      'chromium-eu-strip',
      '-s',
      'src/build/linux/bin/eu-strip.sha1'
    ],
    'pattern':
      '.',
    'name':
      'eu-strip'
  },
  {
    'action': [
      'download_from_google_storage',
      '--no_resume',
      '--platform=win32',
      '--no_auth',
      '--bucket',
      'chromium-drmemory',
      '-s',
      'src/third_party/drmemory/drmemory-windows-sfx.exe.sha1'
    ],
    'pattern':
      '.',
    'name':
      'drmemory'
  },
  {
    'action': [
      'python',
      'src/build/get_syzygy_binaries.py',
      '--output-dir=src/third_party/syzygy/binaries',
      '--revision=a8456d9248a126881dcfb8707ca7dcdae56e1ac7',
      '--overwrite'
    ],
    'pattern':
      '.',
    'name':
      'syzygy-binaries'
  },
  {
    'action': [
      'python',
      'src/build/get_syzygy_binaries.py',
      '--output-dir=src/third_party/kasko',
      '--revision=56f13b37f044639b4c28cb75f327ca5e3db8758e',
      '--resource=kasko.zip',
      '--resource=kasko_symbols.zip',
      '--overwrite'
    ],
    'pattern':
      '.',
    'name':
      'kasko'
  },
  {
    'action': [
      'download_from_google_storage',
      '--no_resume',
      '--platform=win32',
      '--directory',
      '--recursive',
      '--no_auth',
      '--num_threads=16',
      '--bucket',
      'chromium-apache-win32',
      'src/third_party/apache-win32'
    ],
    'pattern':
      '\\.sha1',
    'name':
      'apache_win32'
  },
  {
    'action': [
      'python',
      'src/third_party/instrumented_libraries/scripts/download_binaries.py'
    ],
    'pattern':
      '\\.sha1',
    'name':
      'instrumented_libraries'
  },
  {
    'action': [
      'python',
      'src/tools/remove_stale_pyc_files.py',
      'src/android_webview/tools',
      'src/gpu/gles2_conform_support',
      'src/infra',
      'src/ppapi',
      'src/printing',
      'src/third_party/closure_compiler/build',
      'src/tools'
    ],
    'pattern':
      '.',
    'name':
      'remove_stale_pyc_files'
  },
  {
    'action': [
      'python',
      'src/build/gyp_chromium'
    ],
    'pattern':
      '.',
    'name':
      'gyp'
  }
]

include_rules = [
  '+base',
  '+build',
  '+ipc',
  '+library_loaders',
  '+testing',
  '+third_party/icu/source/common/unicode',
  '+third_party/icu/source/i18n/unicode',
  '+url'
]

skip_child_includes = [
  'breakpad',
  'native_client_sdk',
  'out',
  'sdch',
  'skia',
  'testing',
  'v8',
  'win8'
]
