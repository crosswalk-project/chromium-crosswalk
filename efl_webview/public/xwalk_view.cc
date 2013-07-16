// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "efl_webview/public/xwalk_view.h"
#include "efl_webview/lib/webview.h"

using namespace xwalk;

#define XWALK_VIEW_GET_IMPL_OR_RETURN(xwalk_view, impl, ...)         \
  WebView* impl = ToWebView(xwalk_view);                             \
  do {                                                               \
    if (!impl) {                                                     \
      EINA_LOG_CRIT("no private data for object %p", xwalk_view);    \
      return __VA_ARGS__;                                            \
    }                                                                \
  } while (0)


Evas_Object* xwalk_view_add(Evas_Object* root_window)
{
  return WebView::Create(root_window)->EvasObject();
}

Eina_Bool xwalk_view_url_set(Evas_Object* evas_object, const char* url)
{
  XWALK_VIEW_GET_IMPL_OR_RETURN(evas_object, impl, false);
  EINA_SAFETY_ON_NULL_RETURN_VAL(url, false);

  impl->LoadURL(GURL(url));
  return true;
}

const char* xwalk_view_url_get(const Evas_Object* evas_object)
{
  XWALK_VIEW_GET_IMPL_OR_RETURN(evas_object, impl, "");
  return impl->url();
}

Eina_Bool xwalk_view_reload(Evas_Object* evas_object)
{
  XWALK_VIEW_GET_IMPL_OR_RETURN(evas_object, impl, false);
  impl->Reload();

  return true;
}

Eina_Bool xwalk_view_back(Evas_Object* evas_object)
{
  XWALK_VIEW_GET_IMPL_OR_RETURN(evas_object, impl, false);
  if (impl->CanGoBack()) {
    impl->GoBack();
    return true;
  }
  return false;
}

Eina_Bool xwalk_view_forward(Evas_Object* evas_object)
{
  XWALK_VIEW_GET_IMPL_OR_RETURN(evas_object, impl, false);
  if (impl->CanGoForward()) {
    impl->GoForward();
    return true;
  }
  return false;
}
