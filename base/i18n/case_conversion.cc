// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/i18n/case_conversion.h"

#include "base/strings/string16.h"

#if defined(USE_ICU_ALTERNATIVES_ON_ANDROID)
#include "base/icu_alternatives_on_android/icu_utils.h"
#else
#include "third_party/icu/source/common/unicode/unistr.h"
#endif

namespace base {
namespace i18n {

string16 ToLower(const StringPiece16& string) {
#if defined(USE_ICU_ALTERNATIVES_ON_ANDROID)
  return base::icu_utils::ToLower(string);
#else
  icu::UnicodeString unicode_string(string.data(), string.size());
  unicode_string.toLower();
  return string16(unicode_string.getBuffer(), unicode_string.length());
#endif
}

string16 ToUpper(const StringPiece16& string) {
#if defined(USE_ICU_ALTERNATIVES_ON_ANDROID)
  return base::icu_utils::ToUpper(string);
#else
  icu::UnicodeString unicode_string(string.data(), string.size());
  unicode_string.toUpper();
  return string16(unicode_string.getBuffer(), unicode_string.length());
#endif
}

}  // namespace i18n
}  // namespace base
