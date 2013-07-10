// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EFL_WEBVIEW_LIBWINDOW_H_
#define EFL_WEBVIEW_LIBWINDOW_H_

#include <Evas.h>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "efl_webview/lib/xwalk_export.h"
#include "googleurl/src/gurl.h"

namespace xwalk {

class XWALK_EXPORT WebView {
 public:
  XWALK_EXPORT static WebView* Create(Evas_Object* root_window);
  XWALK_EXPORT static void CommandLineInit(int argc, char** argv);

  ~WebView();

  XWALK_EXPORT Evas_Object* EvasObject();

  XWALK_EXPORT bool CanGoBack() const;
  XWALK_EXPORT bool CanGoForward() const;
  XWALK_EXPORT void GoForward();
  XWALK_EXPORT void GoBack();
  XWALK_EXPORT void Reload();
  XWALK_EXPORT void LoadURL(const GURL&);

 private:
  explicit WebView(Evas_Object*);

  struct Private;
  scoped_ptr<Private> private_;

  DISALLOW_COPY_AND_ASSIGN(WebView);
};

XWALK_EXPORT WebView* ToWebView(Evas_Object*);

}  // namespace xwalk

#endif  // EFL_WEBVIEW_LIBWINDOW_H_
