// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TextCodecICUAlternatives_h
#define TextCodecICUAlternatives_h

#include "wtf/text/TextCodec.h"
#include "wtf/text/TextEncoding.h"

namespace WTF {

class TextCodecICUAlternatives final : public TextCodec {
public:
    TextCodecICUAlternatives(const TextEncoding&);
    static void registerEncodingNames(EncodingNameRegistrar);
    static void registerCodecs(TextCodecRegistrar);

private: 
    static PassOwnPtr<TextCodec> create(const TextEncoding&, const void*);

    virtual String decode(const char* bytes, size_t length, FlushBehavior flush, bool stopOnError, bool& sawError) override;

    virtual CString encode(const UChar* characters, size_t length, UnencodableHandling) override;
    virtual CString encode(const LChar* characters, size_t length, UnencodableHandling) override;

    TextEncoding m_encoding;
};

} // namespace WTF

#endif // TextCodecICUAlternatives_h
