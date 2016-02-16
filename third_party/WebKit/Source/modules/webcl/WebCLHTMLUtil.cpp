// Copyright (C) 2013 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/html/HTMLCanvasElement.h"
#include "core/html/HTMLImageElement.h"
#include "core/html/HTMLVideoElement.h"
#include "core/html/ImageData.h"
#include "core/webcl/WebCLException.h"
#include "platform/graphics/gpu/WebGLImageConversion.h"
#include "platform/graphics/ImageBuffer.h"
#include "modules/webcl/WebCL.h"
#include "modules/webcl/WebCLHTMLUtil.h"

namespace blink {

bool packImageData(Image* image, WebGLImageConversion::ImageHtmlDomSource domSource, unsigned width, unsigned height, Vector<uint8_t>& data) {
    WebGLImageConversion::ImageExtractor imageExtractor(image, domSource, false, false);
    if (!imageExtractor.imagePixelData())
        return false;

    WebGLImageConversion::DataFormat sourceDataFormat = imageExtractor.imageSourceFormat();
    WebGLImageConversion::AlphaOp alphaOp = imageExtractor.imageAlphaOp();
    const void* imagePixelData = imageExtractor.imagePixelData();
    unsigned imageSourceUnpackAlignment = imageExtractor.imageSourceUnpackAlignment();

    return WebGLImageConversion::packImageData(image, imagePixelData, GL_RGBA, GL_UNSIGNED_BYTE, false, alphaOp, sourceDataFormat, width, height, imageSourceUnpackAlignment, data);
}

bool WebCLHTMLUtil::extractDataFromCanvas(HTMLCanvasElement* canvas, Vector<uint8_t>& data, size_t& canvasSize, ExceptionState& es)
{
    // Currently the data is read back from gpu to cpu, and uploaded from cpu to gpu
    // when OpenCL kernel funtion is assigned to run on GPU device.
    // TODO(junmin-zhu): should directly copy or share gpu memory in that case.
    if (!canvas) {
        es.throwWebCLException(WebCLException::INVALID_HOST_PTR, WebCLException::invalidHostPTRMessage);
        return false;
    }

    if (!packImageData(canvas->copiedImage(BackBuffer, PreferAcceleration).get(), WebGLImageConversion::HtmlDomCanvas, canvas->width(), canvas->height(), data)) {
        es.throwWebCLException(WebCLException::INVALID_HOST_PTR, WebCLException::invalidHostPTRMessage);
        return false;
    }

    canvasSize = data.size();
    if (!data.data() || !canvasSize) {
        es.throwWebCLException(WebCLException::INVALID_HOST_PTR, WebCLException::invalidHostPTRMessage);
        return false;
    }

    return true;
}

bool WebCLHTMLUtil::extractDataFromImage(HTMLImageElement* image, Vector<uint8_t>& data, size_t& imageSize, ExceptionState& es)
{
    if (!image || !image->cachedImage()) {
        es.throwWebCLException(WebCLException::INVALID_HOST_PTR, WebCLException::invalidHostPTRMessage);
        return false;
    }

    if (!packImageData(image->cachedImage()->image(), WebGLImageConversion::HtmlDomImage, image->width(), image->height(), data)) {
        es.throwWebCLException(WebCLException::INVALID_HOST_PTR, WebCLException::invalidHostPTRMessage);
        return false;
    }

    imageSize = data.size();
    if (!data.data() || !imageSize) {
        es.throwWebCLException(WebCLException::INVALID_HOST_PTR, WebCLException::invalidHostPTRMessage);
        return false;
    }

    return true;
}

bool WebCLHTMLUtil::extractDataFromImageData(ImageData* srcPixels, void*& hostPtr, size_t& pixelSize, ExceptionState& es)
{
    if (!srcPixels && !srcPixels->data() && !srcPixels->data()->data()) {
        es.throwWebCLException(WebCLException::INVALID_HOST_PTR, WebCLException::invalidHostPTRMessage);
        return false;
    }

    pixelSize = srcPixels->data()->length();
    hostPtr = static_cast<void*>(srcPixels->data()->data());
    if (!hostPtr || !pixelSize) {
        es.throwWebCLException(WebCLException::INVALID_HOST_PTR, WebCLException::invalidHostPTRMessage);
        return false;
    }

    return true;
}

bool WebCLHTMLUtil::extractDataFromVideo(HTMLVideoElement* video, Vector<uint8_t>& data, size_t& videoSize, ExceptionState& es)
{
    // Currently the data is read back from gpu to cpu, and uploaded from cpu to gpu
    // when OpenCL kernel funtion is assigned to run on GPU device.
    // TODO(junmin-zhu): should directly copy or share gpu memory in that case.
    if (!video) {
        es.throwWebCLException(WebCLException::INVALID_HOST_PTR, WebCLException::invalidHostPTRMessage);
        return false;
    }

    RefPtr<Image> image = videoFrameToImage(video);
    if (!image) {
        es.throwWebCLException(WebCLException::INVALID_HOST_PTR, WebCLException::invalidHostPTRMessage);
        return false;
    }

    if (!packImageData(image.get(), WebGLImageConversion::HtmlDomVideo, video->clientWidth(), video->clientHeight(), data)) {
        es.throwWebCLException(WebCLException::INVALID_HOST_PTR, WebCLException::invalidHostPTRMessage);
        return false;
    }
    videoSize = data.size();

    if (!data.data() || !videoSize) {
        es.throwWebCLException(WebCLException::INVALID_HOST_PTR, WebCLException::invalidHostPTRMessage);
        return false;
    }
    return true;
}

PassRefPtr<Image> WebCLHTMLUtil::videoFrameToImage(HTMLVideoElement* video)
{
    if (!video || !video->clientWidth() || !video->clientHeight())
        return nullptr;

    IntSize size(video->clientWidth(), video->clientHeight());
    ImageBuffer* imageBufferObject = m_generatedImageCache.imageBuffer(size);
    if (!imageBufferObject)
        return nullptr;

    IntRect destRect(0, 0, size.width(), size.height());
    video->paintCurrentFrame(imageBufferObject->canvas(), destRect, nullptr);
    return imageBufferObject->newImageSnapshot();
}

WebCLHTMLUtil::WebCLHTMLUtil(unsigned capacity)
    : m_generatedImageCache(capacity)
{
}

WebCLHTMLUtil::ImageBufferCache::ImageBufferCache(unsigned capacity)
    : m_capacity(capacity)
{
    m_buffers.reserveCapacity(capacity);
}

// Get the imageBuffer with the same size as input argument, and swtich it to front for reusing.
ImageBuffer* WebCLHTMLUtil::ImageBufferCache::imageBuffer(const IntSize& size)
{
    unsigned i;
    for (i = 0; i < m_buffers.size(); ++i) {
        ImageBuffer* buf = m_buffers[i].get();
        if (buf->size() != size)
            continue;

        if (i > 0)
            m_buffers[i].swap(m_buffers[0]);

        return buf;
    }

    OwnPtr<ImageBuffer> temp = ImageBuffer::create(size);
    if (!temp)
        return nullptr;

    if (i < m_capacity - 1) {
        m_buffers.append(temp.release());
    } else {
        m_buffers[m_capacity - 1] = temp.release();
        i = m_capacity - 1;
    }

    ImageBuffer* buf = m_buffers[i].get();
    if (i > 0)
        m_buffers[i].swap(m_buffers[0]);

    return buf;
}

} // blink
