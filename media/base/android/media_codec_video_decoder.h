// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_ANDROID_MEDIA_CODEC_VIDEO_DECODER_H_
#define MEDIA_BASE_ANDROID_MEDIA_CODEC_VIDEO_DECODER_H_

#include <set>
#include "media/base/android/media_codec_decoder.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gl/android/scoped_java_surface.h"

namespace media {

// Video decoder for MediaCodecPlayer
class MediaCodecVideoDecoder : public MediaCodecDecoder {
 public:
  // Typedefs for the notification callbacks
  typedef base::Callback<void(const gfx::Size& video_size)>
      VideoSizeChangedCallback;

  // For parameters see media_codec_decoder.h
  // update_current_time_cb: callback that reports current playback time.
  //                         Called for released output frame,
  // video_size_changed_cb: reports the new video size,
  // codec_created_cb: reports that video codec has been created. A controller
  //                   class might use it to release more resources so that this
  //                   decoder can use them.
  MediaCodecVideoDecoder(
      const scoped_refptr<base::SingleThreadTaskRunner>& media_runner,
      const base::Closure& request_data_cb,
      const base::Closure& starvation_cb,
      const base::Closure& stop_done_cb,
      const base::Closure& error_cb,
      const SetTimeCallback& update_current_time_cb,
      const VideoSizeChangedCallback& video_size_changed_cb,
      const base::Closure& codec_created_cb);
  ~MediaCodecVideoDecoder() override;

  const char* class_name() const override;

  bool HasStream() const override;
  void SetDemuxerConfigs(const DemuxerConfigs& configs) override;
  void ReleaseDecoderResources() override;
  void ReleaseMediaCodec() override;

  // Stores the video surface to use with upcoming Configure()
  void SetVideoSurface(gfx::ScopedJavaSurface surface);

  // Returns true if there is a video surface to use.
  bool HasVideoSurface() const;

 protected:
  bool IsCodecReconfigureNeeded(const DemuxerConfigs& curr,
                                const DemuxerConfigs& next) const override;
  ConfigStatus ConfigureInternal() override;
  void AssociateCurrentTimeWithPTS(base::TimeDelta pts) override;
  void DissociatePTSFromTime() override;
  void OnOutputFormatChanged() override;
  void Render(int buffer_index,
              size_t offset,
              size_t size,
              RenderMode render_mode,
              base::TimeDelta pts,
              bool eos_encountered) override;

  int NumDelayedRenderTasks() const override;
  void ReleaseDelayedBuffers() override;

#ifndef NDEBUG
  void VerifyUnitIsKeyFrame(const AccessUnit* unit) const override;
#endif

 private:
  // A helper method that releases output buffers and does
  // post-release checks. Might be called by Render() or posted
  // for later execution.
  void ReleaseOutputBuffer(int buffer_index,
                           base::TimeDelta pts,
                           bool render,
                           bool update_time,
                           bool eos_encountered);

  // Data.

  // Configuration received from demuxer
  DemuxerConfigs configs_;

  // Video surface that we render to.
  gfx::ScopedJavaSurface surface_;

  // Reports current playback time to the callee.
  SetTimeCallback update_current_time_cb_;

  // Informs the callee that video size is changed.
  VideoSizeChangedCallback video_size_changed_cb_;

  // Informs the callee that the MediaCodec is created.
  base::Closure codec_created_cb_;

  // Current video size to be sent with |video_size_changed_cb_|.
  gfx::Size video_size_;

  // Indices of output buffers that are posted for rendering.
  std::set<int> delayed_buffers_;

  // Associate presentation timestamps with time.
  base::TimeTicks start_time_ticks_;
  base::TimeDelta start_pts_;

  // Mantain the last seen PTS for stand-alone EOS.
  base::TimeDelta last_seen_pts_;

  DISALLOW_COPY_AND_ASSIGN(MediaCodecVideoDecoder);
};

}  // namespace media

#endif  // MEDIA_BASE_ANDROID_MEDIA_CODEC_VIDEO_DECODER_H_
