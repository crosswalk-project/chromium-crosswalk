// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "wtf/build_config.h"

#if ENABLE(WEBCL)
#include "modules/webcl/WebCLContext.h"
#include "modules/webcl/WebCLObject.h"

namespace blink {

WebCLObject::~WebCLObject()
{
    if (m_context)
        m_context->untrackReleaseableWebCLObject(createWeakPtr());
}

WebCLObject::WebCLObject(PassRefPtr<WebCLContext> context)
    : m_weakFactory(this)
    , m_context(context)
{
    ASSERT(m_context);
    m_context->trackReleaseableWebCLObject(createWeakPtr());
}

WebCLObject::WebCLObject()
    : m_weakFactory(this)
    , m_context(nullptr)
{
}

PassRefPtr<WebCLContext> WebCLObject::context()
{
    ASSERT(m_context);
    return m_context;
}

void WebCLObject::setContext(PassRefPtr<WebCLContext> context)
{
    m_context = context;
    m_context->trackReleaseableWebCLObject(createWeakPtr());
}

} // namespace blink

#endif // ENABLE(WEBCL)
