// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_ANDROID_AUDIO_DECODER_JOB_H_
#define MEDIA_BASE_ANDROID_AUDIO_DECODER_JOB_H_

#include <jni.h>
#include <vector>

#include "base/callback.h"
#include "media/base/android/media_decoder_job.h"

namespace media {

class AudioCodecBridge;
class AudioTimestampHelper;

// Class for managing audio decoding jobs.
class AudioDecoderJob : public MediaDecoderJob {
 public:
  // Creates a new AudioDecoderJob instance for decoding audio.
  // |request_data_cb| - Callback used to request more data for the decoder.
  // |on_demuxer_config_changed_cb| - Callback used to inform the caller that
  // demuxer config has changed.
  AudioDecoderJob(const base::Closure& request_data_cb,
                  const base::Closure& on_demuxer_config_changed_cb);
  ~AudioDecoderJob() override;

  // MediaDecoderJob implementation.
  bool HasStream() const override;
  void Flush() override;
  void SetDemuxerConfigs(const DemuxerConfigs& configs) override;

  // Sets the volume of the audio output.
  void SetVolume(double volume);
  double volume() const { return volume_; }

  // Sets the base timestamp for |audio_timestamp_helper_|.
  void SetBaseTimestamp(base::TimeDelta base_timestamp);

 private:
  // MediaDecoderJob implementation.
  void ReleaseOutputBuffer(
      int output_buffer_index,
      size_t offset,
      size_t size,
      bool render_output,
      base::TimeDelta current_presentation_timestamp,
      const ReleaseOutputCompletionCallback& callback) override;
  bool ComputeTimeToRender() const override;
  bool AreDemuxerConfigsChanged(const DemuxerConfigs& configs) const override;
  MediaDecoderJobStatus CreateMediaCodecBridgeInternal() override;
  void OnOutputFormatChanged() override;

  // Helper method to set the audio output volume.
  void SetVolumeInternal();

  void ResetTimestampHelper();

  // Audio configs from the demuxer.
  AudioCodec audio_codec_;
  int num_channels_;
  int config_sampling_rate_;
  std::vector<uint8> audio_extra_data_;
  int64 audio_codec_delay_ns_;
  int64 audio_seek_preroll_ns_;
  double volume_;
  int bytes_per_frame_;

  // Audio output sample rate
  int output_sampling_rate_;

  // Frame count to sync with audio codec output
  int64 frame_count_;

  // Base timestamp for the |audio_timestamp_helper_|.
  base::TimeDelta base_timestamp_;

  // Object to calculate the current audio timestamp for A/V sync.
  scoped_ptr<AudioTimestampHelper> audio_timestamp_helper_;

  DISALLOW_COPY_AND_ASSIGN(AudioDecoderJob);
};

}  // namespace media

#endif  // MEDIA_BASE_ANDROID_AUDIO_DECODER_JOB_H_
