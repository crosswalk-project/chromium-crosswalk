// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bindings/modules/v8/V8WebCLContext.h"
#include "core/webcl/WebCLException.h"
#include "modules/webcl/WebCL.h"
#include "modules/webcl/WebCLKernel.h"
#include "modules/webcl/WebCLOpenCL.h"
#include "modules/webcl/WebCLProgram.h"
#include "platform/ThreadSafeFunctional.h"
#include "public/platform/Platform.h"
#include "public/platform/WebTaskRunner.h"
#include "public/platform/WebTraceLocation.h"

namespace blink {

// The holder of WebCLProgram.
class WebCLProgramHolder {
public:
    WeakPtr<WebCLObject> program;
    cl_program program2;
};

static bool isASCIILineBreakOrWhiteSpaceCharacter(UChar c)
{
    return c == '\r' || c == '\n' || c == ' ';
}

WebCLProgram::~WebCLProgram()
{
    release();
    ASSERT(!m_clProgram);
}

PassRefPtr<WebCLProgram> WebCLProgram::create(cl_program program, PassRefPtr<WebCLContext> context, const String& kernelSource)
{
    return adoptRef(new WebCLProgram(program, context, kernelSource));
}

bool WebCLProgram::isExtensionEnabled(RefPtr<blink::WebCLContext> context, const String& name)
{
    return context->isExtensionEnabled(name);
}

static void removeComments(const String& inSource, String& outSource)
{
    enum Mode { DEFAULT, BLOCK_COMMENT, LINE_COMMENT };
    Mode currentMode = DEFAULT;

    ASSERT(!inSource.isNull());
    ASSERT(!inSource.isEmpty());

    outSource = inSource;
    for (unsigned i = 0; i < outSource.length(); ++i) {
        if (currentMode == BLOCK_COMMENT) {
            if (outSource[i] == '*' && outSource[i + 1] == '/') {
                outSource.replace(i++, 2, "  ");
                currentMode = DEFAULT;
                continue;
            }
            outSource.replace(i, 1, " ");
            continue;
        }

        if (currentMode == LINE_COMMENT) {
            if (outSource[i] == '\n' || outSource[i] == '\r') {
                currentMode = DEFAULT;
                continue;
            }
            outSource.replace(i, 1, " ");
            continue;
        }

        if (outSource[i] == '/') {
            if (outSource[i + 1] == '*') {
                outSource.replace(i++, 2, "  ");
                currentMode = BLOCK_COMMENT;
                continue;
            }

            if (outSource[i + 1] == '/') {
                outSource.replace(i++, 2, "  ");
                currentMode = LINE_COMMENT;
                continue;
            }
        }
    }
}

ScriptValue WebCLProgram::getInfo(ScriptState* scriptState, int paramName, ExceptionState& es)
{
    v8::Handle<v8::Object> creationContext = scriptState->context()->Global();
    v8::Isolate* isolate = scriptState->isolate();

    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_PROGRAM, WebCLException::invalidProgramMessage);
        return ScriptValue(scriptState, v8::Null(isolate));
    }

    cl_int err = CL_SUCCESS;
    cl_uint uintUnits = 0;
    Vector<RefPtr<WebCLDevice>> result = context()->getDevices();

    switch(paramName) {
    case CL_PROGRAM_NUM_DEVICES:
        err = clGetProgramInfo(m_clProgram, CL_PROGRAM_NUM_DEVICES, sizeof(cl_uint), &uintUnits, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(uintUnits)));
        WebCLException::throwException(err, es);
        return ScriptValue(scriptState, v8::Null(isolate));
    case CL_PROGRAM_SOURCE:
        return ScriptValue(scriptState, v8String(isolate, m_programSource));
    case CL_PROGRAM_CONTEXT:
        return ScriptValue(scriptState, toV8(context(), creationContext, isolate));
    case CL_PROGRAM_DEVICES:
        return ScriptValue(scriptState, toV8(result, creationContext, isolate));
    default:
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return ScriptValue(scriptState, v8::Null(isolate));
    }
}

ScriptValue WebCLProgram::getBuildInfo(ScriptState* scriptState, WebCLDevice* device, int paramName, ExceptionState& es)
{
    v8::Isolate* isolate = scriptState->isolate();

    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_PROGRAM, WebCLException::invalidProgramMessage);
        return ScriptValue(scriptState, v8::Null(isolate));
    }

    cl_device_id clDevice = nullptr;
    if (device) {
        clDevice = device->getDeviceId();
        if (!clDevice) {
            es.throwWebCLException(WebCLException::INVALID_DEVICE, WebCLException::invalidDeviceMessage);
            return ScriptValue(scriptState, v8::Null(isolate));
        }
        size_t i = 0;
        Vector<RefPtr<WebCLDevice>> deviceList = context()->getDevices();
        for (; i < deviceList.size(); i ++) {
            if (clDevice == deviceList[i]->getDeviceId())
                break;
        }
        if (i == deviceList.size()) {
            es.throwWebCLException(WebCLException::INVALID_DEVICE, WebCLException::invalidDeviceMessage);
            return ScriptValue(scriptState, v8::Null(isolate));
        }
    }

    cl_int err = CL_SUCCESS;
    char *buffer;
    size_t len = 0;
    switch (paramName) {
    case CL_PROGRAM_BUILD_LOG: {
        err = clGetProgramBuildInfo(m_clProgram, clDevice, CL_PROGRAM_BUILD_LOG, 0, nullptr, &len);
        if (err != CL_SUCCESS)
            break;
        buffer = new char[len + 1];
        err = clGetProgramBuildInfo(m_clProgram, clDevice, CL_PROGRAM_BUILD_LOG, len, buffer, nullptr);
        if (err != CL_SUCCESS)
            break;
        String result(buffer);
        delete [] buffer;
        return ScriptValue(scriptState, v8String(isolate, result));
    }
    case CL_PROGRAM_BUILD_OPTIONS: {
        err = clGetProgramBuildInfo(m_clProgram, clDevice, CL_PROGRAM_BUILD_OPTIONS, 0, nullptr, &len);
        if (err != CL_SUCCESS)
            break;
        buffer = new char[len + 1];
        err = clGetProgramBuildInfo(m_clProgram, clDevice, CL_PROGRAM_BUILD_OPTIONS, len, buffer, nullptr);
        if (err != CL_SUCCESS)
            break;
        String result(buffer);
        delete [] buffer;
        return ScriptValue(scriptState, v8String(isolate, result));
    }
    case CL_PROGRAM_BUILD_STATUS:
        cl_build_status buildStatus;
        err = clGetProgramBuildInfo(m_clProgram, clDevice, CL_PROGRAM_BUILD_STATUS, sizeof(cl_build_status), &buildStatus, nullptr);
        if (err == CL_SUCCESS)
            return ScriptValue(scriptState, v8::Integer::NewFromUnsigned(isolate, static_cast<unsigned>(buildStatus)));
        break;
    default:
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return ScriptValue(scriptState, v8::Null(isolate));
    }

    WebCLException::throwException(err, es);
    return ScriptValue(scriptState, v8::Null(isolate));
}

PassRefPtr<WebCLKernel> WebCLProgram::createKernel(const String& kernelName, ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_PROGRAM, WebCLException::invalidProgramMessage);
        return nullptr;
    }

    if (!m_isProgramBuilt) {
        es.throwWebCLException(WebCLException::INVALID_PROGRAM_EXECUTABLE, WebCLException::invalidProgramExecutableMessage);
        return nullptr;
    }

    if (!m_programSource.contains(kernelName)) {
        es.throwWebCLException(WebCLException::INVALID_KERNEL_NAME, WebCLException::invalidKernelNameMessage);
        return nullptr;
    }

    cl_int err = CL_SUCCESS;
    const char* kernelNameStr = strdup(kernelName.utf8().data());
    cl_kernel clKernelId = clCreateKernel(m_clProgram, kernelNameStr, &err);

    if (err != CL_SUCCESS) {
        WebCLException::throwException(err, es);
        return nullptr;
    }
    RefPtr<WebCLKernel> kernel = WebCLKernel::create(clKernelId, context(), this, kernelName);
    return kernel;
}

Vector<RefPtr<WebCLKernel>> WebCLProgram::createKernelsInProgram(ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_PROGRAM, WebCLException::invalidProgramMessage);
        return Vector<RefPtr<WebCLKernel>>();
    }

    if (!m_isProgramBuilt) {
        es.throwWebCLException(WebCLException::INVALID_PROGRAM_EXECUTABLE, WebCLException::invalidProgramExecutableMessage);
        return Vector<RefPtr<WebCLKernel>>();
    }

    cl_uint num = 0;
    cl_int err = clCreateKernelsInProgram(m_clProgram, 0, nullptr, &num);
    if (err != CL_SUCCESS) {
        WebCLException::throwException(err, es);
        return Vector<RefPtr<WebCLKernel>>();
    }

    if (num == 0) {
        es.throwWebCLException(WebCLException::FAILURE, WebCLException::failureMessage);
        return Vector<RefPtr<WebCLKernel>>();
    }

    cl_kernel* kernelBuf = (cl_kernel*)malloc (sizeof(cl_kernel) * num);
    if (!kernelBuf) {
        return Vector<RefPtr<WebCLKernel>>();
    }

    err = clCreateKernelsInProgram(m_clProgram, num, kernelBuf, nullptr);

    if (err != CL_SUCCESS) {
        WebCLException::throwException(err, es);
        return Vector<RefPtr<WebCLKernel>>();
    }

    Vector<char> kernelName;
    size_t bytesOfKernelName = 0;
    Vector<RefPtr<WebCLKernel>> m_kernelList;
    for (size_t i = 0 ; i < num; i++) {
        err = clGetKernelInfo(kernelBuf[i], CL_KERNEL_FUNCTION_NAME, 0, nullptr, &bytesOfKernelName);
        if (err != CL_SUCCESS) {
            continue;
        }

        kernelName.reserveCapacity(bytesOfKernelName);
        kernelName.resize(bytesOfKernelName);

        err = clGetKernelInfo(kernelBuf[i], CL_KERNEL_FUNCTION_NAME, bytesOfKernelName, kernelName.data(), 0);

        if (err != CL_SUCCESS) {
            continue;
        }

        RefPtr<WebCLKernel> kernel = WebCLKernel::create(kernelBuf[i], context(), this, static_cast<const char*>(kernelName.data()));

        if (kernel)
            m_kernelList.append(kernel);
        kernelName.clear();
        bytesOfKernelName = 0;
    }

    return m_kernelList;
}

void WebCLProgram::build(const Vector<RefPtr<WebCLDevice>>& devices, const String& buildOptions, WebCLCallback* callback, ExceptionState& es)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_PROGRAM, WebCLException::invalidProgramMessage);
        return;
    }

    size_t kernel_label = m_programSource.find("__kernel ", 0);
    while (kernel_label != WTF::kNotFound) {
        size_t openBrace = m_programSource.find("{", kernel_label);
        size_t openBraket = m_programSource.reverseFind("(", openBrace);
        size_t space = m_programSource.reverseFind(" ", openBraket);
        String kernelName = m_programSource.substring(space + 1, openBraket - space - 1);
        if (kernelName.length() > 254) {
            // Kernel Name length isn't allowed larger than 255.
            es.throwWebCLException(WebCLException::BUILD_PROGRAM_FAILURE, WebCLException::buildProgramFailureMessage);
            return;
        }

        size_t closeBraket = m_programSource.find(")", openBraket);
        String arguments = m_programSource.substring(openBraket + 1, closeBraket - openBraket - 1);
        if (arguments.contains("struct ") || arguments.contains("image1d_array_t ") || arguments.contains("image1d_buffer_t ") || arguments.contains("image1d_t ") || arguments.contains("image2d_array_t ")) {
            // 1. Kernel structure parameters aren't allowed;
            // 2. Kernel argument "image1d_t", "image1d_array_t", "image2d_array_t" and "image1d_buffer_t" aren't allowed;
            es.throwWebCLException(WebCLException::BUILD_PROGRAM_FAILURE, WebCLException::buildProgramFailureMessage);
            return;
        }

        size_t closeBrace = m_programSource.find("}", openBrace);
        String codeString =  m_programSource.substring(openBrace + 1, closeBrace - openBrace - 1).removeCharacters(isASCIILineBreakOrWhiteSpaceCharacter);
        if (codeString.isEmpty()) {
            // Kernel code isn't empty;
            es.throwWebCLException(WebCLException::BUILD_PROGRAM_FAILURE, WebCLException::buildProgramFailureMessage);
            return;
        }

        kernel_label = m_programSource.find("__kernel ", closeBrace);
    }

    if (buildOptions.length() > 0) {
        static AtomicString& buildOptionDashD = *new AtomicString("-D", AtomicString::ConstructFromLiteral);
        static HashSet<AtomicString>& webCLSupportedBuildOptions = *new HashSet<AtomicString>();
        if (webCLSupportedBuildOptions.isEmpty()) {
            webCLSupportedBuildOptions.add(AtomicString("-cl-opt-disable", AtomicString::ConstructFromLiteral));
            webCLSupportedBuildOptions.add(AtomicString("-cl-single-precision-constant", AtomicString::ConstructFromLiteral));
            webCLSupportedBuildOptions.add(AtomicString("-cl-denorms-are-zero", AtomicString::ConstructFromLiteral));
            webCLSupportedBuildOptions.add(AtomicString("-cl-mad-enable", AtomicString::ConstructFromLiteral));
            webCLSupportedBuildOptions.add(AtomicString("-cl-no-signed-zeros", AtomicString::ConstructFromLiteral));
            webCLSupportedBuildOptions.add(AtomicString("-cl-unsafe-math-optimizations", AtomicString::ConstructFromLiteral));
            webCLSupportedBuildOptions.add(AtomicString("-cl-finite-math-only", AtomicString::ConstructFromLiteral));
            webCLSupportedBuildOptions.add(AtomicString("-cl-fast-relaxed-math", AtomicString::ConstructFromLiteral));
            webCLSupportedBuildOptions.add(AtomicString("-w", AtomicString::ConstructFromLiteral));
            webCLSupportedBuildOptions.add(AtomicString("-Werror", AtomicString::ConstructFromLiteral));
        }

        Vector<String> webCLBuildOptionsVector;
        buildOptions.split(" ", false, webCLBuildOptionsVector);

        for (size_t i = 0; i < webCLBuildOptionsVector.size(); i++) {
            // Every build option must start with a hyphen.
            if (!webCLBuildOptionsVector[i].startsWith("-")) {
                es.throwWebCLException(WebCLException::INVALID_BUILD_OPTIONS, WebCLException::invalidBuildOptionsMessage);
                return;
            }

            if (webCLSupportedBuildOptions.contains(AtomicString(webCLBuildOptionsVector[i])))
                continue;

            if (webCLBuildOptionsVector[i].startsWith(buildOptionDashD)) {
                size_t j;
                for (j = i + 1; j < webCLBuildOptionsVector.size() && !webCLBuildOptionsVector[j].startsWith("-"); ++j) {}
                if (webCLBuildOptionsVector[i].stripWhiteSpace() == buildOptionDashD && j == i + 1) {
                    es.throwWebCLException(WebCLException::INVALID_BUILD_OPTIONS, WebCLException::invalidBuildOptionsMessage);
                    return;
                }

                i = --j;
                continue;
            }

            es.throwWebCLException(WebCLException::INVALID_BUILD_OPTIONS, WebCLException::invalidBuildOptionsMessage);
            return;
        }
    }

    pfnNotify callbackProxyPtr = nullptr;
    WebCLProgramHolder* holder = nullptr;
    if (callback) {
        if (m_buildCallback) {
            es.throwWebCLException(WebCLException::INVALID_OPERATION, WebCLException::invalidOperationMessage);
            return;
        }

        // Store the callback, eventList to HashTable and call callbackProxy.
        m_buildCallback = adoptRef(callback);
        callbackProxyPtr = &callbackProxy;
        holder = new WebCLProgramHolder;
        holder->program = createWeakPtr();
    }

    cl_int err = CL_SUCCESS;
    Vector<cl_device_id> clDevices;
    Vector<RefPtr<WebCLDevice>> contextDevices = context()->getDevices();
    if (devices.size()) {
        Vector<cl_device_id> inputDevices;
        for (auto device : devices)
            inputDevices.append(device->getDeviceId());

        size_t contextDevicesLength = contextDevices.size();
        for (size_t z, i = 0; i < inputDevices.size(); i++) {
            // Check if the inputDevices[i] is part of programs WebCLContext.
            for (z = 0; z < contextDevicesLength; z++) {
                if (contextDevices[z]->getDeviceId() == inputDevices[i]) {
                    break;
                }
            }

            if (z == contextDevicesLength) {
                es.throwWebCLException(WebCLException::INVALID_DEVICE, WebCLException::invalidDeviceMessage);
                return;
            }

            clDevices.append(inputDevices[i]);
        }
    } else {
        for (auto contextDevice : contextDevices)
            clDevices.append(contextDevice->getDeviceId());
    }

    if (!clDevices.size()) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    m_isProgramBuilt = true;
    err = clBuildProgram(m_clProgram, clDevices.size(), clDevices.data(), buildOptions.utf8().data(), callbackProxyPtr, holder);

    if (err != CL_SUCCESS)
        es.throwWebCLException(WebCLException::BUILD_PROGRAM_FAILURE, WebCLException::buildProgramFailureMessage);
}

void WebCLProgram::build(const String& options, WebCLCallback* callback, ExceptionState& es)
{
    Vector<RefPtr<WebCLDevice>> devices;
    build(devices, options, callback, es);
}

void WebCLProgram::release()
{
    if (isReleased())
        return;

    cl_int err = clReleaseProgram(m_clProgram);
    if (err != CL_SUCCESS)
        ASSERT_NOT_REACHED();

    m_clProgram = 0;

    // Release the un-triggered callback.
    m_buildCallback.clear();
}

const String& WebCLProgram::sourceWithCommentsStripped()
{
    if (m_programSourceWithCommentsStripped.isNull())
        removeComments(m_programSource, m_programSourceWithCommentsStripped);

    return m_programSourceWithCommentsStripped;
}

WebCLProgram::WebCLProgram(cl_program program, PassRefPtr<WebCLContext> context, const String& kernelSource)
    : WebCLObject(context)
    , m_buildCallback(nullptr)
    , m_programSource(kernelSource)
    , m_isProgramBuilt(false)
    , m_clProgram(program)
{
}

void WebCLProgram::callbackProxy(cl_program program, void* userData)
{
    OwnPtr<WebCLProgramHolder> holder = adoptPtr(static_cast<WebCLProgramHolder*>(userData));
    holder->program2 = program;

    if (isMainThread()) {
        callbackProxyOnMainThread(holder.release());
        return;
    }

    Platform::current()->mainThread()->taskRunner()->postTask(BLINK_FROM_HERE, threadSafeBind(&WebCLProgram::callbackProxyOnMainThread, holder.release()));
}

void WebCLProgram::callbackProxyOnMainThread(PassOwnPtr<WebCLProgramHolder> holder)
{
    ASSERT(isMainThread());
    RefPtr<WebCLProgram> webProgram(static_cast<WebCLProgram*>(holder->program.get()));
#ifndef NDEBUG
    cl_program program = holder->program2;
#endif

    if (!webProgram)
        return;

    if (webProgram->isReleased()) {
        webProgram->m_buildCallback.clear();
        return;
    }

    ASSERT(program == webProgram->getProgram());

    if (webProgram->m_buildCallback) {
        webProgram->m_buildCallback->handleEvent();
        webProgram->m_buildCallback.clear();
    }
}

} // namespace blink
