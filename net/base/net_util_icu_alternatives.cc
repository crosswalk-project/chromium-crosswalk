// Copyright (c) 2014 Intel Corp. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/net_util.h"

#include <map>
#include <vector>

#include "base/i18n/time_formatting.h"
#include "base/json/string_escape.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/stl_util.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_offset_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "url/gurl.h"

using base::Time;

namespace net {

namespace {

typedef std::vector<size_t> Offsets;

// If |component| is valid, its begin is incremented by |delta|.
void AdjustComponent(int delta, url::Component* component) {
  if (!component->is_valid())
    return;

  DCHECK(delta >= 0 || component->begin >= -delta);
  component->begin += delta;
}

// Adjusts all the components of |parsed| by |delta|, except for the scheme.
void AdjustAllComponentsButScheme(int delta, url::Parsed* parsed) {
  AdjustComponent(delta, &(parsed->username));
  AdjustComponent(delta, &(parsed->password));
  AdjustComponent(delta, &(parsed->host));
  AdjustComponent(delta, &(parsed->port));
  AdjustComponent(delta, &(parsed->path));
  AdjustComponent(delta, &(parsed->query));
  AdjustComponent(delta, &(parsed->ref));
}

void AppendFormattedComponent(const std::string& spec,
                              const url::Component& original_component,
                              base::string16* output,
                              url::Component* output_component,
                              base::OffsetAdjuster::Adjustments* adjustments) {
  DCHECK(output);
  if (original_component.is_nonempty()) {
    size_t original_component_begin =
        static_cast<size_t>(original_component.begin);
    size_t output_component_begin = output->length();
    std::string component_str(spec, original_component_begin,
                              static_cast<size_t>(original_component.len));

    // Transform |component_str| and modify |adjustments| appropriately.
    base::OffsetAdjuster::Adjustments component_transform_adjustments;
    output->append(base::UTF8ToUTF16WithAdjustments(
        component_str, &component_transform_adjustments));

    // Shift all the adjustments made for this component so the offsets are
    // valid for the original string and add them to |adjustments|.
    for (base::OffsetAdjuster::Adjustments::iterator comp_iter =
         component_transform_adjustments.begin();
         comp_iter != component_transform_adjustments.end(); ++comp_iter)
      comp_iter->original_offset += original_component_begin;
    if (adjustments) {
      adjustments->insert(adjustments->end(),
                          component_transform_adjustments.begin(),
                          component_transform_adjustments.end());
    }

    // Set positions of the parsed component.
    if (output_component) {
      output_component->begin = static_cast<int>(output_component_begin);
      output_component->len =
          static_cast<int>(output->length() - output_component_begin);
    }
  } else if (output_component) {
    output_component->reset();
  }
}

}  // namespace

const FormatUrlType kFormatUrlOmitNothing                     = 0;
const FormatUrlType kFormatUrlOmitUsernamePassword            = 1 << 0;
const FormatUrlType kFormatUrlOmitHTTP                        = 1 << 1;
const FormatUrlType kFormatUrlOmitTrailingSlashOnBareHostname = 1 << 2;
const FormatUrlType kFormatUrlOmitAll = kFormatUrlOmitUsernamePassword |
    kFormatUrlOmitHTTP | kFormatUrlOmitTrailingSlashOnBareHostname;

base::string16 IDNToUnicode(const std::string& host,
                            const std::string& languages) {
  return base::ASCIIToUTF16(base::StringPiece(host));
}

std::string GetDirectoryListingEntry(const base::string16& name,
                                     const std::string& raw_bytes,
                                     bool is_dir,
                                     int64 size,
                                     Time modified) {
  std::string result;
  result.append("<script>addRow(");
  base::EscapeJSONString(name, true, &result);
  result.append(",");
  if (raw_bytes.empty()) {
    base::EscapeJSONString(EscapePath(base::UTF16ToUTF8(name)), true, &result);
  } else {
    base::EscapeJSONString(EscapePath(raw_bytes), true, &result);
  }
  if (is_dir) {
    result.append(",1,");
  } else {
    result.append(",0,");
  }

  // Negative size means unknown or not applicable (e.g. directory).
  base::string16 size_string;
  if (size >= 0)
    size_string = FormatBytesUnlocalized(size);
  base::EscapeJSONString(size_string, true, &result);

  result.append(",");

  base::string16 modified_str;
  // |modified| can be NULL in FTP listings.
  if (!modified.is_null()) {
    modified_str = base::TimeFormatShortDateAndTime(modified);
  }
  base::EscapeJSONString(modified_str, true, &result);

  result.append(");</script>\n");

  return result;
}

void AppendFormattedHost(const GURL& url,
                         const std::string& languages,
                         base::string16* output) {
  output->append(base::ASCIIToUTF16(base::StringPiece(url.host())));
}

base::string16 FormatUrlWithOffsets(
    const GURL& url,
    const std::string& languages,
    FormatUrlTypes format_types,
    UnescapeRule::Type unescape_rules,
    url::Parsed* new_parsed,
    size_t* prefix_end,
    std::vector<size_t>* offsets_for_adjustment) {
  base::OffsetAdjuster::Adjustments adjustments;
  const base::string16& format_url_return_value =
      FormatUrlWithAdjustments(url, languages, format_types, unescape_rules,
                               new_parsed, prefix_end, &adjustments);
  base::OffsetAdjuster::AdjustOffsets(adjustments, offsets_for_adjustment);
  if (offsets_for_adjustment) {
    std::for_each(
        offsets_for_adjustment->begin(),
        offsets_for_adjustment->end(),
        base::LimitOffset<std::string>(format_url_return_value.length()));
  }
  return format_url_return_value;
}

base::string16 FormatUrlWithAdjustments(
    const GURL& url,
    const std::string& languages,
    FormatUrlTypes format_types,
    UnescapeRule::Type unescape_rules,
    url::Parsed* new_parsed,
    size_t* prefix_end,
    base::OffsetAdjuster::Adjustments* adjustments) {
  DCHECK(adjustments != NULL);
  adjustments->clear();
  url::Parsed parsed_temp;
  if (!new_parsed)
    new_parsed = &parsed_temp;
  else
    *new_parsed = url::Parsed();

  // Special handling for view-source:.  Don't use content::kViewSourceScheme
  // because this library shouldn't depend on chrome.
  const char* const kViewSource = "view-source";
  // Reject "view-source:view-source:..." to avoid deep recursion.
  const char* const kViewSourceTwice = "view-source:view-source:";
  if (url.SchemeIs(kViewSource) &&
      !StartsWithASCII(url.possibly_invalid_spec(), kViewSourceTwice, false)) {
    NOTIMPLEMENTED();
    return base::ASCIIToUTF16(base::StringPiece(url.GetContent()));
  }

  // We handle both valid and invalid URLs (this will give us the spec
  // regardless of validity).
  const std::string& spec = url.possibly_invalid_spec();
  const url::Parsed& parsed = url.parsed_for_possibly_invalid_spec();

  // Scheme & separators.  These are ASCII.
  base::string16 url_string;
  url_string.insert(
      url_string.end(), spec.begin(),
      spec.begin() + parsed.CountCharactersBefore(url::Parsed::USERNAME, true));
  const char kHTTP[] = "http://";
  const char kFTP[] = "ftp.";
  // URLFixerUpper::FixupURL() treats "ftp.foo.com" as ftp://ftp.foo.com.  This
  // means that if we trim "http://" off a URL whose host starts with "ftp." and
  // the user inputs this into any field subject to fixup (which is basically
  // all input fields), the meaning would be changed.  (In fact, often the
  // formatted URL is directly pre-filled into an input field.)  For this reason
  // we avoid stripping "http://" in this case.
  bool omit_http = (format_types & kFormatUrlOmitHTTP) &&
      EqualsASCII(url_string, kHTTP) &&
      !StartsWithASCII(url.host(), kFTP, true);
  new_parsed->scheme = parsed.scheme;

  // Username & password.
  if ((format_types & kFormatUrlOmitUsernamePassword) != 0) {
    // Remove the username and password fields. We don't want to display those
    // to the user since they can be used for attacks,
    // e.g. "http://google.com:search@evil.ru/"
    new_parsed->username.reset();
    new_parsed->password.reset();
    // Update the adjustments based on removed username and/or password.
    if (parsed.username.is_nonempty() || parsed.password.is_nonempty()) {
      if (parsed.username.is_nonempty() && parsed.password.is_nonempty()) {
        // The seeming off-by-two is to account for the ':' after the username
        // and '@' after the password.
        adjustments->push_back(base::OffsetAdjuster::Adjustment(
            static_cast<size_t>(parsed.username.begin),
            static_cast<size_t>(parsed.username.len + parsed.password.len + 2),
            0));
      } else {
        const url::Component* nonempty_component =
            parsed.username.is_nonempty() ? &parsed.username : &parsed.password;
        // The seeming off-by-one is to account for the '@' after the
        // username/password.
        adjustments->push_back(base::OffsetAdjuster::Adjustment(
            static_cast<size_t>(nonempty_component->begin),
            static_cast<size_t>(nonempty_component->len + 1),
            0));
      }
    }
  } else {
    AppendFormattedComponent(spec, parsed.username,
                             &url_string, &new_parsed->username, adjustments);
    if (parsed.password.is_valid())
      url_string.push_back(':');
    AppendFormattedComponent(spec, parsed.password,
                             &url_string, &new_parsed->password, adjustments);
    if (parsed.username.is_valid() || parsed.password.is_valid())
      url_string.push_back('@');
  }
  if (prefix_end)
    *prefix_end = static_cast<size_t>(url_string.length());

  // Host.
  AppendFormattedComponent(spec, parsed.host,
                           &url_string, &new_parsed->host, adjustments);

  // Port.
  if (parsed.port.is_nonempty()) {
    url_string.push_back(':');
    new_parsed->port.begin = url_string.length();
    url_string.insert(url_string.end(),
                      spec.begin() + parsed.port.begin,
                      spec.begin() + parsed.port.end());
    new_parsed->port.len = url_string.length() - new_parsed->port.begin;
  } else {
    new_parsed->port.reset();
  }

  // Path & query.  Both get the same general unescape & convert treatment.
  if (!(format_types & kFormatUrlOmitTrailingSlashOnBareHostname) ||
      !CanStripTrailingSlash(url)) {
    AppendFormattedComponent(spec, parsed.path,
                             &url_string, &new_parsed->path, adjustments);
  } else {
    if (parsed.path.len > 0) {
      adjustments->push_back(base::OffsetAdjuster::Adjustment(
          parsed.path.begin, parsed.path.len, 0));
    }
  }
  if (parsed.query.is_valid())
    url_string.push_back('?');
  AppendFormattedComponent(spec, parsed.query,
                           &url_string, &new_parsed->query, adjustments);

  // Ref.  This is valid, unescaped UTF-8, so we can just convert.
  if (parsed.ref.is_valid())
    url_string.push_back('#');
  AppendFormattedComponent(spec, parsed.ref,
                           &url_string, &new_parsed->ref, adjustments);

  // If we need to strip out http do it after the fact.
  if (omit_http && StartsWith(url_string, base::ASCIIToUTF16(kHTTP), true)) {
    const size_t kHTTPSize = arraysize(kHTTP) - 1;
    url_string = url_string.substr(kHTTPSize);
    // Because offsets in the |adjustments| are already calculated with respect
    // to the string with the http:// prefix in it, those offsets remain correct
    // after stripping the prefix.  The only thing necessary is to add an
    // adjustment to reflect the stripped prefix.
    adjustments->insert(adjustments->begin(),
        base::OffsetAdjuster::Adjustment(0, kHTTPSize, 0));

    if (prefix_end)
      *prefix_end -= kHTTPSize;

    // Adjust new_parsed.
    DCHECK(new_parsed->scheme.is_valid());
    int delta = -(new_parsed->scheme.len + 3);  // +3 for ://.
    new_parsed->scheme.reset();
    AdjustAllComponentsButScheme(delta, new_parsed);
  }

  return url_string;
}

base::string16 FormatUrl(const GURL& url,
                         const std::string& languages,
                         FormatUrlTypes format_types,
                         UnescapeRule::Type unescape_rules,
                         url::Parsed* new_parsed,
                         size_t* prefix_end,
                         size_t* offset_for_adjustment) {
  Offsets offsets;
  if (offset_for_adjustment)
    offsets.push_back(*offset_for_adjustment);
  base::string16 result = FormatUrlWithOffsets(url, languages, format_types,
      unescape_rules, new_parsed, prefix_end, &offsets);
  if (offset_for_adjustment)
    *offset_for_adjustment = offsets[0];
  return result;
}

}  // namespace net
