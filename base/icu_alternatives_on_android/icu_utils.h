// Copyright 2014 Intel Corp. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_ICU_ALTERNATIVES_ON_ANDROID_ICU_UTILS_H_
#define BASE_ICU_ALTERNATIVES_ON_ANDROID_ICU_UTILS_H_

#include <jni.h>
#include <string.h>

#include <unicode/uchar.h>
#include <unicode/uscript.h>

#include "base/android/jni_android.h"
#include "base/icu_alternatives_on_android/break_iterator_bridge.h"
#include "base/strings/string16.h"
#include "base/strings/string_piece.h"

using base::android::ScopedJavaLocalRef;

namespace base {

namespace icu_utils {

base::string16 IDNToUnicode(const std::string& host,
                            const std::string& languages);
base::string16 ToLower(const StringPiece16& string);
base::string16 ToUpper(const StringPiece16& string);
std::string CountryCodeForCurrentTimezone();
base::string16 FormatNumber(int64 number);
base::string16 FormatNumber(double number, int fractional_digits);
bool EncodeCodePage(const char* codepage, const base::string16& utf16,
                    int on_error, std::string* encoded);
bool DecodeCodePage(const char* codepage, const std::string& decoded,
                    int on_error, base::string16* utf16);
std::string GetConfiguredLocale();
void SetDefaultLocale(const std::string& locale_string);
base::string16 FormatTimeOfDay(long mills, bool hour24, bool keep_am_pm);
base::string16 FormatDate(long mills, int date_type, bool append_time_of_day);
bool Is24HourFormat();
int CompareString16WithCollator(
    const std::string& locale,
    const base::string16& lhs, const base::string16& rhs);
bool IsCharInFilenameIllegal(int code_point);
ScopedJavaLocalRef<jobject> getJavaLocale(const std::string& locale);
base::string16 CutOffStringForEliding(const base::string16& input, size_t max, bool word_break);

UChar32 foldCase(UChar32 c);
int foldCase(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error);
int toLower(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error);
UChar32 toLower(UChar32 c);
UChar32 toUpper(UChar32 c);
int toUpper(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error);
UChar32 toTitleCase(UChar32 c);
bool isArabicChar(UChar32 c);
bool isAlphanumeric(UChar32 c);
bool isSeparatorSpace(UChar32 c);
bool isPrintableChar(UChar32 c);
bool isPunct(UChar32 c);
bool hasLineBreakingPropertyComplexContext(UChar32 c);
UChar32 mirroredChar(UChar32 c);
UCharCategory category(UChar32 c);
UCharDirection direction(UChar32 c);
bool isLower(UChar32 c);
bool isUpper(UChar32 c);
uint8_t combiningClass(UChar32 c);
int32_t decompositionType(UChar32 c);
int umemcasecmp(const UChar* a, const UChar* b, int len);
int32_t normalize(const UChar *source, int32_t sourceLength, int32_t options,
                  UChar *result, int32_t resultLength,bool *error);

class JCollator {
 public:
  JCollator(const char* locale);
  int collate(const UChar*, size_t, const UChar*, size_t) const;
 private:
  std::string m_locale;
};


int collate(const std::string& locale, const UChar* source1, size_t source1Length,
            const UChar* source2, size_t source2Length);

}  // icu_utils

bool RegisterIcuUtilsJni(JNIEnv* env);

}  // namespace base

#endif  //BASE_ICU_ALTERNATIVES_ON_ANDROID_ICU_UTILS_H_
