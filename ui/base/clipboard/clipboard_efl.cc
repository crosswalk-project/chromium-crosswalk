// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/clipboard/clipboard.h"

#include <X11/extensions/Xfixes.h>
#include <X11/Xatom.h>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "base/basictypes.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/utf_string_conversions.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/clipboard/custom_data_helper.h"
#include "ui/base/x/x11_util.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/size.h"

namespace ui {

namespace {

const char kSourceTagType[] = "org.chromium.source-tag";
const char kMimeTypeBitmap[] = "image/bmp";
const char kMimeTypeMozillaURL[] = "text/x-moz-url";
const char kMimeTypePepperCustomData[] = "chromium/x-pepper-custom-data";
const char kMimeTypeWebkitSmartPaste[] = "chromium/x-webkit-paste";

}  // namespace
Clipboard::FormatType::FormatType() {
}

Clipboard::FormatType::~FormatType() {
}

std::string Clipboard::FormatType::Serialize() const {
  return "";
}

Clipboard::FormatType Clipboard::FormatType::Deserialize(
    const std::string&) {
  static FormatType type;
  return type;
}

Clipboard::Clipboard() {
}

Clipboard::~Clipboard() {

}

void Clipboard::WriteObjectsImpl(Buffer buffer,
                                 const ObjectMap& objects,
                                 SourceTag tag) {
}

void Clipboard::WriteText(const char* text_data, size_t text_len) {
}

void Clipboard::WriteHTML(const char* markup_data,
                          size_t markup_len,
                          const char* url_data,
                          size_t url_len) {
}

void Clipboard::WriteRTF(const char* rtf_data, size_t data_len) {
}

// Write an extra flavor that signifies WebKit was the last to modify the
// pasteboard. This flavor has no data.
void Clipboard::WriteWebSmartPaste() {
}

void Clipboard::WriteBitmap(const char* pixel_data, const char* size_data) {
}

void Clipboard::WriteBookmark(const char* title_data, size_t title_len,
                              const char* url_data, size_t url_len) {
}

void Clipboard::WriteData(const FormatType& format,
                          const char* data_data,
                          size_t data_len) {
}

void Clipboard::WriteSourceTag(SourceTag tag) {
}

// We do not use gtk_clipboard_wait_is_target_available because of
// a bug with the gtk clipboard. It caches the available targets
// and does not always refresh the cache when it is appropriate.
bool Clipboard::IsFormatAvailable(const Clipboard::FormatType& format,
                                  Clipboard::Buffer buffer) const {
  return false;
}

void Clipboard::Clear(Clipboard::Buffer buffer) {
}

void Clipboard::ReadAvailableTypes(Clipboard::Buffer buffer,
                                   std::vector<string16>* types,
                                   bool* contains_filenames) const {
}


void Clipboard::ReadText(Clipboard::Buffer buffer, string16* result) const {
}

void Clipboard::ReadAsciiText(Clipboard::Buffer buffer,
                              std::string* result) const {
}

// TODO(estade): handle different charsets.
// TODO(port): set *src_url.
void Clipboard::ReadHTML(Clipboard::Buffer buffer, string16* markup,
                         std::string* src_url, uint32* fragment_start,
                         uint32* fragment_end) const {
}

void Clipboard::ReadRTF(Buffer buffer, std::string* result) const {
}

SkBitmap Clipboard::ReadImage(Buffer buffer) const {
    return SkBitmap();
}

void Clipboard::ReadCustomData(Buffer buffer,
                               const string16& type,
                               string16* result) const {
}

void Clipboard::ReadBookmark(string16* title, std::string* url) const {
  // TODO(estade): implement this.
  NOTIMPLEMENTED();
}

void Clipboard::ReadData(const FormatType& format, std::string* result) const {
}

SourceTag Clipboard::ReadSourceTag(Buffer buffer) const {
  std::string result;
  return Binary2SourceTag(result);
}

uint64 Clipboard::GetSequenceNumber(Buffer buffer) {
  return 0;
}

//static
Clipboard::FormatType Clipboard::GetFormatType(
    const std::string& format_string) {
  static FormatType type;
  return type;
}

// static
const Clipboard::FormatType& Clipboard::GetPlainTextFormatType() {
  static FormatType type;
  return type;
}

// static
const Clipboard::FormatType& Clipboard::GetPlainTextWFormatType() {
  return GetPlainTextFormatType();
}

// static
const Clipboard::FormatType& Clipboard::GetUrlFormatType() {
  return GetPlainTextFormatType();
}

// static
const Clipboard::FormatType& Clipboard::GetUrlWFormatType() {
  return GetPlainTextWFormatType();
}

// static
const Clipboard::FormatType& Clipboard::GetHtmlFormatType() {
  return GetPlainTextWFormatType();
}

// static
const Clipboard::FormatType& Clipboard::GetRtfFormatType() {
  return GetPlainTextWFormatType();
}

// static
const Clipboard::FormatType& Clipboard::GetBitmapFormatType() {
  return GetPlainTextWFormatType();
}

// static
const Clipboard::FormatType& Clipboard::GetWebKitSmartPasteFormatType() {
  return GetPlainTextWFormatType();
}

// static
const Clipboard::FormatType& Clipboard::GetWebCustomDataFormatType() {
  return GetPlainTextWFormatType();
}

// static
const Clipboard::FormatType& Clipboard::GetPepperCustomDataFormatType() {
  return GetPlainTextWFormatType();
}

// static
const Clipboard::FormatType& Clipboard::GetSourceTagFormatType() {
  return GetPlainTextWFormatType();
}

}  // namespace ui
