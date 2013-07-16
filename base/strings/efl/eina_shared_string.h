// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EINA_SHARED_STRING_H_
#define EINA_SHARED_STRING_H_

#include "base/base_export.h"

#include <Eina.h>

#include <string>

namespace base {

class BASE_EXPORT EinaSharedString {
public:
  EinaSharedString() : string_(0) { }
  EinaSharedString(const EinaSharedString& other);
  EinaSharedString(const char* str);
  explicit EinaSharedString(const std::string&);

  ~EinaSharedString();

  Eina_Stringshare* LeakString();

  EinaSharedString& operator=(const EinaSharedString& other);
  EinaSharedString& operator=(const char* str);

  bool operator==(const EinaSharedString& other) const { return string_ == other.string_; }
  bool operator!=(const EinaSharedString& other) const { return !(*this == other); }

  bool operator==(const char* str) const;
  bool operator!=(const char* str) const { return !(*this == str); }

  operator const char* () const { return string_; }

  bool IsNull() const { return !string_; }

  size_t Length() const { return string_ ? static_cast<size_t>(eina_stringshare_strlen(string_)) : 0; }

  static EinaSharedString Adopt(Eina_Stringshare*);

private:
  const char* string_;
};

} // namespace base

using base::EinaSharedString;

#endif // EINA_SHARED_STRING_H_
