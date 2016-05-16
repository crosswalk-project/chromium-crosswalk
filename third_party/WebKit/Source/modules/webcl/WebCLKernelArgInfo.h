// Copyright (C) 2013 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLKernelArgInfo_h
#define WebCLKernelArgInfo_h

#include "bindings/core/v8/ScriptWrappable.h"
#include "wtf/PassRefPtr.h"
#include "wtf/RefCounted.h"
#include "wtf/text/WTFString.h"

namespace blink {

class WebCLKernelArgInfo final : public RefCounted<WebCLKernelArgInfo>, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    enum {
        Char,
        Uchar,
        Short,
        Ushort,
        Int,
        Uint,
        Long,
        Ulong,
        Float,
        Double,
        Buffer,
        Sampler,
        Image,
        Unknown
    };

    static PassRefPtr<WebCLKernelArgInfo> create(const String& addressQualifier, const String& accessQualifier, const String& type, const String& name, const bool isPointerType = false);

    const String& name() const { return m_name; }
    const String& typeName() const { return m_type; }
    const String& addressQualifier() const { return m_addressQualifier; }
    const String& accessQualifier() const { return m_accessQualifier; }
    int type() const { return extractTypeEnum(m_type, m_isPointerType); }
    bool hasLocalAddressQualifier() const { return m_hasLocalAddressQualifier; }
    void setAssociated(bool value) { m_associated = value; }
    bool isAssociated() const { return m_associated; }

private:
    WebCLKernelArgInfo(const String& addressQualifier, const String& accessQualifier, const String& type, const String& name, const bool isPointerType);

    String m_addressQualifier;
    String m_accessQualifier;
    String m_type;
    String m_name;
    bool m_hasLocalAddressQualifier;
    bool m_associated;
    bool m_isPointerType;

    static int extractTypeEnum(const String& typeName, bool isPointerType);
};

} // namespace blink

#endif // WebCLKernelArgInfo_h
