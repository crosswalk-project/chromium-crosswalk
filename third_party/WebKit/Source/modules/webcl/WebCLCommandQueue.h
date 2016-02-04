// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLCommandQueue_h
#define WebCLCommandQueue_h

#include "modules/webcl/WebCLCallback.h"
#include "modules/webcl/WebCLConfig.h"
#include "modules/webcl/WebCLObject.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Threading.h>

namespace blink {

class DOMArrayBufferView;
class ExceptionState;
class HTMLCanvasElement;
class HTMLImageElement;
class HTMLVideoElement;
class ImageData;
class WebCL;
class WebCLBuffer;
class WebCLCommandQueueHolder;
class WebCLDevice;
class WebCLEvent;
class WebCLEventList;
class WebCLImage;
class WebCLKernel;
class WebCLMemoryObject;
class WebCLProgram;
class WebCLSampler;

class WebCLCommandQueue : public WebCLObject, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    ~WebCLCommandQueue() override;
    static PassRefPtr<WebCLCommandQueue> create(cl_command_queue, PassRefPtr<WebCLContext>, WebCLDevice*);

    ScriptValue getInfo(ScriptState*, int, ExceptionState&);
    void enqueueWriteBuffer(WebCLBuffer*, bool, unsigned, unsigned, DOMArrayBufferView*, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);
    void enqueueWriteBuffer(WebCLBuffer*, bool, unsigned, ImageData*, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);
    void enqueueWriteBuffer(WebCLBuffer*, bool, unsigned, HTMLCanvasElement*, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);
    void enqueueWriteBuffer(WebCLBuffer*, bool, unsigned, HTMLImageElement*, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);

    void enqueueWriteBufferRect(WebCLBuffer*, bool, const Vector<unsigned>&, const Vector<unsigned>&, const Vector<unsigned>&, unsigned, unsigned, unsigned, unsigned, DOMArrayBufferView*, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);
    void enqueueWriteBufferRect(WebCLBuffer*, bool, const Vector<unsigned>&, const Vector<unsigned>&, const Vector<unsigned>&, unsigned, unsigned, ImageData*, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);
    void enqueueWriteBufferRect(WebCLBuffer*, bool, const Vector<unsigned>&, const Vector<unsigned>&, const Vector<unsigned>&, unsigned, unsigned, HTMLCanvasElement*, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);
    void enqueueWriteBufferRect(WebCLBuffer*, bool, const Vector<unsigned>&, const Vector<unsigned>&, const Vector<unsigned>&, unsigned, unsigned, HTMLImageElement*, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);

    void enqueueReadBuffer(WebCLBuffer*, bool, unsigned, unsigned, DOMArrayBufferView*, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);
    void enqueueReadBuffer(WebCLBuffer*, bool, unsigned, unsigned, HTMLCanvasElement*, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);

    void enqueueReadBufferRect(WebCLBuffer*, bool, const Vector<unsigned>&, const Vector<unsigned>&, const Vector<unsigned>&, unsigned, unsigned, unsigned, unsigned, DOMArrayBufferView*, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);
    void enqueueReadBufferRect(WebCLBuffer*, bool, const Vector<unsigned>&, const Vector<unsigned>&, const Vector<unsigned>&, unsigned, unsigned, HTMLCanvasElement*, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);

    void enqueueReadImage(WebCLImage*, bool, const Vector<unsigned>&, const Vector<unsigned>&, unsigned, DOMArrayBufferView*, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);
    void enqueueReadImage(WebCLImage*, bool, const Vector<unsigned>&, const Vector<unsigned>&, HTMLCanvasElement*, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);

    void enqueueNDRangeKernel(WebCLKernel*, unsigned, const Vector<double>&, const Vector<double>&, const Vector<double>&, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);

    void enqueueWriteImage(WebCLImage*, bool, const Vector<unsigned>&, const Vector<unsigned>&, unsigned, DOMArrayBufferView*, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);
    void enqueueWriteImage(WebCLImage*, bool, const Vector<unsigned>&, const Vector<unsigned>&, ImageData*, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);
    void enqueueWriteImage(WebCLImage*, bool, const Vector<unsigned>&, const Vector<unsigned>&, HTMLCanvasElement*, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);
    void enqueueWriteImage(WebCLImage*, bool, const Vector<unsigned>&, const Vector<unsigned>&, HTMLImageElement*, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);
    void enqueueWriteImage(WebCLImage*, bool blockingWrite, HTMLVideoElement*, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);

    void enqueueCopyBuffer(WebCLBuffer*, WebCLBuffer*, unsigned, unsigned, unsigned, const Vector<RefPtr<WebCLEvent>>& events, WebCLEvent* event, ExceptionState&);
    void enqueueCopyBufferRect(WebCLBuffer*, WebCLBuffer*, const Vector<unsigned>&, const Vector<unsigned>&, const Vector<unsigned>&, unsigned, unsigned, unsigned, unsigned, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);
    void enqueueCopyImage(WebCLImage*, WebCLImage*, const Vector<unsigned>&, const Vector<unsigned>&, const Vector<unsigned>&, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);
    void enqueueCopyImageToBuffer(WebCLImage*, WebCLBuffer*, const Vector<unsigned>&, const Vector<unsigned>&, unsigned, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);
    void enqueueCopyBufferToImage(WebCLBuffer*, WebCLImage*, unsigned, const Vector<unsigned>&, const Vector<unsigned>&, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);

    void finish(WebCLCallback* whenFinished, ExceptionState&);
    void flush(ExceptionState&);
    void release() override;
    void enqueueBarrier(ExceptionState&);
    void enqueueMarker(WebCLEvent*, ExceptionState&);
    void enqueueWaitForEvents(const Vector<RefPtr<WebCLEvent>>&, ExceptionState&);

    enum SyncMethod {
        ASYNC,
        SYNC
    };
    void finishCommandQueues(SyncMethod);
    unsigned getProperties();
    bool isReleased() const { return !m_clCommandQueue; }

private:
    void enqueueWriteBufferBase(WebCLBuffer*, bool, unsigned, unsigned, void*, size_t, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);
    void enqueueReadBufferBase(WebCLBuffer*, bool, unsigned, unsigned, void*, size_t, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);
    void enqueueReadBufferRectBase(WebCLBuffer*, bool, const Vector<unsigned>&, const Vector<unsigned>&, const Vector<unsigned>&, unsigned, unsigned, unsigned, unsigned, void*, size_t, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);
    void enqueueReadImageBase(WebCLImage*, bool, const Vector<unsigned>&, const Vector<unsigned>&, unsigned, void*, size_t, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);
    void enqueueWriteBufferRectBase(WebCLBuffer*, bool, const Vector<unsigned>&, const Vector<unsigned>&, const Vector<unsigned>&, unsigned, unsigned, unsigned, unsigned, void*, size_t, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);
    void enqueueWriteImageBase(WebCLImage*, bool, const Vector<unsigned>&, const Vector<unsigned>&, unsigned, void*, size_t, const Vector<RefPtr<WebCLEvent>>&, WebCLEvent*, ExceptionState&);
    WebCLCommandQueue(cl_command_queue, PassRefPtr<WebCLContext>, WebCLDevice*);
    Vector<cl_event> WebCLEventVectorToCLEventVector(bool, Vector<RefPtr<WebCLEvent>>, ExceptionState&);
    cl_event* WebCLEventPtrToCLEventPtr(WebCLEvent*, ExceptionState&);
    bool isExtensionEnabled(WebCLContext*, const String& name) const;
    static void callbackProxy(cl_event, cl_int, void*);
    static void callbackProxyOnMainThread(PassOwnPtr<WebCLCommandQueueHolder>);
    void resetEventAndCallback();

    RefPtr<WebCLCallback> m_whenFinishCallback;
    cl_event m_eventForCallback;
    WebCLDevice* m_device;
    cl_command_queue m_clCommandQueue;
};

} // namespace blink

#endif // WebCLCommandQueue_h
