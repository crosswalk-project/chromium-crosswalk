// Copyright (C) 2011, 2013 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/webcl/WebCLException.h"
#include "modules/webcl/WebCL.h"
#include "modules/webcl/WebCLBuffer.h"
#include "modules/webcl/WebCLContext.h"
#include "modules/webcl/WebCLMemoryObject.h"
#include "modules/webcl/WebCLOpenCL.h"

namespace blink {

namespace {

struct CLBufferRegion {
    unsigned origin;
    unsigned size;
};

} // namespace anonymous

WebCLBuffer::~WebCLBuffer()
{
}

PassRefPtr<WebCLBuffer> WebCLBuffer::create(PassRefPtr<WebCLContext> context, unsigned memoryFlags, unsigned sizeInBytes, void* data, ExceptionState& es)
{
    cl_context m_clContext = context->getContext();
    if (!m_clContext) {
        es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
        return nullptr;
    }

    cl_int err = CL_SUCCESS;
    cl_mem clMemObject = nullptr;
    switch (memoryFlags) {
    case CL_MEM_READ_ONLY:
        clMemObject = data ? clCreateBuffer(m_clContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeInBytes, data, &err) : clCreateBuffer(m_clContext, CL_MEM_READ_ONLY, sizeInBytes, nullptr, &err);
        break;
    case CL_MEM_WRITE_ONLY:
        clMemObject = data ? clCreateBuffer(m_clContext, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR, sizeInBytes, data, &err) : clCreateBuffer(m_clContext, CL_MEM_WRITE_ONLY, sizeInBytes, nullptr , &err);
        break;
    case CL_MEM_READ_WRITE:
        clMemObject = data ? clCreateBuffer(m_clContext, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeInBytes, data, &err) : clCreateBuffer(m_clContext, CL_MEM_READ_WRITE, sizeInBytes, nullptr, &err);
        break;
    default:
        es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
        break;
    }

    if (err != CL_SUCCESS) {
        WebCLException::throwException(err, es);
        return nullptr;
    }
    RefPtr<WebCLBuffer> buffer = adoptRef(new WebCLBuffer(clMemObject, context, memoryFlags, sizeInBytes));
    return buffer.release();
}

PassRefPtr<WebCLBuffer> WebCLBuffer::createSubBuffer(unsigned memoryFlags, unsigned origin, unsigned size, ExceptionState& es)
{
    CLBufferRegion bufferCreateInfo = {origin, size};

    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
        return nullptr;
    }

    if (m_parentMemObject) {
        es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
        return nullptr;
    }

    if (m_memoryFlags != CL_MEM_READ_WRITE && m_memoryFlags != memoryFlags) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return nullptr;
    }

    if (origin > sizeInBytes() || size > sizeInBytes()) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return nullptr;
    }

    if (!WebCLInputChecker::isValidMemoryObjectFlag(memoryFlags)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return nullptr;
    }

    cl_int err = CL_SUCCESS;
    cl_mem clMemObject = 0;
    switch (memoryFlags) {
    case CL_MEM_READ_ONLY:
        clMemObject = clCreateSubBuffer(m_clMem, CL_MEM_READ_ONLY, CL_BUFFER_CREATE_TYPE_REGION, &bufferCreateInfo, &err);
        break;
    case CL_MEM_WRITE_ONLY:
        clMemObject =  clCreateSubBuffer(m_clMem, CL_MEM_WRITE_ONLY, CL_BUFFER_CREATE_TYPE_REGION, &bufferCreateInfo, &err);
        break;
    case CL_MEM_READ_WRITE:
        clMemObject =  clCreateSubBuffer(m_clMem, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION, &bufferCreateInfo, &err);
        break;
    default:
        es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
        break;
    }

    if (err != CL_SUCCESS) {
        WebCLException::throwException(err, es);
        return nullptr;
    }

    RefPtr<WebCLBuffer> buffer = adoptRef(new WebCLBuffer(clMemObject, context(), memoryFlags, size, this));
    return buffer.release();
}

WebCLBuffer::WebCLBuffer(cl_mem clMem, PassRefPtr<WebCLContext> context, unsigned memoryFlags, unsigned size, WebCLBuffer* parentBuffer)
    : WebCLMemoryObject(clMem, size, context, parentBuffer)
    , m_memoryFlags(memoryFlags)
{
}

} // namespace blink
