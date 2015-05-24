// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#if ENABLE(WEBCL)
#include "bindings/modules/v8/V8WebCLContext.h"
#include "bindings/modules/v8/V8WebCLMemoryObject.h"
#include "core/webcl/WebCLException.h"
#include "modules/webcl/WebCL.h"
#include "modules/webcl/WebCLMemoryObject.h"
#include "modules/webcl/WebCLOpenCL.h"

namespace blink {

WebCLMemoryObject::~WebCLMemoryObject()
{
    release();
    ASSERT(!m_clMem);
}

PassRefPtr<WebCLMemoryObject> WebCLMemoryObject::create(cl_mem mem, unsigned sizeInBytes, PassRefPtr<WebCLContext> context)
{
    return adoptRef(new WebCLMemoryObject(mem, sizeInBytes, context));
}

ScriptValue WebCLMemoryObject::getInfo(ScriptState* scriptState, int paramName, ExceptionState& es)
{
    v8::Handle<v8::Object> creationContext = scriptState->context()->Global();
    v8::Isolate* isolate = scriptState->isolate();

    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
        return ScriptValue(scriptState, v8::Null(isolate));
    }

    cl_int err = CL_SUCCESS;
    size_t sizetUnits = 0;
    size_t memorySizeValue = 0;
    cl_mem_object_type memType = 0;
    cl_mem_flags memFlags = 0;
    unsigned memCopyHostPtrMask = 0x07;

    switch(paramName) {
    case CL_MEM_SIZE:
        err = clGetMemObjectInfo(m_clMem, CL_MEM_SIZE, sizeof(size_t), &sizetUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(sizetUnits)));
        break;
    case CL_MEM_FLAGS:
        err = clGetMemObjectInfo(m_clMem, CL_MEM_FLAGS, sizeof(cl_mem_flags), &memFlags, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(memFlags & memCopyHostPtrMask)));
        break;
    case CL_MEM_OFFSET:
        err = clGetMemObjectInfo(m_clMem, CL_MEM_OFFSET, sizeof(size_t), &memorySizeValue, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(memorySizeValue)));
        break;
    case CL_MEM_TYPE:
        err = clGetMemObjectInfo(m_clMem, CL_MEM_TYPE, sizeof(cl_mem_object_type), &memType, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(memType)));
        break;
    case CL_MEM_CONTEXT:
        return ScriptValue(scriptState, toV8(context(), creationContext, isolate));
    case CL_MEM_ASSOCIATED_MEMOBJECT:
        if (m_parentMemObject)
            return ScriptValue(scriptState, toV8(m_parentMemObject, creationContext, isolate));
        return ScriptValue(scriptState, v8::Null(isolate));
    default:
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return ScriptValue(scriptState, v8::Null(isolate));
    }

    WebCLException::throwException(err, es);
    return ScriptValue(scriptState, v8::Null(isolate));
}

void WebCLMemoryObject::release()
{
    if (isReleased())
        return;

    cl_int err = clReleaseMemObject(m_clMem);
    if (err != CL_SUCCESS)
        ASSERT_NOT_REACHED();

    m_clMem = 0;
}

WebCLMemoryObject::WebCLMemoryObject(cl_mem mem, unsigned sizeInBytes, PassRefPtr<WebCLContext> context, WebCLMemoryObject* parentBuffer)
    : WebCLObject(context)
    , m_parentMemObject(parentBuffer)
    , m_sizeInBytes(sizeInBytes)
    , m_clMem(mem)
{
}

} // namespace blink

#endif // ENABLE(WEBCL)
