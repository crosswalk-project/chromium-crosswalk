// Copyright (c) 2014 Intel Corp. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/i18n/time_formatting.h"

#include "base/icu_alternatives_on_android/icu_utils.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"

namespace base {

string16 TimeFormatTimeOfDay(const Time& time) {
  return base::icu_utils::FormatTimeOfDay(time.ToJavaTime(), false, true);
}

string16 TimeFormatTimeOfDayWithHourClockType(const Time& time,
                                              HourClockType type,
                                              AmPmClockType ampm) {
  return base::icu_utils::FormatTimeOfDay(time.ToJavaTime(),
                                          type == k24HourClock,
                                          ampm == kKeepAmPm);
}

string16 TimeFormatShortDate(const Time& time) {
  return base::icu_utils::FormatDate(time.ToJavaTime(), 0, false);
}

string16 TimeFormatShortDateNumeric(const Time& time) {
  return base::icu_utils::FormatDate(time.ToJavaTime(), 1, false);
}

string16 TimeFormatShortDateAndTime(const Time& time) {
  return base::icu_utils::FormatDate(time.ToJavaTime(), 0, true);
}

string16 TimeFormatFriendlyDateAndTime(const Time& time) {
  return base::icu_utils::FormatDate(time.ToJavaTime(), 2, true);
}

string16 TimeFormatFriendlyDate(const Time& time) {
  return base::icu_utils::FormatDate(time.ToJavaTime(), 2, false);
}

HourClockType GetHourClockType() {
  if (base::icu_utils::Is24HourFormat())
    return k24HourClock;

  return k12HourClock;
}

}  // namespace base
