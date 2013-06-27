// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_WINDOW_UTILS_EFL_H_
#define CONTENT_BROWSER_RENDERER_HOST_WINDOW_UTILS_EFL_H_

#include "content/common/content_export.h"

namespace WebKit {
struct WebScreenInfo;
}

namespace content {

CONTENT_EXPORT void GetScreenInfoEfl(WebKit::WebScreenInfo* results);

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_WINDOW_UTILS_EFL_H_
