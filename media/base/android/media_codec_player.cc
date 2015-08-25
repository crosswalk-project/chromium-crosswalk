// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/android/media_codec_player.h"

#include "base/bind.h"
#include "base/lazy_instance.h"
#include "base/logging.h"

#define RUN_ON_MEDIA_THREAD(METHOD, ...)                                      \
  do {                                                                        \
    if (!GetMediaTaskRunner()->BelongsToCurrentThread()) {                    \
      GetMediaTaskRunner()->PostTask(                                         \
          FROM_HERE,                                                          \
          base::Bind(&MediaCodecPlayer:: METHOD, weak_this_, ##__VA_ARGS__)); \
      return;                                                                 \
    }                                                                         \
  } while(0)


namespace media {

class MediaThread : public base::Thread {
 public:
  MediaThread() : base::Thread("BrowserMediaThread") {
    Start();
  }
};

// Create media thread
base::LazyInstance<MediaThread>::Leaky
    g_media_thread = LAZY_INSTANCE_INITIALIZER;


scoped_refptr<base::SingleThreadTaskRunner> GetMediaTaskRunner() {
  return g_media_thread.Pointer()->task_runner();
}

// MediaCodecPlayer implementation.

MediaCodecPlayer::MediaCodecPlayer(
    int player_id,
    MediaPlayerManager* manager,
    const RequestMediaResourcesCB& request_media_resources_cb,
    scoped_ptr<DemuxerAndroid> demuxer,
    const GURL& frame_url)
    : MediaPlayerAndroid(player_id,
                         manager,
                         request_media_resources_cb,
                         frame_url),
      ui_task_runner_(base::MessageLoopProxy::current()),
      demuxer_(demuxer.Pass()),
      state_(STATE_PAUSED),
      interpolator_(&default_tick_clock_),
      pending_start_(false),
      pending_seek_(kNoTimestamp()),
      media_weak_factory_(this) {
  DCHECK(ui_task_runner_->BelongsToCurrentThread());

  DVLOG(1) << "MediaCodecPlayer::MediaCodecPlayer: player_id:" << player_id;

  request_resources_cb_ = base::Bind(request_media_resources_cb_, player_id);

  completion_cb_ =
      base::Bind(&MediaPlayerManager::OnPlaybackComplete, manager, player_id);
  seek_done_cb_ =
      base::Bind(&MediaPlayerManager::OnSeekComplete, manager, player_id);
  attach_listener_cb_ = base::Bind(&MediaPlayerAndroid::AttachListener,
                                   WeakPtrForUIThread(), nullptr);
  detach_listener_cb_ =
      base::Bind(&MediaPlayerAndroid::DetachListener, WeakPtrForUIThread());
  metadata_changed_cb_ = base::Bind(&MediaPlayerAndroid::OnMediaMetadataChanged,
                                    WeakPtrForUIThread());
  time_update_cb_ =
      base::Bind(&MediaPlayerAndroid::OnTimeUpdate, WeakPtrForUIThread());

  media_weak_this_ = media_weak_factory_.GetWeakPtr();

  // Finish initializaton on Media thread
  GetMediaTaskRunner()->PostTask(
      FROM_HERE, base::Bind(&MediaCodecPlayer::Initialize, weak_this_));
}

MediaCodecPlayer::~MediaCodecPlayer()
{
  // Media thread
  DVLOG(1) << "MediaCodecPlayer::~MediaCodecPlayer";
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());
}

void MediaCodecPlayer::Initialize() {
  // Media thread
  DVLOG(1) << __FUNCTION__;
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());

  demuxer_->Initialize(this);
}

// MediaPlayerAndroid implementation.

void MediaCodecPlayer::DeleteOnCorrectThread() {
  // UI thread
  DVLOG(1) << __FUNCTION__;
  DCHECK(ui_task_runner_->BelongsToCurrentThread());

  // The listener-related portion of the base class has to be
  // destroyed on UI thread.
  DestroyListenerOnUIThread();

  // Post deletion onto Media thread
  GetMediaTaskRunner()->DeleteSoon(FROM_HERE, this);
}

void MediaCodecPlayer::SetVideoSurface(gfx::ScopedJavaSurface surface) {
  RUN_ON_MEDIA_THREAD(SetVideoSurface, base::Passed(&surface));

  // Media thread
  DVLOG(1) << __FUNCTION__;

  NOTIMPLEMENTED();
}

void MediaCodecPlayer::Start() {
  RUN_ON_MEDIA_THREAD(Start);

  // Media thread
  DVLOG(1) << __FUNCTION__;

  switch (state_) {
    case STATE_PAUSED:
      // Prefetch or wait for initial configuration.
      if (HasAudio() || HasVideo()) {
        SetState(STATE_PREFETCHING);
        StartPrefetchDecoders();
      } else {
        SetState(STATE_WAITING_FOR_CONFIG);
      }
      break;
    case STATE_STOPPING:
    case STATE_WAITING_FOR_SEEK:
      SetPendingStart(true);
      break;
    case STATE_WAITING_FOR_CONFIG:
    case STATE_PREFETCHING:
    case STATE_PLAYING:
    case STATE_WAITING_FOR_SURFACE:
    case STATE_ERROR:
      break;  // Ignore
    default:
      NOTREACHED();
      break;
  }
}

void MediaCodecPlayer::Pause(bool is_media_related_action) {
  RUN_ON_MEDIA_THREAD(Pause, is_media_related_action);

  // Media thread
  DVLOG(1) << __FUNCTION__;

  SetPendingStart(false);

  switch (state_) {
    case STATE_WAITING_FOR_CONFIG:
    case STATE_PREFETCHING:
    case STATE_WAITING_FOR_SURFACE:
      SetState(STATE_PAUSED);
      StopDecoders();
      break;
    case STATE_PLAYING:
      SetState(STATE_STOPPING);
      RequestToStopDecoders();
      break;
    case STATE_PAUSED:
    case STATE_STOPPING:
    case STATE_WAITING_FOR_SEEK:
    case STATE_ERROR:
      break;  // Ignore
    default:
      NOTREACHED();
      break;
  }
}

void MediaCodecPlayer::SeekTo(base::TimeDelta timestamp) {
  RUN_ON_MEDIA_THREAD(SeekTo, timestamp);

  // Media thread
  DVLOG(1) << __FUNCTION__ << " " << timestamp;

  switch (state_) {
    case STATE_PAUSED:
      SetState(STATE_WAITING_FOR_SEEK);
      RequestDemuxerSeek(timestamp);
      break;
    case STATE_WAITING_FOR_CONFIG:
    case STATE_PREFETCHING:
    case STATE_WAITING_FOR_SURFACE:
      SetState(STATE_WAITING_FOR_SEEK);
      StopDecoders();
      SetPendingStart(true);
      RequestDemuxerSeek(timestamp);
      break;
    case STATE_PLAYING:
      SetState(STATE_STOPPING);
      RequestToStopDecoders();
      SetPendingStart(true);
      SetPendingSeek(timestamp);
      break;
    case STATE_STOPPING:
      SetPendingSeek(timestamp);
      break;
    case STATE_WAITING_FOR_SEEK:
      SetPendingSeek(timestamp);
      break;
    case STATE_ERROR:
      break;  // ignore
    default:
      NOTREACHED();
      break;
  }
}

void MediaCodecPlayer::Release() {
  RUN_ON_MEDIA_THREAD(Release);

  // Media thread
  DVLOG(1) << __FUNCTION__;

  NOTIMPLEMENTED();
}

void MediaCodecPlayer::SetVolume(double volume) {
  RUN_ON_MEDIA_THREAD(SetVolume, volume);

  // Media thread
  DVLOG(1) << __FUNCTION__ << " " << volume;

  NOTIMPLEMENTED();
}

int MediaCodecPlayer::GetVideoWidth() {
  // UI thread
  DCHECK(ui_task_runner_->BelongsToCurrentThread());

  NOTIMPLEMENTED();
  return 320;
}

int MediaCodecPlayer::GetVideoHeight() {
  // UI thread
  DCHECK(ui_task_runner_->BelongsToCurrentThread());

  NOTIMPLEMENTED();
  return 240;
}

base::TimeDelta MediaCodecPlayer::GetCurrentTime() {
  // UI thread, Media thread
  NOTIMPLEMENTED();
  return base::TimeDelta();
}

base::TimeDelta MediaCodecPlayer::GetDuration() {
  // UI thread
  DCHECK(ui_task_runner_->BelongsToCurrentThread());

  NOTIMPLEMENTED();
  return base::TimeDelta();
}

bool MediaCodecPlayer::IsPlaying() {
  // UI thread
  DCHECK(ui_task_runner_->BelongsToCurrentThread());

  // TODO(timav): Use another variable since |state_| should only be accessed on
  // Media thread.
  return state_ == STATE_PLAYING || state_ == STATE_STOPPING;
}

bool MediaCodecPlayer::CanPause() {
  // UI thread
  DCHECK(ui_task_runner_->BelongsToCurrentThread());
  NOTIMPLEMENTED();
  return false;
}

bool MediaCodecPlayer::CanSeekForward() {
  // UI thread
  DCHECK(ui_task_runner_->BelongsToCurrentThread());
  NOTIMPLEMENTED();
  return false;
}

bool MediaCodecPlayer::CanSeekBackward() {
  // UI thread
  DCHECK(ui_task_runner_->BelongsToCurrentThread());
  NOTIMPLEMENTED();
  return false;
}

bool MediaCodecPlayer::IsPlayerReady() {
  // UI thread
  DCHECK(ui_task_runner_->BelongsToCurrentThread());
  NOTIMPLEMENTED();
  return true;
}

void MediaCodecPlayer::SetCdm(BrowserCdm* cdm) {
  // UI thread
  DCHECK(ui_task_runner_->BelongsToCurrentThread());
  NOTIMPLEMENTED();
}

// Callbacks from Demuxer.

void MediaCodecPlayer::OnDemuxerConfigsAvailable(
    const DemuxerConfigs& configs) {
  // Media thread
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());

  NOTIMPLEMENTED();
}

void MediaCodecPlayer::OnDemuxerDataAvailable(const DemuxerData& data) {
  // Media thread
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());
  NOTIMPLEMENTED();
}

void MediaCodecPlayer::OnDemuxerSeekDone(
    base::TimeDelta actual_browser_seek_time) {
  // Media thread
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());

  DVLOG(1) << __FUNCTION__ << " actual_time:" << actual_browser_seek_time;

  if (state_ != STATE_WAITING_FOR_SEEK)
    return;  // ignore

  DCHECK(seek_info_.get());
  DCHECK(seek_info_->seek_time != kNoTimestamp());

  // A browser seek must not jump into the past. Ideally, it seeks to the
  // requested time, but it might jump into the future.
  DCHECK(!seek_info_->is_browser_seek ||
         seek_info_->seek_time <= actual_browser_seek_time);

  // Restrict the current time to be equal to seek_time
  // for the next StartPlaybackDecoders() call.

  base::TimeDelta seek_time = seek_info_->is_browser_seek
                                  ? actual_browser_seek_time
                                  : seek_info_->seek_time;

  interpolator_.SetBounds(seek_time, seek_time);
  audio_decoder_->SetBaseTimestamp(seek_time);

  base::TimeDelta pending_seek_time = GetPendingSeek();
  if (pending_seek_time != kNoTimestamp()) {
    // Keep STATE_WAITING_FOR_SEEK
    SetPendingSeek(kNoTimestamp());
    RequestDemuxerSeek(pending_seek_time);
    return;
  }

  if (HasPendingStart()) {
    SetPendingStart(false);
    // Prefetch or wait for initial configuration.
    if (HasAudio() || HasVideo()) {
      SetState(STATE_PREFETCHING);
      StartPrefetchDecoders();
    } else {
      SetState(STATE_WAITING_FOR_CONFIG);
    }
  } else {
    SetState(STATE_PAUSED);
  }

  // Notify the Renderer.
  if (!seek_info_->is_browser_seek)
    ui_task_runner_->PostTask(FROM_HERE, base::Bind(seek_done_cb_, seek_time));

  seek_info_.reset();
}

void MediaCodecPlayer::OnDemuxerDurationChanged(
    base::TimeDelta duration) {
  // Media thread
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());
  DVLOG(1) << __FUNCTION__ << " duration:" << duration;

  duration_ = duration;
}

// Events from Player, called on UI thread

void MediaCodecPlayer::OnMediaMetadataChanged(base::TimeDelta duration,
                                              const gfx::Size& video_size) {
  DCHECK(ui_task_runner_->BelongsToCurrentThread());

  if (duration != kNoTimestamp())
    metadata_cache_.duration = duration;

  if (!video_size.IsEmpty())
    metadata_cache_.video_size = video_size;

  manager()->OnMediaMetadataChanged(player_id(), metadata_cache_.duration,
                                    metadata_cache_.video_size.width(),
                                    metadata_cache_.video_size.height(), true);
}

void MediaCodecPlayer::OnTimeUpdate(base::TimeDelta current_timestamp,
                                    base::TimeTicks current_time_ticks) {
  DCHECK(ui_task_runner_->BelongsToCurrentThread());

  current_time_cache_ = current_timestamp;
  manager()->OnTimeUpdate(player_id(), current_timestamp, current_time_ticks);
}

// Events from Decoders, called on Media thread

void MediaCodecPlayer::RequestDemuxerData(DemuxerStream::Type stream_type) {
  DVLOG(2) << __FUNCTION__ << " streamType:" << stream_type;

  // Use this method instead of directly binding with
  // DemuxerAndroid::RequestDemuxerData() to avoid the race condition on
  // deletion:
  // 1. DeleteSoon is posted from UI to Media thread.
  // 2. RequestDemuxerData callback is posted from Decoder to Media thread.
  // 3. DeleteSoon arrives, we delete the player and detach from
  //    BrowserDemuxerAndroid.
  // 4. RequestDemuxerData is processed by the media thread queue. Since the
  //    weak_ptr was invalidated in (3), this is a no-op. If we used
  //    DemuxerAndroid::RequestDemuxerData() it would arrive and will try to
  //    call the client, but the client (i.e. this player) would not exist.
  demuxer_->RequestDemuxerData(stream_type);
}

void MediaCodecPlayer::OnPrefetchDone() {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());

  if (state_ != STATE_PREFETCHING) {
    DVLOG(1) << __FUNCTION__ << " wrong state " << AsString(state_)
             << " ignoring";
    return;  // Ignore
  }

  DVLOG(1) << __FUNCTION__;

  if (!HasAudio() && !HasVideo()) {
    // No configuration at all after prefetching.
    // This is an error, initial configuration is expected
    // before the first data chunk.
    GetMediaTaskRunner()->PostTask(FROM_HERE, error_cb_);
    return;
  }

  if (HasVideo() && !HasPendingSurface()) {
    SetState(STATE_WAITING_FOR_SURFACE);
    return;
  }

  SetState(STATE_PLAYING);
  StartPlaybackDecoders();
}

void MediaCodecPlayer::OnStopDone() {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());
  DVLOG(1) << __FUNCTION__;

  if (!(audio_decoder_->IsStopped() && video_decoder_->IsStopped()))
    return;  // Wait until other stream is stopped

  // At this point decoder threads should not be running
  if (interpolator_.interpolating())
    interpolator_.StopInterpolating();

  base::TimeDelta seek_time;
  switch (state_) {
    case STATE_STOPPING: {
      base::TimeDelta seek_time = GetPendingSeek();
      if (seek_time != kNoTimestamp()) {
        SetState(STATE_WAITING_FOR_SEEK);
        SetPendingSeek(kNoTimestamp());
        RequestDemuxerSeek(seek_time);
      } else if (HasPendingStart()) {
        SetPendingStart(false);
        SetState(STATE_PREFETCHING);
        StartPrefetchDecoders();
      } else {
        SetState(STATE_PAUSED);
      }
    } break;
    case STATE_PLAYING:
      // Unexpected stop means completion
      SetState(STATE_PAUSED);
      break;
    default:
      // DVLOG(0) << __FUNCTION__ << " illegal state: " << AsString(state_);
      // NOTREACHED();
      // Ignore! There can be a race condition: audio posts OnStopDone,
      // then video posts, then first OnStopDone arrives at which point
      // both streams are already stopped, then second OnStopDone arrives. When
      // the second one arrives, the state us not STATE_STOPPING any more.
      break;
  }

  // DetachListener to UI thread
  ui_task_runner_->PostTask(FROM_HERE, detach_listener_cb_);

  if (AudioFinished() && VideoFinished())
    ui_task_runner_->PostTask(FROM_HERE, completion_cb_);
}

void MediaCodecPlayer::OnError() {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());
  DVLOG(1) << __FUNCTION__;

  // STATE_ERROR blocks all events
  SetState(STATE_ERROR);

  ReleaseDecoderResources();
}

void MediaCodecPlayer::OnStarvation(DemuxerStream::Type type) {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());
  DVLOG(1) << __FUNCTION__ << " stream type:" << type;

  if (state_ != STATE_PLAYING)
    return;  // Ignore

  SetState(STATE_STOPPING);
  RequestToStopDecoders();
  SetPendingStart(true);
}

void MediaCodecPlayer::OnTimeIntervalUpdate(DemuxerStream::Type type,
                                            base::TimeDelta now_playing,
                                            base::TimeDelta last_buffered) {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());

  DVLOG(2) << __FUNCTION__ << ": stream type:" << type << " [" << now_playing
           << "," << last_buffered << "]";

  // I assume that audio stream cannot be added after we get configs by
  // OnDemuxerConfigsAvailable(), but that audio can finish early.

  if (type == DemuxerStream::VIDEO) {
    // Ignore video PTS if there is audio stream or if it's behind current
    // time as set by audio stream.
    if (!AudioFinished() || now_playing < interpolator_.GetInterpolatedTime())
      return;
  }

  interpolator_.SetBounds(now_playing, last_buffered);

  // Post to UI thread
  ui_task_runner_->PostTask(FROM_HERE,
                            base::Bind(time_update_cb_, GetInterpolatedTime(),
                                       base::TimeTicks::Now()));
}

void MediaCodecPlayer::OnVideoCodecCreated() {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());

  // This callback requests resources by releasing other players.
  ui_task_runner_->PostTask(FROM_HERE, request_resources_cb_);
}

void MediaCodecPlayer::OnVideoResolutionChanged(const gfx::Size& size) {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());

  DVLOG(1) << __FUNCTION__ << " " << size.width() << "x" << size.height();

  // Update cache and notify manager on UI thread
  ui_task_runner_->PostTask(
      FROM_HERE, base::Bind(metadata_changed_cb_, kNoTimestamp(), size));
}

// State machine operations, called on Media thread

void MediaCodecPlayer::SetState(PlayerState new_state) {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());

  DVLOG(1) << "SetState:" << AsString(state_) << " -> " << AsString(new_state);
  state_ = new_state;
}

void MediaCodecPlayer::SetPendingSurface(gfx::ScopedJavaSurface surface) {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());
  DVLOG(1) << __FUNCTION__;

  video_decoder_->SetPendingSurface(surface.Pass());
}

bool MediaCodecPlayer::HasPendingSurface() const {
  return video_decoder_->HasPendingSurface();
}

void MediaCodecPlayer::SetPendingStart(bool need_to_start) {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());
  DVLOG(1) << __FUNCTION__ << ": " << need_to_start;
  pending_start_ = need_to_start;
}

bool MediaCodecPlayer::HasPendingStart() const {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());
  return pending_start_;
}

void MediaCodecPlayer::SetPendingSeek(base::TimeDelta timestamp) {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());
  DVLOG(1) << __FUNCTION__ << ": " << timestamp;
  pending_seek_ = timestamp;
}

base::TimeDelta MediaCodecPlayer::GetPendingSeek() const {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());
  return pending_seek_;
}

bool MediaCodecPlayer::HasAudio() const {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());
  return audio_decoder_->HasStream();
}

bool MediaCodecPlayer::HasVideo() const {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());
  return video_decoder_->HasStream();
}

void MediaCodecPlayer::SetDemuxerConfigs(const DemuxerConfigs& configs) {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());
  DVLOG(1) << __FUNCTION__ << " " << configs;

  DCHECK(audio_decoder_);
  DCHECK(video_decoder_);

  // At least one valid codec must be present.
  DCHECK(configs.audio_codec != kUnknownAudioCodec ||
         configs.video_codec != kUnknownVideoCodec);

  if (configs.audio_codec != kUnknownAudioCodec)
    audio_decoder_->SetDemuxerConfigs(configs);

  if (configs.video_codec != kUnknownVideoCodec)
    video_decoder_->SetDemuxerConfigs(configs);

  if (state_ == STATE_WAITING_FOR_CONFIG) {
    SetState(STATE_PREFETCHING);
    StartPrefetchDecoders();
  }
}

void MediaCodecPlayer::StartPrefetchDecoders() {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());
  DVLOG(1) << __FUNCTION__;

  bool do_audio = false;
  bool do_video = false;
  int count = 0;
  if (!AudioFinished()) {
    do_audio = true;
    ++count;
  }
  if (!VideoFinished()) {
    do_video = true;
    ++count;
  }

  DCHECK_LT(0, count);  // at least one decoder should be active

  base::Closure prefetch_cb = base::BarrierClosure(
      count, base::Bind(&MediaCodecPlayer::OnPrefetchDone, media_weak_this_));

  if (do_audio)
    audio_decoder_->Prefetch(prefetch_cb);

  if (do_video)
    video_decoder_->Prefetch(prefetch_cb);
}

void MediaCodecPlayer::StartPlaybackDecoders() {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());
  DVLOG(1) << __FUNCTION__;

  // Configure all streams before the start since
  // we may discover that browser seek is required.

  bool do_audio = !AudioFinished();
  bool do_video = !VideoFinished();

  // If there is nothing to play, the state machine should determine
  // this at the prefetch state and never call this method.
  DCHECK(do_audio || do_video);

  if (do_audio) {
    MediaCodecDecoder::ConfigStatus status = audio_decoder_->Configure();
    if (status != MediaCodecDecoder::CONFIG_OK) {
      GetMediaTaskRunner()->PostTask(FROM_HERE, error_cb_);
      return;
    }
  }

  if (do_video) {
    MediaCodecDecoder::ConfigStatus status = video_decoder_->Configure();
    if (status != MediaCodecDecoder::CONFIG_OK) {
      GetMediaTaskRunner()->PostTask(FROM_HERE, error_cb_);
      return;
    }
  }

  // At this point decoder threads should not be running.
  if (!interpolator_.interpolating())
    interpolator_.StartInterpolating();

  base::TimeDelta current_time = GetInterpolatedTime();

  if (do_audio) {
    if (!audio_decoder_->Start(current_time)) {
      GetMediaTaskRunner()->PostTask(FROM_HERE, error_cb_);
      return;
    }

    // Attach listener on UI thread
    ui_task_runner_->PostTask(FROM_HERE, attach_listener_cb_);
  }

  if (do_video) {
    if (!video_decoder_->Start(current_time)) {
      GetMediaTaskRunner()->PostTask(FROM_HERE, error_cb_);
      return;
    }
  }
}

void MediaCodecPlayer::StopDecoders() {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());
  DVLOG(1) << __FUNCTION__;

  audio_decoder_->SyncStop();
  video_decoder_->SyncStop();
}

void MediaCodecPlayer::RequestToStopDecoders() {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());
  DVLOG(1) << __FUNCTION__;

  bool do_audio = false;
  bool do_video = false;

  if (audio_decoder_->IsPrefetchingOrPlaying())
    do_audio = true;
  if (video_decoder_->IsPrefetchingOrPlaying())
    do_video = true;

  if (!do_audio && !do_video) {
    GetMediaTaskRunner()->PostTask(
        FROM_HERE, base::Bind(&MediaCodecPlayer::OnStopDone, media_weak_this_));
    return;
  }

  if (do_audio)
    audio_decoder_->RequestToStop();
  if (do_video)
    video_decoder_->RequestToStop();
}

void MediaCodecPlayer::RequestDemuxerSeek(base::TimeDelta seek_time,
                                          bool is_browser_seek) {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());
  DVLOG(1) << __FUNCTION__ << " " << seek_time
           << (is_browser_seek ? " browser_seek" : "");

  // Flush decoders before requesting demuxer.
  audio_decoder_->Flush();
  video_decoder_->Flush();

  // Save active seek data. Logically it is attached to STATE_WAITING_FOR_SEEK.
  DCHECK(state_ == STATE_WAITING_FOR_SEEK);
  seek_info_.reset(new SeekInfo(seek_time, is_browser_seek));

  demuxer_->RequestDemuxerSeek(seek_time, is_browser_seek);
}

void MediaCodecPlayer::ReleaseDecoderResources() {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());
  DVLOG(1) << __FUNCTION__;

  if (audio_decoder_)
    audio_decoder_->ReleaseDecoderResources();

  if (video_decoder_)
    video_decoder_->ReleaseDecoderResources();

  // At this point decoder threads should not be running
  if (interpolator_.interpolating())
    interpolator_.StopInterpolating();
}

void MediaCodecPlayer::CreateDecoders() {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());
  DVLOG(1) << __FUNCTION__;

  error_cb_ = base::Bind(&MediaCodecPlayer::OnError, media_weak_this_);

  audio_decoder_.reset(new MediaCodecAudioDecoder(
      GetMediaTaskRunner(), base::Bind(&MediaCodecPlayer::RequestDemuxerData,
                                       media_weak_this_, DemuxerStream::AUDIO),
      base::Bind(&MediaCodecPlayer::OnStarvation, media_weak_this_,
                 DemuxerStream::AUDIO),
      base::Bind(&MediaCodecPlayer::OnStopDone, media_weak_this_), error_cb_,
      base::Bind(&MediaCodecPlayer::OnTimeIntervalUpdate, media_weak_this_,
                 DemuxerStream::AUDIO)));

  video_decoder_.reset(new MediaCodecVideoDecoder(
      GetMediaTaskRunner(), base::Bind(&MediaCodecPlayer::RequestDemuxerData,
                                       media_weak_this_, DemuxerStream::VIDEO),
      base::Bind(&MediaCodecPlayer::OnStarvation, media_weak_this_,
                 DemuxerStream::VIDEO),
      base::Bind(&MediaCodecPlayer::OnStopDone, media_weak_this_), error_cb_,
      base::Bind(&MediaCodecPlayer::OnTimeIntervalUpdate, media_weak_this_,
                 DemuxerStream::VIDEO),
      base::Bind(&MediaCodecPlayer::OnVideoResolutionChanged, media_weak_this_),
      base::Bind(&MediaCodecPlayer::OnVideoCodecCreated, media_weak_this_)));
}

bool MediaCodecPlayer::AudioFinished() const {
  return audio_decoder_->IsCompleted() || !audio_decoder_->HasStream();
}

bool MediaCodecPlayer::VideoFinished() const {
  return video_decoder_->IsCompleted() || !video_decoder_->HasStream();
}

base::TimeDelta MediaCodecPlayer::GetInterpolatedTime() {
  DCHECK(GetMediaTaskRunner()->BelongsToCurrentThread());

  base::TimeDelta interpolated_time = interpolator_.GetInterpolatedTime();
  return std::min(interpolated_time, duration_);
}

#undef RETURN_STRING
#define RETURN_STRING(x) \
  case x:                \
    return #x;

const char* MediaCodecPlayer::AsString(PlayerState state) {
  switch (state) {
    RETURN_STRING(STATE_PAUSED);
    RETURN_STRING(STATE_WAITING_FOR_CONFIG);
    RETURN_STRING(STATE_PREFETCHING);
    RETURN_STRING(STATE_PLAYING);
    RETURN_STRING(STATE_STOPPING);
    RETURN_STRING(STATE_WAITING_FOR_SURFACE);
    RETURN_STRING(STATE_WAITING_FOR_SEEK);
    RETURN_STRING(STATE_ERROR);
  }
  return nullptr;  // crash early
}

}  // namespace media
