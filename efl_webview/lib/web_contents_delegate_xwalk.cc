// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "efl_webview/lib/web_contents_delegate_xwalk.h"

#include <Elementary.h>
#include "content/public/browser/web_contents.h"
#include "efl_webview/lib/web_contents_view_delegate_xwalk.h"

namespace xwalk {

namespace {
const int g_window_width = 800;
const int g_window_height = 600;
} // namespace

WebContentsDelegateXWalk::WebContentsDelegateXWalk(
    content::BrowserContext* browser_context, Evas_Object* view_box)
    : view_box_(view_box)
{
  content::WebContents::CreateParams create_params(browser_context, 0);
  create_params.initial_size = gfx::Size(g_window_width, g_window_height);

  web_contents_.reset(content::WebContents::Create(create_params));
  web_contents_->SetDelegate(this);
}

bool WebContentsDelegateXWalk::TakeFocus(
    content::WebContents* source, bool reverse) {
  elm_object_focus_next(view_box_,
                        reverse ? ELM_FOCUS_PREVIOUS : ELM_FOCUS_NEXT);
  return true;
}

}  // namespace xwalk
