// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLEvent_h
#define WebCLEvent_h

#include "modules/webcl/WebCLCallback.h"
#include "modules/webcl/WebCLConfig.h"
#include "modules/webcl/WebCLObject.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace blink {

class WebCL;
class WebCLCommandQueue;
class WebCLEventHolder;
class ExceptionState;

class WebCLEvent : public WebCLObject, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    ~WebCLEvent() override;
    static PassRefPtr<WebCLEvent> create();

    virtual ScriptValue getInfo(ScriptState*, unsigned, ExceptionState&);
    unsigned getProfilingInfo(int, ExceptionState&);
    void setCallback(unsigned, WebCLCallback*, ExceptionState&);
    void release() override;

    virtual bool isUserEvent() const { return false; }
    virtual int getStatus();
    bool setAssociatedCommandQueue(WebCLCommandQueue* commandQueue);
    cl_event* getEventPtr() { return &m_clEvent; }
    cl_event getEvent() { return m_clEvent; }
    bool isReleased() const { return !m_clEvent; }

protected:
    WebCLEvent(cl_event);

    static void callbackProxy(cl_event, cl_int, void*);
    static void callbackProxyOnMainThread(PassOwnPtr<WebCLEventHolder>);

    Vector<RefPtr<WebCLCallback>> m_callbacks;
    WebCLCommandQueue* m_commandQueue;
    cl_event m_clEvent;
};

} // namespace blink

#endif // WebCLEvent_h
