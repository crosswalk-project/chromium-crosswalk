// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/audio_renderer_sink.h"

namespace media {

int AudioRendererSink::RenderCallback::Render(
    AudioBus* dest,
    uint32_t audio_delay_milliseconds,
    uint32_t frames_skipped) {
  return 0;
}

int AudioRendererSink::RenderCallback::Render(
    AudioBus* dest,
    uint32_t audio_delay_milliseconds,
    uint32_t frames_skipped,
    const StreamPosition& device_position) {
  return Render(dest, audio_delay_milliseconds, frames_skipped);
}

}  // namespace media
