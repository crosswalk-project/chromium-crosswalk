// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCL_h
#define WebCL_h

#include "bindings/core/v8/ScriptWrappable.h"
#include "modules/webcl/WebCLExtension.h"
#include "modules/webcl/WebCLOpenCL.h"
#include "platform/heap/Handle.h"
#include "wtf/HashSet.h"
#include "wtf/PassOwnPtr.h"
#include "wtf/PassRefPtr.h"
#include "wtf/RefCounted.h"
#include "wtf/RefPtr.h"
#include "wtf/Vector.h"
#include "wtf/WeakPtr.h"
#include "wtf/text/StringHash.h"
#include "wtf/text/WTFString.h"
#include <utility>

namespace blink {

typedef unsigned CLenum;
class ExceptionState;
class WebCLCallback;
class WebCLContext;
class WebCLDevice;
class WebCLEvent;
class WebCLHolder;
class WebCLObject;
class WebCLPlatform;

class WebCL final : public RefCounted<WebCL>, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    ~WebCL();
    static PassRefPtr<WebCL> create();

    Vector<RefPtr<WebCLPlatform>> getPlatforms(ExceptionState&);
    PassRefPtr<WebCLContext> createContext(ExceptionState&);
    PassRefPtr<WebCLContext> createContext(unsigned, ExceptionState&);
    PassRefPtr<WebCLContext> createContext(WebCLPlatform*, ExceptionState&);
    PassRefPtr<WebCLContext> createContext(WebCLPlatform*, unsigned, ExceptionState&);
    PassRefPtr<WebCLContext> createContext(WebCLDevice*, ExceptionState&);
    PassRefPtr<WebCLContext> createContext(const Vector<RefPtr<WebCLDevice>>&, ExceptionState&);
    void waitForEvents(const Vector<RefPtr<WebCLEvent>>&, WebCLCallback*, ExceptionState&);
    void releaseAll();
    void trackReleaseableWebCLContext(WeakPtr<WebCLContext>);
    bool enableExtension(const String& name);
    Vector<String> getSupportedExtensions();
    void getEnabledExtensions(HashSet<String>& extensions);

private:
    WebCL();
    static void CL_CALLBACK callbackProxy(cl_event, cl_int, void*);
    static void callbackProxyOnMainThread(PassOwnPtr<WebCLHolder>);
    void waitForEventsImpl(const Vector<RefPtr<WebCLEvent>>&, WebCLCallback*);
    void cachePlatforms();
    void cacheSupportedExtensions();
    // Update the m_callbackRegisterQueue according to OpenCL event, and
    // get the WebCLCallback list if OpenCL event becomes CL_COMPLETE.
    HeapVector<Member<WebCLCallback>> updateCallbacksFromCLEvent(cl_event);

    Vector<RefPtr<WebCLPlatform>> m_platforms;
    WebCLExtension m_extension;
    Vector<WeakPtr<WebCLContext>> m_webCLContexts;

    typedef Vector<std::pair<Vector<WeakPtr<WebCLObject>>, Persistent<WebCLCallback>>> WebCLCallbackRegisterQueue;
    // It contains every un-triggered WebCLCallback and corresponding WebCLEvent list.
    WebCLCallbackRegisterQueue m_callbackRegisterQueue;
    WeakPtrFactory<WebCL> m_weakFactory;
};

} // namespace blink

#endif // WebCL_h
