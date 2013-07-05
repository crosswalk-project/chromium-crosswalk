// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EFL_WEBVIEW_LIB_WEB_CONTENTS_VIEW_DELEGATE_XWALK_H_
#define EFL_WEBVIEW_LIB_WEB_CONTENTS_VIEW_DELEGATE_XWALK_H_

#include "base/compiler_specific.h"
#include "content/public/browser/web_contents_view_delegate.h"

namespace content {
class WebContents;
}

namespace xwalk {

class WebContentsViewDelegateXWalk : public content::WebContentsViewDelegate {
 public:
  explicit WebContentsViewDelegateXWalk(content::WebContents*);
  virtual ~WebContentsViewDelegateXWalk();

  // Overridden from WebContentsViewDelegate:
  virtual void ShowContextMenu(
      const content::ContextMenuParams& params,
      content::ContextMenuSourceType type) OVERRIDE;
  virtual content::WebDragDestDelegate* GetDragDestDelegate() OVERRIDE;
  virtual void Focus() OVERRIDE;

  void SetViewContainerBox(Evas_Object*);

 private:
  content::WebContents* web_contents_;
  Evas_Object* view_box_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsViewDelegateXWalk);
};

} // namespace xwalk

#endif // EFL_WEBVIEW_LIB_WEB_CONTENTS_VIEW_DELEGATE_XWALK_H_
