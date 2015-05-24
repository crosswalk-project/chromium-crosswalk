// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#if ENABLE(WEBCL)
#include "bindings/modules/v8/V8WebCLCommandQueue.h"
#include "bindings/modules/v8/V8WebCLContext.h"
#include "core/webcl/WebCLException.h"
#include "modules/webcl/WebCL.h"
#include "modules/webcl/WebCLEvent.h"
#include "modules/webcl/WebCLOpenCL.h"
#include "platform/ThreadSafeFunctional.h"
#include "public/platform/Platform.h"
#include "public/platform/WebTraceLocation.h"

namespace blink {

// The holder of WebCLEvent.
class WebCLEventHolder {
public:
    WeakPtr<WebCLObject> event;
    cl_int type;
    cl_event event2;
    cl_int type2;
};

WebCLEvent::~WebCLEvent()
{
    release();
    ASSERT(!m_clEvent);
}

PassRefPtr<WebCLEvent> WebCLEvent::create()
{
    return adoptRef(new WebCLEvent(0));
}

ScriptValue WebCLEvent::getInfo(ScriptState* scriptState, unsigned paramName, ExceptionState& es)
{
    v8::Handle<v8::Object> creationContext = scriptState->context()->Global();
    v8::Isolate* isolate = scriptState->isolate();

    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_EVENT, WebCLException::invalidEventMessage);
        return ScriptValue(scriptState, v8::Null(isolate));
    }

    cl_int err = CL_SUCCESS;
    cl_int intUnits = 0;
    cl_command_type commandType = 0;
    switch(paramName) {
    case CL_EVENT_COMMAND_EXECUTION_STATUS:
        err = clGetEventInfo(m_clEvent, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(cl_int), &intUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::New(isolate, static_cast<int>(intUnits)));
        break;
    case CL_EVENT_COMMAND_TYPE:
        err = clGetEventInfo(m_clEvent, CL_EVENT_COMMAND_TYPE, sizeof(cl_command_type), &commandType, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(commandType)));
        break;
    case CL_EVENT_CONTEXT:
        ASSERT(!isUserEvent());
        return ScriptValue(scriptState, toV8(context(), creationContext, isolate));
    case CL_EVENT_COMMAND_QUEUE:
        ASSERT(m_commandQueue);
        ASSERT(!isUserEvent());
        return ScriptValue(scriptState, toV8(m_commandQueue, creationContext, isolate));
    default:
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return ScriptValue(scriptState, v8::Null(isolate));
    }
    WebCLException::throwException(err, es);
    return ScriptValue(scriptState, v8::Null(isolate));
}

int WebCLEvent::getStatus()
{
    cl_int intUnits = 0;
    cl_int err = clGetEventInfo(m_clEvent, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(cl_int), &intUnits, nullptr);
    if (err == CL_SUCCESS)
        return static_cast<int>(intUnits);
    return CL_INVALID_VALUE;
}

unsigned WebCLEvent::getProfilingInfo(int paramName, ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_EVENT, WebCLException::invalidEventMessage);
        return 0;
    }

    int status = getStatus();
    unsigned properties = m_commandQueue ? m_commandQueue->getProperties() : 0;
    if (isUserEvent() || status != CL_COMPLETE || !(properties & CL_QUEUE_PROFILING_ENABLE)) {
        es.throwWebCLException(WebCLException::PROFILING_INFO_NOT_AVAILABLE, WebCLException::profilingInfoNotAvailableMessage);
        return 0;
    }

    cl_int err = CL_SUCCESS;
    cl_ulong ulongUnits = 0;
    switch(paramName) {
    case CL_PROFILING_COMMAND_QUEUED:
        err = clGetEventProfilingInfo(m_clEvent, CL_PROFILING_COMMAND_QUEUED, sizeof(cl_ulong), &ulongUnits, nullptr);
        if (err == CL_SUCCESS)
            return static_cast<unsigned long long>(ulongUnits);
        break;
    case CL_PROFILING_COMMAND_SUBMIT:
        err = clGetEventProfilingInfo(m_clEvent, CL_PROFILING_COMMAND_SUBMIT, sizeof(cl_ulong), &ulongUnits, nullptr);
        if (err == CL_SUCCESS)
            return static_cast<unsigned long long>(ulongUnits);
        break;
    case CL_PROFILING_COMMAND_START:
        err = clGetEventProfilingInfo(m_clEvent, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &ulongUnits, nullptr);
        if (err == CL_SUCCESS)
            return static_cast<unsigned long long>(ulongUnits);
        break;
    case CL_PROFILING_COMMAND_END:
        err = clGetEventProfilingInfo(m_clEvent, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &ulongUnits, nullptr);
        if (err == CL_SUCCESS)
            return static_cast<unsigned long long>(ulongUnits);
        break;
    default:
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return 0;
    }

    WebCLException::throwException(err, es);
    return 0;
}

void WebCLEvent::setCallback(unsigned commandExecCallbackType, WebCLCallback* callback, ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_EVENT, WebCLException::invalidEventMessage);
        return;
    }

    if (commandExecCallbackType != CL_COMPLETE) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    ASSERT(callback);
    if (m_callbacks.size()) {
        m_callbacks.append(adoptRef(callback));
        return;
    }

    m_callbacks.clear();
    m_callbacks.append(adoptRef(callback));
    WebCLEventHolder* holder = new WebCLEventHolder;
    holder->event = createWeakPtr();
    holder->type = commandExecCallbackType;
    cl_int err = clSetEventCallback(m_clEvent, commandExecCallbackType, &callbackProxy, holder);

    if (err != CL_SUCCESS)
        WebCLException::throwException(err, es);
}

void WebCLEvent::release()
{
    if (isReleased())
        return;

    cl_int err = clReleaseEvent(m_clEvent);
    if (err != CL_SUCCESS)
        ASSERT_NOT_REACHED();

    m_clEvent = 0;

    // Release un-triggered callbacks.
    m_callbacks.clear();
}

bool WebCLEvent::setAssociatedCommandQueue(WebCLCommandQueue* commandQueue)
{
    if (m_commandQueue)
        return false;

    m_commandQueue = commandQueue;
    setContext(m_commandQueue->context());
    return true;
}

WebCLEvent::WebCLEvent(cl_event clEvent)
    : WebCLObject()
    , m_commandQueue(nullptr)
    , m_clEvent(clEvent)
{
}

void WebCLEvent::callbackProxy(cl_event event, cl_int type, void* userData)
{
    OwnPtr<WebCLEventHolder> holder = adoptPtr(static_cast<WebCLEventHolder*>(userData));
    holder->event2 = event;
    holder->type2 = type;

    if (!isMainThread()) {
        Platform::current()->mainThread()->taskRunner()->postTask(
            BLINK_FROM_HERE, threadSafeBind(&WebCLEvent::callbackProxyOnMainThread, holder.release()));
        return;
    }

    callbackProxyOnMainThread(holder.release());
}

void WebCLEvent::callbackProxyOnMainThread(PassOwnPtr<WebCLEventHolder> holder)
{
    ASSERT(isMainThread());
    RefPtr<WebCLEvent> webEvent(static_cast<WebCLEvent*>(holder->event.get()));
#ifndef NDEBUG
    cl_event event = holder->event2;
#endif
    cl_int type = holder->type2;

    if (!webEvent)
        return;

    // Ignore the callback if the WebCLEvent is released or OpenCL event is abnormally terminated.
    if (webEvent->isReleased() || type != holder->type) {
        webEvent->m_callbacks.clear();
        return;
    }

    ASSERT(event == webEvent->getEvent());
    Vector<RefPtr<WebCLCallback>> callbacks = webEvent->m_callbacks;
    ASSERT(callbacks.size());
    for (auto callback : callbacks)
        callback->handleEvent();

    webEvent->m_callbacks.clear();
}

} // namespace blink

#endif // ENABLE(WEBCL)
