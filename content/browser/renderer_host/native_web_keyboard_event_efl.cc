// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/native_web_keyboard_event.h"

#include "third_party/WebKit/Source/WebKit/chromium/public/gtk/WebInputEventFactory.h"

namespace content {

NativeWebKeyboardEvent::NativeWebKeyboardEvent()
{
}

NativeWebKeyboardEvent::NativeWebKeyboardEvent(gfx::NativeEvent native_event)
{
}

NativeWebKeyboardEvent::NativeWebKeyboardEvent(const NativeWebKeyboardEvent&)
{
}

NativeWebKeyboardEvent& NativeWebKeyboardEvent::operator=(
    const NativeWebKeyboardEvent& other) {
  return *this;
}

NativeWebKeyboardEvent::~NativeWebKeyboardEvent() {
}

}  // namespace content
