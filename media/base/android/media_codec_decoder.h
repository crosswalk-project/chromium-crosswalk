// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_ANDROID_MEDIA_CODEC_DECODER_H_
#define MEDIA_BASE_ANDROID_MEDIA_CODEC_DECODER_H_

#include "base/android/scoped_java_ref.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "media/base/android/access_unit_queue.h"
#include "media/base/android/demuxer_stream_player_params.h"

namespace media {

class MediaCodecBridge;

// The decoder for MediaCodecPlayer.
// This class accepts the incoming data into AccessUnitQueue and works with
// MediaCodecBridge for decoding and rendering the frames. The MediaCodecPlayer
// has two decoder objects: audio and video.
//
// The decoder works on two threads. The data from demuxer comes on Media
// thread. The commands from MediaCodecPlayer, such as Prefetch, Start,
// RequestToStop also come on the Media thread. The operations with MediaCodec
// buffers and rendering happen on a separate thread called Decoder thread.
// This class creates, starts and stops it as necessary.
//
// Decoder's internal state machine goes through the following states:
//
//  [ Stopped ] <-------------------          (any state except Error)
//       |                         |                   |
//       | Prefetch                |--- internal ------|
//       v                         |   transition      v
//  [ Prefetching ]                |               [ Error ]
//       |                         |
//       | internal transition     |
//       v                         |            Error recovery:
//  [ Prefetched ]                 |
//       |                         |        (any state including Error)
//       | Configure and Start     |                   |
//       v                         |                   | ReleaseDecoderResources
//  [ Running ]                    |                   v
//       |                         |          [ InEmergencyStop ]
//       | RequestToStop           |                   |
//       v                         |                   |(decoder thread stopped)
//  [ Stopping ] -------------------                   v
//                                                [ Stopped ]
//
//  [ Stopped ] --------------------
//       ^                         |
//       |       Flush             |
//       ---------------------------

// (any state except Error)
//       |
//       |  SyncStop
//       v
//  [ InEmergencyStop ]
//       |
//       |(decoder thread stopped)
//       v
//  [ Stopped ]

// Here is the workflow that is expected to be maintained by a caller, which is
// MediaCodecPlayer currently.
//
//  [ Stopped ]
//       |
//       | Prefetch
//       v
//  [ Prefetching ]
//       |
//       | (Enough data received)
//       v
//  [ Prefetched ]
//       |
//       | <---------- SetDemuxerConfigs (*)
//       |
//       | <---------- SetVideoSurface (**)
//       |
//       | Configure --------------------------------------------+
//       |                                                       |
//       v                                                       v
//  ( Config Succeeded )                               ( Key frame required )
//       |                                                       |
//       | Start                                                 |
//       v                                                       |
//   [ Running ] ------------------------------+                 |
//       |                                     |                 |
//       |                                     |                 |
//       | RequestToStop                       | SyncStop        | SyncStop
//       |                                     |                 |
//   [ Stopping ]                              |                 |
//       |                                     |                 |
//       | ( Last frame rendered )             |                 |
//       |                                     |                 |
//       |                                     |                 |
//       v                                     |                 |
//   [ Stopped ] <-----------------------------+-----------------+
//
//
// (*) Demuxer configs is a precondition to Configure(), but MediaCodecPlayer
//     has stricter requirements and they are set before Prefetch().
//
// (**) VideoSurface is a precondition to video decoder Configure(), can be set
//      any time before Configure().

class MediaCodecDecoder {
 public:
  // The result of MediaCodec configuration, used by MediaCodecPlayer.
  enum ConfigStatus {
    kConfigFailure = 0,
    kConfigOk,
    kConfigKeyFrameRequired,
  };

  // The decoder reports current playback time to the MediaCodecPlayer.
  // For audio, the parameters designate the beginning and end of a time
  // interval. The beginning is the estimated time that is playing right now.
  // The end is the playback time of the last buffered data. During normal
  // playback the subsequent intervals overlap.
  // For video both values are PTS of the corresponding frame, i.e. the interval
  // has zero width.
  typedef base::Callback<void(base::TimeDelta, base::TimeDelta)>
      SetTimeCallback;

  // MediaCodecDecoder constructor.
  // Parameters:
  //   media_task_runner:
  //     A task runner for the controlling thread. All public methods should be
  //     called on this thread, and callbacks are delivered on this thread.
  //     The MediaCodecPlayer uses a dedicated (Media) thread for this.
  //   external_request_data_cb:
  //     Called periodically as the amount of internally stored data decreases.
  //     The receiver should call OnDemuxerDataAvailable() with more data.
  //   starvation_cb:
  //     Called when starvation is detected. The decoder state does not change.
  //     The player is supposed to stop and then prefetch the decoder.
  //   stop_done_cb:
  //     Called when async stop request is completed.
  //   error_cb:
  //     Called when a MediaCodec error occurred. If this happens, a player has
  //     to either call ReleaseDecoderResources() or destroy the decoder object.
  //   decoder_thread_name:
  //     The thread name to be passed to decoder thread constructor.
  MediaCodecDecoder(
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
      const base::Closure& external_request_data_cb,
      const base::Closure& starvation_cb,
      const base::Closure& stop_done_cb,
      const base::Closure& error_cb,
      const char* decoder_thread_name);
  virtual ~MediaCodecDecoder();

  virtual const char* class_name() const;

  // MediaCodecDecoder exists through the whole lifetime of the player
  // to support dynamic addition and removal of the streams.
  // This method returns true if the current stream (audio or video)
  // is currently active.
  virtual bool HasStream() const = 0;

  // Stores configuration for the use of upcoming Configure()
  virtual void SetDemuxerConfigs(const DemuxerConfigs& configs) = 0;

  // Stops decoder thread, releases the MediaCodecBridge and other resources.
  virtual void ReleaseDecoderResources();

  // Flushes the MediaCodec, after that resets the AccessUnitQueue and blocks
  // the input. Decoder thread should not be running.
  virtual void Flush();

  // Releases MediaCodecBridge.
  void ReleaseMediaCodec();

  // Returns corresponding conditions.
  bool IsPrefetchingOrPlaying() const;
  bool IsStopped() const;
  bool IsCompleted() const;

  base::android::ScopedJavaLocalRef<jobject> GetMediaCrypto();

  // Starts prefetching: accumulates enough data in AccessUnitQueue.
  // Decoder thread is not running.
  void Prefetch(const base::Closure& prefetch_done_cb);

  // Configures MediaCodec.
  ConfigStatus Configure();

  // Starts the decoder thread and resumes the playback.
  bool Start(base::TimeDelta current_time);

  // Stops the playback process synchronously. This method stops the decoder
  // thread synchronously, and then releases all MediaCodec buffers.
  void SyncStop();

  // Requests to stop the playback and returns.
  // Decoder will stop asynchronously after all the dequeued output buffers
  // are rendered.
  void RequestToStop();

  // Notification posted when asynchronous stop is done or playback completed.
  void OnLastFrameRendered(bool completed);

  // Puts the incoming data into AccessUnitQueue.
  void OnDemuxerDataAvailable(const DemuxerData& data);

 protected:
  // Returns true if the new DemuxerConfigs requires MediaCodec
  // reconfiguration.
  virtual bool IsCodecReconfigureNeeded(const DemuxerConfigs& curr,
                                        const DemuxerConfigs& next) const = 0;

  // Does the part of MediaCodecBridge configuration that is specific
  // to audio or video.
  virtual ConfigStatus ConfigureInternal() = 0;

  // Associates PTS with device time so we can calculate delays.
  // We use delays for video decoder only.
  virtual void SynchronizePTSWithTime(base::TimeDelta current_time) {}

  // Processes the change of the output format, varies by stream.
  virtual void OnOutputFormatChanged() = 0;

  // Renders the decoded frame and releases output buffer, or posts
  // a delayed task to do it at a later time,
  virtual void Render(int buffer_index,
                      size_t size,
                      bool render_output,
                      base::TimeDelta pts,
                      bool eos_encountered) = 0;

  // Returns the number of delayed task (we might have them for video).
  virtual int NumDelayedRenderTasks() const;

  // Releases output buffers that are dequeued and not released yet (video)
  // if the |release| parameter is set and then remove the references to them.
  virtual void ClearDelayedBuffers(bool release) {}

#ifndef NDEBUG
  // For video, checks that access unit is the key frame or stand-alone EOS.
  virtual void VerifyUnitIsKeyFrame(const AccessUnit* unit) const {}
#endif

  // Helper methods.

  // Notifies the decoder if the frame is the last one.
  void CheckLastFrame(bool eos_encountered, bool has_delayed_tasks);

  // Returns true is we are in the process of sync stop.
  bool InEmergencyStop() const { return GetState() == kInEmergencyStop; }

  // Protected data.

  // Object for posting tasks on Media thread.
  scoped_refptr<base::SingleThreadTaskRunner> media_task_runner_;

  // Controls Android MediaCodec
  scoped_ptr<MediaCodecBridge> media_codec_bridge_;

  // We call MediaCodecBridge on this thread for both
  // input and output buffers.
  base::Thread decoder_thread_;

  // The queue of access units.
  AccessUnitQueue au_queue_;

  // Flag forces reconfiguration even if |media_codec_bridge_| exists. Currently
  // is set by video decoder when the video surface changes.
  bool needs_reconfigure_;

 private:
  enum DecoderState {
    kStopped = 0,
    kPrefetching,
    kPrefetched,
    kRunning,
    kStopping,
    kInEmergencyStop,
    kError,
  };

  // Helper method that processes an error from MediaCodec.
  void OnCodecError();

  // Requests data. Ensures there is no more than one request at a time.
  void RequestData();

  // Prefetching callback that is posted to Media thread
  // in the kPrefetching state.
  void PrefetchNextChunk();

  // The callback to do actual playback. Posted to Decoder thread
  // in the kRunning state.
  void ProcessNextFrame();

  // Helper method for ProcessNextFrame.
  // Pushes one input buffer to the MediaCodec if the codec can accept it.
  // Returns false if there was MediaCodec error.
  bool EnqueueInputBuffer();

  // Helper method for ProcessNextFrame.
  // Pulls all currently available output frames and renders them.
  // Returns true if we need to continue decoding process, i.e post next
  // ProcessNextFrame method, and false if we need to stop decoding.
  bool DepleteOutputBufferQueue();

  DecoderState GetState() const;
  void SetState(DecoderState state);
  const char* AsString(DecoderState state);

  // Private Data.

  // External data request callback that is passed to decoder.
  base::Closure external_request_data_cb_;

  // These notifications are called on corresponding conditions.
  base::Closure prefetch_done_cb_;
  base::Closure starvation_cb_;
  base::Closure stop_done_cb_;
  base::Closure error_cb_;

  // Data request callback that is posted by decoder internally.
  base::Closure request_data_cb_;

  // Callback used to post OnCodecError method.
  base::Closure internal_error_cb_;

  // Internal state.
  DecoderState state_;
  mutable base::Lock state_lock_;

  // Flag is set when the EOS is enqueued into MediaCodec. Reset by Flush.
  bool eos_enqueued_;

  // Flag is set when the EOS is received in MediaCodec output. Reset by Flush.
  bool completed_;

  // Flag to ensure we post last frame notification once.
  bool last_frame_posted_;

  // Indicates whether the data request is in progress.
  bool is_data_request_in_progress_;

  // Indicates whether the incoming data should be ignored.
  bool is_incoming_data_invalid_;

#ifndef NDEBUG
  // When set, we check that the following video frame is the key frame.
  bool verify_next_frame_is_key_;
#endif

  // NOTE: Weak pointers must be invalidated before all other member variables.
  base::WeakPtrFactory<MediaCodecDecoder> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaCodecDecoder);
};

}  // namespace media

#endif  // MEDIA_BASE_ANDROID_MEDIA_CODEC_DECODER_H_
