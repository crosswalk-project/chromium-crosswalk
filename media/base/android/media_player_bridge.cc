// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/android/media_player_bridge.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/basictypes.h"
#include "base/logging.h"
#include "base/message_loop/message_loop_proxy.h"
#include "jni/MediaPlayerBridge_jni.h"
#include "media/base/android/media_player_manager.h"
#include "media/base/android/media_resource_getter.h"

using base::android::ConvertUTF8ToJavaString;
using base::android::ScopedJavaLocalRef;

// Time update happens every 250ms.
static const int kTimeUpdateInterval = 250;

static const char* APP_SCHEME = "app";
static const std::string FILE_URL_PREFIX = "file:///android_asset/www";

namespace media {

MediaPlayerBridge::MediaPlayerBridge(
    int player_id,
    const GURL& url,
    const GURL& first_party_for_cookies,
    bool hide_url_log,
    MediaPlayerManager* manager)
    : MediaPlayerAndroid(player_id,
                         manager),
      prepared_(false),
      pending_play_(false),
      url_(url),
      first_party_for_cookies_(first_party_for_cookies),
      hide_url_log_(hide_url_log),
      width_(0),
      height_(0),
      can_pause_(true),
      can_seek_forward_(true),
      can_seek_backward_(true),
      weak_this_(this),
      listener_(base::MessageLoopProxy::current(),
                weak_this_.GetWeakPtr()) {
}

MediaPlayerBridge::~MediaPlayerBridge() {
  Release();
}

void MediaPlayerBridge::Initialize() {
  if (url_.SchemeIs(APP_SCHEME)) {
    cookies_.clear();
    ExtractMediaMetadata(FILE_URL_PREFIX + url_.path());
    return;
  }

  if (url_.SchemeIsFile()) {
    cookies_.clear();
    ExtractMediaMetadata(url_.spec());
    return;
  }

  media::MediaResourceGetter* resource_getter =
      manager()->GetMediaResourceGetter();
  if (url_.SchemeIsFileSystem()) {
    cookies_.clear();
    resource_getter->GetPlatformPathFromFileSystemURL(url_, base::Bind(
        &MediaPlayerBridge::ExtractMediaMetadata, weak_this_.GetWeakPtr()));
    return;
  }

  resource_getter->GetCookies(url_, first_party_for_cookies_, base::Bind(
      &MediaPlayerBridge::OnCookiesRetrieved, weak_this_.GetWeakPtr()));
}

void MediaPlayerBridge::CreateJavaMediaPlayerBridge() {
  JNIEnv* env = base::android::AttachCurrentThread();
  CHECK(env);

  j_media_player_bridge_.Reset(Java_MediaPlayerBridge_create(env));

  SetMediaPlayerListener();
}

void MediaPlayerBridge::SetJavaMediaPlayerBridge(
    jobject j_media_player_bridge) {
  JNIEnv* env = base::android::AttachCurrentThread();
  CHECK(env);

  j_media_player_bridge_.Reset(env, j_media_player_bridge);
}

base::android::ScopedJavaLocalRef<jobject> MediaPlayerBridge::
    GetJavaMediaPlayerBridge() {
  base::android::ScopedJavaLocalRef<jobject> j_bridge(
      j_media_player_bridge_);
  return j_bridge;
}

void MediaPlayerBridge::SetMediaPlayerListener() {
  jobject j_context = base::android::GetApplicationContext();
  DCHECK(j_context);

  listener_.CreateMediaPlayerListener(j_context, j_media_player_bridge_.obj());
}

void MediaPlayerBridge::SetDuration(base::TimeDelta duration) {
  duration_ = duration;
}

void MediaPlayerBridge::SetVideoSurface(gfx::ScopedJavaSurface surface) {
  if (j_media_player_bridge_.is_null()) {
    if (surface.IsEmpty())
      return;
    Prepare();
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  CHECK(env);

  Java_MediaPlayerBridge_setSurface(
      env, j_media_player_bridge_.obj(), surface.j_surface().obj());
}

void MediaPlayerBridge::Prepare() {
  if (j_media_player_bridge_.is_null())
    CreateJavaMediaPlayerBridge();
  if (url_.SchemeIsFileSystem()) {
    manager()->GetMediaResourceGetter()->GetPlatformPathFromFileSystemURL(
            url_, base::Bind(&MediaPlayerBridge::SetDataSource,
                             weak_this_.GetWeakPtr()));
  } else {
    std::string url = url_.SchemeIs(APP_SCHEME) ?
        FILE_URL_PREFIX + url_.path() : url_.spec();
    SetDataSource(url);
  }
}

void MediaPlayerBridge::SetDataSource(const std::string& url) {
  if (j_media_player_bridge_.is_null())
    return;

  JNIEnv* env = base::android::AttachCurrentThread();
  CHECK(env);

  // Create a Java String for the URL.
  ScopedJavaLocalRef<jstring> j_url_string = ConvertUTF8ToJavaString(env, url);
  ScopedJavaLocalRef<jstring> j_cookies = ConvertUTF8ToJavaString(
      env, cookies_);

  jobject j_context = base::android::GetApplicationContext();
  DCHECK(j_context);

  if (Java_MediaPlayerBridge_setDataSource(
      env, j_media_player_bridge_.obj(), j_context, j_url_string.obj(),
      j_cookies.obj(), hide_url_log_)) {
    manager()->RequestMediaResources(player_id());
    Java_MediaPlayerBridge_prepareAsync(
        env, j_media_player_bridge_.obj());
  } else {
    OnMediaError(MEDIA_ERROR_FORMAT);
  }
}

void MediaPlayerBridge::OnCookiesRetrieved(const std::string& cookies) {
  cookies_ = cookies;
  ExtractMediaMetadata(url_.spec());
}

void MediaPlayerBridge::ExtractMediaMetadata(const std::string& url) {
  manager()->GetMediaResourceGetter()->ExtractMediaMetadata(
      url, cookies_, base::Bind(&MediaPlayerBridge::OnMediaMetadataExtracted,
                                weak_this_.GetWeakPtr()));
}

void MediaPlayerBridge::OnMediaMetadataExtracted(
    base::TimeDelta duration, int width, int height, bool success) {
  if (success) {
    duration_ = duration;
    width_ = width;
    height_ = height;
  }
  manager()->OnMediaMetadataChanged(
      player_id(), duration_, width_, height_, success);
}

void MediaPlayerBridge::Start() {
  if (j_media_player_bridge_.is_null()) {
    pending_play_ = true;
    Prepare();
  } else {
    if (prepared_)
      StartInternal();
    else
      pending_play_ = true;
  }
}

void MediaPlayerBridge::Pause(bool is_media_related_action) {
  if (j_media_player_bridge_.is_null()) {
    pending_play_ = false;
  } else {
    if (prepared_ && IsPlaying())
      PauseInternal();
    else
      pending_play_ = false;
  }
}

bool MediaPlayerBridge::IsPlaying() {
  if (!prepared_)
    return pending_play_;

  JNIEnv* env = base::android::AttachCurrentThread();
  CHECK(env);
  jboolean result = Java_MediaPlayerBridge_isPlaying(
      env, j_media_player_bridge_.obj());
  return result;
}

int MediaPlayerBridge::GetVideoWidth() {
  if (!prepared_)
    return width_;
  JNIEnv* env = base::android::AttachCurrentThread();
  return Java_MediaPlayerBridge_getVideoWidth(
      env, j_media_player_bridge_.obj());
}

int MediaPlayerBridge::GetVideoHeight() {
  if (!prepared_)
    return height_;
  JNIEnv* env = base::android::AttachCurrentThread();
  return Java_MediaPlayerBridge_getVideoHeight(
      env, j_media_player_bridge_.obj());
}

void MediaPlayerBridge::SeekTo(const base::TimeDelta& timestamp) {
  // Record the time to seek when OnMediaPrepared() is called.
  pending_seek_ = timestamp;

  if (j_media_player_bridge_.is_null())
    Prepare();
  else if (prepared_)
    SeekInternal(timestamp);
}

base::TimeDelta MediaPlayerBridge::GetCurrentTime() {
  if (!prepared_)
    return pending_seek_;
  JNIEnv* env = base::android::AttachCurrentThread();
  return base::TimeDelta::FromMilliseconds(
      Java_MediaPlayerBridge_getCurrentPosition(
          env, j_media_player_bridge_.obj()));
}

base::TimeDelta MediaPlayerBridge::GetDuration() {
  if (!prepared_)
    return duration_;
  JNIEnv* env = base::android::AttachCurrentThread();
  return base::TimeDelta::FromMilliseconds(
      Java_MediaPlayerBridge_getDuration(
          env, j_media_player_bridge_.obj()));
}

void MediaPlayerBridge::Release() {
  if (j_media_player_bridge_.is_null())
    return;

  time_update_timer_.Stop();
  if (prepared_)
    pending_seek_ = GetCurrentTime();
  prepared_ = false;
  pending_play_ = false;
  SetVideoSurface(gfx::ScopedJavaSurface());

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_MediaPlayerBridge_release(env, j_media_player_bridge_.obj());
  j_media_player_bridge_.Reset();
  manager()->ReleaseMediaResources(player_id());
  listener_.ReleaseMediaPlayerListenerResources();
}

void MediaPlayerBridge::SetVolume(double volume) {
  if (j_media_player_bridge_.is_null())
    return;

  JNIEnv* env = base::android::AttachCurrentThread();
  CHECK(env);
  Java_MediaPlayerBridge_setVolume(
      env, j_media_player_bridge_.obj(), volume);
}

void MediaPlayerBridge::OnVideoSizeChanged(int width, int height) {
  width_ = width;
  height_ = height;
  manager()->OnVideoSizeChanged(player_id(), width, height);
}

void MediaPlayerBridge::OnMediaError(int error_type) {
  manager()->OnError(player_id(), error_type);
}

void MediaPlayerBridge::OnBufferingUpdate(int percent) {
  manager()->OnBufferingUpdate(player_id(), percent);
}

void MediaPlayerBridge::OnPlaybackComplete() {
  time_update_timer_.Stop();
  manager()->OnPlaybackComplete(player_id());
}

void MediaPlayerBridge::OnMediaInterrupted() {
  time_update_timer_.Stop();
  manager()->OnMediaInterrupted(player_id());
}

void MediaPlayerBridge::OnSeekComplete() {
  manager()->OnSeekComplete(player_id(), GetCurrentTime());
}

void MediaPlayerBridge::OnMediaPrepared() {
  if (j_media_player_bridge_.is_null())
    return;

  prepared_ = true;
  duration_ = GetDuration();

  // If media player was recovered from a saved state, consume all the pending
  // events.
  PendingSeekInternal(pending_seek_);

  if (pending_play_) {
    StartInternal();
    pending_play_ = false;
  }

  UpdateAllowedOperations();
  manager()->OnMediaMetadataChanged(
      player_id(), duration_, width_, height_, true);
}

ScopedJavaLocalRef<jobject> MediaPlayerBridge::GetAllowedOperations() {
  JNIEnv* env = base::android::AttachCurrentThread();
  CHECK(env);

  return Java_MediaPlayerBridge_getAllowedOperations(
      env, j_media_player_bridge_.obj());
}

void MediaPlayerBridge::UpdateAllowedOperations() {
  JNIEnv* env = base::android::AttachCurrentThread();
  CHECK(env);

  ScopedJavaLocalRef<jobject> allowedOperations = GetAllowedOperations();

  can_pause_ = Java_AllowedOperations_canPause(env, allowedOperations.obj());
  can_seek_forward_ = Java_AllowedOperations_canSeekForward(
      env, allowedOperations.obj());
  can_seek_backward_ = Java_AllowedOperations_canSeekBackward(
      env, allowedOperations.obj());
}

void MediaPlayerBridge::StartInternal() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_MediaPlayerBridge_start(env, j_media_player_bridge_.obj());
  if (!time_update_timer_.IsRunning()) {
    time_update_timer_.Start(
        FROM_HERE,
        base::TimeDelta::FromMilliseconds(kTimeUpdateInterval),
        this, &MediaPlayerBridge::OnTimeUpdateTimerFired);
  }
}

void MediaPlayerBridge::PauseInternal() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_MediaPlayerBridge_pause(env, j_media_player_bridge_.obj());
  time_update_timer_.Stop();
}

void MediaPlayerBridge::PendingSeekInternal(const base::TimeDelta& time) {
  SeekInternal(time);
}

void MediaPlayerBridge::SeekInternal(base::TimeDelta time) {
  if (time > duration_)
    time = duration_;

  // Seeking to an invalid position may cause media player to stuck in an
  // error state.
  if (time < base::TimeDelta()) {
    DCHECK_EQ(-1.0, time.InMillisecondsF());
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  CHECK(env);
  int time_msec = static_cast<int>(time.InMilliseconds());
  Java_MediaPlayerBridge_seekTo(
      env, j_media_player_bridge_.obj(), time_msec);
}

void MediaPlayerBridge::OnTimeUpdateTimerFired() {
  manager()->OnTimeUpdate(player_id(), GetCurrentTime());
}

bool MediaPlayerBridge::RegisterMediaPlayerBridge(JNIEnv* env) {
  bool ret = RegisterNativesImpl(env);
  DCHECK(g_MediaPlayerBridge_clazz);
  return ret;
}

bool MediaPlayerBridge::CanPause() {
  return can_pause_;
}

bool MediaPlayerBridge::CanSeekForward() {
  return can_seek_forward_;
}

bool MediaPlayerBridge::CanSeekBackward() {
  return can_seek_backward_;
}

bool MediaPlayerBridge::IsPlayerReady() {
  return prepared_;
}

GURL MediaPlayerBridge::GetUrl() {
  return url_;
}

GURL MediaPlayerBridge::GetFirstPartyForCookies() {
  return first_party_for_cookies_;
}

}  // namespace media
