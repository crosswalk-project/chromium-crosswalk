// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLKernel_h
#define WebCLKernel_h

#include "modules/webcl/WebCLKernelArgInfoProvider.h"
#include "modules/webcl/WebCLProgram.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace blink {

class DOMArrayBufferView;
class ExceptionState;
class WebCL;
class WebCLCommandQueue;
class WebCLDevice;
class WebCLKernelArgInfo;
class WebCLMemoryObject;
class WebCLPlatform;
class WebCLSampler;

class WebCLKernel : public WebCLObject, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    ~WebCLKernel() override;
    static PassRefPtr<WebCLKernel> create(cl_kernel, PassRefPtr<WebCLContext>, WebCLProgram*, const String&);

    ScriptValue getInfo(ScriptState*, int, ExceptionState&);
    ScriptValue getWorkGroupInfo(ScriptState*, WebCLDevice*, int, ExceptionState&);
    WebCLKernelArgInfo* getArgInfo(unsigned index, ExceptionState&);
    void setArg(unsigned, const ScriptValue&, ExceptionState&);
    void setArg(unsigned, WebCLMemoryObject*, ExceptionState&);
    void setArg(unsigned, WebCLSampler*, ExceptionState&);
    void setArg(unsigned, DOMArrayBufferView*, ExceptionState&);
    void setArg(unsigned, size_t, const void*, ExceptionState&);
    void release() override;

    unsigned numberOfArguments();
    unsigned associatedArguments();
    WebCLProgram* program() const { return m_program; }
    const String& kernelName() const { return m_kernelName; }
    const Vector<unsigned>& requiredArguments() { return m_argumentInfoProvider.requiredArguments(); }
    cl_kernel getKernel() const { return m_clKernel; }

private:
    WebCLKernel(cl_kernel, PassRefPtr<WebCLContext>, WebCLProgram*, const String&);
    bool isReleased() const { return !m_clKernel; }

    WebCLProgram* m_program;
    String m_kernelName;
    WebCLKernelArgInfoProvider m_argumentInfoProvider;
    cl_kernel m_clKernel;
};

} // namespace blink

#endif // WebCLKernel_h
