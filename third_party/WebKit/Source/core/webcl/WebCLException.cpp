// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "wtf/build_config.h"


#include "core/webcl/WebCLException.h"
#include <CL/cl.h>

namespace blink {

const char WebCLException::successMessage[] = "SUCCESS";
const char WebCLException::deviceNotFoundMessage[] = "DEVICE_NOT_FOUND";
const char WebCLException::deviceNotAvailableMessage[] = "DEVICE_NOT_AVAILABLE";
const char WebCLException::compilerNotAvailableMessage[] = "COMPILER_NOT_AVAILABLE";
const char WebCLException::memObjectAllocationFailureMessage[] = "MEM_OBJECT_ALLOCATION_FAILURE";
const char WebCLException::outOfResourcesMessage[] = "OUT_OF_RESOURCES";
const char WebCLException::outOfHostMemoryMessage[] = "OUT_OF_HOST_MEMORY";
const char WebCLException::profilingInfoNotAvailableMessage[] = "PROFILING_INFO_NOT_AVAILABLE";
const char WebCLException::memCopyOverlapMessage[] = "MEM_COPY_OVERLAP";
const char WebCLException::imageFormatMismatchMessage[] = "IMAGE_FORMAT_MISMATCH";
const char WebCLException::imageFormatNotSupportedMessage[] = "IMAGE_FORMAT_NOT_SUPPORTED";
const char WebCLException::buildProgramFailureMessage[] = "BUILD_PROGRAM_FAILURE";
const char WebCLException::mapFailureMessage[] = "MAP_FAILURE";
const char WebCLException::misalignedSubBufferOffsetMessage[] = "MISALIGNED_SUB_BUFFER_OFFSET";
const char WebCLException::execStatusErrorForEventsInWaitListMessage[] = "EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
const char WebCLException::extensionNotEnabledMessage[] = "EXTENSION_NOT_ENABLED";
const char WebCLException::invalidValueMessage[] = "INVALID_VALUE";
const char WebCLException::invalidDeviceTypeMessage[] = "INVALID_DEVICE_TYPE";
const char WebCLException::invalidPlatformMessage[] = "INVALID_PLATFORM";
const char WebCLException::invalidDeviceMessage[] = "INVALID_DEVICE";
const char WebCLException::invalidContextMessage[] = "INVALID_CONTEXT";
const char WebCLException::invalidQueuePropertiesMessage[] = "INVALID_QUEUE_PROPERTIES";
const char WebCLException::invalidCommandQueueMessage[] = "INVALID_COMMAND_QUEUE";
const char WebCLException::invalidHostPTRMessage[] = "INVALID_HOST_PTR";
const char WebCLException::invalidMemObjectMessage[] = "INVALID_MEM_OBJECT";
const char WebCLException::invalidImageFormatDescriptorMessage[] = "INVALID_IMAGE_FORMAT_DESCRIPTOR";
const char WebCLException::invalidImageSizeMessage[] = "INVALID_IMAGE_SIZE";
const char WebCLException::invalidSamplerMessage[] = "INVALID_SAMPLER";
const char WebCLException::invalidBinaryMessage[] = "INVALID_BINARY";
const char WebCLException::invalidBuildOptionsMessage[] = "INVALID_BUILD_OPTIONS";
const char WebCLException::invalidProgramMessage[] = "INVALID_PROGRAM";
const char WebCLException::invalidProgramExecutableMessage[] = "INVALID_PROGRAM_EXECUTABLE";
const char WebCLException::invalidKernelNameMessage[] = "INVALID_KERNEL_NAME";
const char WebCLException::invalidKernelDefinitionMessage[] = "INVALID_KERNEL_DEFINITION";
const char WebCLException::invalidKernelMessage[] = "INVALID_KERNEL";
const char WebCLException::invalidArgIndexMessage[] = "INVALID_ARG_INDEX";
const char WebCLException::invalidArgValueMessage[] = "INVALID_ARG_VALUE";
const char WebCLException::invalidArgSizeMessage[] = "INVALID_ARG_SIZE";
const char WebCLException::invalidKernelArgsMessage[] = "INVALID_KERNEL_ARGS";
const char WebCLException::invalidWorkDimensionMessage[] = "INVALID_WORK_DIMENSION";
const char WebCLException::invalidWorkGroupSizeMessage[] = "INVALID_WORK_GROUP_SIZE";
const char WebCLException::invalidWorkItemSizeMessage[] = "INVALID_WORK_ITEM_SIZE";
const char WebCLException::invalidGlobalOffsetMessage[] = "INVALID_GLOBAL_OFFSET";
const char WebCLException::invalidEventWaitListMessage[] = "INVALID_EVENT_WAIT_LIST";
const char WebCLException::invalidEventMessage[] = "INVALID_EVENT";
const char WebCLException::invalidOperationMessage[] = "INVALID_OPERATION";
const char WebCLException::invalidGLObjectMessage[] = "INVALID_GL_OBJECT";
const char WebCLException::invalidBufferSizeMessage[] = "INVALID_BUFFER_SIZE";
const char WebCLException::invalidMIPLevelMessage[] = "INVALID_MIP_LEVEL";
const char WebCLException::invalidGlobalWorkSizeMessage[] = "INVALID_GLOBAL_WORK_SIZE";
const char WebCLException::invalidPropertyMessage[] = "INVALID_PROPERTY";
const char WebCLException::failureMessage[] = "FAILURE";

void WebCLException::throwException(int& code, ExceptionState& es)
{
    switch(code) {
    case CL_DEVICE_NOT_FOUND:
        es.throwWebCLException(WebCLException::DEVICE_NOT_FOUND, WebCLException::deviceNotFoundMessage);
        break;
    case CL_DEVICE_NOT_AVAILABLE:
        es.throwWebCLException(WebCLException::DEVICE_NOT_AVAILABLE, WebCLException::deviceNotAvailableMessage);
        break;
    case CL_COMPILER_NOT_AVAILABLE:
        es.throwWebCLException(WebCLException::COMPILER_NOT_AVAILABLE, WebCLException::compilerNotAvailableMessage);
        break;
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:
        es.throwWebCLException(WebCLException::MEM_OBJECT_ALLOCATION_FAILURE, WebCLException::memObjectAllocationFailureMessage);
        break;
    case CL_OUT_OF_RESOURCES:
        es.throwWebCLException(WebCLException::OUT_OF_RESOURCES, WebCLException::outOfResourcesMessage);
        break;
    case CL_OUT_OF_HOST_MEMORY:
        es.throwWebCLException(WebCLException::OUT_OF_HOST_MEMORY, WebCLException::outOfHostMemoryMessage);
        break;
    case CL_PROFILING_INFO_NOT_AVAILABLE:
        es.throwWebCLException(WebCLException::PROFILING_INFO_NOT_AVAILABLE, WebCLException::profilingInfoNotAvailableMessage);
        break;
    case CL_MEM_COPY_OVERLAP:
        es.throwWebCLException(WebCLException::MEM_OBJECT_ALLOCATION_FAILURE, WebCLException::memObjectAllocationFailureMessage);
        break;
    case CL_IMAGE_FORMAT_MISMATCH:
        es.throwWebCLException(WebCLException::IMAGE_FORMAT_MISMATCH, WebCLException::imageFormatMismatchMessage);
        break;
    case CL_IMAGE_FORMAT_NOT_SUPPORTED:
        es.throwWebCLException(WebCLException::IMAGE_FORMAT_NOT_SUPPORTED, WebCLException::imageFormatNotSupportedMessage);
        break;
    case CL_BUILD_PROGRAM_FAILURE:
        es.throwWebCLException(WebCLException::BUILD_PROGRAM_FAILURE, WebCLException::buildProgramFailureMessage);
        break;
    case CL_MAP_FAILURE:
        es.throwWebCLException(WebCLException::MAP_FAILURE, WebCLException::mapFailureMessage);
        break;
    case CL_MISALIGNED_SUB_BUFFER_OFFSET:
        es.throwWebCLException(WebCLException::MISALIGNED_SUB_BUFFER_OFFSET, WebCLException::misalignedSubBufferOffsetMessage);
        break;
    case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
        es.throwWebCLException(WebCLException::EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST, WebCLException::execStatusErrorForEventsInWaitListMessage);
        break;
    case CL_INVALID_VALUE:
        es.throwWebCLException(WebCLException::INVALID_VALUE, WebCLException::invalidValueMessage);
        break;
    case CL_INVALID_DEVICE_TYPE:
        es.throwWebCLException(WebCLException::INVALID_DEVICE_TYPE, WebCLException::invalidDeviceTypeMessage);
        break;
    case CL_INVALID_PLATFORM:
        es.throwWebCLException(WebCLException::INVALID_PLATFORM, WebCLException::invalidPlatformMessage);
        break;
    case CL_INVALID_DEVICE:
        es.throwWebCLException(WebCLException::INVALID_DEVICE, WebCLException::invalidDeviceMessage);
        break;
    case CL_INVALID_CONTEXT:
        es.throwWebCLException(WebCLException::INVALID_CONTEXT, WebCLException::invalidContextMessage);
        break;
    case CL_INVALID_QUEUE_PROPERTIES:
        es.throwWebCLException(WebCLException::INVALID_QUEUE_PROPERTIES, WebCLException::invalidQueuePropertiesMessage);
        break;
    case CL_INVALID_COMMAND_QUEUE:
        es.throwWebCLException(WebCLException::INVALID_COMMAND_QUEUE, WebCLException::invalidCommandQueueMessage);
        break;
    case CL_INVALID_HOST_PTR:
        es.throwWebCLException(WebCLException::INVALID_HOST_PTR, WebCLException::invalidHostPTRMessage);
        break;
    case CL_INVALID_MEM_OBJECT:
        es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidHostPTRMessage);
        break;
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
        es.throwWebCLException(WebCLException::INVALID_IMAGE_FORMAT_DESCRIPTOR, WebCLException::invalidImageFormatDescriptorMessage);
        break;
    case CL_INVALID_IMAGE_SIZE:
        es.throwWebCLException(WebCLException::INVALID_IMAGE_SIZE, WebCLException::invalidImageSizeMessage);
        break;
    case CL_INVALID_SAMPLER:
        es.throwWebCLException(WebCLException::INVALID_SAMPLER, WebCLException::invalidSamplerMessage);
        break;
    case CL_INVALID_BINARY:
        es.throwWebCLException(WebCLException::INVALID_BINARY, WebCLException::invalidBinaryMessage);
        break;
    case CL_INVALID_BUILD_OPTIONS:
        es.throwWebCLException(WebCLException::INVALID_BUILD_OPTIONS, WebCLException::invalidBuildOptionsMessage);
        break;
    case CL_INVALID_PROGRAM:
        es.throwWebCLException(WebCLException::INVALID_PROGRAM, WebCLException::invalidProgramMessage);
        break;
    case CL_INVALID_PROGRAM_EXECUTABLE:
        es.throwWebCLException(WebCLException::INVALID_PROGRAM_EXECUTABLE, WebCLException::invalidProgramExecutableMessage);
        break;
    case CL_INVALID_KERNEL_NAME:
        es.throwWebCLException(WebCLException::INVALID_KERNEL_NAME, WebCLException::invalidKernelNameMessage);
        break;
    case CL_INVALID_KERNEL_DEFINITION:
        es.throwWebCLException(WebCLException::INVALID_KERNEL_DEFINITION, WebCLException::invalidKernelDefinitionMessage);
        break;
    case CL_INVALID_KERNEL:
        es.throwWebCLException(WebCLException::INVALID_KERNEL, WebCLException::invalidKernelMessage);
        break;
    case CL_INVALID_ARG_INDEX:
        es.throwWebCLException(WebCLException::INVALID_ARG_INDEX, WebCLException::invalidKernelArgsMessage);
        break;
    case CL_INVALID_ARG_VALUE:
        es.throwWebCLException(WebCLException::INVALID_ARG_VALUE, WebCLException::invalidArgValueMessage);
        break;
    case CL_INVALID_ARG_SIZE:
        es.throwWebCLException(WebCLException::INVALID_ARG_SIZE, WebCLException::invalidArgSizeMessage);
        break;
    case CL_INVALID_KERNEL_ARGS:
        es.throwWebCLException(WebCLException::INVALID_KERNEL_ARGS, WebCLException::invalidKernelArgsMessage);
        break;
    case CL_INVALID_WORK_DIMENSION:
        es.throwWebCLException(WebCLException::INVALID_WORK_DIMENSION, WebCLException::invalidWorkDimensionMessage);
        break;
    case CL_INVALID_WORK_GROUP_SIZE:
        es.throwWebCLException(WebCLException::INVALID_WORK_GROUP_SIZE, WebCLException::invalidWorkGroupSizeMessage);
        break;
    case CL_INVALID_WORK_ITEM_SIZE:
        es.throwWebCLException(WebCLException::INVALID_WORK_ITEM_SIZE, WebCLException::invalidWorkItemSizeMessage);
        break;
    case CL_INVALID_GLOBAL_OFFSET:
        es.throwWebCLException(WebCLException::INVALID_GLOBAL_OFFSET, WebCLException::invalidGlobalWorkSizeMessage);
        break;
    case CL_INVALID_EVENT_WAIT_LIST:
        es.throwWebCLException(WebCLException::INVALID_EVENT_WAIT_LIST, WebCLException::invalidEventWaitListMessage);
        break;
    case CL_INVALID_EVENT:
        es.throwWebCLException(WebCLException::INVALID_EVENT, WebCLException::invalidEventMessage);
        break;
    case CL_INVALID_OPERATION:
        es.throwWebCLException(WebCLException::INVALID_OPERATION, WebCLException::invalidOperationMessage);
        break;
    case CL_INVALID_GL_OBJECT:
        es.throwWebCLException(WebCLException::INVALID_GL_OBJECT, WebCLException::invalidGLObjectMessage);
        break;
    case CL_INVALID_BUFFER_SIZE:
        es.throwWebCLException(WebCLException::INVALID_BUFFER_SIZE, WebCLException::invalidBufferSizeMessage);
        break;
    case CL_INVALID_MIP_LEVEL:
        es.throwWebCLException(WebCLException::INVALID_MIP_LEVEL, WebCLException::invalidMIPLevelMessage);
        break;
    case CL_INVALID_GLOBAL_WORK_SIZE:
        es.throwWebCLException(WebCLException::INVALID_GLOBAL_WORK_SIZE, WebCLException::invalidGlobalWorkSizeMessage);
        break;
    case CL_INVALID_PROPERTY:
        es.throwWebCLException(WebCLException::INVALID_PROPERTY, WebCLException::invalidPropertyMessage);
        break;
    default:
        es.throwWebCLException(WebCLException::FAILURE, WebCLException::failureMessage);
        break;
    }

    if (es.hadException())
        es.throwIfNeeded();
}

} // namespace blink

