// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLMemoryObject_h
#define WebCLMemoryObject_h

#if ENABLE(WEBCL)
#include "modules/webcl/WebCLConfig.h"
#include "modules/webcl/WebCLObject.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace blink {

class ExceptionState;
class WebCL;
class WebCLContext;

class WebCLMemoryObject : public WebCLObject, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    enum {
        MEMORY,
        BUFFER,
        IMAGE,
        UNKNOWN
    };

    ~WebCLMemoryObject() override;
    static PassRefPtr<WebCLMemoryObject> create(cl_mem, unsigned, PassRefPtr<WebCLContext>);

    ScriptValue getInfo(ScriptState*, int, ExceptionState&);
    void release() override;

    size_t sizeInBytes() const { return m_sizeInBytes; }
    virtual int type() { return MEMORY; }
    cl_mem getMem() const { return m_clMem; }
    bool isReleased() const { return !m_clMem; }

protected:
    WebCLMemoryObject(cl_mem, unsigned, PassRefPtr<WebCLContext>, WebCLMemoryObject* parentBuffer = nullptr);

    WebCLMemoryObject* m_parentMemObject;
    size_t m_sizeInBytes;
    cl_mem m_clMem;
};

} // namespace blink

#endif // ENABLE(WEBCL)
#endif // WebCLMemoryObject_h
