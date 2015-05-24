// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLImage_h
#define WebCLImage_h

#include "modules/webcl/WebCLConfig.h"
#include "modules/webcl/WebCLImageDescriptor.h"
#include "modules/webcl/WebCLMemoryObject.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace blink {

class ExceptionState;
class WebCL;

class WebCLImage : public WebCLMemoryObject {
    DEFINE_WRAPPERTYPEINFO();
public:
    ~WebCLImage() override;
    static PassRefPtr<WebCLImage> create(cl_mem, const WebCLImageDescriptor&, PassRefPtr<WebCLContext>);

    void getInfo(ExceptionState&, WebCLImageDescriptor&);
    const WebCLImageDescriptor& imageDescriptor() { return m_imageDescriptor; }
    int type() override { return IMAGE; }

private:
    WebCLImage(cl_mem, const WebCLImageDescriptor&, PassRefPtr<WebCLContext>);

    WebCLImageDescriptor m_imageDescriptor;
};

} // namespace blink

#endif // WebCLImage_h
