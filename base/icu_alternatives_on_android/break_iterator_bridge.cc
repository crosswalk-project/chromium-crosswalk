// Copyright 2014 Intel Corp. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/icu_alternatives_on_android/break_iterator_bridge.h"

#include "base/android/jni_string.h"
#include "base/strings/latin1_string_conversions.h"
#include "base/strings/string_piece.h"
#include "jni/BreakIteratorBridge_jni.h"

using namespace base::android;

namespace base {

BreakIteratorBridge::BreakIteratorBridge(JNIEnv* env, jobject obj) {
  j_iterator_.Reset(env, obj);
}

BreakIteratorBridge::~BreakIteratorBridge() {
  j_iterator_.Reset();
}

BreakIteratorBridge* BreakIteratorBridge::createLineInstance(const char* locale) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> localeString =
      ConvertUTF8ToJavaString(
          env, base::StringPiece(locale));

  return new BreakIteratorBridge(
      env,Java_BreakIteratorBridge_createLineInstance(
          env, localeString.obj()).obj());
}

BreakIteratorBridge* BreakIteratorBridge::createWordInstance(const char* locale) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> localeString =
      ConvertUTF8ToJavaString(
          env, base::StringPiece(locale));

  return new BreakIteratorBridge(
      env, Java_BreakIteratorBridge_createWordInstance(
          env, localeString.obj()).obj());
}

BreakIteratorBridge* BreakIteratorBridge::createCharacterInstance(const char* locale) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> localeString =
      ConvertUTF8ToJavaString(
          env, base::StringPiece(locale));

  return new BreakIteratorBridge(
      env, Java_BreakIteratorBridge_createCharacterInstance(
          env, localeString.obj()).obj());
}

BreakIteratorBridge* BreakIteratorBridge::createSentenceInstance(const char* locale) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> localeString =
      ConvertUTF8ToJavaString(
          env, base::StringPiece(locale));

  return new BreakIteratorBridge(
      env, Java_BreakIteratorBridge_createSentenceInstance(
          env, localeString.obj()).obj());
}

void BreakIteratorBridge::setText(const unsigned char * text, int length) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> java_text =
      ConvertUTF16ToJavaString(
          env, base::Latin1OrUTF16ToUTF16(length, text, NULL));

  Java_BreakIteratorBridge_setText(env, j_iterator_.obj(), java_text.obj());
}

void BreakIteratorBridge::setText(const UChar * text, int length) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> java_text =
      ConvertUTF16ToJavaString(
          env, StringPiece16(text, length));

  Java_BreakIteratorBridge_setText(env, j_iterator_.obj(), java_text.obj());
}

int32_t BreakIteratorBridge::first(void) {
  JNIEnv* env = AttachCurrentThread();
  return Java_BreakIteratorBridge_first(env, j_iterator_.obj());
}

int32_t BreakIteratorBridge::last(void) {
  JNIEnv* env = AttachCurrentThread();
  return Java_BreakIteratorBridge_last(env, j_iterator_.obj());
}

int32_t BreakIteratorBridge::previous(void) {
  JNIEnv* env = AttachCurrentThread();
  return Java_BreakIteratorBridge_previous(env, j_iterator_.obj());
}

int32_t BreakIteratorBridge::next(void) {
  JNIEnv* env = AttachCurrentThread();
  return Java_BreakIteratorBridge_next(env, j_iterator_.obj());
}

int32_t BreakIteratorBridge::current(void) {
  JNIEnv* env = AttachCurrentThread();
  return Java_BreakIteratorBridge_current(env, j_iterator_.obj());
}

int32_t BreakIteratorBridge::following(int32_t offset) {
  JNIEnv* env = AttachCurrentThread();
  return Java_BreakIteratorBridge_following(env, j_iterator_.obj(), offset);
}

int32_t BreakIteratorBridge::preceding(int32_t offset) {
  JNIEnv* env = AttachCurrentThread();
  return Java_BreakIteratorBridge_preceding(env, j_iterator_.obj(), offset);
}

bool BreakIteratorBridge::isBoundary(int32_t offset) {
  JNIEnv* env = AttachCurrentThread();
  return Java_BreakIteratorBridge_isBoundary(env, j_iterator_.obj(), offset);
}

bool BreakIteratorBridge::isWord() {
  JNIEnv* env = AttachCurrentThread();
  return Java_BreakIteratorBridge_isWord(env, j_iterator_.obj());
}

bool RegisterBreakIteratorBridgeJni(JNIEnv* env) {
  return android::RegisterNativesImpl(env);
}

}  // namespace base
