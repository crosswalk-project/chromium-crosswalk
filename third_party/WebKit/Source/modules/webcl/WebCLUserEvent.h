// Copyright (C) 2013 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLUserEvent_h
#define WebCLUserEvent_h

#include "modules/webcl/WebCLConfig.h"
#include "modules/webcl/WebCLEvent.h"
#include "wtf/PassRefPtr.h"

namespace blink {

class WebCLContext;

class WebCLUserEvent : public WebCLEvent {
    DEFINE_WRAPPERTYPEINFO();
public:
    ~WebCLUserEvent() override;
    static PassRefPtr<WebCLUserEvent> create(PassRefPtr<WebCLContext>, ExceptionState&);

    void setStatus(cl_int, ExceptionState&);
    ScriptValue getInfo(ScriptState* scriptState, unsigned, ExceptionState&) override;

    enum EventStatusSituation { StatusUnset, StatusSet };
    bool isUserEvent() const override { return true; }
    int getStatus() override { return m_executionStatus; }

private:
    WebCLUserEvent(cl_event event, PassRefPtr<WebCLContext>);
    enum EventStatusSituation m_eventStatusSituation;
    int m_executionStatus;
};

} // namespace blink

#endif // WebCLUserEvent_h
