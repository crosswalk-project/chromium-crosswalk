// Copyright 2014 Intel Corp. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_ICU_ALTERNATIVES_ON_ANDROID_BREAK_ITERATOR_BRIDGE_H_
#define BASE_ICU_ALTERNATIVES_ON_ANDROID_BREAK_ITERATOR_BRIDGE_H_

#include <jni.h>
#include <string.h>
#include <unicode/uchar.h>

#include "base/android/jni_android.h"

namespace base {

class BreakIteratorBridge {
 public:
  virtual ~BreakIteratorBridge();
  static BreakIteratorBridge* createLineInstance(const char* locale);
  static BreakIteratorBridge* createWordInstance(const char* locale);
  static BreakIteratorBridge* createCharacterInstance(const char* locale);
  static BreakIteratorBridge* createSentenceInstance(const char* locale);
  void  setText(const unsigned char * text, int length);
  void  setText(const UChar * text, int length);
  int32_t first(void);
  int32_t last(void);
  int32_t previous(void);
  int32_t next(void);
  int32_t current(void);
  int32_t following(int32_t offset);
  int32_t preceding(int32_t offset);
  bool isBoundary(int32_t offset);
  bool isWord();

 private:
  BreakIteratorBridge(JNIEnv* env, jobject obj);
  base::android::ScopedJavaLocalRef<jobject> j_iterator_;
};

/* In case we need it
class JRuleBasedBreakIterator: public BreakIteratorBridge {
 public:
  JRuleBasedBreakIterator();
  int getRuleStatus();
};
*/

bool RegisterBreakIteratorBridgeJni(JNIEnv* env);
}  // namespace base

#endif  //BASE_ICU_ALTERNATIVES_ON_ANDROID_BREAK_ITERATOR_BRIDGE_H_
