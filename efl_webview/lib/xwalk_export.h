// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EFL_WEBVIEW_LIB_XWALK_EXPORT_H_
#define EFL_WEBVIEW_LIB_XWALK_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(XWALK_IMPLEMENTATION)
#define XWALK_EXPORT __declspec(dllexport)
#else
#define XWALK_EXPORT __declspec(dllimport)
#endif  // defined(XWALK_IMPLEMENTATION)

#else // defined(WIN32)
#if defined(XWALK_IMPLEMENTATION)
#define XWALK_EXPORT __attribute__((visibility("default")))
#else
#define XWALK_EXPORT
#endif
#endif

#else // defined(COMPONENT_BUILD)
#define XWALK_EXPORT
#endif

#endif  // EFL_WEBVIEW_LIB_XWALK_EXPORT_H_
