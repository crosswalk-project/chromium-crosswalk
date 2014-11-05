// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/i18n/string_compare.h"

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"

#if defined(USE_ICU_ALTERNATIVES_ON_ANDROID)
#include "base/icu_alternatives_on_android/icu_utils.h"
#endif

namespace base {
namespace i18n {

// Compares the character data stored in two different string16 strings by
// specified Collator instance.
UCollationResult CompareString16WithCollator(
#if defined(USE_ICU_ALTERNATIVES_ON_ANDROID)
    const std::string& locale,
#else
    const icu::Collator* collator,
#endif
    const string16& lhs,
    const string16& rhs) {
#if defined(USE_ICU_ALTERNATIVES_ON_ANDROID)
  int result = base::icu_utils::CompareString16WithCollator(locale, lhs, rhs);
  if (result == 0)
    return UCOL_EQUAL;
  else if (result > 0)
    return UCOL_GREATER;
  else
    return UCOL_LESS;
#else
  DCHECK(collator);
  UErrorCode error = U_ZERO_ERROR;
  UCollationResult result = collator->compare(
      static_cast<const UChar*>(lhs.c_str()), static_cast<int>(lhs.length()),
      static_cast<const UChar*>(rhs.c_str()), static_cast<int>(rhs.length()),
      error);
  DCHECK(U_SUCCESS(error));
  return result;
#endif  // defined(USE_ICU_ALTERNATIVES_ON_ANDROID)
}

}  // namespace i18n
}  // namespace base
