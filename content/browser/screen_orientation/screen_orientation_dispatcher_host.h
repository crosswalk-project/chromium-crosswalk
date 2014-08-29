// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SCREEN_ORIENTATION_SCREEN_ORIENTATION_DISPATCHER_HOST_H_
#define CONTENT_BROWSER_SCREEN_ORIENTATION_SCREEN_ORIENTATION_DISPATCHER_HOST_H_

#include "base/id_map.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents_observer.h"
#include "third_party/WebKit/public/platform/WebLockOrientationError.h"
#include "third_party/WebKit/public/platform/WebScreenOrientationLockType.h"
#include "third_party/WebKit/public/platform/WebScreenOrientationType.h"

namespace content {

class RenderFrameHost;
class ScreenOrientationProvider;
class WebContents;

// ScreenOrientationDispatcherHost receives lock and unlock requests from the
// RenderFrames and dispatch them to the ScreenOrientationProvider. It also
// make sure that the right RenderFrame get replied for each lock request.
class CONTENT_EXPORT ScreenOrientationDispatcherHost
    : public WebContentsObserver {
 public:
  explicit ScreenOrientationDispatcherHost(WebContents* web_contents);
  virtual ~ScreenOrientationDispatcherHost();

  // WebContentsObserver
  virtual bool OnMessageReceived(const IPC::Message&,
                                 RenderFrameHost* render_frame_host) OVERRIDE;

  // Notifies that the lock with the given |request_id| has succeeded.
  // The renderer process will be notified that the lock succeeded only if
  // |request_id| matches the |current_lock_|.
  void NotifyLockSuccess(int request_id);

  // Notifies that the lock with the given |request_id| has failed.
  // The renderer process will be notified that the lock succeeded only if
  // |request_id| matches the |current_lock_|.
  void NotifyLockError(int request_id, blink::WebLockOrientationError error);

  void OnOrientationChange();

  void SetProvider(ScreenOrientationProvider* provider);

 private:
  void OnLockRequest(RenderFrameHost* render_frame_host,
                     blink::WebScreenOrientationLockType orientation,
                     int request_id);
  void OnUnlockRequest(RenderFrameHost* render_frame_host);

  // Returns a RenderFrameHost if the request_id is still valid and the
  // associated RenderFrameHost still exists. Returns NULL otherwise.
  RenderFrameHost* GetRenderFrameHostForRequestID(int request_id);

  void ResetCurrentLock();
  void NotifyLockError(int request_id,
                       RenderFrameHost* render_frame_host,
                       blink::WebLockOrientationError error);

  scoped_ptr<ScreenOrientationProvider> provider_;

  struct LockInformation {
    LockInformation(int request_id, int process_id, int routing_id);
    int request_id;
    int process_id;
    int routing_id;
  };
  // current_lock_ will be NULL if there are no current lock.
  LockInformation* current_lock_;

  DISALLOW_COPY_AND_ASSIGN(ScreenOrientationDispatcherHost);
};

}  // namespace content

#endif // CONTENT_BROWSER_SCREEN_ORIENTATION_SCREEN_ORIENTATION_DISPATCHER_HOST_H_
