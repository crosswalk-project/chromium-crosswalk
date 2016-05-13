// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DOMWindowWebCL_h
#define DOMWindowWebCL_h

#include "core/frame/DOMWindowProperty.h"
#include "platform/Supplementable.h"

namespace blink {

class DOMWindow;
class LocalDOMWindow;
class WebCL;

class DOMWindowWebCL final : public GarbageCollectedFinalized<DOMWindowWebCL>, public HeapSupplement<LocalDOMWindow>, public DOMWindowProperty {
    USING_GARBAGE_COLLECTED_MIXIN(DOMWindowWebCL);
public:
    static DOMWindowWebCL& from(LocalDOMWindow&);
    static WebCL* webcl(DOMWindow&);

    ~DOMWindowWebCL();

    void willDestroyGlobalObjectInFrame() override;
    void willDetachGlobalObjectFromFrame() override;

    DECLARE_TRACE();

private:
    explicit DOMWindowWebCL(LocalDOMWindow&);

    WebCL* webcl();
    static const char* supplementName();

    Member<LocalDOMWindow> m_window;
    RefPtr<WebCL> m_webcl;
};

} // namespace blink

#endif // DOMWindowWebCL_h
