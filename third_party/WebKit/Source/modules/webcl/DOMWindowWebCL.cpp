// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "DOMWindowWebCL.h"

#include "core/dom/Document.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/page/Page.h"
#include "modules/webcl/WebCL.h"

namespace blink {

DOMWindowWebCL::DOMWindowWebCL(LocalDOMWindow& window)
    : DOMWindowProperty(window.frame())
    , m_window(&window)
{
}

DEFINE_TRACE(DOMWindowWebCL)
{
    visitor->trace(m_window);
    HeapSupplement<LocalDOMWindow>::trace(visitor);
    DOMWindowProperty::trace(visitor);
}

DOMWindowWebCL& DOMWindowWebCL::from(LocalDOMWindow& window)
{
    DOMWindowWebCL* supplement = static_cast<DOMWindowWebCL*>(HeapSupplement<LocalDOMWindow>::from(window, supplementName()));
    if (!supplement) {
        supplement = new DOMWindowWebCL(window);
        provideTo(window, supplementName(), supplement);
    }

    return *supplement;
}

void DOMWindowWebCL::willDestroyGlobalObjectInFrame()
{
    m_webcl = nullptr;
    DOMWindowProperty::willDestroyGlobalObjectInFrame();
}

void DOMWindowWebCL::willDetachGlobalObjectFromFrame()
{
    m_webcl = nullptr;
    DOMWindowProperty::willDetachGlobalObjectFromFrame();
}

WebCL* DOMWindowWebCL::webcl(DOMWindow& window)
{
    return from(toLocalDOMWindow(window)).webcl();
}

WebCL* DOMWindowWebCL::webcl()
{
    if (!m_window->document() || !m_window->document()->page())
        return nullptr;

    if (!m_webcl)
        m_webcl = WebCL::create();

    return m_webcl.get();
}

const char* DOMWindowWebCL::supplementName()
{
    return "DOMWindowWebCL";
}

} // namespace blink
