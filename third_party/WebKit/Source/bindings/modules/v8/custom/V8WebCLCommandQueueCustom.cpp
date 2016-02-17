// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "wtf/build_config.h"

#if ENABLE(WEBCL)
#include "bindings/core/v8/V8ArrayBufferView.h"
#include "bindings/core/v8/V8HTMLCanvasElement.h"
#include "bindings/core/v8/V8HTMLImageElement.h"
#include "bindings/core/v8/V8HTMLVideoElement.h"
#include "bindings/core/v8/V8ImageData.h"
#include "bindings/modules/v8/V8WebCLBuffer.h"
#include "bindings/modules/v8/V8WebCLCallback.h"
#include "bindings/modules/v8/V8WebCLCommandQueue.h"
#include "bindings/modules/v8/V8WebCLEvent.h"
#include "bindings/modules/v8/V8WebCLImage.h"
#include "bindings/modules/v8/V8WebCLKernel.h"
#include "bindings/modules/v8/V8WebCLMemoryObject.h"

namespace blink {

void V8WebCLCommandQueue::enqueueCopyBufferMethodCustom(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueCopyBuffer", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 5)) {
        setMinimumArityTypeError(exceptionState, 5, info.Length());
        exceptionState.throwIfNeeded();
        return;
    }

    WebCLCommandQueue* impl = V8WebCLCommandQueue::toImpl(info.Holder());
    WebCLBuffer* srcBuffer;
    WebCLBuffer* dstBuffer;
    unsigned srcOffset;
    unsigned dstOffset;
    unsigned numBytes;
    Vector<RefPtr<WebCLEvent>> eventWaitList;
    WebCLEvent* event = nullptr;
    {
        if (info.Length() > 0 && !V8WebCLBuffer::hasInstance(info[0], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 1 is not of type 'WebCLBuffer'.");
            exceptionState.throwIfNeeded();
            return;
        }

        srcBuffer = V8WebCLBuffer::toImplWithTypeCheck(info.GetIsolate(), info[0]);
        if (info.Length() > 1 && !V8WebCLBuffer::hasInstance(info[1], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 2 is not of type 'WebCLBuffer'.");
            exceptionState.throwIfNeeded();
            return;
        }

        dstBuffer = V8WebCLBuffer::toImplWithTypeCheck(info.GetIsolate(), info[1]);

        srcOffset = toUInt32(info.GetIsolate(), info[2], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        dstOffset = toUInt32(info.GetIsolate(), info[3], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        numBytes = toUInt32(info.GetIsolate(), info[4], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        if (info.Length() > 5 && !isUndefinedOrNull(info[5])) {
            eventWaitList = toRefPtrNativeArray<WebCLEvent, V8WebCLEvent>(info[5], 6, info.GetIsolate(), exceptionState);
            if(exceptionState.throwIfNeeded())
                return;
        }
        if (info.Length() > 6) {
            if (!isUndefinedOrNull(info[6]) && !V8WebCLEvent::hasInstance(info[6], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 7 is not of type 'WebCLEvent'.");
                exceptionState.throwIfNeeded();
                return;
            }
            event = V8WebCLEvent::toImplWithTypeCheck(info.GetIsolate(), info[6]);
        }
    }

    impl->enqueueCopyBuffer(srcBuffer, dstBuffer, srcOffset, dstOffset, numBytes, eventWaitList, event, exceptionState);
    if (exceptionState.hadException())
        exceptionState.throwIfNeeded();
}

void V8WebCLCommandQueue::enqueueCopyBufferRectMethodCustom(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueCopyBufferRect", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 9)) {
        setMinimumArityTypeError(exceptionState, 9, info.Length());
        exceptionState.throwIfNeeded();
        return;
    }

    WebCLCommandQueue* impl = V8WebCLCommandQueue::toImpl(info.Holder());
    WebCLBuffer* srcBuffer;
    WebCLBuffer* dstBuffer;
    Vector<unsigned> srcOrigin;
    Vector<unsigned> dstOrigin;
    Vector<unsigned> region;
    unsigned srcRowPitch;
    unsigned srcSlicePitch;
    unsigned dstRowPitch;
    unsigned dstSlicePitch;
    Vector<RefPtr<WebCLEvent>> eventWaitList;
    WebCLEvent* event = nullptr;
    {
        if (info.Length() > 0 && !V8WebCLBuffer::hasInstance(info[0], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 1 is not of type 'WebCLBuffer'.");
            exceptionState.throwIfNeeded();
            return;
        }

        srcBuffer = V8WebCLBuffer::toImplWithTypeCheck(info.GetIsolate(), info[0]);
        if (info.Length() > 1 && !V8WebCLBuffer::hasInstance(info[1], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 2 is not of type 'WebCLBuffer'.");
            exceptionState.throwIfNeeded();
            return;
        }

        dstBuffer = V8WebCLBuffer::toImplWithTypeCheck(info.GetIsolate(), info[1]);

        srcOrigin = toImplArray<Vector<unsigned>>(info[2], 3, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        dstOrigin = toImplArray<Vector<unsigned>>(info[3], 4, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        region = toImplArray<Vector<unsigned>>(info[4], 5, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        srcRowPitch = toUInt32(info.GetIsolate(), info[5], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        srcSlicePitch = toUInt32(info.GetIsolate(), info[6], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        dstRowPitch = toUInt32(info.GetIsolate(), info[7], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        dstSlicePitch = toUInt32(info.GetIsolate(), info[8], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        if (info.Length() > 9 && !isUndefinedOrNull(info[9])) {
            eventWaitList = toRefPtrNativeArray<WebCLEvent, V8WebCLEvent>(info[9], 10, info.GetIsolate(), exceptionState);
            if(exceptionState.throwIfNeeded())
                return;
        }

        if (info.Length() > 10) {
            if (!isUndefinedOrNull(info[10]) && !V8WebCLEvent::hasInstance(info[10], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 11 is not of type 'WebCLEvent'.");
                exceptionState.throwIfNeeded();
                return;
            }

            event = V8WebCLEvent::toImplWithTypeCheck(info.GetIsolate(), info[10]);
        }
    }
    impl->enqueueCopyBufferRect(srcBuffer, dstBuffer, srcOrigin, dstOrigin, region, srcRowPitch, srcSlicePitch, dstRowPitch, dstSlicePitch, eventWaitList, event, exceptionState);
    if (exceptionState.hadException())
        exceptionState.throwIfNeeded();
}

void V8WebCLCommandQueue::enqueueCopyImageMethodCustom(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueCopyImage", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 5)) {
        setMinimumArityTypeError(exceptionState, 5, info.Length());
        exceptionState.throwIfNeeded();
        return;
    }

    WebCLCommandQueue* impl = V8WebCLCommandQueue::toImpl(info.Holder());
    WebCLImage* srcImage;
    WebCLImage* dstImage;
    Vector<unsigned> srcOrigin;
    Vector<unsigned> dstOrigin;
    Vector<unsigned> region;
    Vector<RefPtr<WebCLEvent>> eventWaitList;
    WebCLEvent* event = nullptr;
    {
        if (info.Length() > 0 && !V8WebCLImage::hasInstance(info[0], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 1 is not of type 'WebCLImage'.");
            exceptionState.throwIfNeeded();
            return;
        }

        srcImage = V8WebCLImage::toImplWithTypeCheck(info.GetIsolate(), info[0]);
        if (info.Length() > 1 && !V8WebCLImage::hasInstance(info[1], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 2 is not of type 'WebCLImage'.");
            exceptionState.throwIfNeeded();
            return;
        }

        dstImage = V8WebCLImage::toImplWithTypeCheck(info.GetIsolate(), info[1]);
        srcOrigin = toImplArray<Vector<unsigned>>(info[2], 3, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        dstOrigin = toImplArray<Vector<unsigned>>(info[3], 4, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        region = toImplArray<Vector<unsigned>>(info[4], 5, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        if (info.Length() > 5 && !isUndefinedOrNull(info[5])) {
            eventWaitList = toRefPtrNativeArray<WebCLEvent, V8WebCLEvent>(info[5], 6, info.GetIsolate(), exceptionState);
            if(exceptionState.throwIfNeeded())
                return;
        }

        if (info.Length() > 6) {
            if (!isUndefinedOrNull(info[6]) && !V8WebCLEvent::hasInstance(info[6], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 7 is not of type 'WebCLEvent'.");
                exceptionState.throwIfNeeded();
                return;
            }

            event = V8WebCLEvent::toImplWithTypeCheck(info.GetIsolate(), info[6]);
        }
    }

    impl->enqueueCopyImage(srcImage, dstImage, srcOrigin, dstOrigin, region, eventWaitList, event, exceptionState);
    if (exceptionState.hadException())
        exceptionState.throwIfNeeded();
}

void V8WebCLCommandQueue::enqueueCopyImageToBufferMethodCustom(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueCopyImageToBuffer", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 5)) {
        setMinimumArityTypeError(exceptionState, 5, info.Length());
        exceptionState.throwIfNeeded();
        return;
    }

    WebCLCommandQueue* impl = V8WebCLCommandQueue::toImpl(info.Holder());
    WebCLImage* srcImage;
    WebCLBuffer* dstBuffer;
    Vector<unsigned> srcOrigin;
    Vector<unsigned> srcRegion;
    unsigned dstOffset;
    Vector<RefPtr<WebCLEvent>> eventWaitList;
    WebCLEvent* event = nullptr;
    {
        if (info.Length() > 0 && !V8WebCLImage::hasInstance(info[0], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 1 is not of type 'WebCLImage'.");
            exceptionState.throwIfNeeded();
            return;
        }

        srcImage = V8WebCLImage::toImplWithTypeCheck(info.GetIsolate(), info[0]);
        if (info.Length() > 1 && !V8WebCLBuffer::hasInstance(info[1], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 2 is not of type 'WebCLBuffer'.");
            exceptionState.throwIfNeeded();
            return;
        }

        dstBuffer = V8WebCLBuffer::toImplWithTypeCheck(info.GetIsolate(), info[1]);

        srcOrigin = toImplArray<Vector<unsigned>>(info[2], 3, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        srcRegion = toImplArray<Vector<unsigned>>(info[3], 4, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        dstOffset = toUInt32(info.GetIsolate(), info[4], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        if (info.Length() > 5 && !isUndefinedOrNull(info[5])) {
            eventWaitList = toRefPtrNativeArray<WebCLEvent, V8WebCLEvent>(info[5], 6, info.GetIsolate(), exceptionState);
            if(exceptionState.throwIfNeeded())
                return;
        }
        if (info.Length() > 6) {
            if (!isUndefinedOrNull(info[6]) && !V8WebCLEvent::hasInstance(info[6], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 7 is not of type 'WebCLEvent'.");
                exceptionState.throwIfNeeded();
                return;
            }

            event = V8WebCLEvent::toImplWithTypeCheck(info.GetIsolate(), info[6]);
        }
    }

    impl->enqueueCopyImageToBuffer(srcImage, dstBuffer, srcOrigin, srcRegion, dstOffset, eventWaitList, event, exceptionState);
    if (exceptionState.hadException())
        exceptionState.throwIfNeeded();
}

void V8WebCLCommandQueue::enqueueCopyBufferToImageMethodCustom(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueCopyBufferToImage", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 5)) {
        setMinimumArityTypeError(exceptionState, 5, info.Length());
        exceptionState.throwIfNeeded();
        return;
    }

    WebCLCommandQueue* impl = V8WebCLCommandQueue::toImpl(info.Holder());
    WebCLBuffer* srcBuffer;
    WebCLImage* dstImage;
    unsigned srcOffset;
    Vector<unsigned> dstOrigin;
    Vector<unsigned> dstRegion;
    Vector<RefPtr<WebCLEvent>> eventWaitList;
    WebCLEvent* event = nullptr;
    {
        if (info.Length() > 0 && !V8WebCLBuffer::hasInstance(info[0], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 1 is not of type 'WebCLBuffer'.");
            exceptionState.throwIfNeeded();
            return;
        }

        srcBuffer = V8WebCLBuffer::toImplWithTypeCheck(info.GetIsolate(), info[0]);
        if (info.Length() > 1 && !V8WebCLImage::hasInstance(info[1], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 2 is not of type 'WebCLImage'.");
            exceptionState.throwIfNeeded();
            return;
        }

        dstImage = V8WebCLImage::toImplWithTypeCheck(info.GetIsolate(), info[1]);

        srcOffset = toUInt32(info.GetIsolate(), info[2], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        dstOrigin = toImplArray<Vector<unsigned>>(info[3], 4, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        dstRegion = toImplArray<Vector<unsigned>>(info[4], 5, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        if (info.Length() > 5 && !isUndefinedOrNull(info[5])) {
            eventWaitList = toRefPtrNativeArray<WebCLEvent, V8WebCLEvent>(info[5], 6, info.GetIsolate(), exceptionState);
            if(exceptionState.throwIfNeeded())
                return;
        }

        if (info.Length() > 6) {
            if (!isUndefinedOrNull(info[6]) && !V8WebCLEvent::hasInstance(info[6], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 7 is not of type 'WebCLEvent'.");
                exceptionState.throwIfNeeded();
                return;
            }

            event = V8WebCLEvent::toImplWithTypeCheck(info.GetIsolate(), info[6]);
        }
    }

    impl->enqueueCopyBufferToImage(srcBuffer, dstImage, srcOffset, dstOrigin, dstRegion, eventWaitList, event, exceptionState);
    if (exceptionState.hadException())
        exceptionState.throwIfNeeded();
}

static void enqueueReadBuffer1Method(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueReadBuffer", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 5)) {
        setMinimumArityTypeError(exceptionState, 5, info.Length());
        exceptionState.throwIfNeeded();
        return;
    }

    WebCLCommandQueue* impl = V8WebCLCommandQueue::toImpl(info.Holder());
    WebCLBuffer* buffer;
    bool blockingRead;
    unsigned bufferOffset;
    unsigned numBytes;
    HTMLCanvasElement* canvas;
    Vector<RefPtr<WebCLEvent>> eventWaitList;
    WebCLEvent* event = nullptr;
    {
        if (info.Length() > 0 && !V8WebCLBuffer::hasInstance(info[0], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 1 is not of type 'WebCLBuffer'.");
            exceptionState.throwIfNeeded();
            return;
        }

        buffer = V8WebCLBuffer::toImplWithTypeCheck(info.GetIsolate(), info[0]);
        blockingRead = info[1]->BooleanValue();

        bufferOffset = toUInt32(info.GetIsolate(), info[2], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        numBytes = toUInt32(info.GetIsolate(), info[3], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        if (info.Length() > 4 && !V8HTMLCanvasElement::hasInstance(info[4], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 5 is not of type 'HTMLCanvasElement'.");
            exceptionState.throwIfNeeded();
            return;
        }

        canvas = V8HTMLCanvasElement::toImplWithTypeCheck(info.GetIsolate(), info[4]);
        if (info.Length() > 5 && !isUndefinedOrNull(info[5])) {
            eventWaitList = toRefPtrNativeArray<WebCLEvent, V8WebCLEvent>(info[5], 6, info.GetIsolate(), exceptionState);
            if(exceptionState.throwIfNeeded())
                return;
        }

        if (info.Length() > 6) {
            if (!isUndefinedOrNull(info[6]) && !V8WebCLEvent::hasInstance(info[6], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 7 is not of type 'WebCLEvent'.");
                exceptionState.throwIfNeeded();
                return;
            }

            event = V8WebCLEvent::toImplWithTypeCheck(info.GetIsolate(), info[6]);
        }
    }

    impl->enqueueReadBuffer(buffer, blockingRead, bufferOffset, numBytes, canvas, eventWaitList, event, exceptionState);
    if (exceptionState.hadException())
        exceptionState.throwIfNeeded();
}

static void enqueueReadBuffer2Method(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueReadBuffer", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 5)) {
        setMinimumArityTypeError(exceptionState, 5, info.Length());
        exceptionState.throwIfNeeded();
        return;
    }

    WebCLCommandQueue* impl = V8WebCLCommandQueue::toImpl(info.Holder());
    WebCLBuffer* buffer;
    bool blockingRead;
    unsigned bufferOffset;
    unsigned numBytes;
    DOMArrayBufferView* hostPtr;
    Vector<RefPtr<WebCLEvent>> eventWaitList;
    WebCLEvent* event = nullptr;
    {
        if (info.Length() > 0 && !V8WebCLBuffer::hasInstance(info[0], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 1 is not of type 'WebCLBuffer'.");
            exceptionState.throwIfNeeded();
            return;
        }

        buffer = V8WebCLBuffer::toImplWithTypeCheck(info.GetIsolate(), info[0]);
        blockingRead = info[1]->BooleanValue();
        bufferOffset = toUInt32(info.GetIsolate(), info[2], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        numBytes = toUInt32(info.GetIsolate(), info[3], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        if (info.Length() > 4 && !V8ArrayBufferView::hasInstance(info[4], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 5 is not of type 'ArrayBufferView'.");
            exceptionState.throwIfNeeded();
            return;
        }

        hostPtr = info[4]->IsArrayBufferView() ? V8ArrayBufferView::toImpl(v8::Handle<v8::ArrayBufferView>::Cast(info[4])) : 0;
        if (info.Length() > 5 && !isUndefinedOrNull(info[5])) {
            eventWaitList = toRefPtrNativeArray<WebCLEvent, V8WebCLEvent>(info[5], 6, info.GetIsolate(), exceptionState);
            if(exceptionState.throwIfNeeded())
                return;
        }

        if (info.Length() > 6) {
            if (!isUndefinedOrNull(info[6]) && !V8WebCLEvent::hasInstance(info[6], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 7 is not of type 'WebCLEvent'.");
                exceptionState.throwIfNeeded();
                return;
            }

            event = V8WebCLEvent::toImplWithTypeCheck(info.GetIsolate(), info[6]);
        }
    }

    impl->enqueueReadBuffer(buffer, blockingRead, bufferOffset, numBytes, hostPtr, eventWaitList, event, exceptionState);
    if (exceptionState.hadException())
        exceptionState.throwIfNeeded();
}

void V8WebCLCommandQueue::enqueueReadBufferMethodCustom(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueReadBuffer", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    switch (std::min(5, info.Length())) {
    case 5:
        if (V8HTMLCanvasElement::hasInstance(info[4], info.GetIsolate())) {
            enqueueReadBuffer1Method(info);
            return;
        }

        enqueueReadBuffer2Method(info);
        return;
    default:
        exceptionState.throwTypeError(ExceptionMessages::notEnoughArguments(5, info.Length()));
        exceptionState.throwIfNeeded();
        return;
    }

    exceptionState.throwWebCLException(WebCLException::INVALID_ARG_VALUE, WebCLException::invalidArgValueMessage);
    exceptionState.throwIfNeeded();
}

static void enqueueReadBufferRect1Method(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueReadBufferRect", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 8)) {
        setMinimumArityTypeError(exceptionState, 8, info.Length());
        exceptionState.throwIfNeeded();
        return;
    }

    WebCLCommandQueue* impl = V8WebCLCommandQueue::toImpl(info.Holder());
    WebCLBuffer* buffer;
    bool blockingRead;
    Vector<unsigned> bufferOrigin;
    Vector<unsigned> hostOrigin;
    Vector<unsigned> region;
    unsigned hostRowPitch;
    unsigned hostSlicePitch;
    HTMLCanvasElement* canvas;
    Vector<RefPtr<WebCLEvent>> eventWaitList;
    WebCLEvent* event = nullptr;
    {
        if (info.Length() > 0 && !V8WebCLBuffer::hasInstance(info[0], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 1 is not of type 'WebCLBuffer'.");
            exceptionState.throwIfNeeded();
            return;
        }

        buffer = V8WebCLBuffer::toImplWithTypeCheck(info.GetIsolate(), info[0]);
        blockingRead = info[1]->BooleanValue();
        bufferOrigin = toImplArray<Vector<unsigned>>(info[2], 3, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        hostOrigin = toImplArray<Vector<unsigned>>(info[3], 4, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        region = toImplArray<Vector<unsigned>>(info[4], 5, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        hostRowPitch = toUInt32(info.GetIsolate(), info[5], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        hostSlicePitch = toUInt32(info.GetIsolate(), info[6], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        if (info.Length() > 7 && !V8HTMLCanvasElement::hasInstance(info[7], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 8 is not of type 'HTMLCanvasElement'.");
            exceptionState.throwIfNeeded();
            return;
        }

        canvas = V8HTMLCanvasElement::toImplWithTypeCheck(info.GetIsolate(), info[7]);
        if (info.Length() > 8 && !isUndefinedOrNull(info[8])) {
            eventWaitList = toRefPtrNativeArray<WebCLEvent, V8WebCLEvent>(info[8], 9, info.GetIsolate(), exceptionState);
            if(exceptionState.throwIfNeeded())
                return;
        }

        if (info.Length() > 9) {
            if (!isUndefinedOrNull(info[9]) && !V8WebCLEvent::hasInstance(info[9], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 10 is not of type 'WebCLEvent'.");
                exceptionState.throwIfNeeded();
                return;
            }

            event = V8WebCLEvent::toImplWithTypeCheck(info.GetIsolate(), info[9]);
        }
    }

    impl->enqueueReadBufferRect(buffer, blockingRead, bufferOrigin, hostOrigin, region, hostRowPitch, hostSlicePitch, canvas, eventWaitList, event, exceptionState);
    if (exceptionState.hadException())
        exceptionState.throwIfNeeded();
}

static void enqueueReadBufferRect2Method(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueReadBufferRect", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 10)) {
        setMinimumArityTypeError(exceptionState, 10, info.Length());
        exceptionState.throwIfNeeded();
        return;
    }

    WebCLCommandQueue* impl = V8WebCLCommandQueue::toImpl(info.Holder());
    WebCLBuffer* buffer;
    bool blockingRead;
    Vector<unsigned> bufferOrigin;
    Vector<unsigned> hostOrigin;
    Vector<unsigned> region;
    unsigned bufferRowPitch;
    unsigned bufferSlicePitch;
    unsigned hostRowPitch;
    unsigned hostSlicePitch;
    DOMArrayBufferView* hostPtr;
    Vector<RefPtr<WebCLEvent>> eventWaitList;
    WebCLEvent* event = nullptr;
    {
        if (info.Length() > 0 && !V8WebCLBuffer::hasInstance(info[0], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 1 is not of type 'WebCLBuffer'.");
            exceptionState.throwIfNeeded();
            return;
        }

        buffer = V8WebCLBuffer::toImplWithTypeCheck(info.GetIsolate(), info[0]);
        blockingRead = info[1]->BooleanValue();
        bufferOrigin = toImplArray<Vector<unsigned>>(info[2], 3, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        hostOrigin = toImplArray<Vector<unsigned>>(info[3], 4, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        region = toImplArray<Vector<unsigned>>(info[4], 5, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        bufferRowPitch = toUInt32(info.GetIsolate(), info[5], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        bufferSlicePitch = toUInt32(info.GetIsolate(), info[6], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        hostRowPitch = toUInt32(info.GetIsolate(), info[7], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        hostSlicePitch = toUInt32(info.GetIsolate(), info[8], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        if (info.Length() > 9 && !V8ArrayBufferView::hasInstance(info[9], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 10 is not of type 'ArrayBufferView'.");
            exceptionState.throwIfNeeded();
            return;
        }

        hostPtr = info[9]->IsArrayBufferView() ? V8ArrayBufferView::toImpl(v8::Handle<v8::ArrayBufferView>::Cast(info[9])) : 0;
        if (info.Length() > 10 && !isUndefinedOrNull(info[10])) {
            eventWaitList = toRefPtrNativeArray<WebCLEvent, V8WebCLEvent>(info[10], 11, info.GetIsolate(), exceptionState);
            if(exceptionState.throwIfNeeded())
                return;
        }

        if (info.Length() > 11) {
            if (!isUndefinedOrNull(info[11]) && !V8WebCLEvent::hasInstance(info[11], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 12 is not of type 'WebCLEvent'.");
                exceptionState.throwIfNeeded();
                return;
            }

            event = V8WebCLEvent::toImplWithTypeCheck(info.GetIsolate(), info[11]);
        }
    }

    impl->enqueueReadBufferRect(buffer, blockingRead, bufferOrigin, hostOrigin, region, bufferRowPitch, bufferSlicePitch, hostRowPitch, hostSlicePitch, hostPtr, eventWaitList, event, exceptionState);
    if (exceptionState.hadException())
        exceptionState.throwIfNeeded();
}

void V8WebCLCommandQueue::enqueueReadBufferRectMethodCustom(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueReadBufferRect", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    switch (std::min(8, info.Length())) {
    case 8:
        if (V8HTMLCanvasElement::hasInstance(info[7], info.GetIsolate())) {
            enqueueReadBufferRect1Method(info);
            return;
        }

        enqueueReadBufferRect2Method(info);
        return;
    default:
        exceptionState.throwTypeError(ExceptionMessages::notEnoughArguments(8, info.Length()));
        exceptionState.throwIfNeeded();
        return;
    }

    exceptionState.throwWebCLException(WebCLException::INVALID_ARG_VALUE, WebCLException::invalidArgValueMessage);
    exceptionState.throwIfNeeded();
}

static void enqueueReadImage1Method(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueReadImage", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 5)) {
        setMinimumArityTypeError(exceptionState, 5, info.Length());
        exceptionState.throwIfNeeded();
        return;
    }

    WebCLCommandQueue* impl = V8WebCLCommandQueue::toImpl(info.Holder());
    WebCLImage* image;
    bool blockingRead;
    Vector<unsigned> origin;
    Vector<unsigned> region;
    HTMLCanvasElement* canvas;
    Vector<RefPtr<WebCLEvent>> eventWaitList;
    WebCLEvent* event = nullptr;
    {
        if (info.Length() > 0 && !V8WebCLImage::hasInstance(info[0], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 1 is not of type 'WebCLImage'.");
            exceptionState.throwIfNeeded();
            return;
        }

        image = V8WebCLImage::toImplWithTypeCheck(info.GetIsolate(), info[0]);
        blockingRead = info[1]->BooleanValue();
        origin = toImplArray<Vector<unsigned>>(info[2], 3, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        region = toImplArray<Vector<unsigned>>(info[3], 4, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        if (info.Length() > 4 && !V8HTMLCanvasElement::hasInstance(info[4], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 5 is not of type 'HTMLCanvasElement'.");
            exceptionState.throwIfNeeded();
            return;
        }

        canvas = V8HTMLCanvasElement::toImplWithTypeCheck(info.GetIsolate(), info[4]);
        if (info.Length() > 5 && !isUndefinedOrNull(info[5])) {
            eventWaitList = toRefPtrNativeArray<WebCLEvent, V8WebCLEvent>(info[5], 6, info.GetIsolate(), exceptionState);
            if(exceptionState.throwIfNeeded())
                return;
        }


        if (info.Length() > 6) {
            if (!isUndefinedOrNull(info[6]) && !V8WebCLEvent::hasInstance(info[6], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 7 is not of type 'WebCLEvent'.");
                exceptionState.throwIfNeeded();
                return;
            }

            event = V8WebCLEvent::toImplWithTypeCheck(info.GetIsolate(), info[6]);
        }
    }

    impl->enqueueReadImage(image, blockingRead, origin, region, canvas, eventWaitList, event, exceptionState);
    if (exceptionState.hadException())
        exceptionState.throwIfNeeded();
}

static void enqueueReadImage2Method(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueReadImage", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 6)) {
        setMinimumArityTypeError(exceptionState, 6, info.Length());
        exceptionState.throwIfNeeded();
        return;
    }

    WebCLCommandQueue* impl = V8WebCLCommandQueue::toImpl(info.Holder());
    WebCLImage* image;
    bool blockingRead;
    Vector<unsigned> origin;
    Vector<unsigned> region;
    unsigned hostRowPitch;
    DOMArrayBufferView* hostPtr;
    Vector<RefPtr<WebCLEvent>> eventWaitList;
    WebCLEvent* event = nullptr;
    {
        if (info.Length() > 0 && !V8WebCLImage::hasInstance(info[0], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 1 is not of type 'WebCLImage'.");
            exceptionState.throwIfNeeded();
            return;
        }

        image = V8WebCLImage::toImplWithTypeCheck(info.GetIsolate(), info[0]);
        blockingRead = info[1]->BooleanValue();
        origin = toImplArray<Vector<unsigned>>(info[2], 3, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        region = toImplArray<Vector<unsigned>>(info[3], 4, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        hostRowPitch = toUInt32(info.GetIsolate(), info[4], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        if (info.Length() > 5 && !V8ArrayBufferView::hasInstance(info[5], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 6 is not of type 'ArrayBufferView'.");
            exceptionState.throwIfNeeded();
            return;
        }

        hostPtr = info[5]->IsArrayBufferView() ? V8ArrayBufferView::toImpl(v8::Handle<v8::ArrayBufferView>::Cast(info[5])) : 0;
        if (info.Length() > 6 && !isUndefinedOrNull(info[6])) {
            eventWaitList = toRefPtrNativeArray<WebCLEvent, V8WebCLEvent>(info[6], 7, info.GetIsolate(), exceptionState);
            if(exceptionState.throwIfNeeded())
                return;
        }

        if (info.Length() > 7) {
            if (!isUndefinedOrNull(info[7]) && !V8WebCLEvent::hasInstance(info[7], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 8 is not of type 'WebCLEvent'.");
                exceptionState.throwIfNeeded();
                return;
            }

            event = V8WebCLEvent::toImplWithTypeCheck(info.GetIsolate(), info[7]);
        }
    }
    impl->enqueueReadImage(image, blockingRead, origin, region, hostRowPitch, hostPtr, eventWaitList, event, exceptionState);
    if (exceptionState.hadException())
        exceptionState.throwIfNeeded();
}

void V8WebCLCommandQueue::enqueueReadImageMethodCustom(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueReadImage", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    switch (std::min(5, info.Length())) {
    case 5:
        if (V8HTMLCanvasElement::hasInstance(info[4], info.GetIsolate())) {
            enqueueReadImage1Method(info);
            return;
        }
        enqueueReadImage2Method(info);
        return;
    default:
        exceptionState.throwTypeError(ExceptionMessages::notEnoughArguments(5, info.Length()));
        exceptionState.throwIfNeeded();
        return;
    }

    exceptionState.throwWebCLException(WebCLException::INVALID_ARG_VALUE, WebCLException::invalidArgValueMessage);
    exceptionState.throwIfNeeded();
}

static void enqueueWriteBuffer1Method(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueWriteBuffer", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 4)) {
        setMinimumArityTypeError(exceptionState, 4, info.Length());
        exceptionState.throwIfNeeded();
        return;
    }

    WebCLCommandQueue* impl = V8WebCLCommandQueue::toImpl(info.Holder());
    WebCLBuffer* buffer;
    bool blockingWrite;
    unsigned bufferOffset;
    ImageData* imageData = nullptr;
    HTMLCanvasElement* canvas = nullptr;
    HTMLImageElement* image = nullptr;
    Vector<RefPtr<WebCLEvent>> eventWaitList;
    WebCLEvent* event = nullptr;
    {
        if (info.Length() > 0 && !V8WebCLBuffer::hasInstance(info[0], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 1 is not of type 'WebCLBuffer'.");
            exceptionState.throwIfNeeded();
            return;
        }

        buffer = V8WebCLBuffer::toImplWithTypeCheck(info.GetIsolate(), info[0]);
        blockingWrite = info[1]->BooleanValue();
        bufferOffset = toUInt32(info.GetIsolate(), info[2], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        if (V8ImageData::hasInstance(info[3], info.GetIsolate())) {
            if (info.Length() > 3 && !V8ImageData::hasInstance(info[3], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 4 is not of type 'ImageData'.");
                exceptionState.throwIfNeeded();
                return;
            }

            imageData = V8ImageData::toImplWithTypeCheck(info.GetIsolate(), info[3]);
        } else if (V8HTMLCanvasElement::hasInstance(info[3], info.GetIsolate())) {
            if (info.Length() > 3 && !V8HTMLCanvasElement::hasInstance(info[3], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 4 is not of type 'HTMLCanvasElement'.");
                exceptionState.throwIfNeeded();
                return;
            }

            canvas = V8HTMLCanvasElement::toImplWithTypeCheck(info.GetIsolate(), info[3]);
        } else if (V8HTMLImageElement::hasInstance(info[3], info.GetIsolate())) {
            if (info.Length() > 3 && !V8HTMLImageElement::hasInstance(info[3], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 4 is not of type 'HTMLImageElement'.");
                exceptionState.throwIfNeeded();
                return;
            }

            image = V8HTMLImageElement::toImplWithTypeCheck(info.GetIsolate(), info[3]);
        }

        if (info.Length() > 4 && !isUndefinedOrNull(info[4])) {
            eventWaitList = toRefPtrNativeArray<WebCLEvent, V8WebCLEvent>(info[4], 5, info.GetIsolate(), exceptionState);
            if(exceptionState.throwIfNeeded())
                return;
        }

        if (info.Length() > 5) {
            if (!isUndefinedOrNull(info[5]) && !V8WebCLEvent::hasInstance(info[5], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 6 is not of type 'WebCLEvent'.");
                exceptionState.throwIfNeeded();
                return;
            }

            event = V8WebCLEvent::toImplWithTypeCheck(info.GetIsolate(), info[5]);
        }
    }

    if (V8ImageData::hasInstance(info[3], info.GetIsolate()))
        impl->enqueueWriteBuffer(buffer, blockingWrite, bufferOffset, imageData, eventWaitList, event, exceptionState);
    else if (V8HTMLCanvasElement::hasInstance(info[3], info.GetIsolate()))
        impl->enqueueWriteBuffer(buffer, blockingWrite, bufferOffset, canvas, eventWaitList, event, exceptionState);
    else if (V8HTMLImageElement::hasInstance(info[3], info.GetIsolate()))
        impl->enqueueWriteBuffer(buffer, blockingWrite, bufferOffset, image, eventWaitList, event, exceptionState);

    if (exceptionState.hadException())
        exceptionState.throwIfNeeded();
}

static void enqueueWriteBuffer2Method(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueWriteBuffer", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 5)) {
        setMinimumArityTypeError(exceptionState, 5, info.Length());
        exceptionState.throwIfNeeded();
        return;
    }

    WebCLCommandQueue* impl = V8WebCLCommandQueue::toImpl(info.Holder());
    WebCLBuffer* bufferId;
    bool blockingWrite;
    unsigned bufferOffset;
    unsigned numBytes;
    DOMArrayBufferView* hostPtr;
    Vector<RefPtr<WebCLEvent>> eventWaitList;
    WebCLEvent* event = nullptr;
    {
        if (info.Length() > 0 && !V8WebCLBuffer::hasInstance(info[0], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 1 is not of type 'WebCLBuffer'.");
            exceptionState.throwIfNeeded();
            return;
        }

        bufferId = V8WebCLBuffer::toImplWithTypeCheck(info.GetIsolate(), info[0]);
        blockingWrite = info[1]->BooleanValue();
        bufferOffset = toUInt32(info.GetIsolate(), info[2], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        numBytes = toUInt32(info.GetIsolate(), info[3], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        if (info.Length() > 4 && !V8ArrayBufferView::hasInstance(info[4], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 5 is not of type 'ArrayBufferView'.");
            exceptionState.throwIfNeeded();
            return;
        }

        hostPtr = info[4]->IsArrayBufferView() ? V8ArrayBufferView::toImpl(v8::Handle<v8::ArrayBufferView>::Cast(info[4])) : 0;
        if (info.Length() > 5 && !isUndefinedOrNull(info[5])) {
            eventWaitList = toRefPtrNativeArray<WebCLEvent, V8WebCLEvent>(info[5], 6, info.GetIsolate(), exceptionState);
            if(exceptionState.throwIfNeeded())
                return;
        }

        if (info.Length() > 6) {
            if (!isUndefinedOrNull(info[6]) && !V8WebCLEvent::hasInstance(info[6], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 7 is not of type 'WebCLEvent'.");
                exceptionState.throwIfNeeded();
                return;
            }

            event = V8WebCLEvent::toImplWithTypeCheck(info.GetIsolate(), info[6]);
        }
    }

    impl->enqueueWriteBuffer(bufferId, blockingWrite, bufferOffset, numBytes, hostPtr, eventWaitList, event, exceptionState);
    if (exceptionState.hadException())
        exceptionState.throwIfNeeded();
}

void V8WebCLCommandQueue::enqueueWriteBufferMethodCustom(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueWriteBuffer", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    switch (std::min(4, info.Length())) {
    case 4:
        if (V8ImageData::hasInstance(info[3], info.GetIsolate()) || V8HTMLCanvasElement::hasInstance(info[3], info.GetIsolate()) || V8HTMLImageElement::hasInstance(info[3], info.GetIsolate())) {
            enqueueWriteBuffer1Method(info);
            return;
        }

        enqueueWriteBuffer2Method(info);
        return;
    default:
        exceptionState.throwTypeError(ExceptionMessages::notEnoughArguments(4, info.Length()));
        exceptionState.throwIfNeeded();
        return;
    }

    exceptionState.throwWebCLException(WebCLException::INVALID_ARG_VALUE, WebCLException::invalidArgValueMessage);
    exceptionState.throwIfNeeded();
}

static void enqueueWriteBufferRect1Method(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueWriteBufferRect", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 8)) {
        setMinimumArityTypeError(exceptionState, 8, info.Length());
        exceptionState.throwIfNeeded();
        return;
    }

    WebCLCommandQueue* impl = V8WebCLCommandQueue::toImpl(info.Holder());
    WebCLBuffer* buffer;
    bool blockingWrite;
    Vector<unsigned> bufferOrigin;
    Vector<unsigned> hostOrigin;
    Vector<unsigned> region;
    unsigned hostRowPitch;
    unsigned hostSlicePitch;
    ImageData* imageData = nullptr;
    HTMLCanvasElement* canvas = nullptr;
    HTMLImageElement* image = nullptr;
    Vector<RefPtr<WebCLEvent>> eventWaitList;
    WebCLEvent* event = nullptr;
    {
        if (info.Length() > 0 && !V8WebCLBuffer::hasInstance(info[0], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 1 is not of type 'WebCLBuffer'.");
            exceptionState.throwIfNeeded();
            return;
        }

        buffer = V8WebCLBuffer::toImplWithTypeCheck(info.GetIsolate(), info[0]);
        blockingWrite = info[1]->BooleanValue();
        bufferOrigin = toImplArray<Vector<unsigned>>(info[2], 3, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        hostOrigin = toImplArray<Vector<unsigned>>(info[3], 4, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        region = toImplArray<Vector<unsigned>>(info[4], 5, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        hostRowPitch = toUInt32(info.GetIsolate(), info[5], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        hostSlicePitch = toUInt32(info.GetIsolate(), info[6], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        if (V8ImageData::hasInstance(info[7], info.GetIsolate())) {
            if (info.Length() > 7 && !V8ImageData::hasInstance(info[7], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 8 is not of type 'ImageData'.");
                exceptionState.throwIfNeeded();
                return;
            }

            imageData = V8ImageData::toImplWithTypeCheck(info.GetIsolate(), info[7]);
        } else if (V8HTMLCanvasElement::hasInstance(info[7], info.GetIsolate())) {
            if (info.Length() > 7 && !V8HTMLCanvasElement::hasInstance(info[7], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 8 is not of type 'HTMLCanvasElement'.");
                exceptionState.throwIfNeeded();
                return;
            }

            canvas = V8HTMLCanvasElement::toImplWithTypeCheck(info.GetIsolate(), info[7]);
        } else if (V8HTMLImageElement::hasInstance(info[7], info.GetIsolate())) {
            if (info.Length() > 7 && !V8HTMLImageElement::hasInstance(info[7], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 8 is not of type 'HTMLImageElement'.");
                exceptionState.throwIfNeeded();
                return;
            }

            image = V8HTMLImageElement::toImplWithTypeCheck(info.GetIsolate(), info[7]);
        }
        if (info.Length() > 8 && !isUndefinedOrNull(info[8])) {
            eventWaitList = toRefPtrNativeArray<WebCLEvent, V8WebCLEvent>(info[8], 9, info.GetIsolate(), exceptionState);
            if(exceptionState.throwIfNeeded())
                return;
        }


        if (info.Length() > 9) {
            if (!isUndefinedOrNull(info[9]) && !V8WebCLEvent::hasInstance(info[9], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 10 is not of type 'WebCLEvent'.");
                exceptionState.throwIfNeeded();
                return;
            }

            event = V8WebCLEvent::toImplWithTypeCheck(info.GetIsolate(), info[9]);
        }
    }

    if (V8ImageData::hasInstance(info[7], info.GetIsolate()))
        impl->enqueueWriteBufferRect(buffer, blockingWrite, bufferOrigin, hostOrigin, region, hostRowPitch, hostSlicePitch, imageData, eventWaitList, event, exceptionState);
    else if (V8HTMLCanvasElement::hasInstance(info[7], info.GetIsolate()))
        impl->enqueueWriteBufferRect(buffer, blockingWrite, bufferOrigin, hostOrigin, region, hostRowPitch, hostSlicePitch, canvas, eventWaitList, event, exceptionState);
    else if (V8HTMLImageElement::hasInstance(info[7], info.GetIsolate()))
        impl->enqueueWriteBufferRect(buffer, blockingWrite, bufferOrigin, hostOrigin, region, hostRowPitch, hostSlicePitch, image, eventWaitList, event, exceptionState);

    if (exceptionState.hadException())
        exceptionState.throwIfNeeded();
}

static void enqueueWriteBufferRect2Method(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueWriteBufferRect", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 10)) {
        setMinimumArityTypeError(exceptionState, 10, info.Length());
        exceptionState.throwIfNeeded();
        return;
    }

    WebCLCommandQueue* impl = V8WebCLCommandQueue::toImpl(info.Holder());
    WebCLBuffer* buffer;
    bool blockingWrite;
    Vector<unsigned> bufferOrigin;
    Vector<unsigned> hostOrigin;
    Vector<unsigned> region;
    unsigned bufferRowPitch;
    unsigned bufferSlicePitch;
    unsigned hostRowPitch;
    unsigned hostSlicePitch;
    DOMArrayBufferView* hostPtr;
    Vector<RefPtr<WebCLEvent>> eventWaitList;
    WebCLEvent* event = nullptr;
    {
        if (info.Length() > 0 && !V8WebCLBuffer::hasInstance(info[0], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 1 is not of type 'WebCLBuffer'.");
            exceptionState.throwIfNeeded();
            return;
        }

        buffer = V8WebCLBuffer::toImplWithTypeCheck(info.GetIsolate(), info[0]);
        blockingWrite = info[1]->BooleanValue();
        bufferOrigin = toImplArray<Vector<unsigned>>(info[2], 3, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        hostOrigin = toImplArray<Vector<unsigned>>(info[3], 4, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        region = toImplArray<Vector<unsigned>>(info[4], 5, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        bufferRowPitch = toUInt32(info.GetIsolate(), info[5], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        bufferSlicePitch = toUInt32(info.GetIsolate(), info[6], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        hostRowPitch = toUInt32(info.GetIsolate(), info[7], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        hostSlicePitch = toUInt32(info.GetIsolate(), info[8], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        if (info.Length() > 9 && !V8ArrayBufferView::hasInstance(info[9], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 10 is not of type 'ArrayBufferView'.");
            exceptionState.throwIfNeeded();
            return;
        }

        hostPtr = info[9]->IsArrayBufferView() ? V8ArrayBufferView::toImpl(v8::Handle<v8::ArrayBufferView>::Cast(info[9])) : 0;
        if (info.Length() > 10 && !isUndefinedOrNull(info[10])) {
            eventWaitList = toRefPtrNativeArray<WebCLEvent, V8WebCLEvent>(info[10], 11, info.GetIsolate(), exceptionState);
            if(exceptionState.throwIfNeeded())
                return;
        }

        if (info.Length() > 11) {
            if (!isUndefinedOrNull(info[11]) && !V8WebCLEvent::hasInstance(info[11], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 12 is not of type 'WebCLEvent'.");
                exceptionState.throwIfNeeded();
                return;
            }

            event = V8WebCLEvent::toImplWithTypeCheck(info.GetIsolate(), info[11]);
        }
    }

    impl->enqueueWriteBufferRect(buffer, blockingWrite, bufferOrigin, hostOrigin, region, bufferRowPitch, bufferSlicePitch, hostRowPitch, hostSlicePitch, hostPtr, eventWaitList, event, exceptionState);
    if (exceptionState.hadException())
        exceptionState.throwIfNeeded();
}

void V8WebCLCommandQueue::enqueueWriteBufferRectMethodCustom(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueWriteBufferRect", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    switch (std::min(8, info.Length())) {
    case 8:
        if (V8ImageData::hasInstance(info[7], info.GetIsolate()) || V8HTMLCanvasElement::hasInstance(info[7], info.GetIsolate()) || V8HTMLImageElement::hasInstance(info[7], info.GetIsolate())) {
            enqueueWriteBufferRect1Method(info);
            return;
        }

        enqueueWriteBufferRect2Method(info);
        return;
    default:
        exceptionState.throwTypeError(ExceptionMessages::notEnoughArguments(8, info.Length()));
        exceptionState.throwIfNeeded();
        return;
    }

    exceptionState.throwWebCLException(WebCLException::INVALID_ARG_VALUE, WebCLException::invalidArgValueMessage);
    exceptionState.throwIfNeeded();
}

static void enqueueWriteImage3Method(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueWriteImage", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 5)) {
        setMinimumArityTypeError(exceptionState, 5, info.Length());
        exceptionState.throwIfNeeded();
        return;
    }

    WebCLCommandQueue* impl = V8WebCLCommandQueue::toImpl(info.Holder());
    WebCLImage* image;
    bool blockingWrite;
    Vector<unsigned> origin;
    Vector<unsigned> region;
    Vector<RefPtr<WebCLEvent>> eventWaitList;
    WebCLEvent* event = nullptr;
    {
        if (info.Length() > 0 && !V8WebCLImage::hasInstance(info[0], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 1 is not of type 'WebCLImage'.");
            exceptionState.throwIfNeeded();
            return;
        }

        image = V8WebCLImage::toImplWithTypeCheck(info.GetIsolate(), info[0]);
        blockingWrite = info[1]->BooleanValue();
        origin = toImplArray<Vector<unsigned>>(info[2], 3, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        region = toImplArray<Vector<unsigned>>(info[3], 4, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        if (info.Length() > 5 && !isUndefinedOrNull(info[5])) {
            eventWaitList = toRefPtrNativeArray<WebCLEvent, V8WebCLEvent>(info[5], 6, info.GetIsolate(), exceptionState);
            if(exceptionState.throwIfNeeded())
                return;
        }

        if (info.Length() > 6) {
            if (!isUndefinedOrNull(info[6]) && !V8WebCLEvent::hasInstance(info[6], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 7 is not of type 'WebCLEvent'.");
                exceptionState.throwIfNeeded();
                return;
            }

            event = V8WebCLEvent::toImplWithTypeCheck(info.GetIsolate(), info[6]);
        }
    }

    if (V8ImageData::hasInstance(info[4], info.GetIsolate())) {
        ImageData* imageData;
        if (info.Length() > 4 && !V8ImageData::hasInstance(info[4], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 5 is not of type 'ImageData'.");
            exceptionState.throwIfNeeded();
            return;
        }

        imageData = V8ImageData::toImplWithTypeCheck(info.GetIsolate(), info[4]);
        impl->enqueueWriteImage(image, blockingWrite, origin, region, imageData, eventWaitList, event, exceptionState);
    } else if (V8HTMLCanvasElement::hasInstance(info[4], info.GetIsolate())) {
        HTMLCanvasElement* canvas;
        if (info.Length() > 4 && !V8HTMLCanvasElement::hasInstance(info[4], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 5 is not of type 'HTMLCanvasElement'.");
            exceptionState.throwIfNeeded();
            return;
        }

        canvas = V8HTMLCanvasElement::toImplWithTypeCheck(info.GetIsolate(), info[4]);
        impl->enqueueWriteImage(image, blockingWrite, origin, region, canvas, eventWaitList, event, exceptionState);
    } else if (V8HTMLImageElement::hasInstance(info[4], info.GetIsolate())) {
        HTMLImageElement* imageElement;
        if (info.Length() > 4 && !V8HTMLImageElement::hasInstance(info[4], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 5 is not of type 'HTMLImageElement'.");
            exceptionState.throwIfNeeded();
            return;
        }

        imageElement = V8HTMLImageElement::toImplWithTypeCheck(info.GetIsolate(), info[4]);
        impl->enqueueWriteImage(image, blockingWrite, origin, region, imageElement, eventWaitList, event, exceptionState);
    }

    if (exceptionState.hadException())
        exceptionState.throwIfNeeded();
}

static void enqueueWriteImage1Method(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueWriteImage", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 3)) {
        setMinimumArityTypeError(exceptionState, 3, info.Length());
        exceptionState.throwIfNeeded();
        return;
    }

    WebCLCommandQueue* impl = V8WebCLCommandQueue::toImpl(info.Holder());
    WebCLImage* image;
    bool blockingWrite;
    HTMLVideoElement* video;
    Vector<RefPtr<WebCLEvent>> eventWaitList;
    WebCLEvent* event = nullptr;
    {
        if (info.Length() > 0 && !V8WebCLImage::hasInstance(info[0], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 1 is not of type 'WebCLImage'.");
            exceptionState.throwIfNeeded();
            return;
        }

        image = V8WebCLImage::toImplWithTypeCheck(info.GetIsolate(), info[0]);
        blockingWrite = info[1]->BooleanValue();
        if (info.Length() > 2 && !V8HTMLVideoElement::hasInstance(info[2], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 3 is not of type 'HTMLVideoElement'.");
            exceptionState.throwIfNeeded();
            return;
        }

        video = V8HTMLVideoElement::toImplWithTypeCheck(info.GetIsolate(), info[2]);
        if (info.Length() > 3 && !isUndefinedOrNull(info[3])) {
            eventWaitList = toRefPtrNativeArray<WebCLEvent, V8WebCLEvent>(info[3], 4, info.GetIsolate(), exceptionState);
            if(exceptionState.throwIfNeeded())
                return;
        }

        if (info.Length() > 4) {
            if (!isUndefinedOrNull(info[4]) && !V8WebCLEvent::hasInstance(info[4], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 5 is not of type 'WebCLEvent'.");
                exceptionState.throwIfNeeded();
                return;
            }

            event = V8WebCLEvent::toImplWithTypeCheck(info.GetIsolate(), info[4]);
        }
    }

    impl->enqueueWriteImage(image, blockingWrite, video, eventWaitList, event, exceptionState);
    if (exceptionState.hadException()) {
        exceptionState.throwIfNeeded();
    }
}

static void enqueueWriteImage2Method(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueWriteImage", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 6)) {
        setMinimumArityTypeError(exceptionState, 6, info.Length());
        exceptionState.throwIfNeeded();
        return;
    }

    WebCLCommandQueue* impl = V8WebCLCommandQueue::toImpl(info.Holder());
    WebCLImage* image;
    bool blockingWrite;
    Vector<unsigned> origin;
    Vector<unsigned> region;
    unsigned hostRowPitch;
    DOMArrayBufferView* hostPtr;
    Vector<RefPtr<WebCLEvent>> eventWaitList;
    WebCLEvent* event = nullptr;
    {
        if (info.Length() > 0 && !V8WebCLImage::hasInstance(info[0], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 1 is not of type 'WebCLImage'.");
            exceptionState.throwIfNeeded();
            return;
        }

        image = V8WebCLImage::toImplWithTypeCheck(info.GetIsolate(), info[0]);
        blockingWrite = info[1]->BooleanValue();
        origin = toImplArray<Vector<unsigned>>(info[2], 3, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        region = toImplArray<Vector<unsigned>>(info[3], 4, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        hostRowPitch = toUInt32(info.GetIsolate(), info[4], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        if (info.Length() > 5 && !V8ArrayBufferView::hasInstance(info[5], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 6 is not of type 'ArrayBufferView'.");
            exceptionState.throwIfNeeded();
            return;
        }

        hostPtr = info[5]->IsArrayBufferView() ? V8ArrayBufferView::toImpl(v8::Handle<v8::ArrayBufferView>::Cast(info[5])) : 0;
        if (info.Length() > 6 && !isUndefinedOrNull(info[6])) {
            eventWaitList = toRefPtrNativeArray<WebCLEvent, V8WebCLEvent>(info[6], 7, info.GetIsolate(), exceptionState);
            if(exceptionState.throwIfNeeded())
                return;
        }

        if (info.Length() > 7) {
            if (!isUndefinedOrNull(info[7]) && !V8WebCLEvent::hasInstance(info[7], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 8 is not of type 'WebCLEvent'.");
                exceptionState.throwIfNeeded();
                return;
            }

            event = V8WebCLEvent::toImplWithTypeCheck(info.GetIsolate(), info[7]);
        }
    }

    impl->enqueueWriteImage(image, blockingWrite, origin, region, hostRowPitch, hostPtr, eventWaitList, event, exceptionState);
    if (exceptionState.hadException()) {
        exceptionState.throwIfNeeded();
    }
}

void V8WebCLCommandQueue::enqueueWriteImageMethodCustom(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueWriteImage", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    switch (std::min(3, info.Length())) {
    case 3:
        if (V8HTMLVideoElement::hasInstance(info[2], info.GetIsolate())) {
            enqueueWriteImage1Method(info);
            return;
        }

        if (info.Length() >= 5) {
            if (V8ImageData::hasInstance(info[4], info.GetIsolate()) || V8HTMLCanvasElement::hasInstance(info[4], info.GetIsolate()) || V8HTMLImageElement::hasInstance(info[4], info.GetIsolate())) {
                enqueueWriteImage3Method(info);
                return;
            }
            enqueueWriteImage2Method(info);
            return;
        }
    default:
        exceptionState.throwTypeError(ExceptionMessages::notEnoughArguments(3, info.Length()));
        exceptionState.throwIfNeeded();
        return;
    }

    exceptionState.throwWebCLException(WebCLException::INVALID_ARG_VALUE, WebCLException::invalidArgValueMessage);
    exceptionState.throwIfNeeded();
}

void V8WebCLCommandQueue::enqueueNDRangeKernelMethodCustom(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "enqueueNDRangeKernel", "WebCLCommandQueue", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 4)) {
        setMinimumArityTypeError(exceptionState, 4, info.Length());
        exceptionState.throwIfNeeded();
        return;
    }

    WebCLCommandQueue* impl = V8WebCLCommandQueue::toImpl(info.Holder());
    WebCLKernel* kernel;
    unsigned workDim;
    Vector<double> offsets;
    Vector<double> globalWorkSize;
    Vector<double> localWorkSize;
    Vector<RefPtr<WebCLEvent>> eventWaitList;
    WebCLEvent* event = nullptr;
    {
        if (info.Length() > 0 && !V8WebCLKernel::hasInstance(info[0], info.GetIsolate())) {
            exceptionState.throwTypeError("parameter 1 is not of type 'WebCLKernel'.");
            exceptionState.throwIfNeeded();
            return;
        }

        kernel = V8WebCLKernel::toImplWithTypeCheck(info.GetIsolate(), info[0]);
        workDim = toUInt32(info.GetIsolate(), info[1], EnforceRange, exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        if (info.Length() > 2 && !isUndefinedOrNull(info[2])){
            offsets = toImplArray<Vector<double>>(info[2], 3, info.GetIsolate(), exceptionState);
            if(exceptionState.throwIfNeeded())
                return;
        }

        globalWorkSize = toImplArray<Vector<double>>(info[3], 4, info.GetIsolate(), exceptionState);
        if(exceptionState.throwIfNeeded())
            return;

        if (info.Length() > 4 && !isUndefinedOrNull(info[4])) {
            localWorkSize = toImplArray<Vector<double>>(info[4], 5, info.GetIsolate(), exceptionState);
            if(exceptionState.throwIfNeeded())
                return;
        }

        if (info.Length() > 5 && !isUndefinedOrNull(info[5])) {
            eventWaitList = toRefPtrNativeArray<WebCLEvent, V8WebCLEvent>(info[5], 6, info.GetIsolate(), exceptionState);
            if(exceptionState.throwIfNeeded())
                return;
        }

        if (info.Length() > 6) {
            if (!isUndefinedOrNull(info[6]) && !V8WebCLEvent::hasInstance(info[6], info.GetIsolate())) {
                exceptionState.throwTypeError("parameter 7 is not of type 'WebCLEvent'.");
                exceptionState.throwIfNeeded();
                return;
            }

            event = V8WebCLEvent::toImplWithTypeCheck(info.GetIsolate(), info[6]);
        }
    }
    impl->enqueueNDRangeKernel(kernel, workDim, offsets, globalWorkSize, localWorkSize, eventWaitList, event, exceptionState);
    if (exceptionState.hadException())
        exceptionState.throwIfNeeded();
}

} // namespace blink

#endif // ENABLE(WEBCL)
