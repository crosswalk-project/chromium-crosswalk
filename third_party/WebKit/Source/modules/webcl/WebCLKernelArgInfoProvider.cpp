// Copyright (C) 2013 Samsung Electronics Corporation. All rights reserved.
// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#if ENABLE(WEBCL)
#include "core/webcl/WebCLException.h"
#include "modules/webcl/WebCLKernel.h"
#include "modules/webcl/WebCLKernelArgInfoProvider.h"
#include "modules/webcl/WebCLProgram.h"

namespace blink {

const size_t notFound = static_cast<size_t>(-1);

static bool isASCIILineBreakCharacter(UChar c)
{
    return c == '\r' || c == '\n';
}

inline bool isEmptySpace(UChar c)
{
    return c <= ' ' && (c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\f');
}

inline bool isStarCharacter(UChar c)
{
    return c == '*';
}

inline bool isPrecededByUnderscores(const String& string, size_t index)
{
    size_t start = index - 2;
    return (start >= 0 && string[start + 1] == '_' && string[start] == '_');
}

WebCLKernelArgInfoProvider::WebCLKernelArgInfoProvider(WebCLKernel* kernel)
    : m_kernel(kernel)
{
    ASSERT(kernel);
    ensureInfo();
}

void WebCLKernelArgInfoProvider::ensureInfo()
{
    if (m_argumentInfoVector.size())
        return;

    const String& source = m_kernel->program()->sourceWithCommentsStripped();
    // 0) find "kernel" string.
    // 1) Check if it is a valid kernel declaration.
    // 2) find the first open braces past "kernel".
    // 3) reverseFind the given kernel name string.
    // 4) if not found go back to (1)
    // 5) if found, parse its argument list.

    size_t kernelNameIndex = 0;
    size_t kernelDeclarationIndex = 0;
    for (size_t startIndex = 0; ; startIndex = kernelDeclarationIndex + 6) {
        kernelDeclarationIndex = source.find("kernel", startIndex);
        if (kernelDeclarationIndex == notFound) {
            return;
        }

        // Check if "kernel" is not a substring of a valid token,
        // e.g. "akernel" or "__kernel_":
        // 1) After "kernel" there has to be an empty space.
        // 2) Before "kernel" there has to be either:
        // 2.1) two underscore characters or
        // 2.2) none, i.e. "kernel" is the first string in the program source or
        // 2.3) an empty space.
        if (!isEmptySpace(source[kernelDeclarationIndex + 6]))
            continue;

        // If the kernel declaration is not at the beginning of the program.
        bool hasTwoUnderscores = isPrecededByUnderscores(source, kernelDeclarationIndex);
        bool isKernelDeclarationAtBeginning = hasTwoUnderscores ? (kernelDeclarationIndex == 2) : (kernelDeclarationIndex == 0);

        if (!isKernelDeclarationAtBeginning) {
            size_t firstPrecedingIndex = kernelDeclarationIndex - (hasTwoUnderscores ? 3 : 1);
            if (!isEmptySpace(source[firstPrecedingIndex]))
                continue;
        }

        size_t openBrace = source.find("{", kernelDeclarationIndex + 6);
        kernelNameIndex = source.reverseFind(m_kernel->kernelName(), openBrace);

        if (kernelNameIndex < kernelDeclarationIndex)
            continue;

        if (kernelNameIndex != notFound)
            break;
    }

    ASSERT(kernelNameIndex);
    size_t requiredIndex = source.reverseFind("required_work_group_size", kernelNameIndex);
    if (requiredIndex != notFound) {
        size_t requiredOpenBracket = source.find("(", requiredIndex);
        size_t requiredCloseBracket = source.find(")", requiredOpenBracket);
        const String& requiredArgumentListStr = source.substring(requiredOpenBracket + 1, requiredCloseBracket - requiredOpenBracket - 1);

        Vector<String> requiredArgumentStrVector;
        requiredArgumentListStr.split(",", requiredArgumentStrVector);
        for (auto requiredArgument : requiredArgumentStrVector) {
            requiredArgument = requiredArgument.removeCharacters(isASCIILineBreakCharacter);
            requiredArgument = requiredArgument.stripWhiteSpace();
            m_requiredArgumentVector.append(requiredArgument.toUInt());
        }
    }

    size_t openBracket = source.find("(", kernelNameIndex);
    size_t closeBracket = source.find(")", openBracket);
    const String& argumentListStr = source.substring(openBracket + 1, closeBracket - openBracket - 1);

    Vector<String> argumentStrVector;
    argumentListStr.split(",", argumentStrVector);
    for (auto argument : argumentStrVector) {
        argument = argument.removeCharacters(isASCIILineBreakCharacter);
        argument = argument.stripWhiteSpace();
        parseAndAppendDeclaration(argument);
    }
}

static void prependUnsignedIfNeeded(Vector<String>& declarationStrVector, String& type)
{
    for (size_t i = 0; i < declarationStrVector.size(); i++) {
        static AtomicString& Unsigned = *new AtomicString("unsigned", AtomicString::ConstructFromLiteral);
        if (declarationStrVector[i] == Unsigned) {
            type = "u" + type;
            declarationStrVector.remove(i);
            return;
        }
    }
}

void WebCLKernelArgInfoProvider::parseAndAppendDeclaration(const String& argumentDeclaration)
{
    // "*" is used to indicate pointer data type, setting isPointerType flag if "*" is present in argumentDeclaration.
    // Since we parse only valid & buildable OpenCL kernels, * in argumentDeclaration must be associated with type only.
    bool isPointerType = false;
    if (argumentDeclaration.contains("*"))
        isPointerType = true;

    Vector<String> declarationStrVector;
    argumentDeclaration.removeCharacters(isStarCharacter).split(" ", declarationStrVector);

    const String& name = extractName(declarationStrVector);
    const String& addressQualifier = extractAddressQualifier(declarationStrVector);
    String type = extractType(declarationStrVector);

    static AtomicString& image2d_t = *new AtomicString("image2d_t", AtomicString::ConstructFromLiteral);
    const String& accessQualifier = (type == image2d_t) ? extractAccessQualifier(declarationStrVector) : "none";
    prependUnsignedIfNeeded(declarationStrVector, type);

    m_argumentInfoVector.append(WebCLKernelArgInfo::create(addressQualifier, accessQualifier, type, name, isPointerType));
}

String WebCLKernelArgInfoProvider::extractAddressQualifier(Vector<String>& declarationStrVector)
{
    static AtomicString* __Private = new AtomicString("__private", AtomicString::ConstructFromLiteral);
    static AtomicString* Private = new AtomicString("private", AtomicString::ConstructFromLiteral);

    static AtomicString* __Global = new AtomicString("__global", AtomicString::ConstructFromLiteral);
    static AtomicString* Global = new AtomicString("global", AtomicString::ConstructFromLiteral);

    static AtomicString* __Constant = new AtomicString("__constant", AtomicString::ConstructFromLiteral);
    static AtomicString* Constant = new AtomicString("constant", AtomicString::ConstructFromLiteral);

    static AtomicString* __Local = new AtomicString("__local", AtomicString::ConstructFromLiteral);
    static AtomicString* Local = new AtomicString("local", AtomicString::ConstructFromLiteral);

    String address = *Private;
    size_t i = 0;
    for (; i < declarationStrVector.size(); i++) {
        const String& candidate = declarationStrVector[i];
        if (candidate == *__Private || candidate == *Private) {
            break;
        } else if (candidate == *__Global || candidate == *Global) {
            address = *Global;
            break;
        } else if (candidate == *__Constant || candidate == *Constant) {
            address = *Constant;
            break;
        } else if (candidate == *__Local || candidate == *Local) {
            address = *Local;
            break;
        }
    }

    if (i < declarationStrVector.size())
        declarationStrVector.remove(i);

    return address;
}

String WebCLKernelArgInfoProvider::extractAccessQualifier(Vector<String>& declarationStrVector)
{
    static AtomicString* __read_only = new AtomicString("__read_only", AtomicString::ConstructFromLiteral);
    static AtomicString* read_only = new AtomicString("read_only", AtomicString::ConstructFromLiteral);

    static AtomicString* __write_only = new AtomicString("__read_only", AtomicString::ConstructFromLiteral);
    static AtomicString* write_only = new AtomicString("write_only", AtomicString::ConstructFromLiteral);

    static AtomicString* __read_write = new AtomicString("__read_write", AtomicString::ConstructFromLiteral);
    static AtomicString* read_write = new AtomicString("read_write", AtomicString::ConstructFromLiteral);

    String access = *read_only;
    size_t i = 0;
    for (; i < declarationStrVector.size(); i++) {
        const String& candidate = declarationStrVector[i];
        if (candidate == *__read_only  || candidate == *read_only) {
            break;
        } else if (candidate == *__write_only || candidate == *write_only) {
            access = *write_only;
            break;
        } else if (candidate == *__read_write || candidate == *read_write) {
            access = *read_write;
            break;
        }
    }

    if (i < declarationStrVector.size())
        declarationStrVector.remove(i);

    return access;
}

String WebCLKernelArgInfoProvider::extractName(Vector<String>& declarationStrVector)
{
    String last = declarationStrVector.last();
    declarationStrVector.removeLast();
    return last;
}

String WebCLKernelArgInfoProvider::extractType(Vector<String>& declarationStrVector)
{
    String type = declarationStrVector.last();
    declarationStrVector.removeLast();
    return type;
}

} // namespace blink

#endif // ENABLE(WEBCL)
