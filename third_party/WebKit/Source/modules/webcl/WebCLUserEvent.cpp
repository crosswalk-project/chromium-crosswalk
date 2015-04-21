// Copyright (C) 2013 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#if ENABLE(WEBCL)
#include "bindings/modules/v8/V8WebCLContext.h"
#include "modules/webcl/WebCL.h"
#include "modules/webcl/WebCLContext.h"
#include "modules/webcl/WebCLCommandQueue.h"
#include "modules/webcl/WebCLOpenCL.h"
#include "modules/webcl/WebCLUserEvent.h"

namespace blink {

WebCLUserEvent::~WebCLUserEvent()
{
}

PassRefPtr<WebCLUserEvent> WebCLUserEvent::create(PassRefPtr<WebCLContext> context, ExceptionState& es)
{
    cl_int userEventError = 0;
    cl_event userEvent = clCreateUserEvent(context->getContext(), &userEventError);
    if (userEventError != CL_SUCCESS) {
        WebCLException::throwException(userEventError, es);
        return nullptr;
    }

    return adoptRef(new WebCLUserEvent(userEvent, context));
}

void WebCLUserEvent::setStatus(cl_int executionStatus, ExceptionState& es)
{
    ASSERT(isUserEvent());
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_EVENT, WebCLException::invalidEventMessage);
        return;
    }

    if (!(executionStatus < 0 || executionStatus == CL_COMPLETE)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    if (m_eventStatusSituation == StatusSet) {
        es.throwWebCLException(WebCLException::INVALID_OPERATION, WebCLException::invalidOperationMessage);
        return;
    }

    m_eventStatusSituation = StatusSet;
    m_executionStatus = executionStatus;

    cl_int err = clSetUserEventStatus(m_clEvent, executionStatus);
    if (err != CL_SUCCESS)
        WebCLException::throwException(err, es);
}

ScriptValue WebCLUserEvent::getInfo(ScriptState* scriptState, unsigned name, ExceptionState& es)
{
    v8::Handle<v8::Object> creationContext = scriptState->context()->Global();
    v8::Isolate* isolate = scriptState->isolate();

    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_EVENT, WebCLException::invalidEventMessage);
        return ScriptValue(scriptState, v8::Null(isolate));
    }

    switch (name) {
    case CL_EVENT_CONTEXT:
        return ScriptValue(scriptState, toV8(context(), creationContext, isolate));
    case CL_EVENT_COMMAND_QUEUE:
        return ScriptValue(scriptState, v8::Null(isolate));
    }

    return WebCLEvent::getInfo(scriptState, name, es);
}

WebCLUserEvent::WebCLUserEvent(cl_event event, PassRefPtr<WebCLContext> context)
    : WebCLEvent(event)
    , m_eventStatusSituation(StatusUnset)
    , m_executionStatus(0)
{
    setContext(context);
}

} // namespace blink

#endif // ENABLE(WEBCL)
