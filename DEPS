vars = {
  'angle_revision':
    '21ce9b02de90b81d3c3fab95f02ad45fbffd7350',
  'boringssl_git':
    'https://boringssl.googlesource.com',
  'boringssl_revision':
    'a07c0fc8f2181d086b1118712e2ceb0d1496fa0b',
  'buildspec_platforms':
    'all',
  'buildtools_revision':
    'b73e5f70d7ac6be98fb2555461f631afc90216ce',
  'chromium_git':
    'https://chromium.googlesource.com',
  'chromiumos_git':
    'https://chromium.googlesource.com/chromiumos',
  'google_toolbox_for_mac_revision':
    'ce47a231ea0b238fbe95538e86cc61d74c234be6',
  'googlecode_url':
    'http://%s.googlecode.com/svn',
  'libvpx_revision':
    'd1c022c097f22987d521643c4802b623e572393a',
  'lighttpd_revision':
    '9dfa55d15937a688a92cbf2b7a8621b0927d06eb',
  'llvm_git':
    'https://llvm.googlesource.com',
  'llvm_url':
    'http://src.chromium.org/llvm-project',
  'lss_revision':
    '6f97298fe3794e92c8c896a6bc06e0b36e4c3de3',
  'nacl_revision':
    '862ca265aa1fdcabe8b3ab518251381bfbf48468',
  'nss_revision':
    '95068068df410e398ac221a9195c999b22bd63e9',
  'openmax_dl_revision':
    '22bb1085a6a0f6f3589a8c3d60ed0a9b82248275',
  'pdfium_git':
    'https://pdfium.googlesource.com',
  'pdfium_revision':
    'f33cdd54d5d5340d8a662048d9cf4abe7d5f0488',
  'sfntly_revision':
    '1bdaae8fc788a5ac8936d68bf24f37d977a13dac',
  'skia_git':
    'https://skia.googlesource.com',
  'skia_revision':
    '465706820d0d373f76ab4831c286115ee0d86b7a',
  'sourceforge_url':
    'http://svn.code.sf.net/p/%(repo)s/code',
  'swarming_revision':
    'b39a448d8522392389b28f6997126a6ab04bfe87',
  'v8_branch':
    'trunk',
  'v8_revision':
    '2e4c5505e85d94b520e853dda3f0cc3f2769e5f0',
  'webkit_revision':
    '117dbc2c497be4e52ff643575d6438febfa9053e',
  'webkit_trunk':
    'http://src.chromium.org/blink/trunk'
}

allowed_hosts = [
  'android.googlesource.com',
  'boringssl.googlesource.com',
  'chromium.googlesource.com',
  'pdfium.googlesource.com'
]

deps = {
  'src/breakpad/src':
    (Var("chromium_git")) + '/external/google-breakpad/src.git@9fcbe255a64d39295ad5f1f15c0b92db3da83c0f',
  'src/buildtools':
    (Var("chromium_git")) + '/chromium/buildtools.git@b73e5f70d7ac6be98fb2555461f631afc90216ce',
  'src/chrome/test/data/perf/canvas_bench':
    (Var("chromium_git")) + '/chromium/canvas_bench.git@a7b40ea5ae0239517d78845a5fc9b12976bfc732',
  'src/chrome/test/data/perf/frame_rate/content':
    (Var("chromium_git")) + '/chromium/frame_rate/content.git@c10272c88463efeef6bb19c9ec07c42bc8fe22b9',
  'src/media/cdm/ppapi/api':
    (Var("chromium_git")) + '/chromium/cdm.git@7377023e384f296cbb27644eb2c485275f1f92e8',
  'src/native_client':
    (Var("chromium_git")) + '/native_client/src/native_client.git@862ca265aa1fdcabe8b3ab518251381bfbf48468',
  'src/sdch/open-vcdiff':
    (Var("chromium_git")) + '/external/open-vcdiff.git@438f2a5be6d809bc21611a94cd37bfc8c28ceb33',
  'src/testing/gmock':
    (Var("chromium_git")) + '/external/googlemock.git@29763965ab52f24565299976b936d1265cb6a271',
  'src/testing/gtest':
    (Var("chromium_git")) + '/external/googletest.git@23574bf2333f834ff665f894c97bef8a5b33a0a9',
  'src/third_party/WebKit':
<<<<<<< HEAD
    (Var("chromium_git")) + '/chromium/blink.git@9b1f68fafb21bde3121c83ca5cb2a885c63f9330',
=======
    (Var("chromium_git")) + '/chromium/blink.git@607abbc77d316988d9121937b7e21d4d3ca51698',
>>>>>>> upstream/master
  'src/third_party/angle':
    (Var("chromium_git")) + '/angle/angle.git@fa9744b09e2478c75a25fd1b497469d429e81591',
  'src/third_party/bidichecker':
    (Var("chromium_git")) + '/external/bidichecker/lib.git@97f2aa645b74c28c57eca56992235c79850fa9e0',
  'src/third_party/boringssl/src':
    (Var("boringssl_git")) + '/boringssl.git@a07c0fc8f2181d086b1118712e2ceb0d1496fa0b',
  'src/third_party/cacheinvalidation/src':
    (Var("chromium_git")) + '/external/google-cache-invalidation-api/src.git@0fbfe801cca467fa986ebe08d34012342aa47e55',
  'src/third_party/cld_2/src':
    (Var("chromium_git")) + '/external/cld2.git@14d9ef8d4766326f8aa7de54402d1b9c782d4481',
  'src/third_party/colorama/src':
    (Var("chromium_git")) + '/external/colorama.git@799604a1041e9b3bc5d2789ecbd7e8db2e18e6b8',
  'src/third_party/crashpad/crashpad':
    (Var("chromium_git")) + '/crashpad/crashpad.git@00c42ae7bdcf40d15c02b87e088c1eb565a51333',
  'src/third_party/dom_distiller_js/dist':
    (Var("chromium_git")) + '/external/github.com/chromium/dom-distiller-dist.git@4948eb81dc84578e58a59b8ef493ce946cd00b0f',
  'src/third_party/ffmpeg':
    (Var("chromium_git")) + '/chromium/third_party/ffmpeg.git@cc2ec2825b0cc25cf27c5843847e7028c1cdb075',
  'src/third_party/flac':
    (Var("chromium_git")) + '/chromium/deps/flac.git@0635a091379d9677f1ddde5f2eec85d0f096f219',
  'src/third_party/hunspell_dictionaries':
    (Var("chromium_git")) + '/chromium/deps/hunspell_dictionaries.git@80796932b89ab36431399d76c5b8391ea471e30a',
  'src/third_party/icu':
    (Var("chromium_git")) + '/chromium/deps/icu.git@f1ad7f9ba957571dc692ea3e187612c685615e19',
  'src/third_party/jsoncpp/source':
    (Var("chromium_git")) + '/external/github.com/open-source-parsers/jsoncpp.git@f572e8e42e22cfcf5ab0aea26574f408943edfa4',
  'src/third_party/leveldatabase/src':
    (Var("chromium_git")) + '/external/leveldb.git@251ebf5dc70129ad3c38193fe6c99a5b0ec6b9fa',
  'src/third_party/libaddressinput/src':
    (Var("chromium_git")) + '/external/libaddressinput.git@61f63da7ae6fa469138d60dec5d6bbecc6ab43d6',
  'src/third_party/libexif/sources':
    (Var("chromium_git")) + '/chromium/deps/libexif/sources.git@ed98343daabd7b4497f97fda972e132e6877c48a',
  'src/third_party/libjingle/source/talk':
    (Var("chromium_git")) + '/external/webrtc/trunk/talk.git@411c9af27df0d133df43df2d1a49962b56f7d7ac',
  'src/third_party/libjpeg_turbo':
    (Var("chromium_git")) + '/chromium/deps/libjpeg_turbo.git@8ee9bdd068effcf5fe876e401829eadb94569750',
  'src/third_party/libphonenumber/src/phonenumbers':
    (Var("chromium_git")) + '/external/libphonenumber/cpp/src/phonenumbers.git@0d6e3e50e17c94262ad1ca3b7d52b11223084bca',
  'src/third_party/libphonenumber/src/resources':
    (Var("chromium_git")) + '/external/libphonenumber/resources.git@b6dfdc7952571ff7ee72643cd88c988cbe966396',
  'src/third_party/libphonenumber/src/test':
    (Var("chromium_git")) + '/external/libphonenumber/cpp/test.git@f351a7e007f9c9995494499120bbc361ca808a16',
  'src/third_party/libsrtp':
    (Var("chromium_git")) + '/chromium/deps/libsrtp.git@9c53f858cddd4d890e405e91ff3af0b48dfd90e6',
  'src/third_party/libvpx':
    (Var("chromium_git")) + '/chromium/deps/libvpx.git@9ee3785876600dc2f3a6291557235feee99b1ad2',
  'src/third_party/libyuv':
    (Var("chromium_git")) + '/external/libyuv.git@35aa92a1ea1bbcca6bf97e69e7a65f1caa987675',
  'src/third_party/mesa/src':
    (Var("chromium_git")) + '/chromium/deps/mesa.git@071d25db04c23821a12a8b260ab9d96a097402f0',
  'src/third_party/openmax_dl':
    (Var("chromium_git")) + '/external/webrtc/deps/third_party/openmax.git@22bb1085a6a0f6f3589a8c3d60ed0a9b82248275',
  'src/third_party/opus/src':
    (Var("chromium_git")) + '/chromium/deps/opus.git@cae696156f1e60006e39821e79a1811ae1933c69',
  'src/third_party/pdfium':
    (Var("pdfium_git")) + '/pdfium.git@3ecc289ce0d1a639a9b3f6c59d10952269692d04',
  'src/third_party/py_trace_event/src':
    (Var("chromium_git")) + '/external/py_trace_event.git@dd463ea9e2c430de2b9e53dea57a77b4c3ac9b30',
  'src/third_party/pyftpdlib/src':
    (Var("chromium_git")) + '/external/pyftpdlib.git@2be6d65e31c7ee6320d059f581f05ae8d89d7e45',
  'src/third_party/pywebsocket/src':
    (Var("chromium_git")) + '/external/pywebsocket/src.git@cb349e87ddb30ff8d1fa1a89be39cec901f4a29c',
  'src/third_party/safe_browsing/testing':
    (Var("chromium_git")) + '/external/google-safe-browsing/testing.git@9d7e8064f3ca2e45891470c9b5b1dce54af6a9d6',
  'src/third_party/scons-2.0.1':
    (Var("chromium_git")) + '/native_client/src/third_party/scons-2.0.1.git@1c1550e17fc26355d08627fbdec13d8291227067',
  'src/third_party/sfntly/cpp/src':
    (Var("chromium_git")) + '/external/sfntly/cpp/src.git@1bdaae8fc788a5ac8936d68bf24f37d977a13dac',
  'src/third_party/skia':
<<<<<<< HEAD
    (Var("chromium_git")) + '/skia.git@21518b1ea326e93676131b1936fcb06a7da53f64',
=======
    (Var("chromium_git")) + '/skia.git@f2b95f63de2ec93ef0cb1c3bbcef88cf0c0368f5',
>>>>>>> upstream/master
  'src/third_party/smhasher/src':
    (Var("chromium_git")) + '/external/smhasher.git@e87738e57558e0ec472b2fc3a643b838e5b6e88f',
  'src/third_party/snappy/src':
    (Var("chromium_git")) + '/external/snappy.git@762bb32f0c9d2f31ba4958c7c0933d22e80c20bf',
  'src/third_party/trace-viewer':
    (Var("chromium_git")) + '/external/trace-viewer.git@e2b5374c4941c450b168d4c7d9110fb91b2d4c19',
  'src/third_party/usrsctp/usrsctplib':
    (Var("chromium_git")) + '/external/usrsctplib.git@36444a999739e9e408f8f587cb4c3ffeef2e50ac',
  'src/third_party/webdriver/pylib':
    (Var("chromium_git")) + '/external/selenium/py.git@5fd78261a75fe08d27ca4835fb6c5ce4b42275bd',
  'src/third_party/webgl/src':
    (Var("chromium_git")) + '/external/khronosgroup/webgl.git@1e318b3f0ad952d22477151eec98d37fa08e5bc7',
  'src/third_party/webpagereplay':
    (Var("chromium_git")) + '/external/github.com/chromium/web-page-replay.git@e53550b73e8e098938a651bc8ceb3681e5980567',
  'src/third_party/webrtc':
    (Var("chromium_git")) + '/external/webrtc/trunk/webrtc.git@e725d9de25a3809bce4626831f385830e91695ff',
  'src/third_party/yasm/source/patched-yasm':
    (Var("chromium_git")) + '/chromium/deps/yasm/patched-yasm.git@4671120cd8558ce62ee8672ebf3eb6f5216f909b',
  'src/tools/deps2git':
    (Var("chromium_git")) + '/chromium/tools/deps2git.git@f04828eb0b5acd3e7ad983c024870f17f17b06d9',
  'src/tools/grit':
    (Var("chromium_git")) + '/external/grit-i18n.git@c1b1591a05209c1ad467e845ba8543c22f9072af',
  'src/tools/gyp':
    (Var("chromium_git")) + '/external/gyp.git@0bb67471bca068996e15b56738fa4824dfa19de0',
  'src/tools/page_cycler/acid3':
    (Var("chromium_git")) + '/chromium/deps/acid3.git@6be0a66a1ebd7ebc5abc1b2f405a945f6d871521',
  'src/tools/swarming_client':
    (Var("chromium_git")) + '/external/swarming.client.git@b39a448d8522392389b28f6997126a6ab04bfe87',
  'src/v8':
    (Var("chromium_git")) + '/v8/v8.git@f2469f248ac14c8fc9b6b2870b75bfbc15ea011f'
}

deps_os = {
  'android': {
    'src/third_party/android_protobuf/src':
      (Var("chromium_git")) + '/external/android_protobuf.git@94f522f907e3f34f70d9e7816b947e62fddbb267',
    'src/third_party/android_tools':
<<<<<<< HEAD
      (Var("chromium_git")) + '/android_tools.git@dc0f059b989df0f0a4c3bf2aebd691c4dfb00c94',
=======
      (Var("chromium_git")) + '/android_tools.git@e0ab396314ac4f2fdbb5538597914a0fd3cb447d',
>>>>>>> upstream/master
    'src/third_party/apache-mime4j':
      (Var("chromium_git")) + '/chromium/deps/apache-mime4j.git@28cb1108bff4b6cf0a2e86ff58b3d025934ebe3a',
    'src/third_party/appurify-python/src':
      (Var("chromium_git")) + '/external/github.com/appurify/appurify-python.git@ee7abd5c5ae3106f72b2a0b9d2cb55094688e867',
    'src/third_party/elfutils/src':
      (Var("chromium_git")) + '/external/elfutils.git@249673729a7e5dbd5de4f3760bdcaa3d23d154d7',
    'src/third_party/findbugs':
      (Var("chromium_git")) + '/chromium/deps/findbugs.git@7f69fa78a6db6dc31866d09572a0e356e921bf12',
    'src/third_party/freetype':
      (Var("chromium_git")) + '/chromium/src/third_party/freetype.git@3165210e3e3f5d93f27b6872fbb122cbfc7b33ce',
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
      (Var("chromium_git")) + '/external/linux-syscall-support/lss.git@6f97298fe3794e92c8c896a6bc06e0b36e4c3de3',
    'src/third_party/mockito/src':
      (Var("chromium_git")) + '/external/mockito/mockito.git@ed99a52e94a84bd7c467f2443b475a22fcc6ba8e',
    'src/third_party/requests/src':
      (Var("chromium_git")) + '/external/github.com/kennethreitz/requests.git@f172b30356d821d180fa4ecfa3e71c7274a32de4',
    'src/third_party/robolectric/lib':
      (Var("chromium_git")) + '/chromium/third_party/robolectric.git@6b63c99a8b6967acdb42cbed0adb067c80efc810'
  },
  'ios': {
    'src/chrome/test/data/perf/canvas_bench': None,
    'src/chrome/test/data/perf/frame_rate/content': None,
    'src/ios/third_party/gcdwebserver/src':
      (Var("chromium_git")) + '/external/github.com/swisspol/GCDWebServer.git@18889793b75d7ee593d62ac88997caad850acdb6',
    'src/native_client': None,
    'src/testing/iossim/third_party/class-dump':
      (Var("chromium_git")) + '/chromium/deps/class-dump.git@89bd40883c767584240b4dade8b74e6f57b9bdab',
    'src/third_party/ffmpeg': None,
    'src/third_party/google_toolbox_for_mac/src':
      (Var("chromium_git")) + '/external/google-toolbox-for-mac.git@ce47a231ea0b238fbe95538e86cc61d74c234be6',
    'src/third_party/hunspell_dictionaries': None,
    'src/third_party/nss':
      (Var("chromium_git")) + '/chromium/deps/nss.git@aab0d08a298b29407397fbb1c4219f99e99431ed',
    'src/third_party/webgl': None
  },
  'mac': {
    'src/chrome/installer/mac/third_party/xz/xz':
      (Var("chromium_git")) + '/chromium/deps/xz.git@eecaf55632ca72e90eb2641376bce7cdbc7284f7',
    'src/chrome/tools/test/reference_build/chrome_mac':
      (Var("chromium_git")) + '/chromium/reference_builds/chrome_mac.git@8dc181329e7c5255f83b4b85dc2f71498a237955',
    'src/third_party/google_toolbox_for_mac/src':
      (Var("chromium_git")) + '/external/google-toolbox-for-mac.git@ce47a231ea0b238fbe95538e86cc61d74c234be6',
    'src/third_party/lighttpd':
      (Var("chromium_git")) + '/chromium/deps/lighttpd.git@9dfa55d15937a688a92cbf2b7a8621b0927d06eb',
    'src/third_party/nss':
      (Var("chromium_git")) + '/chromium/deps/nss.git@aab0d08a298b29407397fbb1c4219f99e99431ed',
    'src/third_party/pdfsqueeze':
      (Var("chromium_git")) + '/external/pdfsqueeze.git@5936b871e6a087b7e50d4cbcb122378d8a07499f'
  },
  'unix': {
    'src/chrome/tools/test/reference_build/chrome_linux':
      (Var("chromium_git")) + '/chromium/reference_builds/chrome_linux64.git@033d053a528e820e1de3e2db766678d862a86b36',
    'src/third_party/chromite':
      (Var("chromiumos_git")) + '/chromite.git@c9bfb471ec08e85d0c8baa35341403013153956c',
    'src/third_party/cros_system_api':
      (Var("chromiumos_git")) + '/platform/system_api.git@f2d7a9a97171f25419ff4294b1dba41302981fa2',
    'src/third_party/fontconfig/src':
      (Var("chromium_git")) + '/external/fontconfig.git@f16c3118e25546c1b749f9823c51827a60aeb5c1',
    'src/third_party/freetype2/src':
      (Var("chromium_git")) + '/chromium/src/third_party/freetype2.git@1dd5f5f4a909866f15c92a45c9702bce290a0151',
    'src/third_party/liblouis/src':
      (Var("chromium_git")) + '/external/liblouis-github.git@5f9c03f2a3478561deb6ae4798175094be8a26c2',
    'src/third_party/lss':
      (Var("chromium_git")) + '/external/linux-syscall-support/lss.git@6f97298fe3794e92c8c896a6bc06e0b36e4c3de3',
    'src/third_party/pyelftools':
      (Var("chromiumos_git")) + '/third_party/pyelftools.git@bdc1d380acd88d4bfaf47265008091483b0d614e',
    'src/third_party/stp/src':
      (Var("chromium_git")) + '/external/github.com/stp/stp.git@fc94a599207752ab4d64048204f0c88494811b62',
    'src/third_party/undoview':
      (Var("chromium_git")) + '/chromium/deps/undoview.git@3ba503e248f3cdbd81b78325a24ece0984637559',
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
      (Var("chromium_git")) + '/chromium/deps/nss.git@aab0d08a298b29407397fbb1c4219f99e99431ed',
    'src/third_party/omaha/src/omaha':
      (Var("chromium_git")) + '/external/omaha.git@098c7a3d157218dab4eed595e8f2fbe5a20a0bae',
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
  # Custom Crosswalk hook.
  {
    'action': [
      'python',
      'src/build/empty_google_play_services_lib.py'
    ],
    'pattern':
      '.',
    'name':
      'empty_google_play_services_lib'
  },

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
      'src/build/download_sdk_extras.py'
    ],
    'pattern':
      '.',
    'name':
      'sdkextras'
  },
  {
    'action': [
      'python',
      'src/chrome/installer/linux/sysroot_scripts/install-debian.wheezy.sysroot.py',
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
      '-s',
      'src/third_party/WebKit',
      '-o',
      'src/build/util/LASTCHANGE.blink'
    ],
    'pattern':
      '.',
    'name':
      'lastchange'
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
      '--revision=24bc1affd8aecfdcbe87c0802372ae766bf2507b',
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
      '--revision=b8adaf37ad3beae939ee2db5ae9c0f8b380d2060',
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
      'src/third_party/mojo/src/mojo/public/tools/download_shell_binary.py',
      '--tools-directory=../../../../../../tools'
    ],
    'pattern':
      '',
    'name':
      'download_mojo_shell'
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
      'src/build/gyp_chromium'
    ],
    'pattern':
      '.',
    'name':
      'gyp'
  },
  {
    'action': [
      'python',
      'src/tools/check_git_config.py',
      '--running-as-hook'
    ],
    'pattern':
      '.',
    'name':
      'check_git_config'
  },
  {
    'action': [
      'python',
      'src/tools/remove_stale_pyc_files.py',
      'src/tools'
    ],
    'pattern':
      'src/tools/.*\\.py',
    'name':
      'remove_stale_pyc_files'
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
