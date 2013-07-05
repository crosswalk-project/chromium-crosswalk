// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EFL_WEBVIEW_LIB_CONTENT_MAIN_DELEGATE_XWALK_H_
#define EFL_WEBVIEW_LIB_CONTENT_MAIN_DELEGATE_XWALK_H_

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "content/public/app/content_main_delegate.h"
#include "efl_webview/lib/content_browser_client_xwalk.h"
#include "efl_webview/lib/content_client_xwalk.h"

namespace xwalk {

class ContentMainDelegateXWalk : public content::ContentMainDelegate {
 public:
  ContentMainDelegateXWalk();

  virtual void PreSandboxStartup() OVERRIDE;
  virtual bool BasicStartupComplete(int* exit_code) OVERRIDE;
  virtual content::ContentBrowserClient* CreateContentBrowserClient() OVERRIDE;

 private:
  scoped_ptr<ContentBrowserClientXWalk> browser_client_;
  ContentClientXWalk content_client_;

  DISALLOW_COPY_AND_ASSIGN(ContentMainDelegateXWalk);
};

}  // namespace xwalk

#endif  // EFL_WEBVIEW_LIB_CONTENT_MAIN_DELEGATE_XWALK_H_
