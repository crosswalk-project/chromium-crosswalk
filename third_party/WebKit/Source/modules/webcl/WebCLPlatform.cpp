// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#if ENABLE(WEBCL)
#include "bindings/core/v8/V8Binding.h"
#include "core/webcl/WebCLException.h"
#include "modules/webcl/WebCL.h"
#include "modules/webcl/WebCLOpenCL.h"
#include "modules/webcl/WebCLPlatform.h"

namespace blink {

WebCLPlatform::~WebCLPlatform()
{
}

PassRefPtr<WebCLPlatform> WebCLPlatform::create(cl_platform_id m_clPlatformId)
{
    return adoptRef(new WebCLPlatform(m_clPlatformId));
}

Vector<RefPtr<WebCLDevice>> WebCLPlatform::getDevices(ExceptionState& es)
{
    return getDevices(CL_DEVICE_TYPE_ALL, es);
}

Vector<RefPtr<WebCLDevice>> WebCLPlatform::getDevices(unsigned deviceType, ExceptionState& es)
{
    if (!m_clPlatformId) {
        es.throwWebCLException(WebCLException::INVALID_PLATFORM, WebCLException::invalidPlatformMessage);
        return Vector<RefPtr<WebCLDevice>>();
    }

    if (deviceType && !WebCLInputChecker::isValidDeviceType(deviceType)) {
        es.throwWebCLException(WebCLException::INVALID_DEVICE_TYPE, WebCLException::invalidDeviceTypeMessage);
        return Vector<RefPtr<WebCLDevice>>();
    }

    if (!deviceType)
        deviceType = CL_DEVICE_TYPE_ALL;

    if (m_cachedDeviceType == deviceType && m_devices.size())
        return m_devices;

    cl_int err = CL_SUCCESS;
    cl_uint numDevices = 0;
    switch(deviceType) {
    case CL_DEVICE_TYPE_GPU:
        err = clGetDeviceIDs(m_clPlatformId, CL_DEVICE_TYPE_GPU, 0, nullptr, &numDevices);
        break;
    case CL_DEVICE_TYPE_CPU:
        err = clGetDeviceIDs(m_clPlatformId, CL_DEVICE_TYPE_CPU, 0, nullptr, &numDevices);
        break;
    case CL_DEVICE_TYPE_ACCELERATOR:
        err = clGetDeviceIDs(m_clPlatformId, CL_DEVICE_TYPE_ACCELERATOR, 0, nullptr, &numDevices);
        break;
    case CL_DEVICE_TYPE_DEFAULT:
        err = clGetDeviceIDs(m_clPlatformId, CL_DEVICE_TYPE_DEFAULT, 0, nullptr, &numDevices);
        break;
    case CL_DEVICE_TYPE_ALL:
        err = clGetDeviceIDs(m_clPlatformId, CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    if (err != CL_SUCCESS) {
        WebCLException::throwException(err, es);
        return Vector<RefPtr<WebCLDevice>>();
    }

    Vector<cl_device_id> clDevices;
    clDevices.resize(numDevices);

    switch (deviceType) {
    case CL_DEVICE_TYPE_GPU:
        err = clGetDeviceIDs(m_clPlatformId, CL_DEVICE_TYPE_GPU, numDevices, clDevices.data(), nullptr);
        break;
    case CL_DEVICE_TYPE_CPU:
        err = clGetDeviceIDs(m_clPlatformId, CL_DEVICE_TYPE_CPU, numDevices, clDevices.data(), nullptr);
        break;
    case CL_DEVICE_TYPE_ACCELERATOR:
        err = clGetDeviceIDs(m_clPlatformId, CL_DEVICE_TYPE_ACCELERATOR, numDevices, clDevices.data(), nullptr);
        break;
    case CL_DEVICE_TYPE_DEFAULT:
        err = clGetDeviceIDs(m_clPlatformId, CL_DEVICE_TYPE_DEFAULT, numDevices, clDevices.data(), nullptr );
        break;
    case CL_DEVICE_TYPE_ALL:
        err = clGetDeviceIDs(m_clPlatformId, CL_DEVICE_TYPE_ALL, numDevices, clDevices.data(), nullptr);
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    if (err != CL_SUCCESS) {
        WebCLException::throwException(err, es);
        return Vector<RefPtr<WebCLDevice>>();
    }

    m_devices.clear();
    for (auto clDevice : clDevices) {
        RefPtr<WebCLDevice> device = WebCLDevice::create(clDevice, this);
        if (device)
            m_devices.append(device);
    }

    clDevices.clear();
    m_cachedDeviceType = deviceType;
    return m_devices;
}

ScriptValue WebCLPlatform::getInfo(ScriptState* scriptState, int platformInfo, ExceptionState& es)
{
    v8::Isolate* isolate = scriptState->isolate();

    if (!m_clPlatformId) {
        es.throwWebCLException(WebCLException::INVALID_PLATFORM, WebCLException::invalidPlatformMessage);
        return ScriptValue(scriptState, v8::Null(isolate));
    }

    switch (platformInfo) {
    case CL_PLATFORM_PROFILE:
        return ScriptValue(scriptState, v8String(isolate, String("WEBCL_PROFILE")));
    case CL_PLATFORM_VERSION:
        return ScriptValue(scriptState, v8String(isolate, String("WebCL 1.0")));
    case CL_PLATFORM_NAME:
    case CL_PLATFORM_VENDOR:
    case CL_PLATFORM_EXTENSIONS:
        return ScriptValue(scriptState, v8String(isolate, emptyString()));
    default:
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return ScriptValue(scriptState, v8::Null(isolate));
    }
}

void WebCLPlatform::cachePlatformExtensions()
{
    char platformString[MULTI_EXTENSIONS_LENGTH] = "";
    Vector<String> extensions;

    if (!m_clPlatformId)
        return;

    cl_int err = clGetPlatformInfo(m_clPlatformId, CL_PLATFORM_EXTENSIONS, sizeof(platformString), &platformString, nullptr);

    if (err != CL_SUCCESS)
        return;

    String temp = String(platformString);
    temp.split(' ', extensions);

    for (auto extension : extensions) {
        if (!extension.containsOnlyWhitespace())
            m_extension.addSupportedCLExtension(String(extension));
    }
}

bool WebCLPlatform::enableExtension(const String& name)
{
    return m_extension.enableExtension(name);
}

Vector<String> WebCLPlatform::getSupportedExtensions()
{
    return m_extension.getSupportedExtensions();
}

void WebCLPlatform::getEnabledExtensions(HashSet<String>& extensions)
{
    m_extension.getEnabledExtensions(extensions);
}

WebCLPlatform::WebCLPlatform(cl_platform_id platform)
    : m_cachedDeviceType(0)
    , m_clPlatformId(platform)
{
    cachePlatformExtensions();
}

} // namespace blink

#endif // ENABLE(WEBCL)
