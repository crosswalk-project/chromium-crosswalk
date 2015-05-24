// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLProgram_h
#define WebCLProgram_h

#if ENABLE(WEBCL)
#include "modules/webcl/WebCLConfig.h"
#include "modules/webcl/WebCLCallback.h"
#include "modules/webcl/WebCLObject.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Threading.h>

namespace blink {

class ExceptionState;
class WebCL;
class WebCLContext;
class WebCLDevice;
class WebCLKernel;
class WebCLProgramHolder;

class WebCLProgram : public WebCLObject, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    ~WebCLProgram() override;
    static PassRefPtr<WebCLProgram> create(cl_program, PassRefPtr<WebCLContext>, const String&);

    ScriptValue getInfo(ScriptState*, int, ExceptionState&);
    ScriptValue getBuildInfo(ScriptState* scriptState, WebCLDevice*, int, ExceptionState&);
    PassRefPtr<WebCLKernel> createKernel(const String&, ExceptionState&);
    Vector<RefPtr<WebCLKernel>> createKernelsInProgram(ExceptionState&);
    void build(const Vector<RefPtr<WebCLDevice>>&, const String&, WebCLCallback*, ExceptionState&);
    void build(const String&, WebCLCallback*, ExceptionState&);
    void release() override;

    const String& sourceWithCommentsStripped();
    const String& getProgramSource() const { return m_programSource; }

private:
    WebCLProgram(cl_program, PassRefPtr<WebCLContext>, const String&);
    bool isReleased() const { return !m_clProgram; }
    bool isExtensionEnabled(RefPtr<blink::WebCLContext>, const String&);
    typedef void (*pfnNotify)(cl_program, void*);
    static void callbackProxy(cl_program, void*);
    static void callbackProxyOnMainThread(PassOwnPtr<WebCLProgramHolder>);
    cl_program getProgram() { return m_clProgram; }

    RefPtr<WebCLCallback> m_buildCallback;
    String m_programSource;
    String m_programSourceWithCommentsStripped;
    bool m_isProgramBuilt;
    cl_program m_clProgram;
};

} // namespace blink

#endif // ENABLE(WEBCL)
#endif // WebCLProgram_h
