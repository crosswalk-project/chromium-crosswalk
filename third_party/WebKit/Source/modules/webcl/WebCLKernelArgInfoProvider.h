// Copyright (C) 2013 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLKernelArgInfoProvider_h
#define WebCLKernelArgInfoProvider_h

#if ENABLE(WEBCL)

#include "modules/webcl/WebCLConfig.h"
#include "modules/webcl/WebCLKernelArgInfo.h"

#include <wtf/RefCounted.h>

namespace blink {

class WebCLKernel;

class WebCLKernelArgInfoProvider {
public:
    explicit WebCLKernelArgInfoProvider(WebCLKernel*);
    const Vector<RefPtr<WebCLKernelArgInfo>>& argumentsInfo() { return m_argumentInfoVector; };

    unsigned numberOfArguments() { return m_argumentInfoVector.size(); }
    const Vector<unsigned>& requiredArguments() { return m_requiredArgumentVector; }

private:
    void ensureInfo();
    void parseAndAppendDeclaration(const String& argumentDeclaration);
    String extractAddressQualifier(Vector<String>& declaration);
    String extractAccessQualifier(Vector<String>& declaration);
    String extractType(Vector<String>& declaration);
    String extractName(Vector<String>& declaration);

    WebCLKernel* m_kernel;
    Vector<RefPtr<WebCLKernelArgInfo>> m_argumentInfoVector;
    Vector<unsigned> m_requiredArgumentVector;
};

} // namespace blink

#endif // ENABLE(WEBCL)
#endif // WebCLKernelArgInfoProvider_h
