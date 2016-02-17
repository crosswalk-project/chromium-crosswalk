// Copyright (C) 2013 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "wtf/build_config.h"

#if ENABLE(WEBCL)

#include "modules/webcl/WebCLExtension.h"

namespace blink {

bool WebCLExtension::enableExtension(const String& name)
{
    if (equalIgnoringCase(name, "WEBCL_html_image")) {
        m_enabledExtensions.append("WEBCL_html_image");
        return true;
    }

    if (equalIgnoringCase(name, "WEBCL_html_video")) {
        m_enabledExtensions.append("WEBCL_html_video");
        return true;
    }

    if (equalIgnoringCase(name, "KHR_fp64")) {
        bool khrFP64 = m_supportedCLExtensions.contains("cl_khr_fp64");
        if (khrFP64)
            m_enabledExtensions.append("KHR_fp64");
        return khrFP64;
    }

    if (equalIgnoringCase(name, "KHR_fp16")) {
        bool khrFP16 = m_supportedCLExtensions.contains("cl_khr_fp16");
        if (khrFP16)
            m_enabledExtensions.append("KHR_fp16");
        return khrFP16;
    }

    return false;
}

Vector<String> WebCLExtension::getSupportedExtensions()
{
    // Filter the opencl supported extension.
    Vector<String> result;
    result.append("WEBCL_html_image");
    result.append("WEBCL_html_video");
    if (m_supportedCLExtensions.contains("cl_khr_fp64"))
        result.append("KHR_fp64");

    if (m_supportedCLExtensions.contains("cl_khr_fp16"))
        result.append("KHR_fp16");

    return result;
}

bool WebCLExtension::isEnabledExtension(const String& name) const
{
    return m_enabledExtensions.contains(name);
}

void WebCLExtension::getEnabledExtensions(HashSet<String>& extensions)
{
    for (auto enabledExtension : m_enabledExtensions) {
        const String& extensionString = enabledExtension;
        if (!extensionString.isEmpty())
            extensions.add(extensionString);
    }
}

void WebCLExtension::addSupportedCLExtension(const String& name)
{
    m_supportedCLExtensions.append(name);
}

} // blink

#endif // ENABLE(WEBCL)
