// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/logging.h"
#include "base/thread_task_runner_handle.h"
#include "base/timer/timer.h"
#include "media/base/android/media_codec_audio_decoder.h"
#include "media/base/android/media_codec_bridge.h"
#include "media/base/android/media_codec_video_decoder.h"
#include "media/base/android/test_data_factory.h"
#include "media/base/android/test_statistics.h"
#include "media/base/buffers.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gl/android/surface_texture.h"

namespace media {

// Helper macro to skip the test if MediaCodecBridge isn't available.
#define SKIP_TEST_IF_MEDIA_CODEC_BRIDGE_IS_NOT_AVAILABLE()        \
  do {                                                            \
    if (!MediaCodecBridge::IsAvailable()) {                       \
      VLOG(0) << "Could not run test - not supported on device."; \
      return;                                                     \
    }                                                             \
  } while (0)

namespace {

const base::TimeDelta kDefaultTimeout = base::TimeDelta::FromMilliseconds(200);
const base::TimeDelta kAudioFramePeriod =
    base::TimeDelta::FromSecondsD(1024.0 / 44100);  // 1024 samples @ 44100 Hz
const base::TimeDelta kVideoFramePeriod = base::TimeDelta::FromMilliseconds(20);

class AudioFactory : public TestDataFactory {
 public:
  AudioFactory(base::TimeDelta duration);
  DemuxerConfigs GetConfigs() const override;

 protected:
  void ModifyAccessUnit(int index_in_chunk, AccessUnit* unit) override;
};

class VideoFactory : public TestDataFactory {
 public:
  VideoFactory(base::TimeDelta duration);
  DemuxerConfigs GetConfigs() const override;

 protected:
  void ModifyAccessUnit(int index_in_chunk, AccessUnit* unit) override;
};

AudioFactory::AudioFactory(base::TimeDelta duration)
    : TestDataFactory("aac-44100-packet-%d", duration, kAudioFramePeriod) {
}

DemuxerConfigs AudioFactory::GetConfigs() const {
  return TestDataFactory::CreateAudioConfigs(kCodecAAC, duration_);
}

void AudioFactory::ModifyAccessUnit(int index_in_chunk, AccessUnit* unit) {
  unit->is_key_frame = true;
}

VideoFactory::VideoFactory(base::TimeDelta duration)
    : TestDataFactory("h264-320x180-frame-%d", duration, kVideoFramePeriod) {
}

DemuxerConfigs VideoFactory::GetConfigs() const {
  return TestDataFactory::CreateVideoConfigs(kCodecH264, duration_,
                                             gfx::Size(320, 180));
}

void VideoFactory::ModifyAccessUnit(int index_in_chunk, AccessUnit* unit) {
  // The frames are taken from High profile and some are B-frames.
  // The first 4 frames appear in the file in the following order:
  //
  // Frames:             I P B P
  // Decoding order:     0 1 2 3
  // Presentation order: 0 2 1 4(3)
  //
  // I keep the last PTS to be 3 for simplicity.

  // Swap pts for second and third frames. Make first frame a key frame.
  switch (index_in_chunk) {
    case 0:  // first frame
      unit->is_key_frame = true;
      break;
    case 1:  // second frame
      unit->timestamp += frame_period_;
      break;
    case 2:  // third frame
      unit->timestamp -= frame_period_;
      break;
    case 3:  // fourth frame, do not modify
      break;
    default:
      NOTREACHED();
      break;
  }
}

}  // namespace (anonymous)

// The test fixture for MediaCodecDecoder

class MediaCodecDecoderTest : public testing::Test {
 public:
  MediaCodecDecoderTest();
  ~MediaCodecDecoderTest() override;

  // Conditions we wait for.
  bool is_prefetched() const { return is_prefetched_; }
  bool is_stopped() const { return is_stopped_; }
  bool is_starved() const { return is_starved_; }

  void SetPrefetched(bool value) { is_prefetched_ = value; }
  void SetStopped(bool value) { is_stopped_ = value; }
  void SetStarved(bool value) { is_starved_ = value; }

 protected:
  typedef base::Callback<bool()> Predicate;

  typedef base::Callback<void(const DemuxerData&)> DataAvailableCallback;

  // Waits for condition to become true or for timeout to expire.
  // Returns true if the condition becomes true.
  bool WaitForCondition(const Predicate& condition,
                        const base::TimeDelta& timeout = kDefaultTimeout);

  void SetDataFactory(scoped_ptr<TestDataFactory> factory) {
    data_factory_ = factory.Pass();
  }

  DemuxerConfigs GetConfigs() const {
    // ASSERT_NE does not compile here because it expects void return value.
    EXPECT_NE(nullptr, data_factory_.get());
    return data_factory_->GetConfigs();
  }

  void CreateAudioDecoder();
  void CreateVideoDecoder();
  void SetVideoSurface();
  void SetStopRequestAtTime(const base::TimeDelta& time) {
    stop_request_time_ = time;
  }

  // Decoder callbacks.
  void OnDataRequested();
  void OnStarvation() { is_starved_ = true; }
  void OnStopDone() { is_stopped_ = true; }
  void OnError() { DVLOG(0) << "MediaCodecDecoderTest::" << __FUNCTION__; }
  void OnUpdateCurrentTime(base::TimeDelta now_playing,
                           base::TimeDelta last_buffered) {
    // Add the |last_buffered| value for PTS. For video it is the same as
    // |now_playing| and is equal to PTS, for audio |last_buffered| should
    // exceed PTS.
    pts_stat_.AddValue(last_buffered);

    if (stop_request_time_ != kNoTimestamp() &&
        now_playing >= stop_request_time_) {
      stop_request_time_ = kNoTimestamp();
      decoder_->RequestToStop();
    }
  }

  void OnVideoSizeChanged(const gfx::Size& video_size) {}
  void OnVideoCodecCreated() {}

  scoped_ptr<MediaCodecDecoder> decoder_;
  scoped_ptr<TestDataFactory> data_factory_;
  Minimax<base::TimeDelta> pts_stat_;

 private:
  bool is_timeout_expired() const { return is_timeout_expired_; }
  void SetTimeoutExpired(bool value) { is_timeout_expired_ = value; }

  base::MessageLoop message_loop_;
  bool is_timeout_expired_;

  bool is_prefetched_;
  bool is_stopped_;
  bool is_starved_;
  base::TimeDelta stop_request_time_;

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  DataAvailableCallback data_available_cb_;
  scoped_refptr<gfx::SurfaceTexture> surface_texture_;

  DISALLOW_COPY_AND_ASSIGN(MediaCodecDecoderTest);
};

MediaCodecDecoderTest::MediaCodecDecoderTest()
    : is_timeout_expired_(false),
      is_prefetched_(false),
      is_stopped_(false),
      is_starved_(false),
      stop_request_time_(kNoTimestamp()),
      task_runner_(base::ThreadTaskRunnerHandle::Get()) {
}

MediaCodecDecoderTest::~MediaCodecDecoderTest() {}

bool MediaCodecDecoderTest::WaitForCondition(const Predicate& condition,
                                             const base::TimeDelta& timeout) {
  // Let the message_loop_ process events.
  // We start the timer and RunUntilIdle() until it signals.

  SetTimeoutExpired(false);

  base::Timer timer(false, false);
  timer.Start(FROM_HERE, timeout,
              base::Bind(&MediaCodecDecoderTest::SetTimeoutExpired,
                         base::Unretained(this), true));

  do {
    if (condition.Run()) {
      timer.Stop();
      return true;
    }
    message_loop_.RunUntilIdle();
  } while (!is_timeout_expired());

  DCHECK(!timer.IsRunning());
  return false;
}

void MediaCodecDecoderTest::CreateAudioDecoder() {
  decoder_ = scoped_ptr<MediaCodecDecoder>(new MediaCodecAudioDecoder(
      task_runner_, base::Bind(&MediaCodecDecoderTest::OnDataRequested,
                               base::Unretained(this)),
      base::Bind(&MediaCodecDecoderTest::OnStarvation, base::Unretained(this)),
      base::Bind(&MediaCodecDecoderTest::OnStopDone, base::Unretained(this)),
      base::Bind(&MediaCodecDecoderTest::OnError, base::Unretained(this)),
      base::Bind(&MediaCodecDecoderTest::OnUpdateCurrentTime,
                 base::Unretained(this))));

  data_available_cb_ = base::Bind(&MediaCodecDecoder::OnDemuxerDataAvailable,
                                  base::Unretained(decoder_.get()));
}

void MediaCodecDecoderTest::CreateVideoDecoder() {
  decoder_ = scoped_ptr<MediaCodecDecoder>(new MediaCodecVideoDecoder(
      task_runner_, base::Bind(&MediaCodecDecoderTest::OnDataRequested,
                               base::Unretained(this)),
      base::Bind(&MediaCodecDecoderTest::OnStarvation, base::Unretained(this)),
      base::Bind(&MediaCodecDecoderTest::OnStopDone, base::Unretained(this)),
      base::Bind(&MediaCodecDecoderTest::OnError, base::Unretained(this)),
      base::Bind(&MediaCodecDecoderTest::OnUpdateCurrentTime,
                 base::Unretained(this)),
      base::Bind(&MediaCodecDecoderTest::OnVideoSizeChanged,
                 base::Unretained(this)),
      base::Bind(&MediaCodecDecoderTest::OnVideoCodecCreated,
                 base::Unretained(this))));

  data_available_cb_ = base::Bind(&MediaCodecDecoder::OnDemuxerDataAvailable,
                                  base::Unretained(decoder_.get()));
}

void MediaCodecDecoderTest::OnDataRequested() {
  if (!data_factory_)
    return;

  DemuxerData data;
  base::TimeDelta delay;
  if (!data_factory_->CreateChunk(&data, &delay))
    return;

  task_runner_->PostDelayedTask(FROM_HERE, base::Bind(data_available_cb_, data),
                                delay);
}

void MediaCodecDecoderTest::SetVideoSurface() {
  surface_texture_ = gfx::SurfaceTexture::Create(0);
  gfx::ScopedJavaSurface surface(surface_texture_.get());
  ASSERT_NE(nullptr, decoder_.get());
  MediaCodecVideoDecoder* video_decoder =
      static_cast<MediaCodecVideoDecoder*>(decoder_.get());
  video_decoder->SetVideoSurface(surface.Pass());
}

TEST_F(MediaCodecDecoderTest, AudioPrefetch) {
  CreateAudioDecoder();

  base::TimeDelta duration = base::TimeDelta::FromMilliseconds(500);
  SetDataFactory(scoped_ptr<TestDataFactory>(new AudioFactory(duration)));

  decoder_->Prefetch(base::Bind(&MediaCodecDecoderTest::SetPrefetched,
                                base::Unretained(this), true));

  EXPECT_TRUE(WaitForCondition(base::Bind(&MediaCodecDecoderTest::is_prefetched,
                                          base::Unretained(this))));
}

TEST_F(MediaCodecDecoderTest, VideoPrefetch) {
  CreateVideoDecoder();

  base::TimeDelta duration = base::TimeDelta::FromMilliseconds(500);
  SetDataFactory(scoped_ptr<VideoFactory>(new VideoFactory(duration)));

  decoder_->Prefetch(base::Bind(&MediaCodecDecoderTest::SetPrefetched,
                                base::Unretained(this), true));

  EXPECT_TRUE(WaitForCondition(base::Bind(&MediaCodecDecoderTest::is_prefetched,
                                          base::Unretained(this))));
}

TEST_F(MediaCodecDecoderTest, AudioConfigureNoParams) {
  SKIP_TEST_IF_MEDIA_CODEC_BRIDGE_IS_NOT_AVAILABLE();

  CreateAudioDecoder();

  // Cannot configure without config parameters.
  EXPECT_EQ(MediaCodecDecoder::kConfigFailure, decoder_->Configure());
}

TEST_F(MediaCodecDecoderTest, AudioConfigureValidParams) {
  SKIP_TEST_IF_MEDIA_CODEC_BRIDGE_IS_NOT_AVAILABLE();

  CreateAudioDecoder();

  base::TimeDelta duration = base::TimeDelta::FromMilliseconds(500);
  scoped_ptr<AudioFactory> factory(new AudioFactory(duration));
  decoder_->SetDemuxerConfigs(factory->GetConfigs());

  EXPECT_EQ(MediaCodecDecoder::kConfigOk, decoder_->Configure());
}

TEST_F(MediaCodecDecoderTest, VideoConfigureNoParams) {
  SKIP_TEST_IF_MEDIA_CODEC_BRIDGE_IS_NOT_AVAILABLE();

  CreateVideoDecoder();

  // decoder_->Configure() searches back for the key frame.
  // We have to prefetch decoder.

  base::TimeDelta duration = base::TimeDelta::FromMilliseconds(500);
  SetDataFactory(scoped_ptr<VideoFactory>(new VideoFactory(duration)));

  decoder_->Prefetch(base::Bind(&MediaCodecDecoderTest::SetPrefetched,
                                base::Unretained(this), true));

  EXPECT_TRUE(WaitForCondition(base::Bind(&MediaCodecDecoderTest::is_prefetched,
                                          base::Unretained(this))));

  SetVideoSurface();

  // Cannot configure without config parameters.
  EXPECT_EQ(MediaCodecDecoder::kConfigFailure, decoder_->Configure());
}

TEST_F(MediaCodecDecoderTest, VideoConfigureNoSurface) {
  SKIP_TEST_IF_MEDIA_CODEC_BRIDGE_IS_NOT_AVAILABLE();

  CreateVideoDecoder();

  // decoder_->Configure() searches back for the key frame.
  // We have to prefetch decoder.

  base::TimeDelta duration = base::TimeDelta::FromMilliseconds(500);
  SetDataFactory(scoped_ptr<VideoFactory>(new VideoFactory(duration)));

  decoder_->Prefetch(base::Bind(&MediaCodecDecoderTest::SetPrefetched,
                                base::Unretained(this), true));

  EXPECT_TRUE(WaitForCondition(base::Bind(&MediaCodecDecoderTest::is_prefetched,
                                          base::Unretained(this))));

  decoder_->SetDemuxerConfigs(GetConfigs());

  // Surface is not set, Configure() should fail.

  EXPECT_EQ(MediaCodecDecoder::kConfigFailure, decoder_->Configure());
}

TEST_F(MediaCodecDecoderTest, VideoConfigureInvalidSurface) {
  SKIP_TEST_IF_MEDIA_CODEC_BRIDGE_IS_NOT_AVAILABLE();

  CreateVideoDecoder();

  // decoder_->Configure() searches back for the key frame.
  // We have to prefetch decoder.

  base::TimeDelta duration = base::TimeDelta::FromMilliseconds(500);
  SetDataFactory(scoped_ptr<VideoFactory>(new VideoFactory(duration)));

  decoder_->Prefetch(base::Bind(&MediaCodecDecoderTest::SetPrefetched,
                                base::Unretained(this), true));

  EXPECT_TRUE(WaitForCondition(base::Bind(&MediaCodecDecoderTest::is_prefetched,
                                          base::Unretained(this))));

  decoder_->SetDemuxerConfigs(GetConfigs());

  // Prepare the surface.
  scoped_refptr<gfx::SurfaceTexture> surface_texture(
      gfx::SurfaceTexture::Create(0));
  gfx::ScopedJavaSurface surface(surface_texture.get());

  // Release the surface texture.
  surface_texture = NULL;

  MediaCodecVideoDecoder* video_decoder =
      static_cast<MediaCodecVideoDecoder*>(decoder_.get());
  video_decoder->SetVideoSurface(surface.Pass());

  EXPECT_EQ(MediaCodecDecoder::kConfigFailure, decoder_->Configure());
}

TEST_F(MediaCodecDecoderTest, VideoConfigureValidParams) {
  SKIP_TEST_IF_MEDIA_CODEC_BRIDGE_IS_NOT_AVAILABLE();

  CreateVideoDecoder();

  // decoder_->Configure() searches back for the key frame.
  // We have to prefetch decoder.

  base::TimeDelta duration = base::TimeDelta::FromMilliseconds(500);
  SetDataFactory(scoped_ptr<VideoFactory>(new VideoFactory(duration)));

  decoder_->Prefetch(base::Bind(&MediaCodecDecoderTest::SetPrefetched,
                                base::Unretained(this), true));

  EXPECT_TRUE(WaitForCondition(base::Bind(&MediaCodecDecoderTest::is_prefetched,
                                          base::Unretained(this))));

  decoder_->SetDemuxerConfigs(GetConfigs());

  SetVideoSurface();

  // Now we can expect Configure() to succeed.

  EXPECT_EQ(MediaCodecDecoder::kConfigOk, decoder_->Configure());
}

TEST_F(MediaCodecDecoderTest, AudioStartWithoutConfigure) {
  SKIP_TEST_IF_MEDIA_CODEC_BRIDGE_IS_NOT_AVAILABLE();

  CreateAudioDecoder();

  // Decoder has to be prefetched and configured before the start.

  // Wrong state: not prefetched
  EXPECT_FALSE(decoder_->Start(base::TimeDelta::FromMilliseconds(0)));

  // Do the prefetch.
  base::TimeDelta duration = base::TimeDelta::FromMilliseconds(500);
  SetDataFactory(scoped_ptr<AudioFactory>(new AudioFactory(duration)));

  // Prefetch to avoid starvation at the beginning of playback.
  decoder_->Prefetch(base::Bind(&MediaCodecDecoderTest::SetPrefetched,
                                base::Unretained(this), true));

  EXPECT_TRUE(WaitForCondition(base::Bind(&MediaCodecDecoderTest::is_prefetched,
                                          base::Unretained(this))));

  // Still, decoder is not configured.
  EXPECT_FALSE(decoder_->Start(base::TimeDelta::FromMilliseconds(0)));
}

// http://crbug.com/518900
TEST_F(MediaCodecDecoderTest, AudioPlayTillCompletion) {
  SKIP_TEST_IF_MEDIA_CODEC_BRIDGE_IS_NOT_AVAILABLE();

  DVLOG(0) << "AudioPlayTillCompletion started";

  CreateAudioDecoder();

  base::TimeDelta duration = base::TimeDelta::FromMilliseconds(500);
  base::TimeDelta timeout = base::TimeDelta::FromMilliseconds(1500);

  SetDataFactory(scoped_ptr<AudioFactory>(new AudioFactory(duration)));

  // Prefetch to avoid starvation at the beginning of playback.
  decoder_->Prefetch(base::Bind(&MediaCodecDecoderTest::SetPrefetched,
                                base::Unretained(this), true));

  EXPECT_TRUE(WaitForCondition(base::Bind(&MediaCodecDecoderTest::is_prefetched,
                                          base::Unretained(this))));

  decoder_->SetDemuxerConfigs(GetConfigs());

  EXPECT_EQ(MediaCodecDecoder::kConfigOk, decoder_->Configure());

  EXPECT_TRUE(decoder_->Start(base::TimeDelta::FromMilliseconds(0)));

  EXPECT_TRUE(WaitForCondition(
      base::Bind(&MediaCodecDecoderTest::is_stopped, base::Unretained(this)),
      timeout));

  EXPECT_TRUE(decoder_->IsStopped());
  EXPECT_TRUE(decoder_->IsCompleted());

  // Last buffered timestamp should be no less than PTS.
  // The number of hits in pts_stat_ depends on the preroll implementation.
  // We might not report the time for the first buffer after preroll that
  // is written to the audio track. pts_stat_.num_values() is either 21 or 22.
  EXPECT_LE(21, pts_stat_.num_values());
  EXPECT_LE(data_factory_->last_pts(), pts_stat_.max());

  DVLOG(0) << "AudioPlayTillCompletion stopping";
}

TEST_F(MediaCodecDecoderTest, VideoPlayTillCompletion) {
  SKIP_TEST_IF_MEDIA_CODEC_BRIDGE_IS_NOT_AVAILABLE();

  CreateVideoDecoder();

  base::TimeDelta duration = base::TimeDelta::FromMilliseconds(500);
  // The first output frame might come out with significant delay. Apparently
  // the codec does initial configuration at this time. We increase the timeout
  // to leave a room of 1 second for this initial configuration.
  base::TimeDelta timeout = base::TimeDelta::FromMilliseconds(1500);
  SetDataFactory(scoped_ptr<VideoFactory>(new VideoFactory(duration)));

  // Prefetch
  decoder_->Prefetch(base::Bind(&MediaCodecDecoderTest::SetPrefetched,
                                base::Unretained(this), true));

  EXPECT_TRUE(WaitForCondition(base::Bind(&MediaCodecDecoderTest::is_prefetched,
                                          base::Unretained(this))));

  decoder_->SetDemuxerConfigs(GetConfigs());

  SetVideoSurface();

  EXPECT_EQ(MediaCodecDecoder::kConfigOk, decoder_->Configure());

  EXPECT_TRUE(decoder_->Start(base::TimeDelta::FromMilliseconds(0)));

  EXPECT_TRUE(WaitForCondition(
      base::Bind(&MediaCodecDecoderTest::is_stopped, base::Unretained(this)),
      timeout));

  EXPECT_TRUE(decoder_->IsStopped());
  EXPECT_TRUE(decoder_->IsCompleted());

  EXPECT_EQ(26, pts_stat_.num_values());
  EXPECT_EQ(data_factory_->last_pts(), pts_stat_.max());
}

TEST_F(MediaCodecDecoderTest, VideoStopAndResume) {
  SKIP_TEST_IF_MEDIA_CODEC_BRIDGE_IS_NOT_AVAILABLE();

  CreateVideoDecoder();

  base::TimeDelta duration = base::TimeDelta::FromMilliseconds(500);
  base::TimeDelta stop_request_time = base::TimeDelta::FromMilliseconds(200);
  base::TimeDelta timeout = base::TimeDelta::FromMilliseconds(1000);

  SetDataFactory(scoped_ptr<VideoFactory>(new VideoFactory(duration)));

  // Prefetch
  decoder_->Prefetch(base::Bind(&MediaCodecDecoderTest::SetPrefetched,
                                base::Unretained(this), true));

  EXPECT_TRUE(WaitForCondition(base::Bind(&MediaCodecDecoderTest::is_prefetched,
                                          base::Unretained(this))));

  decoder_->SetDemuxerConfigs(GetConfigs());

  SetVideoSurface();

  EXPECT_EQ(MediaCodecDecoder::kConfigOk, decoder_->Configure());

  SetStopRequestAtTime(stop_request_time);

  // Start from the beginning.
  EXPECT_TRUE(decoder_->Start(base::TimeDelta::FromMilliseconds(0)));

  EXPECT_TRUE(WaitForCondition(
      base::Bind(&MediaCodecDecoderTest::is_stopped, base::Unretained(this)),
      timeout));

  EXPECT_TRUE(decoder_->IsStopped());
  EXPECT_FALSE(decoder_->IsCompleted());

  base::TimeDelta last_pts = pts_stat_.max();

  EXPECT_GE(last_pts, stop_request_time);

  // Resume playback from last_pts:

  SetPrefetched(false);
  SetStopped(false);

  // Prefetch again.
  decoder_->Prefetch(base::Bind(&MediaCodecDecoderTest::SetPrefetched,
                                base::Unretained(this), true));

  EXPECT_TRUE(WaitForCondition(base::Bind(&MediaCodecDecoderTest::is_prefetched,
                                          base::Unretained(this))));

  // Then start.
  EXPECT_TRUE(decoder_->Start(last_pts));

  // Wait till completion.
  EXPECT_TRUE(WaitForCondition(
      base::Bind(&MediaCodecDecoderTest::is_stopped, base::Unretained(this)),
      timeout));

  EXPECT_TRUE(decoder_->IsStopped());
  EXPECT_TRUE(decoder_->IsCompleted());

  // We should not skip frames in this process.
  EXPECT_EQ(26, pts_stat_.num_values());
  EXPECT_EQ(data_factory_->last_pts(), pts_stat_.max());
}

// http://crbug.com/518900
TEST_F(MediaCodecDecoderTest, DISABLED_AudioStarvationAndStop) {
  SKIP_TEST_IF_MEDIA_CODEC_BRIDGE_IS_NOT_AVAILABLE();

  CreateAudioDecoder();

  base::TimeDelta duration = base::TimeDelta::FromMilliseconds(200);
  base::TimeDelta timeout = base::TimeDelta::FromMilliseconds(400);

  AudioFactory* factory = new AudioFactory(duration);
  factory->SetStarvationMode(true);
  SetDataFactory(scoped_ptr<AudioFactory>(factory));

  // Prefetch.
  decoder_->Prefetch(base::Bind(&MediaCodecDecoderTest::SetPrefetched,
                                base::Unretained(this), true));

  EXPECT_TRUE(WaitForCondition(base::Bind(&MediaCodecDecoderTest::is_prefetched,
                                          base::Unretained(this))));

  // Configure.
  decoder_->SetDemuxerConfigs(GetConfigs());

  EXPECT_EQ(MediaCodecDecoder::kConfigOk, decoder_->Configure());

  // Start.
  EXPECT_TRUE(decoder_->Start(base::TimeDelta::FromMilliseconds(0)));

  // Wait for starvation.
  EXPECT_TRUE(WaitForCondition(
      base::Bind(&MediaCodecDecoderTest::is_starved, base::Unretained(this)),
      timeout));

  EXPECT_FALSE(decoder_->IsStopped());
  EXPECT_FALSE(decoder_->IsCompleted());

  EXPECT_GT(pts_stat_.num_values(), 0);

  // After starvation we should be able to stop decoder.
  decoder_->RequestToStop();

  EXPECT_TRUE(WaitForCondition(
      base::Bind(&MediaCodecDecoderTest::is_stopped, base::Unretained(this))));

  EXPECT_TRUE(decoder_->IsStopped());
  EXPECT_FALSE(decoder_->IsCompleted());
}

}  // namespace media
