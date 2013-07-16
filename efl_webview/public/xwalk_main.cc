// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "efl_webview/public/xwalk_main.h"
#include "efl_webview/lib/webview.h"

void xwalk_init(int argc, char *argv[])
{
  xwalk::WebView::CommandLineInit(argc, argv);
}
