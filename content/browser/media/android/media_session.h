// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_ANDROID_MEDIA_SESSION_H_
#define CONTENT_BROWSER_MEDIA_ANDROID_MEDIA_SESSION_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "base/id_map.h"
#include "content/common/content_export.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {

class MediaSessionBrowserTest;
class MediaSessionObserver;

// MediaSession manages the Android AudioFocus for a given WebContents. It is
// requesting the audio focus, pausing when requested by the system and dropping
// it on demand.
// The audio focus can be of two types: Transient or Content. A Transient audio
// focus will allow other players to duck instead of pausing and will be
// declared as temporary to the system. A Content audio focus will not be
// declared as temporary and will not allow other players to duck. If a given
// WebContents can only have one audio focus at a time, it will be Content in
// case of Transient and Content audio focus are both requested.
// Android system interaction occurs in the Java counterpart to this class.
class CONTENT_EXPORT MediaSession
    : public content::WebContentsObserver,
      protected content::WebContentsUserData<MediaSession> {
 public:
  enum class Type {
    Content,
    Transient
  };

  static bool RegisterMediaSession(JNIEnv* env);

  // Returns the MediaSession associated to this WebContents. Creates one if
  // none is currently available.
  static MediaSession* Get(WebContents* web_contents);

  ~MediaSession() override;

  // Adds the given player to the current media session. Returns whether the
  // player was successfully added. If it returns false, AddPlayer() should be
  // called again later.
  bool AddPlayer(MediaSessionObserver* observer, int player_id, Type type);

  // Removes the given player from the current media session. Abandons audio
  // focus if that was the last player in the session.
  void RemovePlayer(MediaSessionObserver* observer, int player_id);

  // Removes all the players associated with |observer|. Abandons audio focus if
  // these were the last players in the session.
  void RemovePlayers(MediaSessionObserver* observer);

  // Called when the Android system requests the MediaSession to be suspended.
  // Called by Java through JNI.
  void OnSuspend(JNIEnv* env, jobject obj, jboolean temporary);

  // Called when the Android system requests the MediaSession to be resumed.
  // Called by Java through JNI.
  void OnResume(JNIEnv* env, jobject obj);

 protected:
  friend class content::MediaSessionBrowserTest;

  // Resets the |j_media_session_| ref to prevent calling the Java backend
  // during content_browsertests.
  void ResetJavaRefForTest();

  bool IsActiveForTest() const;
  Type audio_focus_type_for_test() const;

  void OnSuspend(bool temporary);
  void OnResume();

 private:
  friend class content::WebContentsUserData<MediaSession>;

  enum class State {
    Active,
    TemporarilySuspended,
    Suspended,
  };

  // Representation of a player for the MediaSession.
  struct PlayerIdentifier {
    PlayerIdentifier(MediaSessionObserver* observer, int player_id);
    PlayerIdentifier(const PlayerIdentifier&) = default;

    void operator=(const PlayerIdentifier&) = delete;
    bool operator==(const PlayerIdentifier& player_identifier) const;

    // Hash operator for base::hash_map<>.
    struct Hash {
      size_t operator()(const PlayerIdentifier& player_identifier) const;
    };

    MediaSessionObserver* observer;
    int player_id;
  };
  using PlayersMap = base::hash_set<PlayerIdentifier, PlayerIdentifier::Hash>;

  explicit MediaSession(WebContents* web_contents);

  // Setup the JNI.
  void Initialize();

  // Requests audio focus to Android using |j_media_session_|.
  // Returns whether the request was granted. If |j_media_session_| is null, it
  // will always return true.
  bool RequestSystemAudioFocus(Type type);

  // To be called after a call to AbandonAudioFocus() in order to call the Java
  // MediaSession if the audio focus really need to be abandoned.
  void AbandonSystemAudioFocusIfNeeded();

  base::android::ScopedJavaGlobalRef<jobject> j_media_session_;
  PlayersMap players_;

  State audio_focus_state_;
  Type audio_focus_type_;

  DISALLOW_COPY_AND_ASSIGN(MediaSession);
};

}  // namespace content

#endif // CONTENT_BROWSER_MEDIA_ANDROID_MEDIA_SESSION_H_
