// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLDevice_h
#define WebCLDevice_h

#include "bindings/core/v8/ScriptValue.h"
#include "bindings/core/v8/ScriptWrappable.h"
#include "modules/webcl/WebCLExtension.h"
#include "modules/webcl/WebCLOpenCL.h"
#include "wtf/HashSet.h"
#include "wtf/PassRefPtr.h"
#include "wtf/RefCounted.h"
#include "wtf/Vector.h"
#include "wtf/text/StringHash.h"
#include "wtf/text/WTFString.h"

namespace blink {

class ExceptionState;
class ScriptState;
class WebCLPlatform;

class WebCLDevice final : public RefCounted<WebCLDevice>, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    ~WebCLDevice();
    static PassRefPtr<WebCLDevice> create(cl_device_id);
    static PassRefPtr<WebCLDevice> create(cl_device_id, WebCLPlatform*);

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
    WebCLDevice(cl_device_id, WebCLPlatform*);

    WebCLPlatform* m_platform;
    WebCLExtension m_extension;
    cl_device_id m_clDeviceId;
};

} // namespace blink

#endif // WebCLDevice_h
