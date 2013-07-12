// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/efl/eina_shared_string.h"

EinaSharedString::EinaSharedString(const EinaSharedString& other)
  : string_(eina_stringshare_ref(other.string_)) {
}

EinaSharedString::EinaSharedString(const char* str)
  : string_(eina_stringshare_add(str)) {
}

EinaSharedString::~EinaSharedString() {
  if (string_)
    eina_stringshare_del(string_);
}

EinaSharedString& EinaSharedString::operator=(const EinaSharedString& other) {
  if (this != &other) {
    if (string_)
      eina_stringshare_del(string_);
    string_ = eina_stringshare_ref(other.string_);
  }
  return *this;
}

EinaSharedString& EinaSharedString::operator=(const char* str) {
  eina_stringshare_replace(&string_, str);
  return *this;
}

bool EinaSharedString::operator==(const char* str) const {
  return (!str || !string_) ? (str == string_) : !strcmp(string_, str);
}

EinaSharedString EinaSharedString::Adopt(Eina_Stringshare* string) {
  EinaSharedString sharedString;
  sharedString.string_ = static_cast<const char*>(string);
  return sharedString;
}

Eina_Stringshare* EinaSharedString::LeakString() {
  Eina_Stringshare* sharedString = string_;
  string_ = 0;

  return sharedString;
}
