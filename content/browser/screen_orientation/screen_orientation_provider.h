// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SCREEN_ORIENTATION_SCREEN_ORIENTATION_PROVIDER_H_
#define CONTENT_BROWSER_SCREEN_ORIENTATION_SCREEN_ORIENTATION_PROVIDER_H_

#include "base/macros.h"
#include "third_party/WebKit/public/platform/WebScreenOrientationLockType.h"

namespace content {

class ScreenOrientationDispatcherHost;
class WebContents;

// Interface that needs to be implemented by any backend that wants to handle
// screen orientation lock/unlock.
class ScreenOrientationProvider {
 public:
  // Lock the screen orientation to |orientations|.
  virtual void LockOrientation(
      int request_id,
      blink::WebScreenOrientationLockType orientations) = 0;

  // Unlock the screen orientation.
  virtual void UnlockOrientation() = 0;

  // Inform about a screen orientation update. It is called to let the provider
  // know if a lock has been resolved.
  virtual void OnOrientationChange() = 0;

  virtual ~ScreenOrientationProvider() {}

 protected:
  friend class ScreenOrientationDispatcherHost;

  static ScreenOrientationProvider* Create(
      ScreenOrientationDispatcherHost* dispatcher_host,
      WebContents* web_contents);

  ScreenOrientationProvider() {}

  DISALLOW_COPY_AND_ASSIGN(ScreenOrientationProvider);
};

} // namespace content

#endif // CONTENT_BROWSER_SCREEN_ORIENTATION_SCREEN_ORIENTATION_PROVIDER_H_
