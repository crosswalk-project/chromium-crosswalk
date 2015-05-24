// Copyright (C) 2014 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#if ENABLE(WEBCL)

#include "modules/webcl/WebCL.h"
#include "modules/webcl/WebCLBuffer.h"
#include "modules/webcl/WebCLCommandQueue.h"
#include "modules/webcl/WebCLContext.h"
#include "modules/webcl/WebCLDevice.h"
#include "modules/webcl/WebCLKernel.h"
#include "modules/webcl/WebCLMemoryObject.h"
#include "modules/webcl/WebCLMemoryUtil.h"
#include "modules/webcl/WebCLObject.h"
#include "modules/webcl/WebCLOpenCL.h"
#include "modules/webcl/WebCLProgram.h"

namespace blink {

const char* programSource = \
    "__kernel void init(__global char* buffer, unsigned offset, unsigned count) "\
    "{                                                                          "\
    "    unsigned i = get_global_id(0);                                         "\
    "    unsigned displacedI = i + offset;                                      "\
    "    if (displacedI < count)                                                "\
    "        buffer[displacedI] = (char)(0);                                    "\
    "}                                                                          "\
    "__kernel void init16(__global char16* buffer, unsigned count)              "\
    "{                                                                          "\
    "    unsigned i = get_global_id(0);                                         "\
    "    if (i < count)                                                         "\
    "        buffer[i] = (char16)(0);                                           "\
    "}                                                                          ";

WebCLMemoryUtil::WebCLMemoryUtil(WebCLContext* context)
    : m_context(context)
{
}

WebCLMemoryUtil::~WebCLMemoryUtil()
{
}

void WebCLMemoryUtil::ensureMemory(WebCLMemoryObject* memoryObject, WebCLCommandQueue* commandQueue, ExceptionState& es)
{
    cl_int err = CL_SUCCESS;
    // The program and kernels are used to intialize OpenCL memory to 0.
    // Every created OpenCL memory should call this function to intialize.
    // TODO(junmin-zhu): Move intialization from buffer creation to buffer operations, such as: enqueueRead/Write/Copy* and enqueueNDRangeKernel function
    // after the third_party WebCL-validator integrated.
    if (!m_program) {
        cl_program clProgramId = clCreateProgramWithSource(m_context->getContext(), 1, (const char**)&programSource, nullptr, &err);
        if (err != CL_SUCCESS) {
            WebCLException::throwException(err, es);
            return;
        }

        m_program = WebCLProgram::create(clProgramId, m_context, String(programSource));
        m_program->build(m_context->getDevices(), emptyString(), nullptr, es);

        cl_kernel clKernelId16 = clCreateKernel(clProgramId, "init16", &err);
        if (err != CL_SUCCESS) {
            WebCLException::throwException(err, es);
            return;
        }

        m_kernelChar16 = WebCLKernel::create(clKernelId16, m_context, m_program.get(), String("init16"));
        cl_kernel clKernelId = clCreateKernel(clProgramId, "init", &err);
        if (err != CL_SUCCESS) {
            WebCLException::throwException(err, es);
            return;
        }

        m_kernelChar = WebCLKernel::create(clKernelId, m_context, m_program.get(), String("init"));
    }

    unsigned count = memoryObject->sizeInBytes() / 16;
    if (count) {
        m_kernelChar16->setArg(0, memoryObject, es);
        if (es.hadException())
            return;

        m_kernelChar16->setArg(1, sizeof(unsigned), &count, es);
        if (es.hadException())
            return;

        Vector<double> globalWorkSize;
        globalWorkSize.append(count);
        Vector<double> globalWorkOffset;
        Vector<double> localWorkSize;

        commandQueue->enqueueNDRangeKernel(m_kernelChar16.get(), globalWorkSize.size(), globalWorkOffset, globalWorkSize, localWorkSize, Vector<RefPtr<WebCLEvent>>(), nullptr, es);
    }

    unsigned remainingBytes = memoryObject->sizeInBytes() % 16;
    if (remainingBytes) {
        m_kernelChar->setArg(0, memoryObject, es);
        if (es.hadException())
            return;

        unsigned offset = count * 16;
        m_kernelChar->setArg(1, sizeof(unsigned), &offset, es);
        if (es.hadException())
            return;

        unsigned totalSize = memoryObject->sizeInBytes();
        m_kernelChar->setArg(2, sizeof(unsigned), &totalSize, es);
        if (es.hadException())
            return;

        Vector<double> globalWorkSize;
        globalWorkSize.append(remainingBytes);
        Vector<double> globalWorkOffset;
        Vector<double> localWorkSize;

        commandQueue->enqueueNDRangeKernel(m_kernelChar.get(), globalWorkSize.size(), globalWorkOffset, globalWorkSize, localWorkSize, Vector<RefPtr<WebCLEvent>>(), nullptr, es);
    }

    commandQueue->finishCommandQueues(WebCLCommandQueue::SyncMethod::SYNC);
}

void WebCLMemoryUtil::commandQueueCreated(WebCLCommandQueue* queue, ExceptionState& es)
{
    if (!queue)
        return;

    m_queues.append(queue->createWeakPtr());
    processPendingMemoryList(es);
}

void WebCLMemoryUtil::bufferCreated(WebCLBuffer* buffer, ExceptionState& es)
{
    if (!buffer)
        return;

    initializeOrQueueMemoryObject(buffer, es);
}

void WebCLMemoryUtil::processPendingMemoryList(ExceptionState& es)
{
    if (!m_pendingBuffers.size())
        return;

    WebCLCommandQueue* queue = validCommandQueue();
    if (queue) {
        for (auto bufferObject : m_pendingBuffers) {
            WebCLBuffer* buffer = static_cast<WebCLBuffer*>(bufferObject.get());
            if (buffer && !buffer->getMem())
                ensureMemory(buffer, queue, es);
        }
    }
    m_pendingBuffers.clear();
}

WebCLCommandQueue* WebCLMemoryUtil::validCommandQueue() const
{
    for (auto queueObject : m_queues) {
        WebCLCommandQueue* queue = static_cast<WebCLCommandQueue*>(queueObject.get());
        if (queue && !queue->isReleased())
            return queue;
    }
    return nullptr;
}

void WebCLMemoryUtil::initializeOrQueueMemoryObject(WebCLBuffer* buffer, ExceptionState& es)
{
    WebCLCommandQueue* queue = validCommandQueue();
    if (!queue) {
        m_pendingBuffers.append(buffer->createWeakPtr());
        return;
    }

    ensureMemory(buffer, queue, es);
}

} // namespace blink

#endif // ENABLE(WEBCL)
