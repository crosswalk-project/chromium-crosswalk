// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLCallback_h
#define WebCLCallback_h

#include "platform/heap/Handle.h"
#include <wtf/RefCounted.h>

namespace blink {
class WebCLCallback : public RefCounted<WebCLCallback> {
public:
    virtual ~WebCLCallback() { }
    DEFINE_INLINE_VIRTUAL_TRACE() { }
    virtual bool handleEvent() = 0;
};

} // namespace blink

#endif // WebCLCallback_h
