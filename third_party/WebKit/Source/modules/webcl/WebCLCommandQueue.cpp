// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#if ENABLE(WEBCL)
#include "bindings/modules/v8/V8WebCLContext.h"
#include "bindings/modules/v8/V8WebCLDevice.h"
#include "core/dom/DOMArrayBuffer.h"
#include "core/dom/DOMArrayBufferView.h"
#include "core/dom/DOMTypedArray.h"
#include "core/html/HTMLCanvasElement.h"
#include "core/html/ImageData.h"
#include "core/webcl/WebCLException.h"
#include "modules/webcl/WebCL.h"
#include "modules/webcl/WebCLBuffer.h"
#include "modules/webcl/WebCLCommandQueue.h"
#include "modules/webcl/WebCLEvent.h"
#include "modules/webcl/WebCLHTMLUtil.h"
#include "modules/webcl/WebCLImage.h"
#include "modules/webcl/WebCLImageDescriptor.h"
#include "modules/webcl/WebCLInputChecker.h"
#include "modules/webcl/WebCLKernel.h"
#include "modules/webcl/WebCLOpenCL.h"
#include "public/platform/Platform.h"
#include "platform/ThreadSafeFunctional.h"

namespace blink {

// The holder of WebCLCommandQueue.
class WebCLCommandQueueHolder {
public:
    cl_event event;
    cl_int type;
    WeakPtr<WebCLObject> commandQueue;
};

WebCLCommandQueue::~WebCLCommandQueue()
{
    release();
    ASSERT(!m_clCommandQueue);
}

PassRefPtr<WebCLCommandQueue> WebCLCommandQueue::create(cl_command_queue commandQueue, PassRefPtr<WebCLContext> context, WebCLDevice* device)
{
    return adoptRef(new WebCLCommandQueue(commandQueue, context, device));
}

ScriptValue WebCLCommandQueue::getInfo(ScriptState* scriptState, int paramName, ExceptionState& es)
{
    v8::Handle<v8::Object> creationContext = scriptState->context()->Global();
    v8::Isolate* isolate = scriptState->isolate();

    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_COMMAND_QUEUE, WebCLException::invalidCommandQueueMessage);
        return ScriptValue(scriptState, v8::Null(isolate));
    }

    cl_int err = CL_SUCCESS;
    cl_command_queue_properties queueProperties = 0;
    switch(paramName) {
    case CL_QUEUE_CONTEXT:
        return ScriptValue(scriptState, toV8(context(), creationContext, isolate));
        break;
    case CL_QUEUE_DEVICE:
        return ScriptValue(scriptState, toV8(m_device, creationContext, isolate));
        break;
    case CL_QUEUE_PROPERTIES:
        err = clGetCommandQueueInfo(m_clCommandQueue, CL_QUEUE_PROPERTIES, sizeof(cl_command_queue_properties), &queueProperties, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(queueProperties)));
        break;
    default:
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return ScriptValue(scriptState, v8::Null(isolate));
    }

    WebCLException::throwException(err, es);
    return ScriptValue(scriptState, v8::Null(isolate));
}

unsigned WebCLCommandQueue::getProperties()
{
    cl_command_queue_properties queueProperties;
    cl_int err = clGetCommandQueueInfo(m_clCommandQueue, CL_QUEUE_PROPERTIES, sizeof(cl_command_queue_properties), &queueProperties, nullptr);
    if (err == CL_SUCCESS)
        return static_cast<unsigned>(queueProperties);

    return 0;
}

void WebCLCommandQueue::finish(WebCLCallback* whenFinished, ExceptionState& es)
{
    if (isReleased() || m_whenFinishCallback) {
        es.throwWebCLException(WebCLException::INVALID_COMMAND_QUEUE, WebCLException::invalidCommandQueueMessage);
        return;
    }

    if (whenFinished) {
        m_whenFinishCallback = adoptRef(whenFinished);
        finishCommandQueues(ASYNC);
    } else {
        finishCommandQueues(SYNC);
    }
}

void WebCLCommandQueue::finishCommandQueues(SyncMethod method)
{
    if (method == ASYNC) {
        cl_int err = clEnqueueMarker(m_clCommandQueue, &m_eventForCallback);
        if (err != CL_SUCCESS || !m_eventForCallback)
            return;
        WebCLCommandQueueHolder* holder = new WebCLCommandQueueHolder;
        holder->commandQueue = createWeakPtr();
        clSetEventCallback(m_eventForCallback, CL_COMPLETE, &callbackProxy, holder);
    } else if (method == SYNC) {
        clFinish(m_clCommandQueue);
    }
}

void WebCLCommandQueue::flush(ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_COMMAND_QUEUE, WebCLException::invalidCommandQueueMessage);
        return;
    }

    cl_int err = clFlush(m_clCommandQueue);
    if (err != CL_SUCCESS)
        WebCLException::throwException(err, es);
}

void WebCLCommandQueue::release()
{
    if (isReleased())
        return;

    // Wait for all the command queue to finish first.
    finishCommandQueues(SYNC);

    cl_int err = clReleaseCommandQueue(m_clCommandQueue);
    if (err != CL_SUCCESS)
        ASSERT_NOT_REACHED();

    m_clCommandQueue = 0;

    // Release the un-triggered callback and its associated event object.
    resetEventAndCallback();
}

void WebCLCommandQueue::enqueueBarrier(ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_COMMAND_QUEUE, WebCLException::invalidCommandQueueMessage);
        return;
    }

    cl_int err = clEnqueueBarrier(m_clCommandQueue);
    if (err != CL_SUCCESS)
        WebCLException::throwException(err, es);
}

void WebCLCommandQueue::enqueueMarker(WebCLEvent* event, ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_COMMAND_QUEUE, WebCLException::invalidCommandQueueMessage);
        return;
    }

    if (event->isUserEvent()) {
        es.throwWebCLException(WebCLException::INVALID_EVENT, WebCLException::invalidEventMessage);
        return;
    }

    cl_event* clEventId = WebCLEventPtrToCLEventPtr(event, es);
    if (event && !clEventId)
        return;

    cl_int err = clEnqueueMarker(m_clCommandQueue, clEventId);
    if (err != CL_SUCCESS)
        WebCLException::throwException(err, es);
}

void WebCLCommandQueue::enqueueWaitForEvents(const Vector<RefPtr<WebCLEvent>>& events, ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_COMMAND_QUEUE, WebCLException::invalidCommandQueueMessage);
        return;
    }

    if (!events.size()) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    Vector<cl_event> clEvents = WebCLEventVectorToCLEventVector(false, events, es);
    if (clEvents.size() != events.size())
        return;

    cl_int err = clEnqueueWaitForEvents(m_clCommandQueue, clEvents.size(), clEvents.data());
    if (err != CL_SUCCESS)
        WebCLException::throwException(err, es);
}

void WebCLCommandQueue::enqueueWriteBufferBase(WebCLBuffer* mem, bool blockingWrite, unsigned offset, unsigned bufferSize, void* ptr, size_t ptrLength, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_COMMAND_QUEUE, WebCLException::invalidCommandQueueMessage);
        return;
    }

    cl_mem clMemId = nullptr;
    if (mem) {
        clMemId = mem->getMem();
        if (!clMemId) {
            es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
            return;
        }
    }

    if (!ptr) {
        es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
        return;
    }

    if (!WebCLInputChecker::compareContext(context().get(), mem->context().get())) {
        es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
        return;
    }

    if (ptrLength < bufferSize || mem->sizeInBytes() < (offset + bufferSize)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    Vector<cl_event> clEvents = WebCLEventVectorToCLEventVector(blockingWrite, events, es);
    if (events.size() && clEvents.size() != events.size())
        return;

    cl_event* clEventId = WebCLEventPtrToCLEventPtr(event, es);
    if (event && !clEventId)
        return;

    cl_int err = blockingWrite ? clEnqueueWriteBuffer(m_clCommandQueue, clMemId, CL_TRUE, offset, bufferSize, ptr, clEvents.size(), clEvents.data(), clEventId) : clEnqueueWriteBuffer(m_clCommandQueue, clMemId, CL_FALSE, offset, bufferSize, ptr, clEvents.size(), clEvents.data(), clEventId);

    if (err != CL_SUCCESS)
        WebCLException::throwException(err, es);
}

void WebCLCommandQueue::enqueueWriteBuffer(WebCLBuffer* mem, bool blockingWrite, unsigned offset, unsigned bufferSize, DOMArrayBufferView* ptr, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (!ptr || !WebCLInputChecker::isValidDataSizeForDOMArrayBufferView(bufferSize, ptr)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }
    enqueueWriteBufferBase(mem, blockingWrite, offset, bufferSize, ptr->baseAddress(), ptr->byteLength(), events, event, es);
}

void WebCLCommandQueue::enqueueWriteBuffer(WebCLBuffer* buffer, bool blockingWrite, unsigned offset, ImageData* srcPixels, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (!isExtensionEnabled(context().get(), "WEBCL_html_image")) {
        es.throwWebCLException(WebCLException::EXTENSION_NOT_ENABLED, WebCLException::extensionNotEnabledMessage);
        return;
    }

    void* hostPtr = 0;
    size_t pixelSize = 0;
    if (!WebCLHTMLUtil::extractDataFromImageData(srcPixels, hostPtr, pixelSize, es))
        return;

    enqueueWriteBufferBase(buffer, blockingWrite, offset, pixelSize, hostPtr, pixelSize, events, event, es);
}

void WebCLCommandQueue::enqueueWriteBuffer(WebCLBuffer* buffer, bool blockingWrite, unsigned offset, HTMLCanvasElement* srcCanvas, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (!isExtensionEnabled(context().get(), "WEBCL_html_image")) {
        es.throwWebCLException(WebCLException::EXTENSION_NOT_ENABLED, WebCLException::extensionNotEnabledMessage);
        return;
    }

    Vector<uint8_t> data;
    size_t canvasSize = 0;
    if (!WebCLHTMLUtil::extractDataFromCanvas(srcCanvas, data, canvasSize, es))
        return;

    void* hostPtr = data.data();
    enqueueWriteBufferBase(buffer, blockingWrite, offset, canvasSize, hostPtr, canvasSize, events, event, es);
}

void WebCLCommandQueue::enqueueWriteBuffer(WebCLBuffer* buffer, bool blockingWrite, unsigned offset, HTMLImageElement* srcImage, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (!isExtensionEnabled(context().get(), "WEBCL_html_image")) {
        es.throwWebCLException(WebCLException::EXTENSION_NOT_ENABLED, WebCLException::extensionNotEnabledMessage);
        return;
    }

    Vector<uint8_t> data;
    size_t imageSize = 0;
    if (!WebCLHTMLUtil::extractDataFromImage(srcImage, data, imageSize, es))
        return;

    void* hostPtr = data.data();
    enqueueWriteBufferBase(buffer, blockingWrite, offset, imageSize, hostPtr, imageSize, events, event, es);
}

void WebCLCommandQueue::enqueueWriteBufferRectBase(WebCLBuffer* mem, bool blockingWrite, const Vector<unsigned>& bufferOrigin, const Vector<unsigned>& hostOrigin, const Vector<unsigned>& region, unsigned bufferRowPitch, unsigned bufferSlicePitch, unsigned hostRowPitch, unsigned hostSlicePitch, void* ptr, size_t ptrLength, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_COMMAND_QUEUE, WebCLException::invalidCommandQueueMessage);
        return;
    }

    cl_mem clMemId = nullptr;
    if (mem) {
        clMemId = mem->getMem();
        if (!clMemId) {
            es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
            return;
        }
    }

    if (!ptr) {
        es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
        return;
    }

    if (!WebCLInputChecker::compareContext(context().get(), mem->context().get())) {
        es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
        return;
    }

    if (bufferOrigin.size() != 3 || hostOrigin.size() != 3 || region.size() != 3) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    Vector<size_t> bufferOriginCopy, hostOriginCopy, regionCopy;
    bufferOriginCopy.appendVector(bufferOrigin);
    hostOriginCopy.appendVector(hostOrigin);
    regionCopy.appendVector(region);

    if (!WebCLInputChecker::isValidRegionForMemoryObject(bufferOriginCopy, regionCopy, bufferRowPitch, bufferSlicePitch, mem->sizeInBytes()) || !WebCLInputChecker::isValidRegionForMemoryObject(hostOriginCopy, regionCopy, hostRowPitch, hostSlicePitch, ptrLength)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    Vector<cl_event> clEvents = WebCLEventVectorToCLEventVector(blockingWrite, events, es);
    if (events.size() && clEvents.size() != events.size())
        return;

    cl_event* clEventId = WebCLEventPtrToCLEventPtr(event, es);
    if (event && !clEventId)
        return;

    cl_int err = blockingWrite? clEnqueueWriteBufferRect(m_clCommandQueue, clMemId, CL_TRUE, bufferOriginCopy.data(), hostOriginCopy.data(), regionCopy.data(), bufferRowPitch, bufferSlicePitch, hostRowPitch, 0, ptr, clEvents.size(), clEvents.data(), clEventId) : clEnqueueWriteBufferRect(m_clCommandQueue, clMemId, CL_FALSE, bufferOriginCopy.data(), hostOriginCopy.data(), regionCopy.data(), bufferRowPitch, bufferSlicePitch, hostRowPitch, 0, ptr, clEvents.size(), clEvents.data(), clEventId);

    if (err != CL_SUCCESS)
        WebCLException::throwException(err, es);
}

void WebCLCommandQueue::enqueueWriteBufferRect(WebCLBuffer* mem, bool blockingWrite, const Vector<unsigned>& bufferOrigin, const Vector<unsigned>& hostOrigin, const Vector<unsigned>& region, unsigned bufferRowPitch, unsigned bufferSlicePitch, unsigned hostRowPitch, unsigned hostSlicePitch, DOMArrayBufferView* ptr, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (!ptr || !WebCLInputChecker::isValidDataSizeForDOMArrayBufferView(hostRowPitch, ptr) || !WebCLInputChecker::isValidDataSizeForDOMArrayBufferView(hostSlicePitch, ptr)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    enqueueWriteBufferRectBase(mem, blockingWrite, bufferOrigin, hostOrigin, region, bufferRowPitch, bufferSlicePitch, hostRowPitch, hostSlicePitch, ptr->baseAddress(), ptr->byteLength(), events, event, es);
}

void WebCLCommandQueue::enqueueWriteBufferRect(WebCLBuffer* buffer, bool blockingWrite, const Vector<unsigned>& bufferOrigin, const Vector<unsigned>& hostOrigin, const Vector<unsigned>& region, unsigned bufferRowPitch, unsigned bufferSlicePitch, ImageData* srcPixels, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (!isExtensionEnabled(context().get(), "WEBCL_html_image")) {
        es.throwWebCLException(WebCLException::EXTENSION_NOT_ENABLED, WebCLException::extensionNotEnabledMessage);
        return;
    }

    void* hostPtr = 0;
    size_t pixelSize = 0;
    if (!WebCLHTMLUtil::extractDataFromImageData(srcPixels, hostPtr, pixelSize, es))
        return;

    enqueueWriteBufferRectBase(buffer, blockingWrite, bufferOrigin, hostOrigin, region, bufferRowPitch, bufferSlicePitch, 0, 0, hostPtr, pixelSize, events, event, es);
}

void WebCLCommandQueue::enqueueWriteBufferRect(WebCLBuffer* buffer, bool blockingWrite, const Vector<unsigned>& bufferOrigin, const Vector<unsigned>& hostOrigin, const Vector<unsigned>& region, unsigned bufferRowPitch, unsigned bufferSlicePitch, HTMLCanvasElement* srcCanvas, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (!isExtensionEnabled(context().get(), "WEBCL_html_image")) {
        es.throwWebCLException(WebCLException::EXTENSION_NOT_ENABLED, WebCLException::extensionNotEnabledMessage);
        return;
    }

    Vector<uint8_t> data;
    size_t canvasSize = 0;
    if (!WebCLHTMLUtil::extractDataFromCanvas(srcCanvas, data, canvasSize, es))
        return;

    void* hostPtr = data.data();
    enqueueWriteBufferRectBase(buffer, blockingWrite, bufferOrigin, hostOrigin, region, bufferRowPitch, bufferSlicePitch, 0, 0, hostPtr, canvasSize, events, event, es);
}

void WebCLCommandQueue::enqueueWriteBufferRect(WebCLBuffer* buffer, bool blockingWrite, const Vector<unsigned>& bufferOrigin, const Vector<unsigned>& hostOrigin, const Vector<unsigned>& region, unsigned bufferRowPitch, unsigned bufferSlicePitch, HTMLImageElement* srcImage, const Vector<RefPtr<WebCLEvent>>& eventWaitlist, WebCLEvent* event, ExceptionState& es)
{
    if (!isExtensionEnabled(context().get(), "WEBCL_html_image")) {
        es.throwWebCLException(WebCLException::EXTENSION_NOT_ENABLED, WebCLException::extensionNotEnabledMessage);
        return;
    }

    Vector<uint8_t> data;
    size_t imageSize = 0;
    if (!WebCLHTMLUtil::extractDataFromImage(srcImage, data, imageSize, es))
        return;

    void* hostPtr = data.data();
    enqueueWriteBufferRectBase(buffer, blockingWrite, bufferOrigin, hostOrigin, region, bufferRowPitch, bufferSlicePitch, 0, 0, hostPtr, imageSize, eventWaitlist, event, es);
}

void WebCLCommandQueue::enqueueReadBufferBase(WebCLBuffer* mem, bool blockingRead, unsigned offset, unsigned bufferSize, void* ptr, size_t ptrLength, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_COMMAND_QUEUE, WebCLException::invalidCommandQueueMessage);
        return;
    }

    if (!WebCLInputChecker::compareContext(context().get(), mem->context().get())) {
        es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
        return;
    }

    cl_mem clMemId = nullptr;
    if (mem) {
        clMemId = mem->getMem();
        if (!clMemId) {
            es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
            return;
        }
    }

    if (ptrLength < bufferSize || mem->sizeInBytes() < (offset + bufferSize)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    Vector<cl_event> clEvents = WebCLEventVectorToCLEventVector(blockingRead, events, es);
    if (events.size() && clEvents.size() != events.size())
        return;

    cl_event* clEventId = WebCLEventPtrToCLEventPtr(event, es);
    if (event && !clEventId)
        return;

    cl_int err = blockingRead ? clEnqueueReadBuffer(m_clCommandQueue, clMemId, CL_TRUE, offset, bufferSize, ptr, clEvents.size(), clEvents.data(), clEventId) : clEnqueueReadBuffer(m_clCommandQueue, clMemId, CL_FALSE, offset, bufferSize, ptr, clEvents.size(), clEvents.data(), clEventId);

    if (err != CL_SUCCESS)
        WebCLException::throwException(err, es);
}

void WebCLCommandQueue::enqueueReadBuffer(WebCLBuffer* mem, bool blockingRead, unsigned offset, unsigned bufferSize, DOMArrayBufferView* ptr, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (!ptr || !WebCLInputChecker::isValidDataSizeForDOMArrayBufferView(bufferSize, ptr)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    enqueueReadBufferBase(mem, blockingRead, offset, bufferSize, ptr->baseAddress(), ptr->byteLength(), events, event, es);
}

void WebCLCommandQueue::enqueueReadBuffer(WebCLBuffer* buffer, bool blockingRead, unsigned offset, unsigned numBytes, HTMLCanvasElement* dstCanvas, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (!isExtensionEnabled(context().get(), "WEBCL_html_image")) {
        es.throwWebCLException(WebCLException::EXTENSION_NOT_ENABLED, WebCLException::extensionNotEnabledMessage);
        return;
    }

    Vector<uint8_t> data;
    size_t canvasSize = 0;
    if (!WebCLHTMLUtil::extractDataFromCanvas(dstCanvas, data, canvasSize, es))
        return;

    void* hostPtr = data.data();
    enqueueReadBufferBase(buffer, blockingRead, offset, numBytes, hostPtr, canvasSize, events, event, es);
}

void WebCLCommandQueue::enqueueReadBufferRectBase(WebCLBuffer* mem, bool blockingRead, const Vector<unsigned>& bufferOrigin, const Vector<unsigned>& hostOrigin, const Vector<unsigned>& region, unsigned bufferRowPitch, unsigned bufferSlicePitch, unsigned hostRowPitch, unsigned hostSlicePitch, void* ptr, size_t ptrLength, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_COMMAND_QUEUE, WebCLException::invalidCommandQueueMessage);
        return;
    }

    if (!WebCLInputChecker::compareContext(context().get(), mem->context().get())) {
        es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
        return;
    }

    cl_mem clMemId = nullptr;
    if (mem) {
        clMemId = mem->getMem();
        if (!clMemId) {
            es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
            return;
        }
    }

    if (!ptr) {
        es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
        return;
    }

    if (bufferOrigin.size() != 3 || hostOrigin.size() != 3 || region.size() != 3) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    Vector<size_t> bufferOriginCopy, hostOriginCopy, regionCopy;
    bufferOriginCopy.appendVector(bufferOrigin);
    hostOriginCopy.appendVector(hostOrigin);
    regionCopy.appendVector(region);

    if (!WebCLInputChecker::isValidRegionForMemoryObject(hostOriginCopy, regionCopy, hostRowPitch, hostSlicePitch, ptrLength) || !WebCLInputChecker::isValidRegionForMemoryObject(bufferOriginCopy, regionCopy, bufferRowPitch, bufferSlicePitch, mem->sizeInBytes())) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    Vector<cl_event> clEvents = WebCLEventVectorToCLEventVector(blockingRead, events, es);
    if (events.size() && clEvents.size() != events.size())
        return;

    cl_event* clEventId = WebCLEventPtrToCLEventPtr(event, es);
    if (event && !clEventId)
        return;

    cl_int err = blockingRead ? clEnqueueReadBufferRect(m_clCommandQueue, clMemId, CL_TRUE, bufferOriginCopy.data(), hostOriginCopy.data(), regionCopy.data(), bufferRowPitch, bufferSlicePitch, hostRowPitch, hostSlicePitch, ptr, clEvents.size(), clEvents.data(), clEventId) : clEnqueueReadBufferRect(m_clCommandQueue, clMemId, CL_FALSE, bufferOriginCopy.data(), hostOriginCopy.data(), regionCopy.data(), bufferRowPitch, bufferSlicePitch, hostRowPitch, hostSlicePitch, ptr, clEvents.size(), clEvents.data(), clEventId);

    if (err != CL_SUCCESS)
        WebCLException::throwException(err, es);
}

void WebCLCommandQueue::enqueueReadBufferRect(WebCLBuffer* mem, bool blockingRead, const Vector<unsigned>& bufferOrigin, const Vector<unsigned>& hostOrigin, const Vector<unsigned>& region, unsigned bufferRowPitch, unsigned bufferSlicePitch, unsigned hostRowPitch, unsigned hostSlicePitch, DOMArrayBufferView* ptr, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (!ptr || !WebCLInputChecker::isValidDataSizeForDOMArrayBufferView(hostRowPitch, ptr) || !WebCLInputChecker::isValidDataSizeForDOMArrayBufferView(hostSlicePitch, ptr)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    enqueueReadBufferRectBase(mem, blockingRead, bufferOrigin, hostOrigin, region, bufferRowPitch, bufferSlicePitch, hostRowPitch, hostSlicePitch, ptr->baseAddress(), ptr->byteLength(), events, event, es);
}

void WebCLCommandQueue::enqueueReadBufferRect(WebCLBuffer* buffer, bool blockingRead, const Vector<unsigned>& bufferOrigin, const Vector<unsigned>& hostOrigin, const Vector<unsigned>& region, unsigned bufferRowPitch, unsigned bufferSlicePitch, HTMLCanvasElement* dstCanvas, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (!isExtensionEnabled(context().get(), "WEBCL_html_image")) {
        es.throwWebCLException(WebCLException::EXTENSION_NOT_ENABLED, WebCLException::extensionNotEnabledMessage);
        return;
    }

    Vector<uint8_t> data;
    size_t canvasSize = 0;
    if (!WebCLHTMLUtil::extractDataFromCanvas(dstCanvas, data, canvasSize, es))
        return;

    void* hostPtr = data.data();
    enqueueReadBufferRectBase(buffer, blockingRead, bufferOrigin, hostOrigin, region, bufferRowPitch, bufferSlicePitch, 0, 0, hostPtr, canvasSize, events, event, es);
}

void WebCLCommandQueue::enqueueReadImageBase(WebCLImage* image, bool blockingRead, const Vector<unsigned>& origin, const Vector<unsigned>& region, unsigned hostRowPitch, void* ptr, size_t ptrLength, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_COMMAND_QUEUE, WebCLException::invalidCommandQueueMessage);
        return;
    }

    if (!WebCLInputChecker::compareContext(context().get(), image->context().get())) {
        es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
        return;
    }

    cl_mem clMemId = nullptr;
    if (image) {
        clMemId = image->getMem();
        if (!clMemId) {
            es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
            return;
        }
    }

    if (origin.size() != 2 || region.size() != 2) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    if (!WebCLInputChecker::isValidRegionForImage(image->imageDescriptor(), origin, region) || !WebCLInputChecker::isValidRegionForHostPtr(region, hostRowPitch, image->imageDescriptor(), ptrLength)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    Vector<size_t, 3> originCopy, regionCopy;
    originCopy.appendVector(origin);
    regionCopy.appendVector(region);
    // No support for 3D-images, so set default values of 0 for all origin & region arrays at 3rd index.
    originCopy.append(0);
    regionCopy.append(1);

    Vector<cl_event> clEvents = WebCLEventVectorToCLEventVector(blockingRead, events, es);
    if (events.size() && clEvents.size() != events.size())
        return;

    cl_event* clEventId = WebCLEventPtrToCLEventPtr(event, es);
    if (event && !clEventId)
        return;

    cl_int err = blockingRead ? clEnqueueReadImage(m_clCommandQueue, clMemId, CL_TRUE, originCopy.data(), regionCopy.data(), hostRowPitch, 0, ptr, clEvents.size(), clEvents.data(), clEventId) : clEnqueueReadImage(m_clCommandQueue, clMemId, CL_FALSE, originCopy.data(), regionCopy.data(), hostRowPitch, 0, ptr, clEvents.size(), clEvents.data(), clEventId);

    if (err != CL_SUCCESS)
        WebCLException::throwException(err, es);
}

void WebCLCommandQueue::enqueueReadImage(WebCLImage* image, bool blockingRead, const Vector<unsigned>& origin, const Vector<unsigned>& region, unsigned hostRowPitch, DOMArrayBufferView* ptr, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (!ptr || !WebCLInputChecker::isValidDataSizeForDOMArrayBufferView(hostRowPitch, ptr)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    enqueueReadImageBase(image, blockingRead, origin, region, hostRowPitch, ptr->baseAddress(), ptr->byteLength(), events, event, es);
}

void WebCLCommandQueue::enqueueReadImage(WebCLImage* image, bool blockingRead, const Vector<unsigned>& origin, const Vector<unsigned>& region, HTMLCanvasElement* dstCanvas, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (!isExtensionEnabled(context().get(), "WEBCL_html_image")) {
        es.throwWebCLException(WebCLException::EXTENSION_NOT_ENABLED, WebCLException::extensionNotEnabledMessage);
        return;
    }

    Vector<uint8_t> data;
    size_t canvasSize = 0;
    if (!WebCLHTMLUtil::extractDataFromCanvas(dstCanvas, data, canvasSize, es))
        return;

    void* hostPtr = data.data();
    enqueueReadImageBase(image, blockingRead, origin, region, 0, hostPtr, canvasSize, events, event, es);
}

void WebCLCommandQueue::enqueueNDRangeKernel(WebCLKernel* kernel, unsigned dim, const Vector<double>& offsets, const Vector<double>& globalWorkSize, const Vector<double>& localWorkSize, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_COMMAND_QUEUE, WebCLException::invalidCommandQueueMessage);
        return;
    }

    cl_kernel clKernelId = nullptr;
    if (kernel) {
        clKernelId = kernel->getKernel();
        if (!clKernelId) {
            es.throwWebCLException(WebCLException::INVALID_KERNEL, WebCLException::invalidKernelMessage);
            return;
        }
    }

    if (!WebCLInputChecker::compareContext(context().get(), kernel->context().get())) {
        es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
        return;
    }

    if (dim > 3) {
        es.throwWebCLException(WebCLException::INVALID_WORK_DIMENSION, WebCLException::invalidWorkDimensionMessage);
        return;
    }

    if (dim != globalWorkSize.size()) {
        es.throwWebCLException(WebCLException::INVALID_GLOBAL_WORK_SIZE, WebCLException::invalidGlobalWorkSizeMessage);
        return;
    }

    if (offsets.size() && dim != offsets.size()) {
        es.throwWebCLException(WebCLException::INVALID_GLOBAL_OFFSET, WebCLException::invalidGlobalOffsetMessage);
        return;
    }
    if (localWorkSize.size() && dim != localWorkSize.size()) {
        es.throwWebCLException(WebCLException::INVALID_WORK_GROUP_SIZE, WebCLException::invalidWorkGroupSizeMessage);
        return;
    }

    const Vector<unsigned>& required = kernel->requiredArguments();
    if (!localWorkSize.size() && required.size()) {
        es.throwWebCLException(WebCLException::INVALID_WORK_GROUP_SIZE, WebCLException::invalidWorkGroupSizeMessage);
        return;
    }

    if (localWorkSize.size() && required.size()) {
        for (unsigned i = 0; i < localWorkSize.size(); i ++) {
            if (localWorkSize[i] != required[i]) {
                es.throwWebCLException(WebCLException::INVALID_WORK_GROUP_SIZE, WebCLException::invalidWorkGroupSizeMessage);
                return;
            }
        }
    }

    const unsigned long long maxWorkSizeValue = (1ULL << 32) - 1;
    for (unsigned i = 0; i < globalWorkSize.size(); i ++) {
        if (globalWorkSize[i] > maxWorkSizeValue) {
            es.throwWebCLException(WebCLException::INVALID_GLOBAL_WORK_SIZE, WebCLException::invalidGlobalWorkSizeMessage);
            return;
        }
        if (offsets.size() && globalWorkSize[i] + offsets[i] > maxWorkSizeValue) {
            es.throwWebCLException(WebCLException::INVALID_GLOBAL_OFFSET, WebCLException::invalidGlobalOffsetMessage);
            return;
        }
        if (localWorkSize.size() && ((unsigned)localWorkSize[i] && (unsigned)globalWorkSize[i] % (unsigned)localWorkSize[i] != 0)) {
            es.throwWebCLException(WebCLException::INVALID_WORK_GROUP_SIZE, WebCLException::invalidWorkGroupSizeMessage);
            return;
        }
    }

    unsigned maxWorkGroupSize = m_device->getMaxWorkGroup();
    Vector<unsigned> maxWorkItemSizes = m_device->getMaxWorkItem();
    unsigned total = 1;
    for (unsigned i = 0; maxWorkItemSizes.size() == localWorkSize.size() && i < localWorkSize.size(); i ++) {
        if (localWorkSize[i] > maxWorkItemSizes[i]) {
            es.throwWebCLException(WebCLException::INVALID_WORK_ITEM_SIZE, WebCLException::invalidWorkItemSizeMessage);
            return;
        }
        total = total * localWorkSize[i];
    }

    if (maxWorkGroupSize && total > maxWorkGroupSize) {
        es.throwWebCLException(WebCLException::INVALID_WORK_GROUP_SIZE, WebCLException::invalidWorkGroupSizeMessage);
        return;
    }

    if (kernel->numberOfArguments() != kernel->associatedArguments()) {
        es.throwWebCLException(WebCLException::INVALID_KERNEL_ARGS, WebCLException::invalidKernelArgsMessage);
        return;
    }

    Vector<size_t> gWorkSizeCopy, lWorkSizeCopy, gWorkOffsetCopy;
    gWorkSizeCopy.appendVector(globalWorkSize);
    gWorkOffsetCopy.appendVector(offsets);
    lWorkSizeCopy.appendVector(localWorkSize);

    Vector<cl_event> clEvents = WebCLEventVectorToCLEventVector(false, events, es);
    if (events.size() && clEvents.size() != events.size())
        return;

    cl_event* clEventId = WebCLEventPtrToCLEventPtr(event, es);
    if (event && !clEventId)
        return;

    cl_int err = clEnqueueNDRangeKernel(m_clCommandQueue, clKernelId, dim, gWorkOffsetCopy.data(), gWorkSizeCopy.data(), lWorkSizeCopy.data(), clEvents.size(), clEvents.data(), clEventId);

    if (err != CL_SUCCESS)
        WebCLException::throwException(err, es);
}

void WebCLCommandQueue::enqueueWriteImageBase(WebCLImage* image, bool blockingWrite, const Vector<unsigned>& origin, const Vector<unsigned>& region, unsigned hostRowPitch, void* ptr, size_t ptrLength, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_COMMAND_QUEUE, WebCLException::invalidCommandQueueMessage);
        return;
    }

    cl_mem clMemId = nullptr;
    if (image) {
        clMemId = image->getMem();
        if (!clMemId) {
            es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
            return;
        }
    }

    if (!WebCLInputChecker::compareContext(context().get(), image->context().get())) {
        es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
        return;
    }

    if (!ptr) {
        es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
        return;
    }

    if (origin.size() != 2 || region.size() != 2) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    if (!WebCLInputChecker::isValidRegionForImage(image->imageDescriptor(), origin, region) || !WebCLInputChecker::isValidRegionForHostPtr(region, hostRowPitch, image->imageDescriptor(), ptrLength)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    Vector<size_t, 3> originCopy, regionCopy;
    originCopy.appendVector(origin);
    regionCopy.appendVector(region);
    // WebCL doesn't support 3D-images as OpenCL, so set default values of 0 for all origin & region arrays at 3rd index.
    originCopy.append(0);
    regionCopy.append(1);

    Vector<cl_event> clEvents = WebCLEventVectorToCLEventVector(blockingWrite, events, es);
    if (events.size() && clEvents.size() != events.size())
        return;

    cl_event* clEventId = WebCLEventPtrToCLEventPtr(event, es);
    if (event && !clEventId)
        return;

    cl_int err = blockingWrite ? clEnqueueWriteImage(m_clCommandQueue, clMemId, CL_TRUE, originCopy.data(), regionCopy.data(), hostRowPitch, 0, ptr, clEvents.size(), clEvents.data(), clEventId) : clEnqueueWriteImage(m_clCommandQueue, clMemId, CL_FALSE, originCopy.data(), regionCopy.data(), hostRowPitch, 0, ptr, clEvents.size(), clEvents.data(), clEventId);

    if (err != CL_SUCCESS)
        WebCLException::throwException(err, es);
}

void WebCLCommandQueue::enqueueWriteImage(WebCLImage* image, bool blockingWrite, const Vector<unsigned>& origin, const Vector<unsigned>& region, unsigned hostRowPitch, DOMArrayBufferView* ptr, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (!ptr || !WebCLInputChecker::isValidDataSizeForDOMArrayBufferView(hostRowPitch, ptr)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    enqueueWriteImageBase(image, blockingWrite, origin, region, hostRowPitch, ptr->baseAddress(), ptr->byteLength(), events, event, es);
}

void WebCLCommandQueue::enqueueWriteImage(WebCLImage* image, bool blockingWrite, const Vector<unsigned>& origin, const Vector<unsigned>& region, ImageData* srcPixels, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (!isExtensionEnabled(context().get(), "WEBCL_html_image")) {
        es.throwWebCLException(WebCLException::EXTENSION_NOT_ENABLED, WebCLException::extensionNotEnabledMessage);
        return;
    }

    void* hostPtr = 0;
    size_t pixelSize = 0;
    if (!WebCLHTMLUtil::extractDataFromImageData(srcPixels, hostPtr, pixelSize, es))
        return;

    enqueueWriteImageBase(image, blockingWrite, origin, region, 0, hostPtr, pixelSize, events, event, es);
}

void WebCLCommandQueue::enqueueWriteImage(WebCLImage* image, bool blockingWrite, const Vector<unsigned>& origin, const Vector<unsigned>& region, HTMLCanvasElement* srcCanvas, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (!isExtensionEnabled(context().get(), "WEBCL_html_image")) {
        es.throwWebCLException(WebCLException::EXTENSION_NOT_ENABLED, WebCLException::extensionNotEnabledMessage);
        return;
    }

    Vector<uint8_t> data;
    size_t canvasSize = 0;
    if (!WebCLHTMLUtil::extractDataFromCanvas(srcCanvas, data, canvasSize, es))
        return;

    void* hostPtr = data.data();
    enqueueWriteImageBase(image, blockingWrite, origin, region, 0, hostPtr, canvasSize, events, event, es);
}

void WebCLCommandQueue::enqueueWriteImage(WebCLImage* image, bool blockingWrite, const Vector<unsigned>& origin, const Vector<unsigned>& region, HTMLImageElement* srcImage, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (!isExtensionEnabled(context().get(), "WEBCL_html_image")) {
        es.throwWebCLException(WebCLException::EXTENSION_NOT_ENABLED, WebCLException::extensionNotEnabledMessage);
        return;
    }

    Vector<uint8_t> data;
    size_t imageSize = 0;
    if (!WebCLHTMLUtil::extractDataFromImage(srcImage, data, imageSize, es))
        return;

    void* hostPtr = data.data();
    enqueueWriteImageBase(image, blockingWrite, origin, region, 0, hostPtr, imageSize, events, event, es);
}

void WebCLCommandQueue::enqueueWriteImage(WebCLImage* image, bool blockingWrite, HTMLVideoElement* srcVideo, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (!isExtensionEnabled(context().get(), "WEBCL_html_video")) {
        es.throwWebCLException(WebCLException::EXTENSION_NOT_ENABLED, WebCLException::extensionNotEnabledMessage);
        return;
    }

    Vector<uint8_t> data;
    size_t videoSize = 0;
    if (!context()->getHTMLUtil()->extractDataFromVideo(srcVideo, data, videoSize, es))
        return;
    void* hostPtr = data.data();

    Vector<unsigned> origin;
    origin.reserveCapacity(2);
    origin.append(0);
    origin.append(0);

    Vector<unsigned> region;
    region.reserveCapacity(2);
    region.append(image->imageDescriptor().width());
    region.append(image->imageDescriptor().height());

    enqueueWriteImageBase(image, blockingWrite, origin, region, 0, hostPtr, videoSize, events, event, es);
}

void WebCLCommandQueue::enqueueCopyBuffer(WebCLBuffer* srcBuffer, WebCLBuffer* dstBuffer, unsigned srcOffset, unsigned dstOffset, unsigned cb, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_COMMAND_QUEUE, WebCLException::invalidCommandQueueMessage);
        return;
    }

    cl_mem clSrcBufferId = nullptr;
    if (srcBuffer) {
        clSrcBufferId = srcBuffer->getMem();
        if (!clSrcBufferId) {
            es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
            return;
        }
    }

    cl_mem clDstBufferId = nullptr;
    if (dstBuffer) {
        clDstBufferId = dstBuffer->getMem();
        if (!clDstBufferId) {
            es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
            return;
        }
    }

    if (!WebCLInputChecker::compareContext(context().get(), srcBuffer->context().get()) || !WebCLInputChecker::compareContext(context().get(), dstBuffer->context().get())) {
        es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
        return;
    }

    if (WebCLInputChecker::isRegionOverlapping(srcBuffer, dstBuffer, srcOffset, dstOffset, cb)) {
        es.throwWebCLException(WebCLException::MEM_COPY_OVERLAP, WebCLException::memCopyOverlapMessage);
        return;
    }

    if ((srcOffset + cb) > srcBuffer->sizeInBytes()
        || (dstOffset + cb) > dstBuffer->sizeInBytes()) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    Vector<cl_event> clEvents = WebCLEventVectorToCLEventVector(false, events, es);
    if (events.size() && clEvents.size() != events.size())
        return;

    cl_event* clEventId = WebCLEventPtrToCLEventPtr(event, es);
    if (event && !clEventId)
        return;

    cl_int err = clEnqueueCopyBuffer(m_clCommandQueue, clSrcBufferId, clDstBufferId, srcOffset, dstOffset, cb, clEvents.size(), clEvents.data(), clEventId);

    if (err != CL_SUCCESS)
        WebCLException::throwException(err, es);
}

void WebCLCommandQueue::enqueueCopyBufferRect(WebCLBuffer* srcBuffer, WebCLBuffer* dstBuffer, const Vector<unsigned>& srcOrigin, const Vector<unsigned>& dstOrigin, const Vector<unsigned>& region, unsigned srcRowPitch, unsigned srcSlicePitch, unsigned dstRowPitch, unsigned dstSlicePitch, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_COMMAND_QUEUE, WebCLException::invalidCommandQueueMessage);
        return;
    }

    cl_mem clSrcBufferId = nullptr;
    if (srcBuffer) {
        clSrcBufferId = srcBuffer->getMem();
        if (!clSrcBufferId) {
            es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
            return;
        }
    }

    cl_mem clDstBufferId = nullptr;
    if (dstBuffer) {
        clDstBufferId = dstBuffer->getMem();
        if (!clDstBufferId) {
            es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
            return;
        }
    }

    if (!WebCLInputChecker::compareContext(context().get(), srcBuffer->context().get()) || !WebCLInputChecker::compareContext(context().get(), dstBuffer->context().get())) {
        es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
        return;
    }

    if (srcOrigin.size() != 3 || dstOrigin.size() != 3 || region.size() != 3) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    size_t srcOffset = srcOrigin[2] * srcSlicePitch + srcOrigin[1] * srcRowPitch + srcOrigin[0];
    size_t dstOffset = dstOrigin[2] * dstSlicePitch + dstOrigin[1] * dstRowPitch + dstOrigin[0];
    size_t numBytes = region[2] * region[1] * region[0];
    if (WebCLInputChecker::isRegionOverlapping(srcBuffer, dstBuffer, srcOffset, dstOffset, numBytes)) {
        es.throwWebCLException(WebCLException::MEM_COPY_OVERLAP, WebCLException::memCopyOverlapMessage);
        return;
    }

    Vector<size_t> srcOriginCopy, dstOriginCopy, regionCopy;
    srcOriginCopy.appendVector(srcOrigin);
    dstOriginCopy.appendVector(dstOrigin);
    regionCopy.appendVector(region);

    if (!WebCLInputChecker::isValidRegionForMemoryObject(srcOriginCopy, regionCopy, srcRowPitch, srcSlicePitch, srcBuffer->sizeInBytes()) || !WebCLInputChecker::isValidRegionForMemoryObject(dstOriginCopy, regionCopy, dstRowPitch, dstSlicePitch, dstBuffer->sizeInBytes())) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    Vector<cl_event> clEvents = WebCLEventVectorToCLEventVector(false, events, es);
    if (events.size() && clEvents.size() != events.size())
        return;

    cl_event* clEventId = WebCLEventPtrToCLEventPtr(event, es);
    if (event && !clEventId)
        return;

    cl_int err = clEnqueueCopyBufferRect(m_clCommandQueue, clSrcBufferId, clDstBufferId, srcOriginCopy.data(), dstOriginCopy.data(), regionCopy.data(), srcRowPitch, srcSlicePitch, dstRowPitch, dstSlicePitch, clEvents.size(), clEvents.data(), clEventId);

    if (err != CL_SUCCESS)
        WebCLException::throwException(err, es);
}

void WebCLCommandQueue::enqueueCopyImage(WebCLImage* srcImage, WebCLImage* dstImage, const Vector<unsigned>& srcOrigin, const Vector<unsigned>& dstOrigin, const Vector<unsigned>& region, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_COMMAND_QUEUE, WebCLException::invalidCommandQueueMessage);
        return;
    }

    cl_mem clSrcImageId = nullptr;
    if (srcImage) {
        clSrcImageId = srcImage->getMem();
        if (!clSrcImageId) {
            es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
            return;
        }
    }

    cl_mem clDstImageId = nullptr;
    if (dstImage) {
        clDstImageId = dstImage->getMem();
        if (!clDstImageId) {
            es.throwWebCLException(WebCLException::INVALID_IMAGE_SIZE, WebCLException::invalidImageSizeMessage);
            return;
        }
    }

    if (!WebCLInputChecker::compareContext(context().get(), srcImage->context().get()) || !WebCLInputChecker::compareContext(context().get(), dstImage->context().get())) {
        es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
        return;
    }

    if (!WebCLInputChecker::compareImageFormat(srcImage->imageDescriptor(), dstImage->imageDescriptor())) {
        es.throwWebCLException(WebCLException::IMAGE_FORMAT_MISMATCH, WebCLException::imageFormatMismatchMessage);
        return;
    }

    if (srcOrigin.size() != 2 || dstOrigin.size() != 2 || region.size() != 2) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    if (!WebCLInputChecker::isValidRegionForImage(srcImage->imageDescriptor(), srcOrigin, region) || !WebCLInputChecker::isValidRegionForImage(dstImage->imageDescriptor(), dstOrigin, region)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    Vector<size_t, 3> srcOriginCopy, dstOriginCopy, regionCopy;
    srcOriginCopy.appendVector(srcOrigin);
    dstOriginCopy.appendVector(dstOrigin);
    regionCopy.appendVector(region);

    // No support for 3D-images, so set default values of 0 for all origin & region arrays at 3rd index.
    srcOriginCopy.append(0);
    dstOriginCopy.append(0);
    regionCopy.append(1);

    Vector<cl_event> clEvents = WebCLEventVectorToCLEventVector(false, events, es);
    if (events.size() && clEvents.size() != events.size())
        return;

    cl_event* clEventId = WebCLEventPtrToCLEventPtr(event, es);
    if (event && !clEventId)
        return;

    cl_int err = clEnqueueCopyImage(m_clCommandQueue, clSrcImageId, clDstImageId, srcOriginCopy.data(), dstOriginCopy.data(), regionCopy.data(), clEvents.size(), clEvents.data(), clEventId);

    if (err != CL_SUCCESS)
        WebCLException::throwException(err, es);
}

void WebCLCommandQueue::enqueueCopyImageToBuffer(WebCLImage* srcImage, WebCLBuffer* dstBuffer, const Vector<unsigned>& srcOrigin, const Vector<unsigned>& region, unsigned offset, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_COMMAND_QUEUE, WebCLException::invalidCommandQueueMessage);
        return;
    }

    cl_mem clSrcImageId = nullptr;
    if (srcImage) {
        clSrcImageId = srcImage->getMem();
        if (!clSrcImageId) {
            es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
            return;
        }
    }

    cl_mem clDstBufferId = nullptr;
    if (dstBuffer) {
        clDstBufferId = dstBuffer->getMem();
        if (!clDstBufferId) {
            es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
            return;
        }
    }

    if (!WebCLInputChecker::compareContext(context().get(), srcImage->context().get()) || !WebCLInputChecker::compareContext(context().get(), dstBuffer->context().get())) {
        es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
        return;
    }

    if (srcOrigin.size() != 2 || region.size() != 2) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    if (!WebCLInputChecker::isValidRegionForBuffer(dstBuffer->sizeInBytes(), region, offset, srcImage->imageDescriptor()) || !WebCLInputChecker::isValidRegionForImage(srcImage->imageDescriptor(), srcOrigin, region)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    Vector<size_t, 3> srcOriginCopy, regionCopy;
    srcOriginCopy.appendVector(srcOrigin);
    regionCopy.appendVector(region);
    // No support for 3D-images, so set default values of 0 for all origin & region arrays at 3rd index.
    srcOriginCopy.append(0);
    regionCopy.append(1);

    Vector<cl_event> clEvents = WebCLEventVectorToCLEventVector(false, events, es);
    if (events.size() && clEvents.size() != events.size())
        return;

    cl_event* clEventId = WebCLEventPtrToCLEventPtr(event, es);
    if (event && !clEventId)
        return;

    cl_int err = clEnqueueCopyImageToBuffer(m_clCommandQueue, clSrcImageId, clDstBufferId, srcOriginCopy.data(), regionCopy.data(), offset, clEvents.size(), clEvents.data(), clEventId);
    if (err != CL_SUCCESS)
        WebCLException::throwException(err, es);
}

void WebCLCommandQueue::enqueueCopyBufferToImage(WebCLBuffer* srcBuffer, WebCLImage* dstImage, unsigned offset, const Vector<unsigned>& dstOrigin, const Vector<unsigned>& region, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_COMMAND_QUEUE, WebCLException::invalidCommandQueueMessage);
        return;
    }

    cl_mem clSrcBufferId = nullptr;
    if (srcBuffer) {
        clSrcBufferId = srcBuffer->getMem();
        if (!clSrcBufferId) {
            es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
            return;
        }
    }

    cl_mem clDstImageId = nullptr;
    if (dstImage) {
        clDstImageId = dstImage->getMem();
        if (!clDstImageId) {
            es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
            return;
        }
    }

    if (!WebCLInputChecker::compareContext(context().get(), srcBuffer->context().get()) || !WebCLInputChecker::compareContext(context().get(), dstImage->context().get())) {
        es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
        return;
    }

    if (dstOrigin.size() != 2 || region.size() != 2) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    if (!WebCLInputChecker::isValidRegionForBuffer(srcBuffer->sizeInBytes(), region, offset, dstImage->imageDescriptor()) || !WebCLInputChecker::isValidRegionForImage(dstImage->imageDescriptor(), dstOrigin, region)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    Vector<size_t, 3> targetOriginCopy, regionCopy;
    targetOriginCopy.appendVector(dstOrigin);
    regionCopy.appendVector(region);

    // No support for 3D-images, so set default values of 0 for all origin & region arrays at 3rd index.
    targetOriginCopy.append(0);
    regionCopy.append(1);

    Vector<cl_event> clEvents = WebCLEventVectorToCLEventVector(false, events, es);
    if (events.size() && clEvents.size() != events.size())
        return;

    cl_event* clEventId = WebCLEventPtrToCLEventPtr(event, es);
    if (event && !clEventId)
        return;

    cl_int err = clEnqueueCopyBufferToImage(m_clCommandQueue, clSrcBufferId, clDstImageId, offset, targetOriginCopy.data(), regionCopy.data(), clEvents.size(), clEvents.data(), clEventId);
    if (err != CL_SUCCESS)
        WebCLException::throwException(err, es);
}

WebCLCommandQueue::WebCLCommandQueue(cl_command_queue commandQueue, PassRefPtr<WebCLContext> context, WebCLDevice* device)
    : WebCLObject(context)
    , m_whenFinishCallback(nullptr)
    , m_eventForCallback(0)
    , m_device(device)
    , m_clCommandQueue(commandQueue)
{
}

Vector<cl_event> WebCLCommandQueue::WebCLEventVectorToCLEventVector(bool blocking, Vector<RefPtr<WebCLEvent>> events, ExceptionState& es)
{
    Vector<cl_event> clEvents;
    for (auto event : events) {
        if (event->isReleased()) {
            es.throwWebCLException(WebCLException::INVALID_EVENT_WAIT_LIST, WebCLException::invalidEventWaitListMessage);
            break;
        }
        if (!WebCLInputChecker::compareContext(context().get(), event->context().get())) {
            es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
            break;
        }
        if (blocking && events[0]->getStatus() == CL_INVALID_VALUE) {
            es.throwWebCLException(WebCLException::EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST, WebCLException::execStatusErrorForEventsInWaitListMessage);
            break;
        }
        if (blocking && (event->isUserEvent() || !event->setAssociatedCommandQueue(this))) {
            es.throwWebCLException(WebCLException::INVALID_EVENT_WAIT_LIST, WebCLException::invalidEventWaitListMessage);
            break;
        }

        clEvents.append(event->getEvent());
    }
    return clEvents;
}

cl_event* WebCLCommandQueue::WebCLEventPtrToCLEventPtr(WebCLEvent* event, ExceptionState& es)
{
    cl_event* clEventId = nullptr;
    if (event) {
        clEventId = event->getEventPtr();
        if (!clEventId || !event->setAssociatedCommandQueue(this)) {
            es.throwWebCLException(WebCLException::INVALID_EVENT, WebCLException::invalidEventMessage);
            return nullptr;
        }
    }
    return clEventId;
}

bool WebCLCommandQueue::isExtensionEnabled(WebCLContext* context, const String& name) const
{
    return context->isExtensionEnabled(name);
}

void WebCLCommandQueue::callbackProxy(cl_event event, cl_int type, void* userData)
{
    OwnPtr<WebCLCommandQueueHolder> holder = adoptPtr(static_cast<WebCLCommandQueueHolder*>(userData));
    holder->event = event;
    holder->type = type;

    if (!isMainThread()) {
        Platform::current()->mainThread()->taskRunner()->postTask(BLINK_FROM_HERE, threadSafeBind(&WebCLCommandQueue::callbackProxyOnMainThread, holder.release()));
        return;
    }

    callbackProxyOnMainThread(holder.release());
}

void WebCLCommandQueue::callbackProxyOnMainThread(PassOwnPtr<WebCLCommandQueueHolder> holder)
{
    ASSERT(isMainThread());
    RefPtr<WebCLCommandQueue> webCommandQueue(static_cast<WebCLCommandQueue*>(holder->commandQueue.get()));
    cl_event event = holder->event;
    cl_int type = holder->type;

    if (!webCommandQueue)
        return;

    // Ignore the callback if the WebCLCommandQueue is released or OpenCL event is abnormally terminated.
    // It's possible for its OpenCL object to not be released yet.
    if (type != CL_COMPLETE || webCommandQueue->isReleased()) {
        webCommandQueue->resetEventAndCallback();
        return;
    }

    cl_event clEvent = webCommandQueue->m_eventForCallback;
    if (event == clEvent) {
        if (webCommandQueue->m_whenFinishCallback) {
            webCommandQueue->m_whenFinishCallback->handleEvent();
        }

        webCommandQueue->resetEventAndCallback();
    }
}

void WebCLCommandQueue::resetEventAndCallback()
{
    if (m_eventForCallback)
        clReleaseEvent(m_eventForCallback);

    m_eventForCallback = 0;
    m_whenFinishCallback.clear();
}

} // namespace blink

#endif // ENABLE(WEBCL)
