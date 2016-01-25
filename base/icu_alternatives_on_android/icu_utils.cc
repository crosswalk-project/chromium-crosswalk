// Copyright 2014 Intel Corp. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/icu_alternatives_on_android/icu_utils.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/strings/string_piece.h"
#include "jni/IcuUtils_jni.h"

using namespace base::android;

namespace base {

namespace icu_utils {
string16 IDNToUnicode(const std::string& host,
                      const std::string& languages) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> java_host =
      ConvertUTF8ToJavaString(
          env, base::StringPiece(host));
  ScopedJavaLocalRef<jstring> java_lang =
      ConvertUTF8ToJavaString(
          env, base::StringPiece(languages));

  ScopedJavaLocalRef<jstring> java_result =
      android::Java_IcuUtils_idnToUnicode(
          env, java_host.obj(), java_lang.obj());

  return ConvertJavaStringToUTF16(java_result);
}

string16 ToLower(const StringPiece16& string) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> sourceString =
      ConvertUTF16ToJavaString(env, string);
  ScopedJavaLocalRef<jstring> java_result =
      Java_IcuUtils_toLowerString(env, sourceString.obj());
  return ConvertJavaStringToUTF16(java_result);
}

string16 ToUpper(const StringPiece16& string) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> sourceString =
      ConvertUTF16ToJavaString(env, string);
  ScopedJavaLocalRef<jstring> java_result =
      Java_IcuUtils_toUpperString(env, sourceString.obj());
  return ConvertJavaStringToUTF16(java_result);
}

std::string CountryCodeForCurrentTimezone() {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> java_result =
      Java_IcuUtils_countryCodeForCurrentTimeZone(env);
  return ConvertJavaStringToUTF8(java_result);
}

base::string16 FormatNumber(int64 number) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> java_result =
      Java_IcuUtils_formatInteger(env, number);
  return ConvertJavaStringToUTF16(java_result);
}

base::string16 FormatNumber(double number, int fractional_digits) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> java_result =
      Java_IcuUtils_formatFloat(env, number, fractional_digits);
  return ConvertJavaStringToUTF16(java_result);
}

bool EncodeCodePage(const char* codepage, const base::string16& utf16,
                    int on_error, std::string* encoded) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> codepageString =
      ConvertUTF8ToJavaString(env, codepage);
  jcharArray src = env->NewCharArray(utf16.length());
  env->SetCharArrayRegion(src, 0, utf16.length(), utf16.c_str());// uint16 is jchar.
  ScopedJavaLocalRef<jbyteArray> result = Java_IcuUtils_encodeCodePage(
      env, codepageString.obj(), src, on_error);
  if (result.obj() == NULL) {
    encoded->clear();
    return false;
  }
  jbyte* data = env->GetByteArrayElements(result.obj(), NULL);
  jsize data_len = env->GetArrayLength(result.obj());
  if (data == NULL) {
    return false;
  }
  char* data_c = (char*) malloc(data_len + 1);
  memcpy(data_c, data, data_len);
  data_c[data_len] = 0;
  encoded->assign(data_c);
  free(data_c);
  env->ReleaseByteArrayElements(result.obj(), data, JNI_ABORT);
  return true;
}

bool DecodeCodePage(const char* codepage, const std::string& encoded,
                    int on_error, base::string16* utf16) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> codepageString =
      ConvertUTF8ToJavaString(env, codepage);
  int encoded_len = encoded.length();
  jbyteArray src = env->NewByteArray(encoded_len);
  jbyte* encoded_j = (jbyte*) malloc(encoded_len * sizeof(jbyte));
  memcpy(encoded_j, encoded.c_str(), encoded_len * sizeof(jbyte));
  env->SetByteArrayRegion(src, 0, encoded_len, encoded_j);
  ScopedJavaLocalRef<jcharArray> result = Java_IcuUtils_decodeCodePage(
      env, codepageString.obj(), src, on_error);
  if (result.obj() == NULL) {
    utf16->clear();
    return false;
  }
  jchar* data = env->GetCharArrayElements(result.obj(), NULL);
  jsize data_len = env->GetArrayLength(result.obj());
  if (data == NULL) {
    return false;
  }
  utf16->assign(data, data_len);
  env->ReleaseCharArrayElements(result.obj(), data, JNI_ABORT);
  free(encoded_j);
  return true;
}

std::string GetConfiguredLocale() {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> java_result =
      Java_IcuUtils_getConfiguredLocale(env);
  return ConvertJavaStringToUTF8(java_result);
}

void SetDefaultLocale(const std::string& locale_string) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> localeString =
      ConvertUTF8ToJavaString(env, locale_string);
  Java_IcuUtils_setDefaultLocale(env, localeString.obj());
}

base::string16 FormatTimeOfDay(long mills, bool hour24, bool keep_am_pm) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> java_result =
      Java_IcuUtils_formatTimeOfDay(
          env, mills,
          hour24 ? JNI_TRUE : JNI_FALSE,
          keep_am_pm ? JNI_TRUE : JNI_FALSE);
  return ConvertJavaStringToUTF16(java_result);
}

base::string16 FormatDate(long mills, int date_type, bool append_time_of_day) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> java_result =
      Java_IcuUtils_formatDate(env, mills, date_type,
                               append_time_of_day ? JNI_TRUE : JNI_FALSE);
  return ConvertJavaStringToUTF16(java_result);
}

bool Is24HourFormat() {
  JNIEnv* env = AttachCurrentThread();

  return JNI_TRUE == Java_IcuUtils_is24HourFormat(
      env, base::android::GetApplicationContext());
}

int CompareString16WithCollator(const std::string& locale,
                                const base::string16& lhs,
                                const base::string16& rhs) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> loc_string =
      ConvertUTF8ToJavaString(env, locale);
  ScopedJavaLocalRef<jstring> lhs_string =
      ConvertUTF16ToJavaString(env, lhs);
  ScopedJavaLocalRef<jstring> rhs_string =
      ConvertUTF16ToJavaString(env, rhs);
  return Java_IcuUtils_collateWithLocale(
      env, loc_string.obj(),lhs_string.obj(), rhs_string.obj());
}

bool IsCharInFilenameIllegal(int code_point) {
  JNIEnv* env = AttachCurrentThread();

  return JNI_TRUE == Java_IcuUtils_isCodePointInFilenameIllegal(
      env, code_point);
}

ScopedJavaLocalRef<jobject> getJavaLocale(const std::string& locale) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> localeString =
      ConvertUTF8ToJavaString(env, locale);

  return Java_IcuUtils_getLocale(env, localeString.obj());
}

base::string16 CutOffStringForEliding(
    const base::string16& input, size_t max, bool word_break) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> inputString =
      ConvertUTF16ToJavaString(env, input);
  ScopedJavaLocalRef<jstring> resultString =
      Java_IcuUtils_cutOffStringForEliding(
          env, inputString.obj(), max, word_break ? JNI_TRUE : JNI_FALSE);

  return ConvertJavaStringToUTF16(resultString);
}

UChar32 foldCase(UChar32 c) {
  return toLower(c);
}

int copy(UChar* result, int resultLength, const UChar* src, int srcLength) {
  int length = (resultLength < srcLength ? resultLength : srcLength);
  memcpy(result, src, length);
  return length;
}

int foldCase(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error) {
  return toLower(result, resultLength, src, srcLength, error);
}

int toLower(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> sourceString =
      ConvertUTF16ToJavaString(
          env, StringPiece16(src, srcLength));

  ScopedJavaLocalRef<jstring> java_result =
      Java_IcuUtils_toLowerString(env, sourceString.obj());

  string16 resultString = ConvertJavaStringToUTF16(java_result);
  *error = false;
  return copy(result, resultLength, resultString.data(), resultString.size());
}

UChar32 toLower(UChar32 c) {
  JNIEnv* env = AttachCurrentThread();
  return Java_IcuUtils_toLower(env, c);
}

UChar32 toUpper(UChar32 c) {
  JNIEnv* env = AttachCurrentThread();
  return Java_IcuUtils_toUpper(env, c);
}

int toUpper(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> sourceString =
      ConvertUTF16ToJavaString(
          env, StringPiece16(src, srcLength));

  ScopedJavaLocalRef<jstring> java_result =
      Java_IcuUtils_toUpperString(env, sourceString.obj());

  string16 resultString = ConvertJavaStringToUTF16(java_result);
  *error = false;
  return copy(result, resultLength, resultString.data(), resultString.size());
}

UChar32 toTitleCase(UChar32 c) {
  JNIEnv* env = AttachCurrentThread();
  return Java_IcuUtils_toTitleCase(env, c);
}

bool isArabicChar(UChar32 c) {
  JNIEnv* env = AttachCurrentThread();
  return Java_IcuUtils_isArabicChar(env, c);
}

bool isAlphanumeric(UChar32 c) {
  JNIEnv* env = AttachCurrentThread();
  return Java_IcuUtils_isAlphanumeric(env, c);
}

bool isSeparatorSpace(UChar32 c) {
  JNIEnv* env = AttachCurrentThread();
  return Java_IcuUtils_isSeparatorSpace(env, c);
}

bool isPrintableChar(UChar32 c) {
  JNIEnv* env = AttachCurrentThread();
  return Java_IcuUtils_isPrintableChar(env, c);
}

bool isPunct(UChar32 c) {
  JNIEnv* env = AttachCurrentThread();
  return Java_IcuUtils_isPunct(env, c);
}

bool hasLineBreakingPropertyComplexContext(UChar32 c) {
  // Don't support this now. Need UCharacter(ICU4j).
  // Impact line breaks in THAI,LAO,KHmer.
  return false;
}

UChar32 mirroredChar(UChar32 c) {
  JNIEnv* env = AttachCurrentThread();
  return Java_IcuUtils_mirroredChar(env, c);
}

UCharCategory category(UChar32 c) {
  JNIEnv* env = AttachCurrentThread();
  jint result = Java_IcuUtils_category(env, c);
  return static_cast<UCharCategory>(result);
}

UCharDirection direction(UChar32 c) {
  JNIEnv* env = AttachCurrentThread();
  jint result = Java_IcuUtils_direction(env, c);
  return static_cast<UCharDirection>(result);
}

bool isLower(UChar32 c) {
  JNIEnv* env = AttachCurrentThread();
  return Java_IcuUtils_isLower(env, c);
}

bool isUpper(UChar32 c) {
  JNIEnv* env = AttachCurrentThread();
  return Java_IcuUtils_isUpper(env, c);
}

uint8_t combiningClass(UChar32 c) {
  // FIXME(Xingnan): functinality missing
  return 0;
}

int32_t decompositionType(UChar32 c) {
  // FIXME(wang16): functionality missing.
  return 0;
}

int umemcasecmp(const UChar* a, const UChar* b, int len) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> aString =
      ConvertUTF16ToJavaString(
          env, StringPiece16(a, len));
  ScopedJavaLocalRef<jstring> bString =
      ConvertUTF16ToJavaString(
          env, StringPiece16(b, len));

  return Java_IcuUtils_compareToIgnoreCase(env, aString.obj(), bString.obj());
}

int32_t normalize(const UChar *source, int32_t sourceLength, int32_t options,
                  UChar *result, int32_t resultLength,bool *error) {
  // FIXME(Xingnan): mapping options if needed, now only the default option
  // UNORM_NFC is used.
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> sourceString =
      ConvertUTF16ToJavaString(
          env, StringPiece16(source, sourceLength));

  ScopedJavaLocalRef<jstring> java_result =
      Java_IcuUtils_normalize(env, sourceString.obj());

  string16 resultString = ConvertJavaStringToUTF16(java_result);
  *error = false;
  return copy(result, resultLength, resultString.data(), resultString.size());
}

JCollator::JCollator(const char* locale)
    : m_locale(locale) { }

int JCollator::collate(const UChar* source1, size_t source1Length,
                       const UChar* source2, size_t source2Length) const {
  return base::icu_utils::collate(m_locale, source1, source1Length,
                                  source2, source2Length);
}


int collate(const std::string& locale, const UChar* source1, size_t source1Length,
            const UChar* source2, size_t source2Length) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> locString =
      ConvertUTF8ToJavaString(env, locale);
  ScopedJavaLocalRef<jstring> string1 =
      ConvertUTF16ToJavaString(
          env, StringPiece16(source1, source1Length));
  ScopedJavaLocalRef<jstring> string2 =
      ConvertUTF16ToJavaString(
          env, StringPiece16(source2, source2Length));

  return Java_IcuUtils_collateWithLocale(
      env, locString.obj(), string1.obj(), string2.obj());
}

}  // namespace icu_utils

bool RegisterIcuUtilsJni(JNIEnv* env) {
  return android::RegisterNativesImpl(env);
}

}  // namespace base

