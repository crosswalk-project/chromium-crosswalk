// Copyright (c) 2014 Intel Corp. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/i18n/break_iterator.h"

#include "base/logging.h"

namespace base {
namespace i18n {

const size_t npos = -1;

BreakIterator::BreakIterator(const string16& str, BreakType break_type)
    : iter_(NULL),
      string_(str),
      break_type_(break_type),
      prev_(npos),
      pos_(0) {
}

BreakIterator::~BreakIterator() {
  if (iter_)
    delete static_cast<base::BreakIteratorBridge*>(iter_);
}

bool BreakIterator::Init() {
  switch (break_type_) {
    case BREAK_CHARACTER:
      iter_ = base::BreakIteratorBridge::createCharacterInstance("");
      break;
    case BREAK_WORD:
      iter_ = base::BreakIteratorBridge::createWordInstance("");
      break;
    case BREAK_LINE:
      iter_ = base::BreakIteratorBridge::createSentenceInstance("");
      break;
    case BREAK_NEWLINE:
      iter_ = base::BreakIteratorBridge::createLineInstance("");
      break;
    default:
      NOTREACHED() << "invalid break_type_";
      return false;
  }
  // Move the iterator to the beginning of the string.
  (static_cast<base::BreakIteratorBridge*>(iter_))->first();
  return true;
}

bool BreakIterator::Advance() {
  int32_t pos;
  prev_ = pos_;
  pos = (static_cast<base::BreakIteratorBridge*>(iter_))->next();
  // Java BreakIterator.DONE = -1
  if (pos == -1) {
    pos_ = npos;
    return false;
  }
  pos_ = static_cast<size_t>(pos);
  return true;
}

bool BreakIterator::IsWord() const {
  if (break_type_ != BREAK_WORD)
    return false;

  return (static_cast<BreakIteratorBridge*>(iter_))->isWord();
}

bool BreakIterator::IsEndOfWord(size_t position) const {
  if (break_type_ != BREAK_WORD)
    return false;

  base::BreakIteratorBridge* iter =
      static_cast<base::BreakIteratorBridge*>(iter_);
  bool boundary = iter->isBoundary(static_cast<int32_t>(position));
  return (boundary && IsWord());
}

bool BreakIterator::IsStartOfWord(size_t position) const {
  if (break_type_ != BREAK_WORD)
    return false;

  base::BreakIteratorBridge* iter =
      static_cast<base::BreakIteratorBridge*>(iter_);
  bool boundary = iter->isBoundary(static_cast<int32_t>(position));
  if (!boundary)
    return false;
  iter->next();
  return IsWord();
}

string16 BreakIterator::GetString() const {
  DCHECK(prev_ != npos && pos_ != npos);
  return string_.substr(prev_, pos_ - prev_);
}

}  // namespace i18n
}  // namespace base
