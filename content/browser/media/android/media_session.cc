// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/android/media_session.h"

#include "base/android/jni_android.h"
#include "content/browser/media/android/media_session_observer.h"
#include "jni/MediaSession_jni.h"

namespace content {

DEFINE_WEB_CONTENTS_USER_DATA_KEY(MediaSession);

MediaSession::PlayerIdentifier::PlayerIdentifier(MediaSessionObserver* observer,
                                                 int player_id)
    : observer(observer),
      player_id(player_id) {
}

bool MediaSession::PlayerIdentifier::operator==(
    const PlayerIdentifier& other) const {
  return this->observer == other.observer && this->player_id == other.player_id;
}

size_t MediaSession::PlayerIdentifier::Hash::operator()(
    const PlayerIdentifier& player_identifier) const {
  size_t hash = BASE_HASH_NAMESPACE::hash<MediaSessionObserver*>()(
      player_identifier.observer);
  hash += BASE_HASH_NAMESPACE::hash<int>()(player_identifier.player_id);
  return hash;
}

// static
bool content::MediaSession::RegisterMediaSession(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

// static
MediaSession* MediaSession::Get(WebContents* web_contents) {
  MediaSession* session = FromWebContents(web_contents);
  if (!session) {
    CreateForWebContents(web_contents);
    session = FromWebContents(web_contents);
    session->Initialize();
  }
  return session;
}

MediaSession::~MediaSession() {
  DCHECK(players_.empty());
  DCHECK(audio_focus_state_ == State::Suspended);
}

bool MediaSession::AddPlayer(MediaSessionObserver* observer,
                             int player_id,
                             Type type) {
  // If the audio focus is already granted and is of type Content, there is
  // nothing to do. If it is granted of type Transient the requested type is
  // also transient, there is also nothing to do. Otherwise, the session needs
  // to request audio focus again.
  if (audio_focus_state_ == State::Active &&
      (audio_focus_type_ == Type::Content || audio_focus_type_ == type)) {
    players_.insert(PlayerIdentifier(observer, player_id));
    return true;
  }

  State old_audio_focus_state = audio_focus_state_;
  audio_focus_state_ = RequestSystemAudioFocus(type) ? State::Active
                                                     : State::Suspended;
  audio_focus_type_ = type;

  if (audio_focus_state_ != State::Active)
    return false;

  // The session should be reset if a player is starting while all players are
  // suspended.
  if (old_audio_focus_state != State::Active)
    players_.clear();

  players_.insert(PlayerIdentifier(observer, player_id));

  return true;
}

void MediaSession::RemovePlayer(MediaSessionObserver* observer,
                                int player_id) {
  auto it = players_.find(PlayerIdentifier(observer, player_id));
  if (it != players_.end())
    players_.erase(it);

  AbandonSystemAudioFocusIfNeeded();
}

void MediaSession::RemovePlayers(MediaSessionObserver* observer) {
  for (auto it = players_.begin(); it != players_.end();) {
    if (it->observer == observer)
      players_.erase(it++);
    else
      ++it;
  }

  AbandonSystemAudioFocusIfNeeded();
}

void MediaSession::OnSuspend(JNIEnv* env, jobject obj, jboolean temporary) {
  OnSuspend(temporary);
}

void MediaSession::OnResume(JNIEnv* env, jobject obj) {
  OnResume();
}

void MediaSession::ResetJavaRefForTest() {
  j_media_session_.Reset();
}

bool MediaSession::IsActiveForTest() const {
  return audio_focus_state_ == State::Active;
}

MediaSession::Type MediaSession::audio_focus_type_for_test() const {
  return audio_focus_type_;
}

void MediaSession::OnSuspend(bool temporary) {
  if (temporary)
    audio_focus_state_ = State::TemporarilySuspended;
  else
    audio_focus_state_ = State::Suspended;

  for (const auto& it : players_)
    it.observer->OnSuspend(it.player_id);
}

void MediaSession::OnResume() {
  audio_focus_state_ = State::Active;

  for (const auto& it : players_)
    it.observer->OnResume(it.player_id);
}

MediaSession::MediaSession(WebContents* web_contents)
    : WebContentsObserver(web_contents),
      audio_focus_state_(State::Suspended),
      audio_focus_type_(Type::Transient) {
}

void MediaSession::Initialize() {
  JNIEnv* env = base::android::AttachCurrentThread();
  DCHECK(env);
  j_media_session_.Reset(Java_MediaSession_createMediaSession(
      env,
      base::android::GetApplicationContext(),
      reinterpret_cast<intptr_t>(this)));
}

bool MediaSession::RequestSystemAudioFocus(Type type) {
  // During tests, j_media_session_ might be null.
  if (j_media_session_.is_null())
    return true;

  JNIEnv* env = base::android::AttachCurrentThread();
  DCHECK(env);
  return Java_MediaSession_requestAudioFocus(env, j_media_session_.obj(),
                                             type == Type::Transient);
}

void MediaSession::AbandonSystemAudioFocusIfNeeded() {
  if (audio_focus_state_ == State::Suspended || !players_.empty())
    return;

  // During tests, j_media_session_ might be null.
  if (!j_media_session_.is_null()) {
    JNIEnv* env = base::android::AttachCurrentThread();
    DCHECK(env);
    Java_MediaSession_abandonAudioFocus(env, j_media_session_.obj());
  }

  audio_focus_state_ = State::Suspended;
}

}  // namespace content
