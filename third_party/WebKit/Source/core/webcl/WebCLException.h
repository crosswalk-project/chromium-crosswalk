// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLException_h
#define WebCLException_h

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/ScriptWrappable.h"
#include <wtf/ThreadSafeRefCounted.h>

#define WEBCLEXCEPTIONOFFSET 0

namespace blink {

class WebCLException : public ThreadSafeRefCounted<WebCLException>, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    static PassRefPtr<WebCLException> create(unsigned code, const String& name, const String& message)
    {
        return adoptRef(new WebCLException(code, name, message));
    }

    enum WebCLExceptionCode {
        SUCCESS                                   = WEBCLEXCEPTIONOFFSET,
        DEVICE_NOT_FOUND                          = WEBCLEXCEPTIONOFFSET + 1,
        DEVICE_NOT_AVAILABLE                      = WEBCLEXCEPTIONOFFSET + 2,
        COMPILER_NOT_AVAILABLE                    = WEBCLEXCEPTIONOFFSET + 3,
        MEM_OBJECT_ALLOCATION_FAILURE             = WEBCLEXCEPTIONOFFSET + 4,
        OUT_OF_RESOURCES                          = WEBCLEXCEPTIONOFFSET + 5,
        OUT_OF_HOST_MEMORY                        = WEBCLEXCEPTIONOFFSET + 6,
        PROFILING_INFO_NOT_AVAILABLE              = WEBCLEXCEPTIONOFFSET + 7,
        MEM_COPY_OVERLAP                          = WEBCLEXCEPTIONOFFSET + 8,
        IMAGE_FORMAT_MISMATCH                     = WEBCLEXCEPTIONOFFSET + 9,
        IMAGE_FORMAT_NOT_SUPPORTED                = WEBCLEXCEPTIONOFFSET + 10,
        BUILD_PROGRAM_FAILURE                     = WEBCLEXCEPTIONOFFSET + 11,
        MAP_FAILURE                               = WEBCLEXCEPTIONOFFSET + 12,
        MISALIGNED_SUB_BUFFER_OFFSET              = WEBCLEXCEPTIONOFFSET + 13,
        EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST = WEBCLEXCEPTIONOFFSET + 14,
        EXTENSION_NOT_ENABLED                     = WEBCLEXCEPTIONOFFSET + 15,

        INVALID_VALUE                             = WEBCLEXCEPTIONOFFSET + 30,
        INVALID_DEVICE_TYPE                       = WEBCLEXCEPTIONOFFSET + 31,
        INVALID_PLATFORM                          = WEBCLEXCEPTIONOFFSET + 32,
        INVALID_DEVICE                            = WEBCLEXCEPTIONOFFSET + 33,
        INVALID_CONTEXT                           = WEBCLEXCEPTIONOFFSET + 34,
        INVALID_QUEUE_PROPERTIES                  = WEBCLEXCEPTIONOFFSET + 35,
        INVALID_COMMAND_QUEUE                     = WEBCLEXCEPTIONOFFSET + 36,
        INVALID_HOST_PTR                          = WEBCLEXCEPTIONOFFSET + 37,
        INVALID_MEM_OBJECT                        = WEBCLEXCEPTIONOFFSET + 38,
        INVALID_IMAGE_FORMAT_DESCRIPTOR           = WEBCLEXCEPTIONOFFSET + 39,
        INVALID_IMAGE_SIZE                        = WEBCLEXCEPTIONOFFSET + 40,
        INVALID_SAMPLER                           = WEBCLEXCEPTIONOFFSET + 41,
        INVALID_BINARY                            = WEBCLEXCEPTIONOFFSET + 42,
        INVALID_BUILD_OPTIONS                     = WEBCLEXCEPTIONOFFSET + 43,
        INVALID_PROGRAM                           = WEBCLEXCEPTIONOFFSET + 44,
        INVALID_PROGRAM_EXECUTABLE                = WEBCLEXCEPTIONOFFSET + 45,
        INVALID_KERNEL_NAME                       = WEBCLEXCEPTIONOFFSET + 46,
        INVALID_KERNEL_DEFINITION                 = WEBCLEXCEPTIONOFFSET + 47,
        INVALID_KERNEL                            = WEBCLEXCEPTIONOFFSET + 48,
        INVALID_ARG_INDEX                         = WEBCLEXCEPTIONOFFSET + 49,
        INVALID_ARG_VALUE                         = WEBCLEXCEPTIONOFFSET + 50,
        INVALID_ARG_SIZE                          = WEBCLEXCEPTIONOFFSET + 51,
        INVALID_KERNEL_ARGS                       = WEBCLEXCEPTIONOFFSET + 52,
        INVALID_WORK_DIMENSION                    = WEBCLEXCEPTIONOFFSET + 53,
        INVALID_WORK_GROUP_SIZE                   = WEBCLEXCEPTIONOFFSET + 54,
        INVALID_WORK_ITEM_SIZE                    = WEBCLEXCEPTIONOFFSET + 55,
        INVALID_GLOBAL_OFFSET                     = WEBCLEXCEPTIONOFFSET + 56,
        INVALID_EVENT_WAIT_LIST                   = WEBCLEXCEPTIONOFFSET + 57,
        INVALID_EVENT                             = WEBCLEXCEPTIONOFFSET + 58,
        INVALID_OPERATION                         = WEBCLEXCEPTIONOFFSET + 59,
        INVALID_GL_OBJECT                         = WEBCLEXCEPTIONOFFSET + 60,
        INVALID_BUFFER_SIZE                       = WEBCLEXCEPTIONOFFSET + 61,
        INVALID_MIP_LEVEL                         = WEBCLEXCEPTIONOFFSET + 62,
        INVALID_GLOBAL_WORK_SIZE                  = WEBCLEXCEPTIONOFFSET + 63,
        INVALID_PROPERTY                          = WEBCLEXCEPTIONOFFSET + 64,
        FAILURE                                   = WEBCLEXCEPTIONOFFSET + 65,
    };

    static const char successMessage[];
    static const char deviceNotFoundMessage[];
    static const char deviceNotAvailableMessage[];
    static const char compilerNotAvailableMessage[];
    static const char memObjectAllocationFailureMessage[];
    static const char outOfResourcesMessage[];
    static const char outOfHostMemoryMessage[];
    static const char profilingInfoNotAvailableMessage[];
    static const char memCopyOverlapMessage[];
    static const char imageFormatMismatchMessage[];
    static const char imageFormatNotSupportedMessage[];
    static const char buildProgramFailureMessage[];
    static const char mapFailureMessage[];
    static const char misalignedSubBufferOffsetMessage[];
    static const char execStatusErrorForEventsInWaitListMessage[];
    static const char extensionNotEnabledMessage[];
    static const char invalidValueMessage[];
    static const char invalidDeviceTypeMessage[];
    static const char invalidPlatformMessage[];
    static const char invalidDeviceMessage[];
    static const char invalidContextMessage[];
    static const char invalidQueuePropertiesMessage[];
    static const char invalidCommandQueueMessage[];
    static const char invalidHostPTRMessage[];
    static const char invalidMemObjectMessage[];
    static const char invalidImageFormatDescriptorMessage[];
    static const char invalidImageSizeMessage[];
    static const char invalidSamplerMessage[];
    static const char invalidBinaryMessage[];
    static const char invalidBuildOptionsMessage[];
    static const char invalidProgramMessage[];
    static const char invalidProgramExecutableMessage[];
    static const char invalidKernelNameMessage[];
    static const char invalidKernelDefinitionMessage[];
    static const char invalidKernelMessage[];
    static const char invalidArgIndexMessage[];
    static const char invalidArgValueMessage[];
    static const char invalidArgSizeMessage[];
    static const char invalidKernelArgsMessage[];
    static const char invalidWorkDimensionMessage[];
    static const char invalidWorkGroupSizeMessage[];
    static const char invalidWorkItemSizeMessage[];
    static const char invalidGlobalOffsetMessage[];
    static const char invalidEventWaitListMessage[];
    static const char invalidEventMessage[];
    static const char invalidOperationMessage[];
    static const char invalidGLObjectMessage[];
    static const char invalidBufferSizeMessage[];
    static const char invalidMIPLevelMessage[];
    static const char invalidGlobalWorkSizeMessage[];
    static const char invalidPropertyMessage[];
    static const char failureMessage[];

    static void throwException(int& code, ExceptionState& es);
 
    unsigned code() const { return m_code; }
    String name() const { return m_name.isolatedCopy(); }
    String message() const { return m_message.isolatedCopy(); }

private:
    WebCLException(unsigned code, const String& name, const String& message)
        : m_code(code)
        , m_name(name.isolatedCopy())
        , m_message(message.isolatedCopy())
    {
    }
    unsigned m_code;
    String m_name;
    String m_message;
};

} // namespace blink

#endif // WebCLException_h
