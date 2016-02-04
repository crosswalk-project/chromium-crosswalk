// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLDevice_h
#define WebCLDevice_h

#include "modules/webcl/WebCLConfig.h"
#include "modules/webcl/WebCLExtension.h"
#include "modules/webcl/WebCLPlatform.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace blink {

class ExceptionState;
class WebCL;
class WebCLGetInfo;
class WebCLPlatform;

class WebCLDevice : public RefCounted<WebCLDevice>, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    ~WebCLDevice();
    static PassRefPtr<WebCLDevice> create(cl_device_id);
    static PassRefPtr<WebCLDevice> create(cl_device_id, WebCLPlatform* platform);

    ScriptValue getInfo(ScriptState*, unsigned, ExceptionState&);

    void cacheDeviceExtensions();
    bool enableExtension(const String& name);
    void getEnabledExtensions(HashSet<String>& extensions);
    Vector<String> getSupportedExtensions();
    unsigned getQueueProperties();
    unsigned long long getMaxMemAllocSize();
    unsigned getImage2DMaxWidth();
    unsigned getImage2DMaxHeight();
    unsigned getMaxWorkGroup();
    Vector<unsigned> getMaxWorkItem();
    PassRefPtr<WebCLPlatform> getPlatform() const;
    cl_device_id getDeviceId() { return m_clDeviceId; }

private:
    WebCLDevice(cl_device_id, WebCLPlatform* platform);

    WebCLPlatform* m_platform;
    WebCLExtension m_extension;
    cl_device_id m_clDeviceId;
};

} // namespace blink

#endif // WebCLDevice_h
