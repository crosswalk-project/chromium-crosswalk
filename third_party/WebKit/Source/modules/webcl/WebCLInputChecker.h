// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLInputChecker_h
#define WebCLInputChecker_h

#include "core/dom/DOMArrayBufferView.h"
#include "modules/webcl/WebCLConfig.h"

namespace blink {

class WebCLBuffer;
class WebCLContext;
class WebCLImage;
class WebCLImageDescriptor;
class WebCLKernel;

namespace WebCLInputChecker {

bool isValidDeviceType(unsigned);
bool isValidDeviceInfoType(unsigned);
bool isValidMemoryObjectFlag(unsigned);
bool isValidAddressingMode(unsigned);
bool isValidFilterMode(unsigned);
bool isValidChannelType(unsigned);
bool isValidChannelOrder(unsigned);
bool isValidCommandQueueProperty(unsigned);
bool isValidKernelArgIndex(WebCLKernel*, unsigned index);
bool isValidDataSizeForDOMArrayBufferView(unsigned size, DOMArrayBufferView*);
bool isValidRegionForMemoryObject(const Vector<size_t>& origin, const Vector<size_t>& region, size_t rowPitch, size_t slicePitch, size_t length);
bool isValidRegionForHostPtr(const Vector<unsigned>& region, size_t rowPitch, const WebCLImageDescriptor&, size_t length);
bool isValidRegionForImage(const WebCLImageDescriptor&, const Vector<unsigned>& origin, const Vector<unsigned>& region);
bool isValidRegionForBuffer(const size_t bufferLength, const Vector<unsigned>& region, const size_t offset, const WebCLImageDescriptor&);
bool isRegionOverlapping(WebCLImage*, WebCLImage*, const Vector<unsigned>& srcOrigin, const Vector<unsigned>& dstOrigin, const Vector<unsigned>& region);
bool isRegionOverlapping(WebCLBuffer*, WebCLBuffer*, const unsigned srcOffset, const unsigned dstOffset, const unsigned numBytes);
bool compareContext(WebCLContext*, WebCLContext*);
bool compareImageFormat(const WebCLImageDescriptor&, const WebCLImageDescriptor&);
} // namespace WebCLInputChecker

} // namespace blink

#endif // WebCLInputChecker_h
