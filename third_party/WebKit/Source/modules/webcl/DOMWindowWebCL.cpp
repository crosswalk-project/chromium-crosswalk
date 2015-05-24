// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#if ENABLE(WEBCL)

#include "core/dom/Document.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/page/Page.h"
#include "DOMWindowWebCL.h"
#include "modules/webcl/WebCL.h"

namespace blink {

DOMWindowWebCL::DOMWindowWebCL(LocalDOMWindow& window)
    : DOMWindowProperty(window.frame())
    , m_window(window)
{
}

DEFINE_EMPTY_DESTRUCTOR_WILL_BE_REMOVED(DOMWindowWebCL);

void DOMWindowWebCL::trace(Visitor* visitor)
{
    visitor->trace(m_webcl);
    WillBeHeapSupplement<LocalDOMWindow>::trace(visitor);
    DOMWindowProperty::trace(visitor);
}

DOMWindowWebCL& DOMWindowWebCL::from(LocalDOMWindow& window)
{
    DOMWindowWebCL* supplement = static_cast<DOMWindowWebCL*>(WillBeHeapSupplement<LocalDOMWindow>::from(window, supplementName()));
    if (!supplement) {
        supplement = new DOMWindowWebCL(window);
        provideTo(window, supplementName(), adoptPtrWillBeNoop(supplement));
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
    if (!m_window.document() || !m_window.document()->page())
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

#endif // ENABLE(WEBCL)
