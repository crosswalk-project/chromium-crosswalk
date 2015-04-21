// Copyright (C) 2011, 2012, 2013 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLBuffer_h
#define WebCLBuffer_h

#if ENABLE(WEBCL)

#include "modules/webcl/WebCLConfig.h"
#include "modules/webcl/WebCLMemoryObject.h"

namespace blink {

class WebCL;
class WebCLContext;

class WebCLBuffer : public WebCLMemoryObject {
    DEFINE_WRAPPERTYPEINFO();
public:
    ~WebCLBuffer() override;
    static PassRefPtr<WebCLBuffer> create(PassRefPtr<WebCLContext>, unsigned, unsigned, void*, ExceptionState&);
    PassRefPtr<WebCLBuffer> createSubBuffer(unsigned, unsigned, unsigned, ExceptionState&);

    int type() override { return BUFFER; }

private:
    WebCLBuffer(cl_mem, PassRefPtr<WebCLContext>, unsigned, unsigned, WebCLBuffer* parentBuffer = nullptr);

    unsigned m_memoryFlags;
};

} // namespace blink

#endif // ENABLE(WEBCL)
#endif // WebCLBuffer_h
