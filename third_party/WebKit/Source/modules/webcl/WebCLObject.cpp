// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/webcl/WebCLObject.h"

#include "modules/webcl/WebCLContext.h"

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
