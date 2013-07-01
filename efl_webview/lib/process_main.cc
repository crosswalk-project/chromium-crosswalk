// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "efl_webview/lib/process_main.h"

#include "base/command_line.h"
#include "content/public/app/content_main.h"
#include "content/public/common/content_switches.h"

namespace xwalk {

int process_main(int argc, char** argv) {
  CommandLine::Init(argc, argv);
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  std::string process_type =
      command_line.GetSwitchValueASCII(switches::kProcessType);
  if (process_type == "")
    return 1;

  return content::ContentMain(argc, const_cast<const char**>(argv), 0);
}

}  // namespace xwalk
