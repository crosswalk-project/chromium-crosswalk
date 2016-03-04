// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SCREEN_ORIENTATION_SCREEN_ORIENTATION_DELEGATE_WIN_H_
#define CONTENT_BROWSER_SCREEN_ORIENTATION_SCREEN_ORIENTATION_DELEGATE_WIN_H_

#include "content/public/browser/screen_orientation_delegate.h"

namespace content {

class ScreenOrientationDelegateWin
    : public content::ScreenOrientationDelegate {
 public:
  ScreenOrientationDelegateWin();
  ~ScreenOrientationDelegateWin() override;

 private:
  // content::ScreenOrientationDelegate:
  bool FullScreenRequired(content::WebContents* web_contents) override;
  void Lock(content::WebContents* web_contents,
            blink::WebScreenOrientationLockType lock_orientation) override;
  bool ScreenOrientationProviderSupported() override;
  void Unlock(content::WebContents* web_contents) override;

  DISALLOW_COPY_AND_ASSIGN(ScreenOrientationDelegateWin);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SCREEN_ORIENTATION_SCREEN_ORIENTATION_DELEGATE_WIN_H_
