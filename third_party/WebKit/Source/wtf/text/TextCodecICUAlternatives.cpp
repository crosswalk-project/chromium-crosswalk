// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "wtf/text/TextCodecICUAlternatives.h"
#include "wtf/PassOwnPtr.h"
#include "wtf/text/CharacterNames.h"
#include "wtf/text/WTFString.h"
#include "base/icu_alternatives_on_android/icu_utils.h"

namespace WTF {

TextCodecICUAlternatives::TextCodecICUAlternatives(const TextEncoding& encoding)
    : m_encoding(encoding)
{
}

void TextCodecICUAlternatives::registerEncodingNames(EncodingNameRegistrar registrar)
{
    //TODO, add more charsets supported by Java.
    registrar("Shift-JIS", "Shift-JIS");
}

PassOwnPtr<TextCodec> TextCodecICUAlternatives::create(const TextEncoding& encoding, const void*)
{
    return adoptPtr(new TextCodecICUAlternatives(encoding));
}

void TextCodecICUAlternatives::registerCodecs(TextCodecRegistrar registrar)
{
    registrar("Shift-JIS", create, 0);
}

String TextCodecICUAlternatives::decode(const char* bytes, size_t length, FlushBehavior flush, bool stopOnError, bool& sawError)
{ 
    if(!bytes || !length)
        return String();  
    bool result = false;
    const std::string encoded(bytes);
    int on_error = 0; //stop on error.
    if (!stopOnError) 
        on_error = 1; //skip on error.
    const char* codepage = m_encoding.name();
    base::string16 utf16;
    result = base::icu_utils::DecodeCodePage(codepage, encoded, on_error, &utf16);
    sawError = !result;
    return String(utf16.data());
}

CString TextCodecICUAlternatives::encode(const UChar* characters, size_t length, UnencodableHandling)
{
   if (!characters || !length)
       return CString();
   bool result = false;
   base::string16 utf16(characters, length);
   std::string encoded;
   result = base::icu_utils::EncodeCodePage(m_encoding.name(), utf16, 0, &encoded);
   if (!result)
      return CString();
   return CString(encoded.c_str());    
}
CString TextCodecICUAlternatives::encode(const LChar* characters, size_t length, UnencodableHandling)
{
    //CharsetEncoder only accept sixteen-bit Unicode characters as legal input. 
    return CString();
}

} // namespace WTF
