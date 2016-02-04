// Copyright (C) 2013 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLObject_h
#define WebCLObject_h

#include "core/webcl/WebCLException.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/WeakPtr.h>

namespace blink {
class ExceptionState;
class WebCLContext;

// WebCLObject is the base class of WebCommandQueue, WebCLProgram, WebCLKernel,
// WebCLMemoryObject/WebCLBuffer/WebCLImage, WebCLEvent/WebCLUserEvent, WebCLSampler.
// They are owned by WebCLContext through weakptr for lifecycle tracking.
class WebCLObject : public RefCounted<WebCLObject> {
public:
    virtual ~WebCLObject();
    WeakPtr<WebCLObject> createWeakPtr() { return m_weakFactory.createWeakPtr(); }
    PassRefPtr<WebCLContext> context();

    void setContext(PassRefPtr<WebCLContext> context);
    virtual void release() { ASSERT_NOT_REACHED(); }

protected:
    explicit WebCLObject(PassRefPtr<WebCLContext> context);
    // Some object isn't associated with WebContext in constructor by developer.
    // but at runtime by OpenCL. Such as: WebCLEvent
    WebCLObject();

    WeakPtrFactory<WebCLObject> m_weakFactory;
    RefPtr<WebCLContext> m_context;
};

} // namespace blink

#endif // WebCLObject_h
