// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bindings/modules/v8/V8WebCLDevice.h"
#include "core/html/HTMLCanvasElement.h"
#include "core/html/HTMLImageElement.h"
#include "core/html/HTMLVideoElement.h"
#include "core/html/ImageData.h"
#include "core/webcl/WebCLException.h"
#include "platform/graphics/Image.h"
#include "platform/graphics/ImageBuffer.h"
#include "bindings/core/v8/V8Binding.h"
#include "modules/webcl/WebCL.h"
#include "modules/webcl/WebCLBuffer.h"
#include "modules/webcl/WebCLContext.h"
#include "modules/webcl/WebCLCommandQueue.h"
#include "modules/webcl/WebCLDevice.h"
#include "modules/webcl/WebCLHTMLUtil.h"
#include "modules/webcl/WebCLImage.h"
#include "modules/webcl/WebCLImageDescriptor.h"
#include "modules/webcl/WebCLInputChecker.h"
#include "modules/webcl/WebCLKernel.h"
#include "modules/webcl/WebCLMemoryObject.h"
#include "modules/webcl/WebCLMemoryUtil.h"
#include "modules/webcl/WebCLObject.h"
#include "modules/webcl/WebCLOpenCL.h"
#include "modules/webcl/WebCLSampler.h"
#include "modules/webcl/WebCLUserEvent.h"

namespace blink {

WebCLContext::~WebCLContext()
{
    releaseAll();
    ASSERT(!m_clContext);
}

PassRefPtr<WebCLContext> WebCLContext::create(cl_context contextId, WebCL* webCL, const Vector<RefPtr<WebCLDevice>>& devices, HashSet<String>& enabledExtensions)
{
    return adoptRef(new WebCLContext(contextId, webCL, devices, enabledExtensions));
}

ScriptValue WebCLContext::getInfo(ScriptState* scriptState, int paramName, ExceptionState& es)
{
    v8::Handle<v8::Object> creationContext = scriptState->context()->Global();
    v8::Isolate* isolate = scriptState->isolate();

    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
        return ScriptValue(scriptState, v8::Null(isolate));
    }

    cl_int err = CL_SUCCESS;
    cl_uint uintUnits = 0;
    Vector<RefPtr<WebCLDevice>> result;
    switch(paramName) {
    case CL_CONTEXT_NUM_DEVICES:
        err = clGetContextInfo(m_clContext,CL_CONTEXT_NUM_DEVICES, sizeof(cl_uint), &uintUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(uintUnits)));
        WebCLException::throwException(err, es);
        return ScriptValue(scriptState, v8::Null(isolate));
    case CL_CONTEXT_DEVICES:
        return ScriptValue(scriptState, toV8(m_devices, creationContext, isolate));
    default:
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return ScriptValue(scriptState, v8::Null(isolate));
    }
}

PassRefPtr<WebCLCommandQueue> WebCLContext::createCommandQueue(WebCLDevice* device, unsigned commandQueueProp, ExceptionState& es)
{
    if (!WebCLInputChecker::isValidCommandQueueProperty(commandQueueProp)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return nullptr;
    }

    cl_device_id clDevice = nullptr;
    // NOTE: if device is null, it will be selected by any WebCLDevice that matches Properties
    if (!device) {
        for (auto deviceItem : m_devices) {
            unsigned properties = deviceItem->getQueueProperties();
            if (!commandQueueProp || (properties && (properties & commandQueueProp))) {
                device = deviceItem.get();
                clDevice = deviceItem->getDeviceId();
                break;
            } else {
                es.throwWebCLException(WebCLException::INVALID_QUEUE_PROPERTIES, WebCLException::invalidQueuePropertiesMessage);
                return nullptr;
            }
        }
    } else {
        clDevice = device->getDeviceId();
        size_t i = 0;
        for (i = 0; i < m_devices.size(); ++i) {
            if (m_devices[i]->getDeviceId() == clDevice)
                break;
        }
        if (i == m_devices.size()) {
            es.throwWebCLException(WebCLException::INVALID_DEVICE, WebCLException::invalidDeviceMessage);
            return nullptr;
        }
    }

    cl_int err = CL_SUCCESS;
    cl_command_queue clCommandQueueId = nullptr;
    switch (commandQueueProp) {
    case CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE:
        clCommandQueueId = clCreateCommandQueue(m_clContext, clDevice, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err);
        break;
    case CL_QUEUE_PROFILING_ENABLE:
        clCommandQueueId = clCreateCommandQueue(m_clContext, clDevice, CL_QUEUE_PROFILING_ENABLE, &err);
        break;
    default:
        clCommandQueueId = clCreateCommandQueue(m_clContext, clDevice, 0, &err);
        break;
    }

    if (err != CL_SUCCESS) {
        WebCLException::throwException(err, es);
        return nullptr;
    }

    RefPtr<WebCLCommandQueue> commandQueue = WebCLCommandQueue::create(clCommandQueueId, this, device);
    if (!commandQueue) {
        es.throwWebCLException(WebCLException::INVALID_COMMAND_QUEUE, WebCLException::invalidCommandQueueMessage);
        return nullptr;
    }

    if (commandQueue)
        m_memoryUtil->commandQueueCreated(commandQueue.get(), es);

    return commandQueue.release();
}

PassRefPtr<WebCLCommandQueue> WebCLContext::createCommandQueue(int properties,ExceptionState& es)
{
    return createCommandQueue(nullptr, properties, es);
}

PassRefPtr<WebCLCommandQueue> WebCLContext::createCommandQueue(WebCLDevice* device, ExceptionState& es)
{
    return createCommandQueue(device, 0, es);
}

PassRefPtr<WebCLCommandQueue> WebCLContext::createCommandQueue(ExceptionState& es)
{
    return createCommandQueue(nullptr, 0, es);
}

PassRefPtr<WebCLProgram> WebCLContext::createProgram(const String& kernelSource, ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
        return nullptr;
    }

    if (!kernelSource.length()) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return nullptr;
    }

    const char* source = strdup(kernelSource.utf8().data());
    cl_int err = CL_SUCCESS;
    cl_program clProgramId = clCreateProgramWithSource(m_clContext, 1, (const char**)&source, nullptr, &err);
    if (err != CL_SUCCESS) {
        WebCLException::throwException(err, es);
        return nullptr;
    }

    RefPtr<WebCLProgram> program = WebCLProgram::create(clProgramId, this, kernelSource);
    if (!program) {
        es.throwWebCLException(WebCLException::INVALID_PROGRAM, WebCLException::invalidProgramMessage);
        return nullptr;
    }

    return program.release();
}

PassRefPtr<WebCLSampler> WebCLContext::createSampler(bool normCords, unsigned addrMode, unsigned fltrMode, ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
        return nullptr;
    }

    if (!WebCLInputChecker::isValidAddressingMode(addrMode) || !WebCLInputChecker::isValidFilterMode(fltrMode)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return nullptr;
    }

    if (!normCords && (addrMode == CL_ADDRESS_REPEAT || addrMode == CL_ADDRESS_MIRRORED_REPEAT)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return nullptr;
    }

    cl_bool normalizedCoords = CL_FALSE;
    if (normCords)
        normalizedCoords = CL_TRUE;

    cl_addressing_mode addressingMode;
    switch(addrMode) {
    case CL_ADDRESS_CLAMP_TO_EDGE:
        addressingMode = CL_ADDRESS_CLAMP_TO_EDGE;
        break;
    case CL_ADDRESS_CLAMP:
        addressingMode = CL_ADDRESS_CLAMP;
        break;
    case CL_ADDRESS_REPEAT:
        addressingMode = CL_ADDRESS_REPEAT;
        break;
    case CL_ADDRESS_MIRRORED_REPEAT:
        addressingMode = CL_ADDRESS_MIRRORED_REPEAT;
        break;
    default:
        es.throwWebCLException(WebCLException::FAILURE, WebCLException::failureMessage);
        return nullptr;
    }

    cl_filter_mode filterMode = CL_FILTER_NEAREST;
    switch(fltrMode) {
    case CL_FILTER_LINEAR:
        filterMode = CL_FILTER_LINEAR;
        break;
    case CL_FILTER_NEAREST :
        filterMode = CL_FILTER_NEAREST ;
        break;
    default:
        es.throwWebCLException(WebCLException::FAILURE, WebCLException::failureMessage);
        return nullptr;
    }

    cl_int err = CL_SUCCESS;
    cl_sampler clSamplerId = clCreateSampler(m_clContext, normalizedCoords, addressingMode, filterMode, &err);
    if (err != CL_SUCCESS) {
        WebCLException::throwException(err, es);
        return nullptr;
    }

    RefPtr<WebCLSampler> sampler = WebCLSampler::create(clSamplerId, normCords, addrMode, fltrMode, this);
    if (!sampler) {
        es.throwWebCLException(WebCLException::FAILURE, WebCLException::failureMessage);
        return nullptr;
    }

    return sampler.release();
}

PassRefPtr<WebCLUserEvent> WebCLContext::createUserEvent(ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
        return nullptr;
    }

    RefPtr<WebCLUserEvent> event = WebCLUserEvent::create(this, es);
    return event.release();
}

PassRefPtr<WebCLBuffer> WebCLContext::createBufferBase(unsigned memFlags, unsigned sizeInBytes, void* hostPtr, ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
        return nullptr;
    }

    if (!WebCLInputChecker::isValidMemoryObjectFlag(memFlags)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return nullptr;
    }

    unsigned num = 0 ;
    for (size_t i = 0; i < m_devices.size(); ++i) {
        unsigned maxMemAllocSize = m_devices[i]->getMaxMemAllocSize();
        if (maxMemAllocSize && maxMemAllocSize < sizeInBytes) {
            num++;
        }
    }

    if (num != 0 && num == m_devices.size()) {
        es.throwWebCLException(WebCLException::INVALID_BUFFER_SIZE, WebCLException::invalidBufferSizeMessage);
        return nullptr;
    }

    if (sizeInBytes == 0) {
        es.throwWebCLException(WebCLException::INVALID_BUFFER_SIZE, WebCLException::invalidBufferSizeMessage);
        return nullptr;
    }
    RefPtr<WebCLBuffer> buffer = WebCLBuffer::create(this, memFlags, sizeInBytes, hostPtr, es);
    if (!hostPtr && buffer)
        m_memoryUtil->bufferCreated(buffer.get(), es);

    return buffer.release();
}

PassRefPtr<WebCLBuffer> WebCLContext::createBuffer(unsigned memFlags, unsigned sizeInBytes, DOMArrayBufferView* hostPtr, ExceptionState& es)
{
    RefPtr<DOMArrayBuffer> buffer;
    if (hostPtr) {
        if (hostPtr->byteLength() < sizeInBytes) {
            es.throwWebCLException(WebCLException::INVALID_HOST_PTR, WebCLException::invalidHostPTRMessage);
            return nullptr;
        }

        if (!hostPtr->buffer()) {
            es.throwWebCLException(WebCLException::INVALID_HOST_PTR, WebCLException::invalidHostPTRMessage);
            return nullptr;
        }
        buffer = hostPtr->buffer();
    }

    return createBufferBase(memFlags, sizeInBytes, buffer ? buffer->data() : nullptr, es);
}

PassRefPtr<WebCLBuffer> WebCLContext::createBuffer(unsigned memFlags, unsigned sizeInBytes, ExceptionState& es)
{
    return createBuffer(memFlags, sizeInBytes, nullptr, es);
}

PassRefPtr<WebCLBuffer> WebCLContext::createBuffer(unsigned memoryFlags, ImageData* srcPixels, ExceptionState& es)
{
    if (!isExtensionEnabled("WEBCL_html_image")) {
        es.throwWebCLException(WebCLException::EXTENSION_NOT_ENABLED, WebCLException::extensionNotEnabledMessage);
        return nullptr;
    }

    void* hostPtr = 0;
    size_t pixelSize = 0;
    if (!WebCLHTMLUtil::extractDataFromImageData(srcPixels, hostPtr, pixelSize, es))
        return nullptr;

    return createBufferBase(memoryFlags, pixelSize, hostPtr, es);
}

PassRefPtr<WebCLBuffer> WebCLContext::createBuffer(unsigned memoryFlags, HTMLCanvasElement* srcCanvas, ExceptionState& es)
{
    if (!isExtensionEnabled("WEBCL_html_image")) {
        es.throwWebCLException(WebCLException::EXTENSION_NOT_ENABLED, WebCLException::extensionNotEnabledMessage);
        return nullptr;
    }

    Vector<uint8_t> data;
    size_t canvasSize = 0;
    if (!WebCLHTMLUtil::extractDataFromCanvas(srcCanvas, data, canvasSize, es))
        return nullptr;

    void* hostPtr = data.data();
    return createBufferBase(memoryFlags, canvasSize, hostPtr, es);
}

PassRefPtr<WebCLBuffer> WebCLContext::createBuffer(unsigned memoryFlags, HTMLImageElement* srcImage, ExceptionState& es)
{
    if (!isExtensionEnabled("WEBCL_html_image")) {
        es.throwWebCLException(WebCLException::EXTENSION_NOT_ENABLED, WebCLException::extensionNotEnabledMessage);
        return nullptr;
    }

    Vector<uint8_t> data;
    size_t bufferSize = 0;
    if (!WebCLHTMLUtil::extractDataFromImage(srcImage, data, bufferSize, es))
        return nullptr;

    void* hostPtr = data.data();
    return createBufferBase(memoryFlags, bufferSize, hostPtr, es);
}

PassRefPtr<WebCLImage> WebCLContext::createImage2DBase(unsigned flags, unsigned width, unsigned height, unsigned rowPitch, unsigned channelOrder, unsigned channelType, void* data, ExceptionState& es)
{
    if (!width || !height) {
        es.throwWebCLException(WebCLException::INVALID_IMAGE_FORMAT_DESCRIPTOR, WebCLException::invalidImageFormatDescriptorMessage);
        return nullptr;
    }

    if (!WebCLInputChecker::isValidMemoryObjectFlag(flags)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return nullptr;
    }

    ASSERT(data);
    flags |= CL_MEM_COPY_HOST_PTR;
    WebCLImageDescriptor imageDescriptor;
    imageDescriptor.setWidth(width);
    imageDescriptor.setHeight(height);
    imageDescriptor.setRowPitch(rowPitch);
    imageDescriptor.setChannelOrder(channelOrder);
    imageDescriptor.setChannelType(channelType);
    cl_image_format image_format = {channelOrder, channelType};

    cl_int err = CL_SUCCESS;
    cl_mem clMemId = clCreateImage2D(m_clContext, flags, &image_format, width, height, rowPitch, data, &err);
    if (err != CL_SUCCESS) {
        WebCLException::throwException(err, es);
        return nullptr;
    }

    RefPtr<WebCLImage> image = WebCLImage::create(clMemId, imageDescriptor, this);
    return image.release();
}

PassRefPtr<WebCLImage> WebCLContext::createImage(unsigned flags, const WebCLImageDescriptor& descriptor, DOMArrayBufferView* hostPtr, ExceptionState& es)
{
    if (!WebCLInputChecker::isValidMemoryObjectFlag(flags)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return nullptr;
    }

    unsigned width =  descriptor.width();
    unsigned height = descriptor.height();
    unsigned rowPitch = descriptor.rowPitch();
    unsigned channelOrder = descriptor.channelOrder();
    unsigned channelType = descriptor.channelType();
    unsigned numberOfChannels = WebCLContext::numberOfChannelsForChannelOrder(channelOrder);
    unsigned bytesPerChannel = WebCLContext::bytesPerChannelType(channelType);

    if (!WebCLInputChecker::isValidChannelOrder(channelOrder) || !WebCLInputChecker::isValidChannelType(channelType)) {
        es.throwWebCLException(WebCLException::INVALID_IMAGE_FORMAT_DESCRIPTOR, WebCLException::invalidImageFormatDescriptorMessage);
        return nullptr;
    }

    if ((!hostPtr && rowPitch) || (hostPtr && rowPitch > 0 && rowPitch < (width * numberOfChannels * bytesPerChannel))) {
        es.throwWebCLException(WebCLException::INVALID_IMAGE_SIZE, WebCLException::invalidImageSizeMessage);
        return nullptr;
    }

    if (!supportsWidthHeight(width, height, es)) {
        es.throwWebCLException(WebCLException::INVALID_IMAGE_SIZE, WebCLException::invalidImageSizeMessage);
        return nullptr;
    }
    RefPtr<DOMArrayBuffer> buffer;
    if (hostPtr) {
        unsigned byteLength = hostPtr->byteLength();
        if ((rowPitch && byteLength < (rowPitch * height)) || byteLength < (width * height * numberOfChannels * bytesPerChannel)) {
            es.throwWebCLException(WebCLException::INVALID_HOST_PTR, WebCLException::invalidHostPTRMessage);
            return nullptr;
        }
        buffer = hostPtr->buffer();
    } else {
        buffer = DOMArrayBuffer::create(width * height * numberOfChannels * bytesPerChannel, 1);
    }

    return createImage2DBase(flags, width, height, rowPitch, channelOrder, channelType, buffer->data(), es);
}

PassRefPtr<WebCLImage> WebCLContext::createImage(unsigned memFlags, const WebCLImageDescriptor& descriptor, ExceptionState& es)
{
    return createImage(memFlags, descriptor, nullptr, es);
}

PassRefPtr<WebCLImage> WebCLContext::createImage(unsigned flags, HTMLCanvasElement* srcCanvas, ExceptionState& es)
{
    if (!isExtensionEnabled("WEBCL_html_image")) {
        es.throwWebCLException(WebCLException::EXTENSION_NOT_ENABLED, WebCLException::extensionNotEnabledMessage);
        return nullptr;
    }

    unsigned width = srcCanvas->width();
    unsigned height = srcCanvas->height();
    if (!supportsWidthHeight(width, height, es)) {
        es.throwWebCLException(WebCLException::INVALID_IMAGE_SIZE, WebCLException::invalidImageSizeMessage);
        return nullptr;
    }

    Vector<uint8_t> data;
    size_t canvasSize = 0;
    if (!WebCLHTMLUtil::extractDataFromCanvas(srcCanvas, data, canvasSize, es))
        return nullptr;
    void* hostPtr = data.data();
    return createImage2DBase(flags, width, height, 0, CL_RGBA, CL_UNORM_INT8, hostPtr, es);
}

PassRefPtr<WebCLImage> WebCLContext::createImage(unsigned flags, HTMLImageElement* srcImage, ExceptionState& es)
{
    if (!isExtensionEnabled("WEBCL_html_image")) {
        es.throwWebCLException(WebCLException::EXTENSION_NOT_ENABLED, WebCLException::extensionNotEnabledMessage);
        return nullptr;
    }

    unsigned width = srcImage->width();
    unsigned height = srcImage->height();
    if (!supportsWidthHeight(width, height, es)) {
        es.throwWebCLException(WebCLException::INVALID_IMAGE_SIZE, WebCLException::invalidImageSizeMessage);
        return nullptr;
    }

    Vector<uint8_t> data;
    size_t bufferSize = 0;
    if (!WebCLHTMLUtil::extractDataFromImage(srcImage, data, bufferSize, es))
        return nullptr;

    void* hostPtr = data.data();
    return createImage2DBase(flags, width, height, 0, CL_RGBA, CL_UNORM_INT8, hostPtr, es);
}

PassRefPtr<WebCLImage> WebCLContext::createImage(unsigned flags, HTMLVideoElement* video, ExceptionState& es)
{
    if (!isExtensionEnabled("WEBCL_html_video")) {
        es.throwWebCLException(WebCLException::EXTENSION_NOT_ENABLED, WebCLException::extensionNotEnabledMessage);
        return nullptr;
    }

    if (CL_MEM_READ_ONLY != flags) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return nullptr;
    }

    unsigned width =  video->clientWidth();
    unsigned height = video->clientHeight();
    if (!supportsWidthHeight(width, height, es)) {
        es.throwWebCLException(WebCLException::INVALID_IMAGE_SIZE, WebCLException::invalidImageSizeMessage);
        return nullptr;
    }

    Vector<uint8_t> data;
    size_t videoSize = 0;
    if (!m_HTMLUtil->extractDataFromVideo(video, data, videoSize, es))
        return nullptr;

    void* hostPtr = data.data();
    return createImage2DBase(flags, width, height, 0, CL_RGBA, CL_UNORM_INT8, hostPtr, es);
}

PassRefPtr<WebCLImage> WebCLContext::createImage(unsigned flags, ImageData* srcPixels, ExceptionState& es)
{
    if (!isExtensionEnabled("WEBCL_html_image")) {
        es.throwWebCLException(WebCLException::EXTENSION_NOT_ENABLED, WebCLException::extensionNotEnabledMessage);
        return nullptr;
    }

    void* hostPtr = 0;
    size_t pixelSize = 0;
    if(!WebCLHTMLUtil::extractDataFromImageData(srcPixels, hostPtr, pixelSize, es))
        return nullptr;

    unsigned width = srcPixels->width();
    unsigned height = srcPixels->height();
    return createImage2DBase(flags, width, height, 0, CL_RGBA, CL_UNORM_INT8, hostPtr, es);
}

Nullable<HeapVector<WebCLImageDescriptor>> WebCLContext::getSupportedImageFormats(ExceptionState& es)
{
    return getSupportedImageFormats(CL_MEM_READ_WRITE, es);
}

Nullable<HeapVector<WebCLImageDescriptor>> WebCLContext::getSupportedImageFormats(unsigned memFlags, ExceptionState& es)
{
    HeapVector<WebCLImageDescriptor> supportedImageDescriptor;
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
        return supportedImageDescriptor;
    }

    if (!WebCLInputChecker::isValidMemoryObjectFlag(memFlags)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return supportedImageDescriptor;
    }

    cl_uint numberOfSupportedImageFormats = 0;
    cl_int err = clGetSupportedImageFormats(m_clContext, memFlags, CL_MEM_OBJECT_IMAGE2D, 0, 0, &numberOfSupportedImageFormats);

    if (err != CL_SUCCESS) {
        es.throwWebCLException(WebCLException::INVALID_IMAGE_SIZE, WebCLException::invalidImageSizeMessage);
        return supportedImageDescriptor;
    }

    Vector<cl_image_format> supportedImages;
    supportedImages.reserveCapacity(numberOfSupportedImageFormats);
    supportedImages.resize(numberOfSupportedImageFormats);

    err = clGetSupportedImageFormats(m_clContext, memFlags, CL_MEM_OBJECT_IMAGE2D, numberOfSupportedImageFormats, supportedImages.data(), 0);
    if (err != CL_SUCCESS) {
        WebCLException::throwException(err, es);
    } else {
        for (size_t i = 0; i < static_cast<unsigned>(numberOfSupportedImageFormats); ++i) {
            if (WebCLInputChecker::isValidChannelOrder(supportedImages[i].image_channel_order) && WebCLInputChecker::isValidChannelType(supportedImages[i].image_channel_data_type)) {
                WebCLImageDescriptor des;
                des.setChannelOrder(supportedImages[i].image_channel_order);
                des.setChannelType(supportedImages[i].image_channel_data_type);
                supportedImageDescriptor.append(des);
            }
        }
    }

    return supportedImageDescriptor;
}

void WebCLContext::release()
{
    if (isReleased())
        return;

    cl_int err = clReleaseContext(m_clContext);
    if (err != CL_SUCCESS)
        ASSERT_NOT_REACHED();

    m_clContext = 0;
}

void WebCLContext::releaseAll()
{
    if (isReleased())
        return;

    if (m_webCLObjects.size()) {
        for (int i = m_webCLObjects.size() - 1; i >= 0; i--) {
            WebCLObject* object = m_webCLObjects[i].get();
            if (!object)
                continue;
            object->release();
        }
        m_webCLObjects.clear();
    }

    release();
}

void WebCLContext::trackReleaseableWebCLObject(WeakPtr<WebCLObject> object)
{
    m_webCLObjects.append(object);
}

void WebCLContext::untrackReleaseableWebCLObject(WeakPtr<WebCLObject> object)
{
    size_t i = m_webCLObjects.find(object);
    if (i != kNotFound)
        m_webCLObjects.remove(i);
}

unsigned WebCLContext::bytesPerChannelType(unsigned channelType)
{
    switch(channelType) {
    case CL_SNORM_INT8:
    case CL_UNORM_INT8:
    case CL_SIGNED_INT8:
    case CL_UNSIGNED_INT8:
        return 1;
    case CL_SNORM_INT16:
    case CL_UNORM_INT16:
    case CL_SIGNED_INT16:
    case CL_UNSIGNED_INT16:
    case CL_HALF_FLOAT:
        return 2;
    case CL_SIGNED_INT32:
    case CL_UNSIGNED_INT32:
    case CL_FLOAT:
        return 4;
    case CL_UNORM_SHORT_565:
    case CL_UNORM_SHORT_555:
    case CL_UNORM_INT_101010:
        break;
    }

    return 0;
}

unsigned WebCLContext::numberOfChannelsForChannelOrder(unsigned order)
{
    switch (order) {
    case CL_R:
    case CL_A:
    case CL_INTENSITY:
    case CL_LUMINANCE:
        return 1;
    case CL_RG:
    case CL_RA:
    case CL_Rx:
        return 2;
    case CL_RGB:
    case CL_RGx:
        return 3;
    case CL_RGBA:
    case CL_BGRA:
    case CL_ARGB:
    case CL_RGBx:
        return 4;
    }

    return 0;
}

WebCLContext::WebCLContext(cl_context context, WebCL* webCL, const Vector<RefPtr<WebCLDevice>>& devices, HashSet<String>& enabledExtensions)
    : m_devices(devices)
    , m_enabledExtensions(enabledExtensions)
    , m_weakFactory(this)
    , m_clContext(context)
{
    if (isExtensionEnabled("WEBCL_html_video"))
        m_HTMLUtil = adoptPtr(new WebCLHTMLUtil());
    else
        m_HTMLUtil = nullptr;

    m_memoryUtil = adoptPtr(new WebCLMemoryUtil(this));
    webCL->trackReleaseableWebCLContext(m_weakFactory.createWeakPtr());
}

// Private method to check if {width, height} can be accomodated in any of the context devices.
bool WebCLContext::supportsWidthHeight(unsigned width, unsigned height, ExceptionState& es)
{
    if (m_devices.size() && !m_deviceMaxValues.size()) {
        unsigned deviceMaxWidth = 0, deviceMaxHeight = 0;
        for (auto device : m_devices) {
            deviceMaxWidth = device->getImage2DMaxWidth();
            deviceMaxHeight = device->getImage2DMaxHeight();
            if (deviceMaxWidth && deviceMaxHeight)
                m_deviceMaxValues.add(device.get(), std::make_pair(deviceMaxWidth, deviceMaxHeight));
        }
    }

    for (auto it : m_deviceMaxValues) {
       if (width <= it.value.first && height <= it.value.second)
           return true;
    }

    return false;
}

bool WebCLContext::isExtensionEnabled(const String& name) const
{
    return m_enabledExtensions.contains(name);
}

} // namespace blink
