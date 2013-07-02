// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "efl_webview/lib/content_browser_client_xwalk.h"

#include "content/public/browser/browser_main_parts.h"
#include "content/public/common/main_function_params.h"
#include "efl_webview/lib/browser_context_xwalk.h"
#include "efl_webview/lib/web_runtime_context.h"

namespace xwalk {

class BrowserMainPartsXWalk : public content::BrowserMainParts
{
 public:
  BrowserMainPartsXWalk(const content::MainFunctionParams& parameters)
      : content::BrowserMainParts()
      , parameters_(parameters)
      , run_message_loop_(true)
  { }

  virtual void PreMainMessageLoopStart() OVERRIDE { }
  virtual void PostMainMessageLoopStart() OVERRIDE { }
  virtual void PreEarlyInitialization() OVERRIDE { }

  virtual void PreMainMessageLoopRun() OVERRIDE {
    if (parameters_.ui_task) {
      parameters_.ui_task->Run();
      delete parameters_.ui_task;
      run_message_loop_ = false;
    }
  }

  virtual bool MainMessageLoopRun(int* result_code) OVERRIDE {
    return !run_message_loop_;
  }

  virtual void PostMainMessageLoopRun() OVERRIDE { }

 private:
  const content::MainFunctionParams& parameters_;
  bool run_message_loop_;

  DISALLOW_COPY_AND_ASSIGN(BrowserMainPartsXWalk);
};

content::BrowserMainParts *ContentBrowserClientXWalk::CreateBrowserMainParts(
    const content::MainFunctionParams &parameters) {
  return new BrowserMainPartsXWalk(parameters);
}

net::URLRequestContextGetter* ContentBrowserClientXWalk::CreateRequestContext(
    content::BrowserContext* content_browser_context,
    content::ProtocolHandlerMap* protocol_handlers) {
  DCHECK(content_browser_context == WebRuntimeContext::current()->BrowserContext());
  return static_cast<BrowserContextXWalk*>(content_browser_context)->CreateRequestContext(protocol_handlers);
}

} // namespace xwalk
