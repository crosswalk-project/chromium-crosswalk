// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#if ENABLE(WEBCL)
#include "bindings/modules/v8/V8WebCLContext.h"
#include "core/webcl/WebCLException.h"
#include "modules/webcl/WebCL.h"
#include "modules/webcl/WebCLOpenCL.h"
#include "modules/webcl/WebCLSampler.h"

namespace blink {

WebCLSampler::~WebCLSampler()
{
    release();
    ASSERT(!m_clSampler);
}

PassRefPtr<WebCLSampler> WebCLSampler::create(cl_sampler sampler, bool normCoords, unsigned addressingMode, unsigned filterMode, PassRefPtr<WebCLContext> context)
{
    return adoptRef(new WebCLSampler(sampler, normCoords, addressingMode, filterMode, context));
}

ScriptValue WebCLSampler::getInfo(ScriptState* scriptState, cl_sampler_info paramName, ExceptionState& es)
{
    v8::Handle<v8::Object> creationContext = scriptState->context()->Global();
    v8::Isolate* isolate = scriptState->isolate();

    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_SAMPLER, WebCLException::invalidSamplerMessage);
        return ScriptValue(scriptState, v8::Null(isolate));
    }

    switch (paramName) {
    case CL_SAMPLER_NORMALIZED_COORDS:
        return ScriptValue(scriptState,  v8::Boolean::New(isolate, m_normCoords));
        break;
    case CL_SAMPLER_CONTEXT:
        return ScriptValue(scriptState, toV8(context(), creationContext, isolate));
        break;
    case CL_SAMPLER_ADDRESSING_MODE:
        return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(m_addressingMode)));
        break;
    case CL_SAMPLER_FILTER_MODE:
        return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(m_filterMode)));
        break;
    default:
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return ScriptValue(scriptState, v8::Null(isolate));
    }

    return ScriptValue(scriptState, v8::Null(isolate));
}

void WebCLSampler::release()
{
    if (isReleased())
        return;

    cl_int err = clReleaseSampler(m_clSampler);
    if (err != CL_SUCCESS)
        ASSERT_NOT_REACHED();

    m_clSampler = 0;
}

WebCLSampler::WebCLSampler(cl_sampler sampler, bool normCoords, unsigned addressingMode, unsigned filterMode, PassRefPtr<WebCLContext> context)
    : WebCLObject(context)
    , m_normCoords(normCoords)
    , m_addressingMode(addressingMode)
    , m_filterMode(filterMode)
    , m_clSampler(sampler)
{
}

} // namespace blink

#endif // ENABLE(WEBCL)
