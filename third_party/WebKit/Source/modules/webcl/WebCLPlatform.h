// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLPlatform_h
#define WebCLPlatform_h

#include "bindings/core/v8/ScriptValue.h"
#include "bindings/core/v8/ScriptWrappable.h"
#include "modules/webcl/WebCLExtension.h"
#include "modules/webcl/WebCLOpenCL.h"
#include "wtf/HashSet.h"
#include "wtf/PassRefPtr.h"
#include "wtf/RefCounted.h"
#include "wtf/RefPtr.h"
#include "wtf/Vector.h"
#include "wtf/text/StringHash.h"
#include "wtf/text/WTFString.h"

namespace blink {

class ExceptionState;
class ScriptState;
class WebCLDevice;

class WebCLPlatform final : public RefCounted<WebCLPlatform>, public ScriptWrappable {
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
