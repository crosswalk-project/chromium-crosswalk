// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "base/values.h"
#include "printing/metafile.h"
#include "printing/print_dialog_gtk_interface.h"
#include "printing/print_job_constants.h"
#include "printing/units.h"

namespace printing {

// static
PrintingContext* PrintingContext::Create(const std::string& app_locale) {
  return 0;
}

}  // namespace printing

