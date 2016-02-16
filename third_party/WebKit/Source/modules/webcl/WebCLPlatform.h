// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLPlatform_h
#define WebCLPlatform_h

#include "modules/webcl/WebCL.h"
#include "modules/webcl/WebCLConfig.h"
#include "modules/webcl/WebCLExtension.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace blink {

class ExceptionState;
class WebCLCommandQueue;
class WebCLImage;

class WebCLPlatform : public RefCounted<WebCLPlatform>, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    ~WebCLPlatform();
    static PassRefPtr<WebCLPlatform> create(cl_platform_id);

    Vector<RefPtr<WebCLDevice>> getDevices(unsigned, ExceptionState&);
    Vector<RefPtr<WebCLDevice>> getDevices(ExceptionState&);
    ScriptValue getInfo(ScriptState*, int, ExceptionState&);

    void cachePlatformExtensions();
    bool enableExtension(const String& name);
    Vector<String> getSupportedExtensions();
    void getEnabledExtensions(HashSet<String>& extensions);
    cl_platform_id getPlatformId() { return m_clPlatformId; }

private:
    WebCLPlatform(cl_platform_id);

    Vector<RefPtr<WebCLDevice>> m_devices;
    unsigned m_cachedDeviceType;
    WebCLExtension m_extension;
    cl_platform_id m_clPlatformId;
};

} // namespace blink

#endif // WebCLPlatform_h
