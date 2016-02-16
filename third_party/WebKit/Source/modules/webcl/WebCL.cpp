// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/webcl/WebCLException.h"
#include "modules/webcl/WebCL.h"
#include "modules/webcl/WebCLOpenCL.h"
#include "modules/webcl/WebCLCallback.h"
#include "modules/webcl/WebCLCommandQueue.h"
#include "modules/webcl/WebCLConfig.h"
#include "modules/webcl/WebCLContext.h"
#include "modules/webcl/WebCLDevice.h"
#include "modules/webcl/WebCLExtension.h"
#include "modules/webcl/WebCLEvent.h"
#include "modules/webcl/WebCLImage.h"
#include "modules/webcl/WebCLInputChecker.h"
#include "modules/webcl/WebCLKernel.h"
#include "modules/webcl/WebCLMemoryObject.h"
#include "modules/webcl/WebCLOpenCL.h"
#include "modules/webcl/WebCLPlatform.h"
#include "modules/webcl/WebCLProgram.h"
#include "modules/webcl/WebCLSampler.h"
#include "platform/ThreadSafeFunctional.h"
#include "public/platform/Platform.h"
#include "public/platform/WebTaskRunner.h"
#include "public/platform/WebTraceLocation.h"
#include <wtf/MainThread.h>

namespace blink {

// The holder of WebCL.
class WebCLHolder {
public:
    cl_event event;
    cl_int type;
    WeakPtr<WebCL> webcl;
};

static inline void getAllEnabledExtensions(WebCL* cl, PassRefPtr<WebCLPlatform> platform, const Vector<RefPtr<WebCLDevice>>& devices, HashSet<String>& enabledExtensions)
{
    cl->getEnabledExtensions(enabledExtensions);
    platform->getEnabledExtensions(enabledExtensions);

    for (auto device : devices)
        device->getEnabledExtensions(enabledExtensions);
}

PassRefPtr<WebCL> WebCL::create()
{
    static bool libraryLoaded = false;
    /*  load libs in lib list. */
    if (!libraryLoaded) {
        if (init(nullptr, 0))
            libraryLoaded = true;
    }

    return libraryLoaded ? adoptRef(new WebCL()) : nullptr;
}

WebCL::~WebCL()
{
    releaseAll();
}

Vector<RefPtr<WebCLPlatform>> WebCL::getPlatforms(ExceptionState& es)
{
    if (!m_platforms.size()) {
        es.throwWebCLException(WebCLException::INVALID_PLATFORM, WebCLException::invalidPlatformMessage);
        return Vector<RefPtr<WebCLPlatform>>();
    }

    return m_platforms;
}

static void validateWebCLEventList(const Vector<RefPtr<WebCLEvent>>& events, ExceptionState& es, bool isSyncCall)
{
    if (!events.size()) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return;
    }

    if (events[0]->isReleased() || (events[0]->isUserEvent() && isSyncCall)) {
        es.throwWebCLException(WebCLException::INVALID_EVENT_WAIT_LIST, WebCLException::invalidEventWaitListMessage);
        return;
    }

    if ((!events[0]->isUserEvent() && events[0]->getStatus() == CL_INVALID_VALUE) ||
        (events[0]->isUserEvent() && events[0]->getStatus() < CL_SUCCESS)) {
        es.throwWebCLException(WebCLException::EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST, WebCLException::execStatusErrorForEventsInWaitListMessage);
        return;
    }

    WebCLContext* referenceContext = events[0]->context().get();

    for (auto event : events) {
        if (event->isReleased() || (event->isUserEvent() && isSyncCall)) {
            es.throwWebCLException(WebCLException::INVALID_EVENT_WAIT_LIST, WebCLException::invalidEventWaitListMessage);
            return;
        }

        ASSERT(event->context());
        if (!WebCLInputChecker::compareContext(event->context().get(), referenceContext)) {
            es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
            return;
        }

        if ((!event->isUserEvent() && event->getStatus() == CL_INVALID_VALUE) ||
            (event->isUserEvent() && event->getStatus() < CL_SUCCESS)) {
            es.throwWebCLException(WebCLException::EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST, WebCLException::execStatusErrorForEventsInWaitListMessage);
            return;
        }
    }
}

PassRefPtr<WebCLContext> WebCL::createContext(ExceptionState& es)
{
    return createContext(CL_DEVICE_TYPE_DEFAULT, es);
}

PassRefPtr<WebCLContext> WebCL::createContext(unsigned deviceType, ExceptionState& es)
{
    if (!m_platforms.size())
        return nullptr;

    return createContext(m_platforms[0].get(), deviceType, es);
}

PassRefPtr<WebCLContext> WebCL::createContext(WebCLPlatform* platform, ExceptionState& es)
{
    return createContext(platform, CL_DEVICE_TYPE_DEFAULT, es);
}

PassRefPtr<WebCLContext> WebCL::createContext(WebCLPlatform* platform, unsigned deviceType, ExceptionState& es)
{
    if (!platform) {
        es.throwWebCLException(WebCLException::INVALID_PLATFORM, WebCLException::invalidPlatformMessage);
        return nullptr;
    }

    if (!WebCLInputChecker::isValidDeviceType(deviceType)) {
        es.throwWebCLException(WebCLException::INVALID_DEVICE_TYPE, WebCLException::invalidDeviceTypeMessage);
        return nullptr;
    }

    Vector<RefPtr<WebCLDevice>> devices = platform->getDevices(deviceType, es);
    Vector<cl_device_id> clDevices;
    for (auto device : devices)
        clDevices.append(device->getDeviceId());

    if (!clDevices.size()) {
        es.throwWebCLException(WebCLException::DEVICE_NOT_FOUND, WebCLException::deviceNotFoundMessage);
        return nullptr;
    }

    cl_int err = CL_SUCCESS;
    cl_context clContextId = clCreateContext(nullptr, clDevices.size(), clDevices.data(), nullptr, nullptr, &err);
    if (err != CL_SUCCESS) {
        WebCLException::throwException(err, es);
        return nullptr;
    }

    // Check all the enabled extensions and cache it to avoid enabling after context creation.
    HashSet<String> enabledExtensions;
    getAllEnabledExtensions(this, platform, devices, enabledExtensions);
    RefPtr<WebCLContext> context = WebCLContext::create(clContextId, this, devices, enabledExtensions);
    if (!context) {
        es.throwWebCLException(WebCLException::FAILURE, WebCLException::failureMessage);
        return nullptr;
    }
    return context;
}

PassRefPtr<WebCLContext> WebCL::createContext(WebCLDevice* device, ExceptionState& es)
{
    Vector<RefPtr<WebCLDevice>> devices;
    devices.append(device);
    return createContext(devices, es);
}

PassRefPtr<WebCLContext> WebCL::createContext(const Vector<RefPtr<WebCLDevice>>& devices, ExceptionState& es)
{
    cl_int err = CL_SUCCESS;
    cl_context clContextId = 0;

    if (!devices.size()) {
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        return nullptr;
    }

    Vector<cl_device_id> clDeviceList;
    for (auto device : devices)
        clDeviceList.append(device->getDeviceId());

    if (!clDeviceList.size()) {
        es.throwWebCLException(WebCLException::INVALID_DEVICE, WebCLException::invalidDeviceMessage);
        return nullptr;
    }

    clContextId = clCreateContext(nullptr, clDeviceList.size(), clDeviceList.data(), nullptr, nullptr, &err);
    if (err != CL_SUCCESS) {
        WebCLException::throwException(err, es);
        return nullptr;
    }

    // Check all the enabled extensions and cache it to avoid enabling after context creation.
    HashSet<String> enabledExtensions;
    getAllEnabledExtensions(this, devices[0]->getPlatform(), devices, enabledExtensions);
    RefPtr<WebCLContext> context = WebCLContext::create(clContextId, this, devices, enabledExtensions);
    if (!context) {
        es.throwWebCLException(WebCLException::FAILURE, WebCLException::failureMessage);
        return nullptr;
    }

    return context;
}

void WebCL::waitForEvents(const Vector<RefPtr<WebCLEvent>>& events, WebCLCallback* whenFinished, ExceptionState& es)
{
    validateWebCLEventList(events, es, !whenFinished);
    if (es.hadException()) {
        es.throwIfNeeded();
        return;
    }

    waitForEventsImpl(events, whenFinished);
}

void WebCL::releaseAll()
{
    if (m_webCLContexts.size()) {
        for (int i = m_webCLContexts.size() - 1; i >= 0; i--) {
            WebCLContext* context = m_webCLContexts[i].get();
            if (!context)
                continue;

            context->releaseAll();
        }
        m_webCLContexts.clear();
    }
}

void WebCL::trackReleaseableWebCLContext(WeakPtr<WebCLContext> context)
{
    m_webCLContexts.append(context);
}

bool WebCL::enableExtension(const String& name)
{
    return m_extension.enableExtension(name);
}

Vector<String> WebCL::getSupportedExtensions()
{
    return m_extension.getSupportedExtensions();
}

void WebCL::getEnabledExtensions(HashSet<String>& extensions)
{
    m_extension.getEnabledExtensions(extensions);
}

WebCL::WebCL()
    : m_weakFactory(this)
{
    cachePlatforms();
    cacheSupportedExtensions();
}

Vector<RefPtr<WebCLCallback>> WebCL::updateCallbacksFromCLEvent(cl_event event)
{
    Vector<RefPtr<WebCLCallback>> callbacks;
    if (m_callbackRegisterQueue.size()) {
        for (int i = m_callbackRegisterQueue.size() - 1; i >= 0; i--) {
            if (m_callbackRegisterQueue[i].first.size()) {
                for (int j = m_callbackRegisterQueue[i].first.size() - 1; j >= 0; j--) {
                    WebCLEvent* webEvent = static_cast<WebCLEvent*>(m_callbackRegisterQueue[i].first[j].get());
                    if (webEvent && event == webEvent->getEvent())
                        m_callbackRegisterQueue[i].first.remove(j);
                }

                if (!m_callbackRegisterQueue[i].first.size()) {
                    callbacks.append(m_callbackRegisterQueue[i].second);
                    m_callbackRegisterQueue.remove(i);
                }
            }
        }
    }

    return callbacks;
}

void WebCL::callbackProxy(cl_event event, cl_int type, void* userData)
{
    OwnPtr<WebCLHolder> holder = adoptPtr(static_cast<WebCLHolder*>(userData));
    holder->event = event;
    holder->type = type;

    if (!isMainThread()) {
        Platform::current()->mainThread()->taskRunner()->postTask(BLINK_FROM_HERE, threadSafeBind(&WebCL::callbackProxyOnMainThread, holder.release()));
        return;
    }

    callbackProxyOnMainThread(holder.release());
}

void WebCL::callbackProxyOnMainThread(PassOwnPtr<WebCLHolder> holder)
{
    ASSERT(isMainThread());
    RefPtr<WebCL> webcl(holder->webcl.get());
    cl_event event = holder->event;
    cl_int type = holder->type;

    // Ignore the callback if the WebCL is destructed.
    if (!webcl)
        return;

    Vector<RefPtr<WebCLCallback>> callbacks = webcl->updateCallbacksFromCLEvent(event);
    // Ignore the callback if the OpenCL event is abnormally terminated.
    if (type != CL_COMPLETE) {
        return;
    }

    for (auto callback : callbacks) {
        if (callback)
            callback->handleEvent();
    }
}

void WebCL::waitForEventsImpl(const Vector<RefPtr<WebCLEvent>>& events, WebCLCallback* callback)
{
    Vector<cl_event> clEvents;
    Vector<WeakPtr<WebCLObject>> webEvents;
    WebCLHolder* holder = new WebCLHolder;
    holder->webcl = m_weakFactory.createWeakPtr();

    for (auto event : events) {
        clEvents.append(event->getEvent());
        webEvents.append(event->createWeakPtr());
    }

    if (!callback) {
        clWaitForEvents(clEvents.size(), clEvents.data());
    } else {
        m_callbackRegisterQueue.append(std::make_pair(webEvents, adoptRef(callback)));
        for (auto clEvent : clEvents)
            clSetEventCallback(clEvent, CL_COMPLETE, &callbackProxy, holder);
    }
}

void WebCL::cachePlatforms()
{
    if (m_platforms.size() > 0)
        return;

    cl_uint numPlatforms = 0;
    cl_int err = clGetPlatformIDs(0, nullptr, &numPlatforms);

    if (err != CL_SUCCESS)
        return;

    Vector<cl_platform_id> clPlatforms;
    clPlatforms.resize(numPlatforms);
    err = clGetPlatformIDs(numPlatforms, clPlatforms.data(), nullptr);
    if (err != CL_SUCCESS)
        return;

    for (auto clPlatform : clPlatforms) {
        RefPtr<WebCLPlatform> platform = WebCLPlatform::create(clPlatform);
        if (platform)
            m_platforms.append(platform);
    }
    clPlatforms.clear();
}

void WebCL::cacheSupportedExtensions()
{
    if (!m_platforms.size())
        return;

    Vector<String> supportedExtensions = m_platforms[0]->getSupportedExtensions();
    if (m_platforms.size() == 1) {
        // If there is only one platform, WebCL extensions is equal to this platform's extension.
        return;
    }

    for (auto platform : m_platforms) {
        Vector<String> temporary = platform->getSupportedExtensions();

        Vector<String> toBeRemoved;
        for (auto supportedExtension : supportedExtensions) {
            Vector<String>::iterator iter = temporary.begin();
            for (; iter != temporary.end(); ++iter) {
                if (supportedExtension == *iter)
                    break;
            }
            if (iter == temporary.end())
                toBeRemoved.append(supportedExtension);
        }

        for (auto stringToBeRemoved : toBeRemoved)
            supportedExtensions.remove(supportedExtensions.find(stringToBeRemoved));
    }

    for (auto supportedExtension : supportedExtensions)
        m_extension.addSupportedCLExtension(supportedExtension);
}

} // namespace blink
