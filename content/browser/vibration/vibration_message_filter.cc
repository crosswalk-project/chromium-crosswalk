// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/vibration/vibration_message_filter.h"

#include <algorithm>

#include "base/safe_numerics.h"
#include "content/common/view_messages.h"
#include "content/port/browser/vibration_provider.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"
#include "third_party/WebKit/public/platform/WebVibration.h"

namespace content {

// Minimum duration of a vibration is 1 millisecond.
const int64 kMinimumVibrationDurationMs = 1;

VibrationMessageFilter::VibrationMessageFilter() {
  provider_.reset(GetContentClient()->browser()->OverrideVibrationProvider());
#if !defined(OS_ANDROID) && !defined(OS_TIZEN_MOBILE)
  if (!provider_.get())
    provider_.reset(CreateProvider());
#endif
}

VibrationMessageFilter::~VibrationMessageFilter() {
}

bool VibrationMessageFilter::OnMessageReceived(
    const IPC::Message& message,
    bool* message_was_ok) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_EX(VibrationMessageFilter,
                           message,
                           *message_was_ok)
    IPC_MESSAGE_HANDLER(ViewHostMsg_Vibrate, OnVibrate)
    IPC_MESSAGE_HANDLER(ViewHostMsg_CancelVibration, OnCancelVibration)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP_EX()
  return handled;
}

void VibrationMessageFilter::OnVibrate(int64 milliseconds) {
  if (!provider_.get())
    return;

  // Though the Blink implementation already sanitizes vibration times, don't
  // trust any values passed from the renderer.
  milliseconds = std::max(kMinimumVibrationDurationMs, std::min(milliseconds,
          base::checked_numeric_cast<int64>(WebKit::kVibrationDurationMax)));

  provider_->Vibrate(milliseconds);
}

void VibrationMessageFilter::OnCancelVibration() {
  if (!provider_.get())
    return;

  provider_->CancelVibration();
}

#if !defined(OS_ANDROID) && !defined(OS_TIZEN_MOBILE)
// static
VibrationProvider* VibrationMessageFilter::CreateProvider() {
  return NULL;
}
#endif
}  // namespace content
