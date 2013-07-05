// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EFL_WEBVIEW_LIB_WEB_CONTENTS_DELEGATE_XWALK_H_
#define EFL_WEBVIEW_LIB_WEB_CONTENTS_DELEGATE_XWALK_H_

#include <Evas.h>

#include "base/memory/scoped_ptr.h"
#include "content/public/browser/web_contents_delegate.h"

namespace content {
class WebContentsViewDelegate;
}

namespace xwalk {

class WebContentsDelegateXWalk : public content::WebContentsDelegate
{
 public:
  WebContentsDelegateXWalk(content::BrowserContext*, Evas_Object*);
  virtual bool TakeFocus(content::WebContents* source,
                         bool reverse) OVERRIDE;

  content::WebContents* WebContents() { return web_contents_.get(); }

 private:
  Evas_Object* view_box_;
  scoped_ptr<content::WebContents> web_contents_;
};

} // namespace xwalk

#endif // EFL_WEBVIEW_LIB_WEB_CONTENTS_DELEGATE_XWALK_H_
