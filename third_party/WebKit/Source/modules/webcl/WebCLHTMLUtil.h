// Copyright (C) 2013 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLHTMLUtil_h
#define WebCLHTMLUtil_h

#if ENABLE(WEBCL)
#include <wtf/RefCounted.h>

namespace blink {

class ExceptionState;
class HTMLCanvasElement;
class HTMLImageElement;
class HTMLVideoElement;
class ImageData;

class WebCLHTMLUtil {
public:
    explicit WebCLHTMLUtil(unsigned capacity = 4);
    static bool extractDataFromCanvas(HTMLCanvasElement*, Vector<uint8_t>& data, size_t& canvasSize, ExceptionState&);
    static bool extractDataFromImage(HTMLImageElement*, Vector<uint8_t>& data, size_t& canvasSize, ExceptionState&);
    static bool extractDataFromImageData(ImageData*, void*& hostPtr, size_t& pixelSize, ExceptionState&);
    bool extractDataFromVideo(HTMLVideoElement*, Vector<uint8_t>& data, size_t& videoSize, ExceptionState&);
private:
    PassRefPtr<Image> videoFrameToImage(HTMLVideoElement*);

    // Fixed-size cache of reusable image buffers for extractDataFromVideo calls.
    class ImageBufferCache {
        public:
            ImageBufferCache(unsigned capacity);
            ImageBuffer* imageBuffer(const IntSize&);
        private:
            Vector<OwnPtr<ImageBuffer>> m_buffers;
            unsigned m_capacity;
    };
    ImageBufferCache m_generatedImageCache;
};

} // namespace blink

#endif // ENABLE(WEBCL)
#endif // WebCLHTMLUtil_h
