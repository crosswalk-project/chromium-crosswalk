// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_ANDROID_MEDIA_CODEC_AUDIO_DECODER_H_
#define MEDIA_BASE_ANDROID_MEDIA_CODEC_AUDIO_DECODER_H_

#include "media/base/android/media_codec_decoder.h"

namespace media {

class AudioTimestampHelper;

// Audio decoder for MediaCodecPlayer
class MediaCodecAudioDecoder : public MediaCodecDecoder {
 public:
  // For parameters see media_codec_decoder.h
  // update_current_time_cb: callback that reports current playback time.
  //                         Called for each rendered frame.
  MediaCodecAudioDecoder(
      const scoped_refptr<base::SingleThreadTaskRunner>& media_runner,
      const base::Closure& request_data_cb,
      const base::Closure& starvation_cb,
      const base::Closure& stop_done_cb,
      const base::Closure& error_cb,
      const SetTimeCallback& update_current_time_cb);
  ~MediaCodecAudioDecoder() override;

  const char* class_name() const override;

  bool HasStream() const override;
  void SetDemuxerConfigs(const DemuxerConfigs& configs) override;
  void ReleaseDecoderResources() override;
  void Flush() override;

  // Sets the volume of the audio output.
  void SetVolume(double volume);

  // Sets the base timestamp for |audio_timestamp_helper_|.
  void SetBaseTimestamp(base::TimeDelta base_timestamp);

 protected:
  bool IsCodecReconfigureNeeded(const DemuxerConfigs& curr,
                                const DemuxerConfigs& next) const override;
  ConfigStatus ConfigureInternal() override;
  void OnOutputFormatChanged() override;
  void Render(int buffer_index,
              size_t offset,
              size_t size,
              RenderMode render_mode,
              base::TimeDelta pts,
              bool eos_encountered) override;

 private:
  // A helper method to set the volume.
  void SetVolumeInternal();

  // Recreates |audio_timestamp_helper_|, called when sampling rate is changed.
  void ResetTimestampHelper();

  // Data.

  // Configuration received from demuxer
  DemuxerConfigs configs_;

  // Requested volume
  double volume_;

  // Number of bytes per audio frame. Depends on the output format and the
  // number of channels.
  int bytes_per_frame_;

  // The sampling rate received from decoder.
  int output_sampling_rate_;

  // Frame count to sync with audio codec output.
  int64 frame_count_;

  // Base timestamp for the |audio_timestamp_helper_|.
  base::TimeDelta base_timestamp_;

  // Object to calculate the current audio timestamp for A/V sync.
  scoped_ptr<AudioTimestampHelper> audio_timestamp_helper_;

  // Reports current playback time to the callee.
  SetTimeCallback update_current_time_cb_;

  DISALLOW_COPY_AND_ASSIGN(MediaCodecAudioDecoder);
};

}  // namespace media

#endif  // MEDIA_BASE_ANDROID_MEDIA_CODEC_DECODER_H_
