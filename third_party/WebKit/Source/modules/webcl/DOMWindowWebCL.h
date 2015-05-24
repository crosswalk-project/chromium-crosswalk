// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DOMWindowWebCL_h
#define DOMWindowWebCL_h

#include "core/frame/DOMWindowProperty.h"
#include "platform/Supplementable.h"

namespace blink {

class LocalDOMWindow;
class WebCL;

class DOMWindowWebCL : public NoBaseWillBeGarbageCollected<DOMWindowWebCL>, public WillBeHeapSupplement<LocalDOMWindow>, public DOMWindowProperty {
    WILL_BE_USING_GARBAGE_COLLECTED_MIXIN(DOMWindowWebCL);
    DECLARE_EMPTY_VIRTUAL_DESTRUCTOR_WILL_BE_REMOVED(DOMWindowWebCL);
public:
    static DOMWindowWebCL& from(LocalDOMWindow&);
    static WebCL* webcl(DOMWindow&);

    void willDestroyGlobalObjectInFrame() override;
    void willDetachGlobalObjectFromFrame() override;

    void trace(Visitor*);

private:
    explicit DOMWindowWebCL(LocalDOMWindow&);

    WebCL* webcl();
    static const char* supplementName();

    LocalDOMWindow& m_window;
    RefPtr<WebCL> m_webcl;
};

} // namespace blink

#endif // DOMWindowWebCL_h
