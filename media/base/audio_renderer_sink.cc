// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/audio_renderer_sink.h"

namespace media {

int AudioRendererSink::RenderCallback::Render(AudioBus* dest,
                                              int audio_delay_milliseconds) {
  return 0;
}

int AudioRendererSink::RenderCallback::Render(
    AudioBus* dest,
    int audio_delay_milliseconds,
    const StreamPosition& device_position) {
  return Render(dest, audio_delay_milliseconds);
}

}  // namespace media
