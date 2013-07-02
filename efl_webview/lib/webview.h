// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_WINDOW_H_
#define XWALK_WINDOW_H_

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "content/common/content_export.h"
#include "googleurl/src/gurl.h"

#include <Evas.h>

namespace xwalk {

class CONTENT_EXPORT WebView {
 public:
  CONTENT_EXPORT static WebView* Create(Evas_Object* root_window);
  CONTENT_EXPORT static void CommandLineInit(int argc, char** argv);

  ~WebView();

  CONTENT_EXPORT Evas_Object* EvasObject();

  CONTENT_EXPORT void Forward();
  CONTENT_EXPORT void Back();
  CONTENT_EXPORT void LoadURL(const GURL&);

 private:
  explicit WebView(Evas_Object*);

  struct Private;
  scoped_ptr<Private> private_;

  DISALLOW_COPY_AND_ASSIGN(WebView);
};

}  // namespace xwalk

#endif  // XWALK_WINDOW_H_
