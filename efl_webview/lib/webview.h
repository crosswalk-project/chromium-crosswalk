// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EFL_WEBVIEW_LIBWINDOW_H_
#define EFL_WEBVIEW_LIBWINDOW_H_

#include <Evas.h>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "googleurl/src/gurl.h"

namespace xwalk {

class WebView {
 public:
  static WebView* Create(Evas_Object* root_window);
  static void CommandLineInit(int argc, char** argv);

  ~WebView();

  Evas_Object* EvasObject();

  bool CanGoBack() const;
  bool CanGoForward() const;
  void GoForward();
  void GoBack();
  void Reload();
  void LoadURL(const GURL&);

 private:
  explicit WebView(Evas_Object*);

  struct Private;
  scoped_ptr<Private> private_;

  DISALLOW_COPY_AND_ASSIGN(WebView);
};

WebView* ToWebView(Evas_Object*);

}  // namespace xwalk

#endif  // EFL_WEBVIEW_LIBWINDOW_H_
