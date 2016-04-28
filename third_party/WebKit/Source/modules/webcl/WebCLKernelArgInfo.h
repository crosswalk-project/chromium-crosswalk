// Copyright (C) 2013 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLKernelArgInfo_h
#define WebCLKernelArgInfo_h

#include "modules/webcl/WebCLConfig.h"
#include "wtf/RefCounted.h"

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

    static PassRefPtr<WebCLKernelArgInfo> create(const String& addressQualifier, const String& accessQualifier, const String& type, const String& name, const bool isPointerType = false)
    {
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
            return Buffer;

        static String image2dLiteral("image2d_t");
        if (typeName == image2dLiteral)
            return Image;

        static String samplerLiteral("sampler_t");
        if (typeName == samplerLiteral)
            return Sampler;

        static String ucharLiteral("uchar");
        if (typeName.contains(ucharLiteral))
            return Uchar;
        static String charLiteral("char");
        if (typeName.contains(charLiteral))
            return Char;

        static String ushortLiteral("ushort");
        if (typeName.contains(ushortLiteral))
            return Ushort;
        static String shortLiteral("short");
        if (typeName.contains(shortLiteral))
            return Short;

        static String uintLiteral("uint");
        if (typeName.contains(uintLiteral))
            return Uint;
        static String intLiteral("int");
        if (typeName.contains(intLiteral))
            return Int;

        static String ulongLiteral("ulong");
        if (typeName.contains(ulongLiteral))
            return Ulong;
        static String longLiteral("long");
        if (typeName.contains(longLiteral))
            return Long;

        static String floatLiteral("float");
        if (typeName.contains(floatLiteral))
            return Float;

        static String doubleLiteral("double");
        if (typeName.contains(doubleLiteral))
            return Double;

        return Unknown;
    }
};

} // namespace blink

#endif // WebCLKernelArgInfo_h
