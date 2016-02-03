// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "wtf/build_config.h"

#if ENABLE(WEBCL)
#include "bindings/modules/v8/V8WebCLPlatform.h"
#include "core/webcl/WebCLException.h"
#include "modules/webcl/WebCL.h"
#include "modules/webcl/WebCLContext.h"
#include "modules/webcl/WebCLDevice.h"
#include "modules/webcl/WebCLOpenCL.h"

namespace blink {

WebCLDevice::~WebCLDevice()
{
    // Unlike WebCLContext / WebCLCommandQueue / WebCLProgram / ...,
    // WebCLDevice does not need to call a release() method here:
    // 1) OpenCL 1.1 runtime has no clReleaseDevice() or an alternative, so
    //    there's no need to release the device.
    // 2) The OpenCL 1.2 (or above) spec implies that clReleaseDevice() is only
    //    meaningful for sub devices, but no sub device is created in our WebCL
    //    1.0 implementation.
}

PassRefPtr<WebCLDevice> WebCLDevice::create(cl_device_id deviceId)
{
    return adoptRef(new WebCLDevice(deviceId, nullptr));
}

PassRefPtr<WebCLDevice> WebCLDevice::create(cl_device_id deviceId, WebCLPlatform* platform)
{
    return adoptRef(new WebCLDevice(deviceId, platform));
}

unsigned WebCLDevice::getQueueProperties()
{
    cl_command_queue_properties queueProperties = 0;
    clGetDeviceInfo(m_clDeviceId, CL_DEVICE_QUEUE_PROPERTIES, sizeof(cl_command_queue_properties), &queueProperties, nullptr);
    return static_cast<unsigned>(queueProperties);
}

unsigned long long WebCLDevice::getMaxMemAllocSize()
{
    cl_ulong ulongUnits = 0;
    clGetDeviceInfo(m_clDeviceId, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(cl_ulong), &ulongUnits, nullptr);
    return static_cast<unsigned long long>(ulongUnits);
}

unsigned WebCLDevice::getImage2DMaxWidth()
{
    size_t sizetUnits = 0;
    clGetDeviceInfo(m_clDeviceId, CL_DEVICE_IMAGE2D_MAX_WIDTH, sizeof(size_t), &sizetUnits, nullptr);
    return static_cast<unsigned>(sizetUnits);
}

unsigned WebCLDevice::getImage2DMaxHeight()
{
    size_t sizetUnits = 0;
    clGetDeviceInfo(m_clDeviceId, CL_DEVICE_IMAGE2D_MAX_HEIGHT, sizeof(size_t), &sizetUnits, nullptr);
    return static_cast<unsigned>(sizetUnits);
}

unsigned WebCLDevice::getMaxWorkGroup()
{
    size_t sizetUnits = 0;
    clGetDeviceInfo(m_clDeviceId, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &sizetUnits, nullptr);
    return static_cast<unsigned>(sizetUnits);
}

Vector<unsigned> WebCLDevice::getMaxWorkItem()
{
    size_t sizetUnits = 0;
    size_t sizetArray[3] = {0};
    cl_int err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(size_t), &sizetUnits, nullptr);
    if (err == CL_SUCCESS) {
       err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(sizetArray), &sizetArray, nullptr);
       if (err == CL_SUCCESS) {
           Vector<unsigned> values;
           for (unsigned i = 0; i < static_cast<unsigned>(sizetUnits); ++i)
               values.append(static_cast<unsigned>(sizetArray[i]));
           return values;
       }
    }
    return Vector<unsigned>();
}

PassRefPtr<WebCLPlatform> WebCLDevice::getPlatform() const {
    return m_platform;
}

ScriptValue WebCLDevice::getInfo(ScriptState* scriptState, unsigned deviceType, ExceptionState& es)
{
    v8::Handle<v8::Object> creationContext = scriptState->context()->Global();
    v8::Isolate* isolate = scriptState->isolate();

    if (!m_clDeviceId) {
        es.throwWebCLException(WebCLException::INVALID_DEVICE, WebCLException::invalidDeviceMessage);
        return ScriptValue(scriptState, v8::Null(isolate));
    }

    if (!WebCLInputChecker::isValidDeviceInfoType(deviceType)) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return ScriptValue(scriptState, v8::Null(isolate));
    }

    cl_int err = CL_SUCCESS;
    char deviceString[1024];
    cl_uint uintUnits = 0;
    size_t sizetUnits = 0;
    cl_ulong ulongUnits = 0;
    cl_uint infoValue = 0;
    cl_bool boolUnits = false;
    cl_device_type type = 0;
    cl_device_fp_config deviceFPConfig = 0;
    cl_device_mem_cache_type globalType = 0;
    cl_device_exec_capabilities exec = 0;
    cl_device_local_mem_type localType = 0;

    switch(deviceType) {
    case CL_DEVICE_PROFILE:
        return ScriptValue(scriptState, v8String(isolate, String("WEBCL_PROFILE")));
    case CL_DEVICE_VERSION:
        return ScriptValue(scriptState, v8String(isolate, String("WebCL 1.0")));
    case CL_DEVICE_OPENCL_C_VERSION:
        return ScriptValue(scriptState, v8String(isolate, String("WebCL C 1.0")));
    case CL_DEVICE_EXTENSIONS:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_EXTENSIONS, sizeof(deviceString), &deviceString, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8String(isolate, String(deviceString)));
        break;
    case CL_DEVICE_NAME:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_NAME, sizeof(deviceString), &deviceString, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8String(isolate, String(deviceString)));
        break;
    case CL_DEVICE_VENDOR:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_VENDOR, sizeof(deviceString), &deviceString, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8String(isolate, String(deviceString)));
        break;
    case CL_DRIVER_VERSION:
        err = clGetDeviceInfo(m_clDeviceId, CL_DRIVER_VERSION, sizeof(deviceString), &deviceString, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8String(isolate, String(deviceString)));
        break;
    case CL_DEVICE_ADDRESS_BITS:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_ADDRESS_BITS, sizeof(cl_uint), &uintUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(uintUnits)));
        break;
    case CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, sizeof(cl_uint), &uintUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(uintUnits)));
        break;
    case CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR, sizeof(cl_uint), &uintUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(uintUnits)));
        break;
    case CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT, sizeof(cl_uint), &uintUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(uintUnits)));
        break;
    case CL_DEVICE_NATIVE_VECTOR_WIDTH_INT:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_INT, sizeof(cl_uint), &uintUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(uintUnits)));
        break;
    case CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG, sizeof(cl_uint), &uintUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(uintUnits)));
        break;
    case CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT, sizeof(cl_uint), &uintUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(uintUnits)));
        break;
    case CL_DEVICE_MAX_CLOCK_FREQUENCY:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint), &uintUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(uintUnits)));
        break;
    case CL_DEVICE_MAX_CONSTANT_ARGS:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_MAX_CONSTANT_ARGS, sizeof(cl_uint), &uintUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(uintUnits)));
        break;
    case CL_DEVICE_MAX_READ_IMAGE_ARGS:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_MAX_READ_IMAGE_ARGS, sizeof(cl_uint), &uintUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(uintUnits)));
        break;
    case CL_DEVICE_MAX_SAMPLERS:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_MAX_SAMPLERS, sizeof(cl_uint), &uintUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(uintUnits)));
        break;
    case CL_DEVICE_MAX_WRITE_IMAGE_ARGS:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_MAX_WRITE_IMAGE_ARGS, sizeof(cl_uint), &uintUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(uintUnits)));
        break;
    case CL_DEVICE_MEM_BASE_ADDR_ALIGN:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof(cl_uint), &uintUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(uintUnits)));
        break;
    case CL_DEVICE_VENDOR_ID:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_VENDOR_ID, sizeof(cl_uint), &uintUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(uintUnits)));
        break;
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, sizeof(cl_uint), &uintUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(uintUnits)));
        break;
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, sizeof(cl_uint), &uintUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(uintUnits)));
        break;
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, sizeof(cl_uint), &uintUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(uintUnits)));
        break;
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, sizeof(cl_uint), &uintUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(uintUnits)));
        break;
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, sizeof(cl_uint), &uintUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(uintUnits)));
        break;
    case CL_DEVICE_MAX_COMPUTE_UNITS:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &uintUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(uintUnits)));
        break;
    case CL_DEVICE_IMAGE2D_MAX_HEIGHT: {
        unsigned result = getImage2DMaxHeight();
        if (result)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, result));
        break;
    }
    case CL_DEVICE_IMAGE2D_MAX_WIDTH: {
        unsigned result = getImage2DMaxWidth();
        if (result)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, result));
        break;
    }
    case CL_DEVICE_IMAGE3D_MAX_DEPTH:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_IMAGE3D_MAX_DEPTH, sizeof(size_t), &sizetUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(sizetUnits)));
        break;
    case CL_DEVICE_IMAGE3D_MAX_HEIGHT:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_IMAGE3D_MAX_HEIGHT, sizeof(size_t), &sizetUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(sizetUnits)));
        break;
    case CL_DEVICE_IMAGE3D_MAX_WIDTH:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_IMAGE3D_MAX_WIDTH, sizeof(size_t), &sizetUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(sizetUnits)));
        break;
    case CL_DEVICE_MAX_PARAMETER_SIZE:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_MAX_PARAMETER_SIZE, sizeof(size_t), &sizetUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(sizetUnits)));
        break;
    case CL_DEVICE_MAX_WORK_GROUP_SIZE: {
        unsigned result  = getMaxWorkGroup();
        if (result)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, result));
        break;
    }
    case CL_DEVICE_MAX_WORK_ITEM_SIZES: {
        Vector<unsigned> result = getMaxWorkItem();
        if (result.size())
            return ScriptValue(scriptState, toV8(result, creationContext, isolate));
        break;
    }
    case CL_DEVICE_PROFILING_TIMER_RESOLUTION:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_PROFILING_TIMER_RESOLUTION, sizeof(size_t), &sizetUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(sizetUnits)));
        break;
    case CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(size_t), &sizetUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(sizetUnits)));
        break;
    case CL_DEVICE_LOCAL_MEM_SIZE:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &ulongUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned long long>(ulongUnits)));
        break;
    case CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, sizeof(cl_ulong), &ulongUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned long long>(ulongUnits)));
        break;
    case CL_DEVICE_MAX_MEM_ALLOC_SIZE: {
        unsigned long long result = getMaxMemAllocSize();
        if (result)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, result));
        break;
    }
    case CL_DEVICE_GLOBAL_MEM_CACHE_SIZE:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, sizeof(cl_ulong), &ulongUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned long long>(ulongUnits)));
        break;
    case CL_DEVICE_GLOBAL_MEM_SIZE:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), &ulongUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned long long>(ulongUnits)));
        break;
    case CL_DEVICE_AVAILABLE:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_AVAILABLE, sizeof(cl_bool), &boolUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Boolean::New(isolate, static_cast<bool>(boolUnits)));
        break;
    case CL_DEVICE_COMPILER_AVAILABLE:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_COMPILER_AVAILABLE, sizeof(cl_bool), &boolUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Boolean::New(isolate, static_cast<bool>(boolUnits)));
        break;
    case CL_DEVICE_HOST_UNIFIED_MEMORY:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_HOST_UNIFIED_MEMORY, sizeof(cl_bool), &boolUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Boolean::New(isolate, static_cast<bool>(boolUnits)));
        break;
    case CL_DEVICE_ENDIAN_LITTLE:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_ENDIAN_LITTLE, sizeof(cl_bool), &boolUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Boolean::New(isolate, static_cast<bool>(boolUnits)));
        break;
    case CL_DEVICE_ERROR_CORRECTION_SUPPORT:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_ERROR_CORRECTION_SUPPORT, sizeof(cl_bool), &boolUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Boolean::New(isolate, static_cast<bool>(boolUnits)));
        break;
    case CL_DEVICE_IMAGE_SUPPORT:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_IMAGE_SUPPORT, sizeof(cl_bool), &boolUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Boolean::New(isolate, static_cast<bool>(boolUnits)));
        break;
    case CL_DEVICE_TYPE:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_TYPE, sizeof(type), &type, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(type)));
        break;
    case CL_DEVICE_SINGLE_FP_CONFIG:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_SINGLE_FP_CONFIG, sizeof(deviceFPConfig), &deviceFPConfig, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(deviceFPConfig)));
        break;
    case CL_DEVICE_QUEUE_PROPERTIES: {
        unsigned result = getQueueProperties();
        if (result)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, result));
        break;
    }
    case CL_DEVICE_PLATFORM:
        return ScriptValue(scriptState, toV8(m_platform, creationContext, isolate));
    case CL_DEVICE_EXECUTION_CAPABILITIES:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_EXECUTION_CAPABILITIES, sizeof(cl_device_exec_capabilities), &exec, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(exec)));
        break;
    case CL_DEVICE_GLOBAL_MEM_CACHE_TYPE:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_GLOBAL_MEM_CACHE_TYPE, sizeof(cl_device_mem_cache_type), &globalType, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(globalType)));
        break;
    case CL_DEVICE_LOCAL_MEM_TYPE:
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_LOCAL_MEM_TYPE, sizeof(cl_device_local_mem_type), &localType, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(localType)));
        break;
    case CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE:
        if (!m_extension.isEnabledExtension("KHR_fp64"))
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, 0));
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE, sizeof(cl_uint), &infoValue, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(infoValue)));
        break;
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE:
        if (!m_extension.isEnabledExtension("KHR_fp64"))
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, 0));
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, sizeof(cl_uint), &infoValue, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(infoValue)));
        break;
    case CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF:
        if (!m_extension.isEnabledExtension("KHR_fp16"))
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, 0));
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF, sizeof(cl_uint), &infoValue, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(infoValue)));
        break;
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF:
        if (!m_extension.isEnabledExtension("KHR_fp16"))
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, 0));
        err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF, sizeof(cl_uint), &infoValue, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(infoValue)));
        break;
    default:
        es.throwWebCLException(WebCLException::FAILURE, WebCLException::failureMessage);
        return ScriptValue(scriptState, v8::Null(isolate));
    }

    WebCLException::throwException(err, es);
    return ScriptValue(scriptState, v8::Null(isolate));
}

void WebCLDevice::cacheDeviceExtensions()
{
    char deviceString[MULTI_EXTENSIONS_LENGTH] = "";
    Vector<String> extensions;

    if (!m_clDeviceId)
        return;

    cl_int err = clGetDeviceInfo(m_clDeviceId, CL_DEVICE_EXTENSIONS, sizeof(deviceString), &deviceString, nullptr);

    if (err != CL_SUCCESS)
        return;

    String temp = String(deviceString);
    temp.split(' ', extensions);

    for (auto extension : extensions) {
        if (!extension.containsOnlyWhitespace())
            m_extension.addSupportedCLExtension(String(extension));
    }
}

bool WebCLDevice::enableExtension(const String& name)
{
    return m_extension.enableExtension(name);
}

Vector<String> WebCLDevice::getSupportedExtensions()
{
    return m_extension.getSupportedExtensions();
}

void WebCLDevice::getEnabledExtensions(HashSet<String>& extensions)
{
    m_extension.getEnabledExtensions(extensions);
}

WebCLDevice::WebCLDevice(cl_device_id device, WebCLPlatform* platform)
    : m_platform(platform)
    , m_clDeviceId(device)
{
    cacheDeviceExtensions();
}

} // namespace blink

#endif // ENABLE(WEBCL)
