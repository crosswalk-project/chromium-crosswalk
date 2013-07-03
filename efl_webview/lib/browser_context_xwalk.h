// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EFL_WEBVIEW_LIBBROWSER_CONTEXT_XWALK_H_
#define EFL_WEBVIEW_LIBBROWSER_CONTEXT_XWALK_H_

#include "content/shell/shell_browser_context.h"

namespace xwalk {

class BrowserContextXWalk : public content::ShellBrowserContext {
 public:
  BrowserContextXWalk()
      : content::ShellBrowserContext(false)
  { }
  virtual ~BrowserContextXWalk() {}

  net::URLRequestContextGetter* CreateRequestContext(
      content::ProtocolHandlerMap* protocol_handlers) {
    return content::ShellBrowserContext::CreateRequestContext(
        protocol_handlers);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(BrowserContextXWalk);
};

}  // namespace xwalk

#endif  // EFL_WEBVIEW_LIBBROWSER_CONTEXT_XWALK_H_
