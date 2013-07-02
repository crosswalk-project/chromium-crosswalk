// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "efl_webview/lib/web_runtime_context.h"

#include "base/command_line.h"
#include "base/base_paths.h"
#include "base/path_service.h"
#include "content/public/app/content_main_delegate.h"
#include "content/public/app/content_main_runner.h"
#include "content/public/browser/browser_main_runner.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/main_function_params.h"
#include "efl_webview/lib/browser_context_xwalk.h"
#include "efl_webview/lib/content_browser_client_xwalk.h"

namespace xwalk {

namespace {

class ContentMainDelegateXWalk : public content::ContentMainDelegate {
 public:
  ContentMainDelegateXWalk() { }

  content::ContentBrowserClient* CreateContentBrowserClient()   {
    browser_client_.reset(new ContentBrowserClientXWalk);
    return browser_client_.get();
  }

 private:
  scoped_ptr<ContentBrowserClientXWalk> browser_client_;

  DISALLOW_COPY_AND_ASSIGN(ContentMainDelegateXWalk);
};

WebRuntimeContext* g_context = 0;
// TODO: it should be passed via build system.
const char g_sub_process_name[] = "efl_process";

void SubprocessPathInit() {
  base::FilePath current_directory;
  CHECK(PathService::Get(base::FILE_EXE, &current_directory));
  current_directory = current_directory.DirName();

  // TODO: use more elegant way.
  base::FilePath subprocess_path(current_directory.value() + "/" + g_sub_process_name);

  CommandLine::ForCurrentProcess()->AppendSwitchPath(switches::kBrowserSubprocessPath, subprocess_path);
}

} // namespace

WebRuntimeContext::WebRuntimeContext() {
  DCHECK(!g_context);
  g_context = this;

  static content::ContentMainRunner *runner = 0;
  if (!runner) {
    runner = content::ContentMainRunner::Create();
    runner->Initialize(0, 0, new ContentMainDelegateXWalk);
  }

  SubprocessPathInit();

  static content::BrowserMainRunner *browserRunner = 0;
  if (!browserRunner) {
    browserRunner = content::BrowserMainRunner::Create();
    browserRunner->Initialize(content::MainFunctionParams(*CommandLine::ForCurrentProcess()));
  }

  base::ThreadRestrictions::SetIOAllowed(true);

  // Once the MessageLoop has been created, attach a top-level RunLoop.
  run_loop_.reset(new base::RunLoop);
  run_loop_->BeforeRun();

  browser_context_.reset(new BrowserContextXWalk);
}

WebRuntimeContext::~WebRuntimeContext() {
  run_loop_->AfterRun();

  DCHECK(g_context == this);
  g_context = 0;
}

scoped_refptr<WebRuntimeContext> WebRuntimeContext::current() {
  scoped_refptr<WebRuntimeContext> current = g_context;
  if (!current)
    current = new WebRuntimeContext;
  DCHECK(g_context == current);
  return current;
}

content::BrowserContext* WebRuntimeContext::BrowserContext() {
  return browser_context_.get();
}

}  // namespace xwalk
