// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_io.h"

namespace media {

int AudioOutputStream::AudioSourceCallback::OnMoreData(
    AudioBus* dest,
    uint32_t total_bytes_delay,
    uint32_t frames_skipped) {
  return 0;
}

int AudioOutputStream::AudioSourceCallback::OnMoreData(
    AudioBus* dest,
    uint32_t total_bytes_delay,
    uint32_t frames_skipped,
    const StreamPosition& device_position) {
  return OnMoreData(dest, total_bytes_delay, frames_skipped);
}

}  // namespace media
