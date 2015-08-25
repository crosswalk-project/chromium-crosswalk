// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/android/demuxer_stream_player_params.h"
#include <iomanip>

namespace media {

DemuxerConfigs::DemuxerConfigs()
    : audio_codec(kUnknownAudioCodec),
      audio_channels(0),
      audio_sampling_rate(0),
      is_audio_encrypted(false),
      audio_codec_delay_ns(-1),
      audio_seek_preroll_ns(-1),
      video_codec(kUnknownVideoCodec),
      is_video_encrypted(false) {}

DemuxerConfigs::~DemuxerConfigs() {}

AccessUnit::AccessUnit() : is_end_of_stream(false), is_key_frame(false) {}

AccessUnit::~AccessUnit() {}

DemuxerData::DemuxerData() : type(DemuxerStream::UNKNOWN) {}

DemuxerData::~DemuxerData() {}

}  // namespace media

std::ostream& operator<<(std::ostream& os, const media::AccessUnit& au) {
  os << "status:" << au.status << (au.is_end_of_stream ? " EOS" : "")
     << (au.is_key_frame ? " KEY_FRAME" : "") << " pts:" << au.timestamp
     << " size:" << au.data.size();
  return os;
}

std::ostream& operator<<(std::ostream& os, const media::DemuxerConfigs& conf) {
  os << "duration:" << conf.duration;

  if (conf.audio_codec == media::kUnknownAudioCodec &&
      conf.video_codec == media::kUnknownVideoCodec) {
    os << " no audio, no video";
    return os;
  }

  if (conf.audio_codec != media::kUnknownAudioCodec) {
    os << " audio:" << media::AsString(conf.audio_codec)
       << " channels:" << conf.audio_channels
       << " rate:" << conf.audio_sampling_rate
       << (conf.is_audio_encrypted ? " encrypted" : "")
       << " delay (ns):" << conf.audio_codec_delay_ns
       << " preroll (ns):" << conf.audio_seek_preroll_ns;

    if (!conf.audio_extra_data.empty()) {
      os << " extra:{" << std::hex;
      for (uint8 byte : conf.audio_extra_data)
        os << " " << std::setfill('0') << std::setw(2) << (int)byte;
      os << "}" << std::dec;
    }
  }

  if (conf.video_codec != media::kUnknownVideoCodec) {
    os << " video:" << media::AsString(conf.video_codec) << " "
       << conf.video_size.width() << "x" << conf.video_size.height()
       << (conf.is_video_encrypted ? " encrypted" : "");
  }

  return os;
}
