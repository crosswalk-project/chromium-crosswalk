// Copyright (c) 2013 Samsung Electronics Corporation. All rights reserved.
// Copyright (c) 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/webcl/WebCLKernelArgInfo.h"

namespace blink {

PassRefPtr<WebCLKernelArgInfo> WebCLKernelArgInfo::create(const String& addressQualifier, const String& accessQualifier, const String& type, const String& name, const bool isPointerType)
{
    return adoptRef(new WebCLKernelArgInfo(addressQualifier, accessQualifier, type, name, isPointerType));
}

WebCLKernelArgInfo::WebCLKernelArgInfo(const String& addressQualifier, const String& accessQualifier, const String& type, const String& name, const bool isPointerType)
    : m_addressQualifier(addressQualifier)
    , m_accessQualifier(accessQualifier)
    , m_type(type)
    , m_name(name)
    , m_associated(false)
    , m_isPointerType(isPointerType)
{
    m_hasLocalAddressQualifier = (m_addressQualifier == "local");
}

int WebCLKernelArgInfo::extractTypeEnum(const String& typeName, bool isPointerType)
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

} // namespace blink
