// Copyright (C) 2013 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLExtension_h
#define WebCLExtension_h

#if ENABLE(WEBCL)

#include "core/webcl/WebCLException.h"
#include "modules/webcl/WebCLConfig.h"

namespace blink {

class WebCLExtension {
public:
    ~WebCLExtension() {}
    WebCLExtension() {}

    bool enableExtension(const String& name);
    Vector<String> getSupportedExtensions();
    bool isEnabledExtension(const String& name) const;
    void getEnabledExtensions(HashSet<String>&);
    void addSupportedCLExtension(const String& name);

private:
    Vector<String> m_enabledExtensions;
    Vector<String> m_supportedCLExtensions;
};

} // blink

#endif // ENABLE(WEBCL)
#endif // WebCLExtension_h
