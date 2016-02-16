// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/webcl/WebCLException.h"
#include "modules/webcl/WebCL.h"
#include "modules/webcl/WebCLContext.h"
#include "modules/webcl/WebCLImage.h"
#include "modules/webcl/WebCLOpenCL.h"

namespace blink {

WebCLImage::~WebCLImage()
{
}

PassRefPtr<WebCLImage> WebCLImage::create(cl_mem image, const WebCLImageDescriptor& imageDescriptor, PassRefPtr<WebCLContext> context)
{
    return adoptRef(new WebCLImage(image, imageDescriptor, context));
}

void WebCLImage::getInfo(ExceptionState& es, WebCLImageDescriptor& descriptor)
{
    if (isReleased()) {
        es.throwWebCLException(WebCLException::INVALID_MEM_OBJECT, WebCLException::invalidMemObjectMessage);
        return;
    }

    descriptor.setWidth(m_imageDescriptor.width());
    descriptor.setHeight(m_imageDescriptor.height());
    descriptor.setRowPitch(m_imageDescriptor.rowPitch());
    descriptor.setChannelOrder(m_imageDescriptor.channelOrder());
    descriptor.setChannelType(m_imageDescriptor.channelType());
}

WebCLImage::WebCLImage(cl_mem image, const WebCLImageDescriptor& imageDescriptor, PassRefPtr<WebCLContext> context)
    : WebCLMemoryObject(image, 0, context)
    , m_imageDescriptor(imageDescriptor)
{
    size_t memorySizeValue = 0;
    cl_int err = clGetMemObjectInfo(image, CL_MEM_SIZE, sizeof(size_t), &memorySizeValue, nullptr);
    if (err == CL_SUCCESS)
        m_sizeInBytes = memorySizeValue;

    size_t actualRowPitch = 0;
    err = clGetImageInfo(image, CL_IMAGE_ROW_PITCH, sizeof(size_t), &actualRowPitch, nullptr);
    if (err == CL_SUCCESS)
        m_imageDescriptor.setRowPitch(actualRowPitch);
}

} // namespace blink
