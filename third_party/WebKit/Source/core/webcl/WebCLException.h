// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLException_h
#define WebCLException_h

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/ScriptWrappable.h"
#include "wtf/ThreadSafeRefCounted.h"

#define WEBCLEXCEPTIONOFFSET 0

namespace blink {

class CORE_EXPORT WebCLException final : public ThreadSafeRefCounted<WebCLException>, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    static PassRefPtr<WebCLException> create(unsigned code, const String& name, const String& message);

    enum WebCLExceptionCode {
        Success                            = WEBCLEXCEPTIONOFFSET,
        DeviceNotFound                     = WEBCLEXCEPTIONOFFSET + 1,
        DeviceNotAvailable                 = WEBCLEXCEPTIONOFFSET + 2,
        CompilerNotAvailable               = WEBCLEXCEPTIONOFFSET + 3,
        MemObjectAllocationFailure         = WEBCLEXCEPTIONOFFSET + 4,
        OutOfResources                     = WEBCLEXCEPTIONOFFSET + 5,
        OutOfHostMemory                    = WEBCLEXCEPTIONOFFSET + 6,
        ProfilingInfoNotAvailable          = WEBCLEXCEPTIONOFFSET + 7,
        MemCopyOverlap                     = WEBCLEXCEPTIONOFFSET + 8,
        ImageFormatMismatch                = WEBCLEXCEPTIONOFFSET + 9,
        ImageFormatNotSupported            = WEBCLEXCEPTIONOFFSET + 10,
        BuildProgramFailure                = WEBCLEXCEPTIONOFFSET + 11,
        MapFailure                         = WEBCLEXCEPTIONOFFSET + 12,
        MisalignedSubBufferOffset          = WEBCLEXCEPTIONOFFSET + 13,
        ExecStatusErrorForEventsInWaitList = WEBCLEXCEPTIONOFFSET + 14,
        ExtensionNotEnabled                = WEBCLEXCEPTIONOFFSET + 15,

        InvalidValue                       = WEBCLEXCEPTIONOFFSET + 30,
        InvalidDeviceType                  = WEBCLEXCEPTIONOFFSET + 31,
        InvalidPlatform                    = WEBCLEXCEPTIONOFFSET + 32,
        InvalidDevice                      = WEBCLEXCEPTIONOFFSET + 33,
        InvalidContext                     = WEBCLEXCEPTIONOFFSET + 34,
        InvalidQueueProperties             = WEBCLEXCEPTIONOFFSET + 35,
        InvalidCommandQueue                = WEBCLEXCEPTIONOFFSET + 36,
        InvalidHostPtr                     = WEBCLEXCEPTIONOFFSET + 37,
        InvalidMemObject                   = WEBCLEXCEPTIONOFFSET + 38,
        InvalidImageFormatDescriptor       = WEBCLEXCEPTIONOFFSET + 39,
        InvalidImageSize                   = WEBCLEXCEPTIONOFFSET + 40,
        InvalidSampler                     = WEBCLEXCEPTIONOFFSET + 41,
        InvalidBinary                      = WEBCLEXCEPTIONOFFSET + 42,
        InvalidBuildOptions                = WEBCLEXCEPTIONOFFSET + 43,
        InvalidProgram                     = WEBCLEXCEPTIONOFFSET + 44,
        InvalidProgramExecutable           = WEBCLEXCEPTIONOFFSET + 45,
        InvalidKernelName                  = WEBCLEXCEPTIONOFFSET + 46,
        InvalidKernelDefinition            = WEBCLEXCEPTIONOFFSET + 47,
        InvalidKernel                      = WEBCLEXCEPTIONOFFSET + 48,
        InvalidArgIndex                    = WEBCLEXCEPTIONOFFSET + 49,
        InvalidArgValue                    = WEBCLEXCEPTIONOFFSET + 50,
        InvalidArgSize                     = WEBCLEXCEPTIONOFFSET + 51,
        InvalidKernelArgs                  = WEBCLEXCEPTIONOFFSET + 52,
        InvalidWorkDimension               = WEBCLEXCEPTIONOFFSET + 53,
        InvalidWorkGroupSize               = WEBCLEXCEPTIONOFFSET + 54,
        InvalidWorkItemSize                = WEBCLEXCEPTIONOFFSET + 55,
        InvalidGlobalOffset                = WEBCLEXCEPTIONOFFSET + 56,
        InvalidEventWaitList               = WEBCLEXCEPTIONOFFSET + 57,
        InvalidEvent                       = WEBCLEXCEPTIONOFFSET + 58,
        InvalidOperation                   = WEBCLEXCEPTIONOFFSET + 59,
        InvalidGLObject                    = WEBCLEXCEPTIONOFFSET + 60,
        InvalidBufferSize                  = WEBCLEXCEPTIONOFFSET + 61,
        InvalidMipLevel                    = WEBCLEXCEPTIONOFFSET + 62,
        InvalidGlobalWorkSize              = WEBCLEXCEPTIONOFFSET + 63,
        InvalidProperty                    = WEBCLEXCEPTIONOFFSET + 64,
        Failure                            = WEBCLEXCEPTIONOFFSET + 65,
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

    static void throwException(int& code, ExceptionState&);
    unsigned code() const { return m_code; }
    String name() const;
    String message() const;

private:
    WebCLException(unsigned code, const String& name, const String& message);
    unsigned m_code;
    String m_name;
    String m_message;
};

} // namespace blink

#endif // WebCLException_h
