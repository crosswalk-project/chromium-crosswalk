// Copyright (C) 2013 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLKernelArgInfo_h
#define WebCLKernelArgInfo_h

#if ENABLE(WEBCL)
#include "modules/webcl/WebCLConfig.h"

#include <wtf/RefCounted.h>

namespace blink {

class WebCLKernelArgInfo : public RefCounted<WebCLKernelArgInfo>, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    enum {
        CHAR,
        UCHAR,
        SHORT,
        USHORT,
        INT,
        UINT,
        LONG,
        ULONG,
        FLOAT,
        DOUBLE,
        BUFFER,
        SAMPLER,
        IMAGE,
        UNKNOWN
    };

    static PassRefPtr<WebCLKernelArgInfo> create(const String& addressQualifier, const String& accessQualifier, const String& type, const String& name, const bool isPointerType = false) {
        return adoptRef(new WebCLKernelArgInfo(addressQualifier, accessQualifier, type, name, isPointerType));
    }

    const String& name() const { return m_name; }
    const String& typeName() const { return m_type; }
    const String& addressQualifier() const { return m_addressQualifier; }
    const String& accessQualifier() const { return m_accessQualifier; }
    int type() const { return extractTypeEnum(m_type, m_isPointerType); }
    bool hasLocalAddressQualifier() const { return m_hasLocalAddressQualifier; }
    void setAssociated(bool value) { m_associated = value; }
    bool isAssociated() const { return m_associated; }

private:
    WebCLKernelArgInfo(const String& addressQualifier, const String& accessQualifier, const String& type, const String& name, const bool isPointerType)
        : m_addressQualifier(addressQualifier)
        , m_accessQualifier(accessQualifier)
        , m_type(type)
        , m_name(name)
        , m_associated(false)
        , m_isPointerType(isPointerType)
    {
        m_hasLocalAddressQualifier = (m_addressQualifier == "local");
    }

    String m_addressQualifier;
    String m_accessQualifier;
    String m_type;
    String m_name;
    bool m_hasLocalAddressQualifier;
    bool m_associated;
    bool m_isPointerType;

    static inline int extractTypeEnum(const String& typeName, bool isPointerType)
    {
        if (isPointerType)
            return BUFFER;

        static String image2d_t("image2d_t");
        if (typeName == image2d_t)
            return IMAGE;

        static String sampler_t("sampler_t");
        if (typeName == sampler_t)
            return SAMPLER;

        static String ucharLiteral("uchar");
        if (typeName.contains(ucharLiteral))
            return UCHAR;
        static String charLiteral("char");
        if (typeName.contains(charLiteral))
            return CHAR;

        static String ushortLiteral("ushort");
        if (typeName.contains(ushortLiteral))
            return USHORT;
        static String shortLiteral("short");
        if (typeName.contains(shortLiteral))
            return SHORT;

        static String uintLiteral("uint");
        if (typeName.contains(uintLiteral))
            return UINT;
        static String intLiteral("int");
        if (typeName.contains(intLiteral))
            return INT;

        static String ulongLiteral("ulong");
        if (typeName.contains(ulongLiteral))
            return ULONG;
        static String longLiteral("long");
        if (typeName.contains(longLiteral))
            return LONG;

        static String floatLiteral("float");
        if (typeName.contains(floatLiteral))
            return FLOAT;

        static String doubleLiteral("double");
        if (typeName.contains(doubleLiteral))
            return DOUBLE;

        return UNKNOWN;
    }
};

} // namespace blink

#endif // ENABLE(WEBCL)
#endif // WebCLKernelArgInfo_h
