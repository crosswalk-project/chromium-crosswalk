// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_I18N_STRING_COMPARE_H_
#define BASE_I18N_STRING_COMPARE_H_

#include <algorithm>
#include <string>
#include <vector>

#include "base/i18n/base_i18n_export.h"
#include "base/strings/string16.h"
#include "third_party/icu/source/i18n/unicode/coll.h"

namespace base {
namespace i18n {

// Compares the two strings using the specified collator.
BASE_I18N_EXPORT UCollationResult CompareString16WithCollator(
#if defined(USE_ICU_ALTERNATIVES_ON_ANDROID)
    const std::string& locale,
#else
    const icu::Collator* collator,
#endif
    const string16& lhs,
    const string16& rhs);

}  // namespace i18n
}  // namespace base

#endif  // BASE_I18N_STRING_COMPARATOR_H_
