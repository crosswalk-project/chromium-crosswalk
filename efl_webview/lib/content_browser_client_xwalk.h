// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EFL_WEBVIEW_LIBCONTENT_BROWSER_CLIENT_XWALK_H_
#define EFL_WEBVIEW_LIBCONTENT_BROWSER_CLIENT_XWALK_H_

#include "content/public/browser/content_browser_client.h"

namespace xwalk {

class ContentBrowserClientXWalk : public content::ContentBrowserClient {
 public:
  ContentBrowserClientXWalk() { }

  // ContentBrowserClient overrides.
  virtual content::BrowserMainParts* CreateBrowserMainParts(
      const content::MainFunctionParams& parameters) OVERRIDE;
  virtual net::URLRequestContextGetter* CreateRequestContext(
      content::BrowserContext* browser_context,
      content::ProtocolHandlerMap* protocol_handlers) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(ContentBrowserClientXWalk);
};

} // namespace xwalk

#endif // EFL_WEBVIEW_LIBCONTENT_BROWSER_CLIENT_XWALK_H_
