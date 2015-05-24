// Copyright (C) 2011 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLSampler_h
#define WebCLSampler_h

#if ENABLE(WEBCL)
#include "modules/webcl/WebCLConfig.h"
#include "modules/webcl/WebCLObject.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace blink {

class ExceptionState;
class WebCL;

class WebCLSampler : public WebCLObject, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    ~WebCLSampler() override;
    static PassRefPtr<WebCLSampler> create(cl_sampler, bool, unsigned, unsigned, PassRefPtr<WebCLContext>);

    ScriptValue getInfo(ScriptState*, cl_sampler_info, ExceptionState&);
    void release() override;

    cl_sampler getSampler() const { return m_clSampler; }

private:
    WebCLSampler(cl_sampler, bool, unsigned, unsigned, PassRefPtr<WebCLContext>);
    bool isReleased() const { return !m_clSampler; }

    bool m_normCoords;
    unsigned m_addressingMode;
    unsigned m_filterMode;
    cl_sampler m_clSampler;
};

} // namespace blink

#endif // ENABLE(WEBCL)
#endif // WebCLSampler_h
