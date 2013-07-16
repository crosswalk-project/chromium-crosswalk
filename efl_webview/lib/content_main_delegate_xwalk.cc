// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "efl_webview/lib/content_main_delegate_xwalk.h"

#include "efl_webview/lib/content_browser_client_xwalk.h"
#include "ui/base/resource/resource_bundle.h"

namespace xwalk {

ContentMainDelegateXWalk::ContentMainDelegateXWalk() { }

void ContentMainDelegateXWalk::PreSandboxStartup() {
  ui::ResourceBundle::InitSharedInstanceWithLocale("en-US", NULL);
}

bool ContentMainDelegateXWalk::BasicStartupComplete(int* exit_code) {
  content::SetContentClient(&content_client_);
  return false;
}

content::ContentBrowserClient*
    ContentMainDelegateXWalk::CreateContentBrowserClient() {
  browser_client_.reset(new ContentBrowserClientXWalk);
  return browser_client_.get();
}

}  // namespace xwalk
