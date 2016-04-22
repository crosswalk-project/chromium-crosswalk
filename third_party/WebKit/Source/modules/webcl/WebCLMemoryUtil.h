// Copyright (C) 2014 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLMemoryUtil_h
#define WebCLMemoryUtil_h

#include "core/webcl/WebCLException.h"
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/WeakPtr.h>

namespace blink {

class WebCLBuffer;
class WebCLCommandQueue;
class WebCLContext;
class WebCLMemoryObject;
class WebCLKernel;
class WebCLObject;
class WebCLProgram;

// WebCLMemoryUtil is a helper class to intialize the OpenCL memory to 0.
// It leverages the OpenCL kernel function to do it.
class WebCLMemoryUtil {
public:
    explicit WebCLMemoryUtil(WebCLContext*);
    ~WebCLMemoryUtil();

    void bufferCreated(WebCLBuffer*, ExceptionState&);
    void commandQueueCreated(WebCLCommandQueue*, ExceptionState&);

private:
    void ensureMemory(WebCLMemoryObject*, WebCLCommandQueue*, ExceptionState&);
    void processPendingMemoryList(ExceptionState&);
    WebCLCommandQueue* validCommandQueue() const;
    void initializeOrQueueMemoryObject(WebCLBuffer*, ExceptionState&);

    WebCLContext* m_context;
    RefPtr<WebCLProgram> m_program;
    RefPtr<WebCLKernel> m_kernelChar;
    RefPtr<WebCLKernel> m_kernelChar16;

    Vector<WeakPtr<WebCLObject>> m_pendingBuffers;
    Vector<WeakPtr<WebCLObject>> m_queues;
};

} // namespace blink

#endif // WebCLMemoryUtil_h
