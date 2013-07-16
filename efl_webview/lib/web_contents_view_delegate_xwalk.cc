// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "efl_webview/lib/web_contents_view_delegate_xwalk.h"

#include <Elementary.h>

namespace xwalk {

WebContentsViewDelegateXWalk::WebContentsViewDelegateXWalk(
    content::WebContents* web_contents)
    : web_contents_(web_contents)
    , view_box_(NULL) {
}

WebContentsViewDelegateXWalk::~WebContentsViewDelegateXWalk() {
}

void WebContentsViewDelegateXWalk::Focus() {
  if (!view_box_)
    return;
  elm_object_focus_set(view_box_, EINA_TRUE);
}

void WebContentsViewDelegateXWalk::ShowContextMenu(
    const content::ContextMenuParams& params,
    content::ContextMenuSourceType type) {
}

content::WebDragDestDelegate*
    WebContentsViewDelegateXWalk::GetDragDestDelegate() {
  return NULL;
}

void WebContentsViewDelegateXWalk::SetViewContainerBox(Evas_Object* view_box) {
  view_box_ = view_box;
}

}  // namespace xwalk
