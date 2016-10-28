// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/android/media_jni_registrar.h"

#include "base/android/jni_android.h"
#include "base/android/jni_registrar.h"
#include "base/macros.h"

#include "media/audio/android/audio_manager_android.h"
#include "media/audio/android/audio_record_input.h"
#include "media/base/android/media_codec_util.h"
#include "media/base/android/media_drm_bridge.h"
#include "media/base/android/media_player_bridge.h"
#include "media/base/android/media_player_listener.h"
#include "media/base/android/sdk_media_codec_bridge.h"
#include "media/base/android/webaudio_media_codec_bridge.h"
#include "media/capture/video/android/video_capture_device_android.h"
#include "media/capture/video/android/video_capture_device_factory_android.h"

namespace media {

static base::android::RegistrationMethod kMediaRegisteredMethods[] = {
    {"AudioManagerAndroid", AudioManagerAndroid::RegisterAudioManager},
    {"AudioRecordInput", AudioRecordInputStream::RegisterAudioRecordInput},
    {"MediaDrmBridge", MediaDrmBridge::RegisterMediaDrmBridge},
    {"MediaPlayerBridge", MediaPlayerBridge::RegisterMediaPlayerBridge},
    {"MediaPlayerListener", MediaPlayerListener::RegisterMediaPlayerListener},
    {"SdkMediaCodecBridge", SdkMediaCodecBridge::RegisterSdkMediaCodecBridge},
    {"MediaCodecUtil", MediaCodecUtil::RegisterMediaCodecUtil},
    {"WebAudioMediaCodecBridge",
     WebAudioMediaCodecBridge::RegisterWebAudioMediaCodecBridge},
};

bool RegisterJni(JNIEnv* env) {
  return base::android::RegisterNativeMethods(
      env, kMediaRegisteredMethods, arraysize(kMediaRegisteredMethods));
}

}  // namespace media
