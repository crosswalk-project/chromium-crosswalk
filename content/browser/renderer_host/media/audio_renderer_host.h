// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// AudioRendererHost serves audio related requests from AudioRenderer which
// lives inside the render process and provide access to audio hardware.
//
// This class is owned by BrowserRenderProcessHost, and instantiated on UI
// thread, but all other operations and method calls happen on IO thread, so we
// need to be extra careful about the lifetime of this object. AudioManager is a
// singleton and created in IO thread, audio output streams are also created in
// the IO thread, so we need to destroy them also in IO thread. After this class
// is created, a task of OnInitialized() is posted on IO thread in which
// singleton of AudioManager is created.
//
// Here's an example of a typical IPC dialog for audio:
//
//   Renderer                     AudioRendererHost
//      |                               |
//      |         CreateStream >        |
//      |     < NotifyStreamCreated     |
//      |                               |
//      |          PlayStream >         |
//      |  < NotifyStreamStateChanged   | kAudioStreamPlaying
//      |                               |
//      |         PauseStream >         |
//      |  < NotifyStreamStateChanged   | kAudioStreamPaused
//      |                               |
//      |          PlayStream >         |
//      |  < NotifyStreamStateChanged   | kAudioStreamPlaying
//      |             ...               |
//      |         CloseStream >         |
//      v                               v

// A SyncSocket pair is used to signal buffer readiness between processes.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_AUDIO_RENDERER_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_AUDIO_RENDERER_HOST_H_

#include <map>
#if defined(OS_TIZEN)
#include <string>
#endif

#include "base/atomic_ref_count.h"
#include "base/gtest_prod_util.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/process/process.h"
#include "base/sequenced_task_runner_helpers.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "media/audio/audio_io.h"
#include "media/audio/audio_logging.h"
#include "media/audio/audio_output_controller.h"
#include "media/audio/simple_sources.h"

namespace media {
class AudioManager;
class AudioParameters;
}

namespace content {

class AudioMirroringManager;
class MediaInternals;
class MediaStreamManager;
class ResourceContext;

class CONTENT_EXPORT AudioRendererHost : public BrowserMessageFilter {
 public:
  // Called from UI thread from the owner of this object.
  AudioRendererHost(int render_process_id,
                    media::AudioManager* audio_manager,
                    AudioMirroringManager* mirroring_manager,
                    MediaInternals* media_internals,
                    MediaStreamManager* media_stream_manager);

  // Calls |callback| with the list of AudioOutputControllers for this object.
  void GetOutputControllers(
      const RenderProcessHost::GetAudioOutputControllersCallback&
          callback) const;

  // BrowserMessageFilter implementation.
  void OnChannelClosing() override;
  void OnDestruct() const override;
  bool OnMessageReceived(const IPC::Message& message) override;

  // Returns true if any streams managed by this host are actively playing.  Can
  // be called from any thread.
  bool HasActiveAudio();

  // Returns true if any streams managed by the RenderFrame identified by
  // |render_frame_id| are actively playing. Can be called from any thread.
  bool RenderFrameHasActiveAudio(int render_frame_id) const;

#if defined(OS_TIZEN)
  // Sets an application ID and class properties, which are used to tag audio
  // streams in pulseaudio/Murphy.
  virtual void SetMediaStreamProperties(const std::string& app_id,
                                        const std::string& app_class);
  const std::string& app_id() const { return app_id_; }
  const std::string& app_class() const { return app_class_; }
#endif

 private:
  friend class AudioRendererHostTest;
  friend class BrowserThread;
  friend class base::DeleteHelper<AudioRendererHost>;
  friend class MockAudioRendererHost;
  friend class TestAudioRendererHost;
  FRIEND_TEST_ALL_PREFIXES(AudioRendererHostTest, CreateMockStream);
  FRIEND_TEST_ALL_PREFIXES(AudioRendererHostTest, MockStreamDataConversation);

  class AudioEntry;
  typedef std::map<int, AudioEntry*> AudioEntryMap;

  ~AudioRendererHost() override;

  // Methods called on IO thread ----------------------------------------------

  // Audio related IPC message handlers.

  // Creates an audio output stream with the specified format whose data is
  // produced by an entity in the RenderFrame referenced by |render_frame_id|.
  // |session_id| is used for unified IO to find out which input device to be
  // opened for the stream. For clients that do not use unified IO,
  // |session_id| will be ignored.
  // Upon success/failure, the peer is notified via the NotifyStreamCreated
  // message.
  void OnCreateStream(int stream_id,
                      int render_frame_id,
                      int session_id,
                      const media::AudioParameters& params);

  // Play the audio stream referenced by |stream_id|.
  void OnPlayStream(int stream_id);

  // Pause the audio stream referenced by |stream_id|.
  void OnPauseStream(int stream_id);

  // Close the audio stream referenced by |stream_id|.
  void OnCloseStream(int stream_id);

  // Set the volume of the audio stream referenced by |stream_id|.
  void OnSetVolume(int stream_id, double volume);

  // Complete the process of creating an audio stream. This will set up the
  // shared memory or shared socket in low latency mode and send the
  // NotifyStreamCreated message to the peer.
  void DoCompleteCreation(int stream_id);

  // Send playing/paused status to the renderer.
  void DoNotifyStreamStateChanged(int stream_id, bool is_playing);

  RenderProcessHost::AudioOutputControllerList DoGetOutputControllers() const;

  // Send an error message to the renderer.
  void SendErrorMessage(int stream_id);

  // Delete an audio entry, notifying observers first.  This is called by
  // AudioOutputController after it has closed.
  void DeleteEntry(scoped_ptr<AudioEntry> entry);

  // Send an error message to the renderer, then close the stream.
  void ReportErrorAndClose(int stream_id);

  // A helper method to look up a AudioEntry identified by |stream_id|.
  // Returns NULL if not found.
  AudioEntry* LookupById(int stream_id);

  // A helper method to update the number of playing streams and alert the
  // ResourceScheduler when the renderer starts or stops playing an audiostream.
  void UpdateNumPlayingStreams(AudioEntry* entry, bool is_playing);

  // ID of the RenderProcessHost that owns this instance.
  const int render_process_id_;

  media::AudioManager* const audio_manager_;
  AudioMirroringManager* const mirroring_manager_;
  scoped_ptr<media::AudioLog> audio_log_;

  // Used to access to AudioInputDeviceManager.
  MediaStreamManager* media_stream_manager_;

  // A map of stream IDs to audio sources.
  AudioEntryMap audio_entries_;

  // The number of streams in the playing state.
  base::AtomicRefCount num_playing_streams_;

#if defined(OS_TIZEN)
  // Application ID and class for the Murphy resource set.
  std::string app_id_;
  std::string app_class_;
#endif

  DISALLOW_COPY_AND_ASSIGN(AudioRendererHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_AUDIO_RENDERER_HOST_H_
