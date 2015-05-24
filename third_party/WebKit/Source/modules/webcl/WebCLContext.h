// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLContext_h
#define WebCLContext_h

#if ENABLE(WEBCL)
#include "bindings/core/v8/Nullable.h"
#include "modules/webcl/WebCLConfig.h"
#include "modules/webcl/WebCLDevice.h"
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/WeakPtr.h>

namespace blink {

class ExceptionState;
class DOMArrayBufferView;
class HTMLCanvasElement;
class HTMLImageElement;
class HTMLVideoElement;
class ImageBuffer;
class ImageData;
class IntSize;
class WebCL;
class WebCLBuffer;
class WebCLCommandQueue;
class WebCLHTMLUtil;
class WebCLImage;
class WebCLImageDescriptor;
class WebCLMemoryUtil;
class WebCLObject;
class WebCLProgram;
class WebCLSampler;
class WebCLUserEvent;

class WebCLContext : public RefCounted<WebCLContext>, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    ~WebCLContext();
    static PassRefPtr<WebCLContext> create(cl_context, WebCL*, const Vector<RefPtr<WebCLDevice>>&, HashSet<String>&);

    ScriptValue getInfo(ScriptState*, int, ExceptionState&);
    PassRefPtr<WebCLCommandQueue> createCommandQueue(WebCLDevice*, unsigned, ExceptionState&);
    PassRefPtr<WebCLCommandQueue> createCommandQueue(int, ExceptionState&);
    PassRefPtr<WebCLCommandQueue> createCommandQueue(WebCLDevice*, ExceptionState&);
    PassRefPtr<WebCLCommandQueue> createCommandQueue(ExceptionState& es);
    PassRefPtr<WebCLProgram> createProgram(const String&, ExceptionState&);
    PassRefPtr<WebCLSampler> createSampler(bool, unsigned, unsigned, ExceptionState&);
    PassRefPtr<WebCLUserEvent> createUserEvent(ExceptionState&);
    PassRefPtr<WebCLBuffer> createBuffer(unsigned, unsigned, DOMArrayBufferView*, ExceptionState&);
    PassRefPtr<WebCLBuffer> createBuffer(unsigned, unsigned, ExceptionState&);
    PassRefPtr<WebCLBuffer> createBuffer(unsigned, ImageData*, ExceptionState&);
    PassRefPtr<WebCLBuffer> createBuffer(unsigned, HTMLCanvasElement*, ExceptionState&);
    PassRefPtr<WebCLBuffer> createBuffer(unsigned, HTMLImageElement*, ExceptionState&);
    PassRefPtr<WebCLImage> createImage(unsigned, const WebCLImageDescriptor&, DOMArrayBufferView*, ExceptionState&);
    PassRefPtr<WebCLImage> createImage(unsigned, const WebCLImageDescriptor&, ExceptionState&);
    PassRefPtr<WebCLImage> createImage(unsigned, ImageData*, ExceptionState&);
    PassRefPtr<WebCLImage> createImage(unsigned, HTMLCanvasElement*, ExceptionState&);
    PassRefPtr<WebCLImage> createImage(unsigned, HTMLImageElement*, ExceptionState&);
    PassRefPtr<WebCLImage> createImage(unsigned, HTMLVideoElement*, ExceptionState&);
    Nullable<HeapVector<WebCLImageDescriptor>> getSupportedImageFormats(ExceptionState&);
    Nullable<HeapVector<WebCLImageDescriptor>> getSupportedImageFormats(unsigned, ExceptionState&);
    void release();
    void releaseAll();

    void trackReleaseableWebCLObject(WeakPtr<WebCLObject>);
    void untrackReleaseableWebCLObject(WeakPtr<WebCLObject>);
    static unsigned bytesPerChannelType(unsigned);
    static unsigned numberOfChannelsForChannelOrder(unsigned);
    bool isExtensionEnabled(const String& name) const;
    const Vector<RefPtr<WebCLDevice>>& getDevices() { return m_devices; }
    void setDevices(const Vector<RefPtr<WebCLDevice>>& deviceList) { m_devices = deviceList; }
    WebCLHTMLUtil* getHTMLUtil() const { return m_HTMLUtil.get(); }
    cl_context getContext() const { return m_clContext; }

private:
    WebCLContext(cl_context, WebCL*, const Vector<RefPtr<WebCLDevice>>&, HashSet<String>&);
    bool isReleased() const { return !m_clContext; }

    typedef HashMap<WebCLDevice*, std::pair<unsigned, unsigned>> MaximumWidthAndHeightForDevice;
    PassRefPtr<WebCLImage> createImage2DBase(unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, void*, ExceptionState&);
    PassRefPtr<WebCLBuffer> createBufferBase(unsigned memoryFlags, unsigned size, void* data, ExceptionState&);
    bool supportsWidthHeight(unsigned width, unsigned height, ExceptionState&);

    Vector<RefPtr<WebCLDevice>> m_devices;
    HashSet<String> m_enabledExtensions;
    MaximumWidthAndHeightForDevice m_deviceMaxValues;
    OwnPtr<WebCLMemoryUtil> m_memoryUtil;
    OwnPtr<WebCLHTMLUtil> m_HTMLUtil;
    Vector<WeakPtr<WebCLObject>> m_webCLObjects;
    WeakPtrFactory<WebCLContext> m_weakFactory;
    cl_context m_clContext;
};

} // namespace blink

#endif // ENABLE(WEBCL)
#endif // WebCLContext_h
