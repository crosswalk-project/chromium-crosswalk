// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/screen_orientation/screen_orientation_provider.h"

namespace content {

#if !defined(OS_ANDROID)
// static
ScreenOrientationProvider* ScreenOrientationProvider::Create(
    ScreenOrientationDispatcherHost* dispatcher_host,
    WebContents* web_contents) {
  return NULL;
}
#endif // !defined(OS_ANDROID)

}  // namespace content
