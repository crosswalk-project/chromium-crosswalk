// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "efl_webview/lib/webview.h"

#include <Elementary.h>
#include "content/browser/web_contents/web_contents_view_efl.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "efl_webview/lib/web_runtime_context.h"

namespace xwalk {

namespace {
const int g_window_width = 800;
const int g_window_height = 600;

class WebContentsDelegateXWalk : public content::WebContentsDelegate
{
 public:
  explicit WebContentsDelegateXWalk(content::BrowserContext*);
  content::WebContents* WebContents() { return web_contents_.get(); }

 private:
  scoped_ptr<content::WebContents> web_contents_;
};

WebContentsDelegateXWalk::WebContentsDelegateXWalk(
    content::BrowserContext* browser_context)
{
  content::WebContents::CreateParams create_params(browser_context, 0);
  create_params.initial_size = gfx::Size(g_window_width, g_window_height);

  web_contents_.reset(content::WebContents::Create(create_params));
  web_contents_->SetDelegate(this);
}

} // namespace

struct WebView::Private {
  Evas_Object* root_window;
  Evas_Object* view_box;
  scoped_refptr<WebRuntimeContext> context;
  scoped_ptr<WebContentsDelegateXWalk> webContentsDelegate;
};

// static
WebView* WebView::Create(Evas_Object* root_window) {
  return new WebView(root_window);
}

WebView::WebView(Evas_Object* root_window)
    : private_(new Private) {
  private_->root_window = root_window;
  private_->context = WebRuntimeContext::current();
  content::BrowserContext* browser_context =
      private_->context->BrowserContext();
  private_->webContentsDelegate.reset(
      new WebContentsDelegateXWalk(browser_context));

  private_->view_box = elm_box_add(private_->root_window);
  content::WebContentsView* content_view =
      private_->webContentsDelegate->WebContents()->GetView();
  static_cast<content::WebContentsViewEfl*>(content_view)->
      SetViewContainerBox(private_->view_box);
}

WebView::~WebView() {
  evas_object_del(private_->view_box);
}

void WebView::Forward() {
  private_->webContentsDelegate->WebContents()->GetController().GoForward();
}

void WebView::Back() {
  private_->webContentsDelegate->WebContents()->GetController().GoBack();
}

Evas_Object* WebView::EvasObject() {
  return private_->view_box;
}

}  // namespace xwalk
