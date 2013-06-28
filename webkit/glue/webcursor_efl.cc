// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/glue/webcursor.h"

#include "base/logging.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebCursorInfo.h"

using WebKit::WebCursorInfo;

gfx::NativeCursor WebCursor::GetNativeCursor() {
  return 0;
}

void WebCursor::InitPlatformData() {
}

bool WebCursor::SerializePlatformData(Pickle* pickle) const {
  return true;
}

bool WebCursor::DeserializePlatformData(PickleIterator* iter) {
  return true;
}

bool WebCursor::IsPlatformDataEqual(const WebCursor& other) const {
  return true;
}

void WebCursor::CleanupPlatformData() {
}

void WebCursor::CopyPlatformData(const WebCursor& other) {
}
