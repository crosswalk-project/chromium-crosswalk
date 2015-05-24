// Copyright (C) 2013 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#if ENABLE(WEBCL)

#include "modules/webcl/WebCLBuffer.h"
#include "modules/webcl/WebCLContext.h"
#include "modules/webcl/WebCLImage.h"
#include "modules/webcl/WebCLImageDescriptor.h"
#include "modules/webcl/WebCLInputChecker.h"
#include "modules/webcl/WebCLKernel.h"

namespace blink {
namespace WebCLInputChecker {

bool isValidDeviceType(unsigned deviceType)
{
    switch (deviceType) {
    case CL_DEVICE_TYPE_CPU:
    case CL_DEVICE_TYPE_GPU:
    case CL_DEVICE_TYPE_ACCELERATOR:
    case CL_DEVICE_TYPE_DEFAULT:
    case CL_DEVICE_TYPE_ALL:
        return true;
    }

    return false;
}

bool isValidDeviceInfoType(unsigned infoType)
{
    switch (infoType) {
    case CL_DEVICE_EXTENSIONS:
    case CL_DEVICE_PROFILE:
    case CL_DEVICE_NAME:
    case CL_DEVICE_VENDOR:
    case CL_DEVICE_VENDOR_ID:
    case CL_DEVICE_VERSION:
    case CL_DRIVER_VERSION:
    case CL_DEVICE_OPENCL_C_VERSION:
    case CL_DEVICE_ADDRESS_BITS:
    case CL_DEVICE_MAX_CONSTANT_ARGS:
    case CL_DEVICE_MAX_READ_IMAGE_ARGS:
    case CL_DEVICE_MAX_SAMPLERS:
    case CL_DEVICE_MAX_WRITE_IMAGE_ARGS:
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR:
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT:
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT:
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG:
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT:
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE:
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF:
    case CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR:
    case CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT:
    case CL_DEVICE_NATIVE_VECTOR_WIDTH_INT:
    case CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG:
    case CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT:
    case CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE:
    case CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF:
    case CL_DEVICE_MAX_CLOCK_FREQUENCY:
    case CL_DEVICE_IMAGE2D_MAX_HEIGHT:
    case CL_DEVICE_IMAGE2D_MAX_WIDTH:
    case CL_DEVICE_IMAGE3D_MAX_HEIGHT:
    case CL_DEVICE_IMAGE3D_MAX_WIDTH:
    case CL_DEVICE_IMAGE3D_MAX_DEPTH:
    case CL_DEVICE_MAX_PARAMETER_SIZE:
    case CL_DEVICE_MAX_WORK_GROUP_SIZE:
    case CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:
    case CL_DEVICE_LOCAL_MEM_SIZE:
    case CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE:
    case CL_DEVICE_MAX_MEM_ALLOC_SIZE:
    case CL_DEVICE_AVAILABLE:
    case CL_DEVICE_ENDIAN_LITTLE:
    case CL_DEVICE_HOST_UNIFIED_MEMORY:
    case CL_DEVICE_IMAGE_SUPPORT:
    case CL_DEVICE_TYPE:
    case CL_DEVICE_QUEUE_PROPERTIES:
    case CL_DEVICE_PLATFORM:
    case CL_DEVICE_LOCAL_MEM_TYPE:
    case CL_DEVICE_MAX_WORK_ITEM_SIZES:
    case CL_DEVICE_MAX_COMPUTE_UNITS:
    case CL_DEVICE_GLOBAL_MEM_SIZE:
    case CL_DEVICE_MEM_BASE_ADDR_ALIGN:
    case CL_DEVICE_SINGLE_FP_CONFIG:
    case CL_DEVICE_COMPILER_AVAILABLE:
    case CL_DEVICE_EXECUTION_CAPABILITIES:
    case CL_DEVICE_ERROR_CORRECTION_SUPPORT:
    case CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE:
    case CL_DEVICE_GLOBAL_MEM_CACHE_TYPE:
    case CL_DEVICE_PROFILING_TIMER_RESOLUTION:
    case CL_DEVICE_GLOBAL_MEM_CACHE_SIZE:
        return true;
    }

    return false;
}

bool isValidMemoryObjectFlag(unsigned memoryObjectFlag)
{
    switch (memoryObjectFlag) {
    case CL_MEM_READ_ONLY:
    case CL_MEM_WRITE_ONLY:
    case CL_MEM_READ_WRITE:
        return true;
    }

    return false;
}

bool isValidAddressingMode(unsigned value)
{
    switch(value) {
    case CL_ADDRESS_CLAMP_TO_EDGE:
    case CL_ADDRESS_CLAMP:
    case CL_ADDRESS_REPEAT:
    case CL_ADDRESS_MIRRORED_REPEAT:
        return true;
    }

    return false;
}

bool isValidFilterMode(unsigned value)
{
    switch(value) {
    case CL_FILTER_NEAREST:
    case CL_FILTER_LINEAR:
        return true;
    }

    return false;
}

bool isValidChannelOrder(unsigned value)
{
    switch (value) {
    case CL_R:
    case CL_A:
    case CL_RG:
    case CL_RA:
    case CL_RGB:
    case CL_RGBA:
    case CL_BGRA:
    case CL_ARGB:
    case CL_INTENSITY:
    case CL_LUMINANCE:
    case CL_Rx:
    case CL_RGx:
    case CL_RGBx:
        return true;
    }

    return false;
}

bool isValidChannelType(unsigned value)
{
    switch (value) {
    case CL_SNORM_INT8:
    case CL_SNORM_INT16:
    case CL_UNORM_INT8:
    case CL_UNORM_INT16:
    case CL_UNORM_SHORT_565:
    case CL_UNORM_SHORT_555:
    case CL_UNORM_INT_101010:
    case CL_SIGNED_INT8:
    case CL_SIGNED_INT16:
    case CL_SIGNED_INT32:
    case CL_UNSIGNED_INT8:
    case CL_UNSIGNED_INT16:
    case CL_UNSIGNED_INT32:
    case CL_HALF_FLOAT:
    case CL_FLOAT:
        return true;
    }

    return false;
}

bool isValidCommandQueueProperty(unsigned value)
{
    switch (value) {
    case 0: // 0 as integer value CommandQueueProperty is optional.
    case CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE:
    case CL_QUEUE_PROFILING_ENABLE:
        return true;
    }

    return false;
}

bool isValidKernelArgIndex(WebCLKernel* kernel, unsigned index)
{
    ASSERT(kernel);
    return index < kernel->numberOfArguments();
}

bool isValidDataSizeForDOMArrayBufferView(unsigned size, DOMArrayBufferView* arrayBufferView)
{
    ASSERT(arrayBufferView);

    unsigned bytesPerElement = 1;
    switch (arrayBufferView->type()) {
    case DOMArrayBufferView::TypeInt8:
    case DOMArrayBufferView::TypeUint8:
    case DOMArrayBufferView::TypeUint8Clamped:
        bytesPerElement = 1;
        break;
    case DOMArrayBufferView::TypeInt16:
    case DOMArrayBufferView::TypeUint16:
        bytesPerElement = 2;
        break;
    case DOMArrayBufferView::TypeInt32:
    case DOMArrayBufferView::TypeUint32:
    case DOMArrayBufferView::TypeFloat32:
        bytesPerElement = 4;
        break;
    case DOMArrayBufferView::TypeFloat64:
        bytesPerElement = 8;
        break;
    default:
        ASSERT_NOT_REACHED();
        return false;
    }

    return !(size % bytesPerElement);
}

bool isValidRegionForMemoryObject(const Vector<size_t>& origin, const Vector<size_t>& region, size_t rowPitch, size_t slicePitch, size_t length)
{
    size_t regionArea = region[0] * region[1] * region[2];
    if (!regionArea)
        return false;

    if (rowPitch) {
        // Validate User given rowPitch, region read = rowPitch * number of rows * number of slices.
        // The rowPitch is used to move the pointer to the next read the next row. By default its set to
        // row width. With user sent values we must ensure the read is within the bounds.
        size_t maximumReadPtrValue = rowPitch * region[1] * region[2];
        if (maximumReadPtrValue > length)
            return false;
    }

    if (slicePitch) {
        // Validate User given slicePitch , region read = slicePitch * number of slices.
        // The slicePitch is used to move the pointer for the next slice. Default value is size of slice
        // in bytes ( region[1] * rowPitch). Must be validated identical to rowPitch to avoid out of bound memory access.
        size_t maximumReadPtrValue = slicePitch * region[2];
        if (maximumReadPtrValue > length)
            return false;
    }

    // If row_pitch is 0, row_pitch is computed as region[0].
    rowPitch = rowPitch ? rowPitch : region[0];
    if (rowPitch < region[0])
        return false;

    // If slice_pitch is 0, slice_pitch is computed as region[1] * row_pitch.
    slicePitch = slicePitch ? slicePitch : (region[1] * rowPitch);
    if (slicePitch < rowPitch * region[1])
        return false;

    // The offset in bytes is computed as origin[2] * host_slice_pitch + origin[1] * rowPitch + origin[0].
    size_t offset = origin[2] * slicePitch + origin[1]  * rowPitch + origin[0];

    return (regionArea + offset) <= length;
}

bool isValidRegionForImage(const WebCLImageDescriptor& descriptor, const Vector<unsigned>& origin, const Vector<unsigned>& region)
{
    size_t regionArea = region[0] * region[1];
    if (!regionArea)
        return false;

    size_t height = descriptor.height();
    size_t width = descriptor.width();
    size_t offsetFromOrigin = origin[1] * height + origin[0];
    return (offsetFromOrigin + regionArea) <= (height * width);
}

bool isValidRegionForBuffer(const size_t bufferLength, const Vector<unsigned>& region, const size_t offset, const WebCLImageDescriptor& descriptor)
{
    // The size in bytes of the region to be copied from buffer is width * height * bytes/image element.
    size_t bytesCopied = region[0] * region[1]
        * WebCLContext::bytesPerChannelType(descriptor.channelType())
        * WebCLContext::numberOfChannelsForChannelOrder(descriptor.channelOrder());

    return (offset+ bytesCopied) <= bufferLength;
}

bool isValidRegionForHostPtr(const Vector<unsigned>& region, size_t rowPitch, const WebCLImageDescriptor& descriptor, size_t length)
{
    /*
    *  Validate the hostPtr length passed to enqueue*Image* API's. Since hostPtr are not validated by OpenCL
    *  Out of Bound access may cause crashes. So validating with rowPitch & region being read.
    *  rowPitch is used to move the pointer to next row for write/read.
    */
    size_t imageBytesPerPixel = WebCLContext::bytesPerChannelType(descriptor.channelType())
        * WebCLContext::numberOfChannelsForChannelOrder(descriptor.channelOrder());
    rowPitch = rowPitch ? rowPitch : region[0] * imageBytesPerPixel;
    if (rowPitch * region[1] > length)
        return false;

    size_t regionArea = region[0] * region[1];
    if (!regionArea)
        return false;

    return (regionArea <= length);
}

static bool valueInRange(size_t value, size_t minimum, size_t maximum)
{
    return ((value >= minimum) && (value <= maximum));
}

bool isRegionOverlapping(WebCLImage* source, WebCLImage* destination, const Vector<unsigned>& sourceOrigin, const Vector<unsigned>& destinationOrigin, const Vector<unsigned>& region)
{
    if (!source || !destination)
        return false;

    if (sourceOrigin.size() != 2 || destinationOrigin.size() != 2 || region.size() != 2)
        return false;

    if (source->getMem() != destination->getMem())
        return false;

    bool xOverlap = valueInRange(destinationOrigin[0], sourceOrigin[0], (region[0] + sourceOrigin[0])) || valueInRange(sourceOrigin[0], destinationOrigin[0], (destinationOrigin[0] + region[0]));
    bool yOverlap = valueInRange(destinationOrigin[1], sourceOrigin[1], (region[1] + sourceOrigin[1])) || valueInRange(sourceOrigin[1], destinationOrigin[1], (destinationOrigin[1] + region[1]));

    return xOverlap && yOverlap;
}

bool isRegionOverlapping(WebCLBuffer* srcBuffer, WebCLBuffer* destBuffer, const unsigned srcOffset, const unsigned dstOffset, const unsigned numBytes)
{
    if (!srcBuffer || !destBuffer)
        return false;

    if (srcBuffer->getMem() != destBuffer->getMem())
        return false;

    return valueInRange(dstOffset, srcOffset, (srcOffset + numBytes)) || valueInRange(srcOffset, dstOffset, (dstOffset + numBytes));
}

bool compareContext(WebCLContext* context1, WebCLContext* context2)
{
    if (!context1 || !context2)
        return false;

    return context1->getContext() == context2->getContext();
}

bool compareImageFormat(const WebCLImageDescriptor& srcDescriptor, const WebCLImageDescriptor& dstDescriptor)
{
    return (srcDescriptor.channelOrder() == dstDescriptor.channelOrder()) && (srcDescriptor.channelType() == dstDescriptor.channelType());
}

} // namespace WebCLInputChecker
} // namespace blink

#endif // ENABLE(WEBCL)
