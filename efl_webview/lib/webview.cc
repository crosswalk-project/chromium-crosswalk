// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "efl_webview/lib/webview.h"

#include <Elementary.h>
#include "base/command_line.h"
#include "base/file_util.h"
#include "base/files/file_path.h"
#include "content/browser/web_contents/web_contents_view_efl.h"
#include "content/public/browser/web_contents.h"
#include "efl_webview/lib/web_contents_delegate_xwalk.h"
#include "efl_webview/lib/web_contents_view_delegate_xwalk.h"
#include "efl_webview/lib/web_runtime_context.h"
#include "net/base/net_util.h"

#include <map>

using std::map;

namespace xwalk {

static inline map<Evas_Object*, WebView*>& EvasObjectToWebViewMap() // FIXME: Temporary solution until web view has its own smart class.
{
  static map<Evas_Object*, WebView*> map;
  return map;
}

struct WebView::Private {
  Evas_Object* root_window;
  Evas_Object* view_box;
  scoped_refptr<WebRuntimeContext> context;
  scoped_ptr<WebContentsDelegateXWalk> webContentsDelegate;
  static GURL s_startup_url;
};

GURL WebView::Private::s_startup_url = GURL();

// static
WebView* WebView::Create(Evas_Object* root_window) {
  return new WebView(root_window);
}

// static
void WebView::CommandLineInit(int argc, char** argv) {
  CommandLine::Init(argc, argv);

  CommandLine* command_line = CommandLine::ForCurrentProcess();
  const CommandLine::StringVector& args = command_line->GetArgs();

  if (args.empty())
    return;

  GURL url(args[0]);
  if (!(url.is_valid() && url.has_scheme()))
    url = net::FilePathToFileURL(base::FilePath(args[0]));

  WebView::Private::s_startup_url = GURL(url);
}

WebView::WebView(Evas_Object* root_window)
    : private_(new Private) {
  {
    if (!WebView::Private::s_startup_url.is_valid())
      WebView::Private::s_startup_url = GURL("about:blank");
  }

  private_->root_window = root_window;
  private_->view_box = elm_box_add(private_->root_window);
  // FIXME: In future this has to be a separate Smart Class representing web view.
  // 'this' should be set as smart_data->priv
  EvasObjectToWebViewMap()[private_->view_box] = this;

  elm_object_focus_allow_set(private_->view_box, EINA_TRUE);

  private_->context = WebRuntimeContext::current();
  content::BrowserContext* browser_context =
      private_->context->BrowserContext();
  private_->webContentsDelegate.reset(
      new WebContentsDelegateXWalk(browser_context, private_->view_box));

  content::WebContentsViewEfl* content_view =
      static_cast<content::WebContentsViewEfl*>(private_->
          webContentsDelegate->WebContents()->GetView());
  content_view->SetViewContainerBox(private_->view_box);
  static_cast<WebContentsViewDelegateXWalk*>(content_view->delegate())->
      SetViewContainerBox(private_->view_box);

  LoadURL(WebView::Private::s_startup_url);
}

WebView::~WebView() { // FIXME : And by the way who will invoke it?
  EvasObjectToWebViewMap().erase(private_->view_box);
  evas_object_del(private_->view_box);  
}

bool WebView::CanGoBack() const
{
  return private_->webContentsDelegate->WebContents()->GetController().CanGoBack();
}

bool WebView::CanGoForward() const
{
  return private_->webContentsDelegate->WebContents()->GetController().CanGoForward();
}

void WebView::GoForward() {
  private_->webContentsDelegate->WebContents()->GetController().GoForward();
}

void WebView::GoBack() {
  private_->webContentsDelegate->WebContents()->GetController().GoBack();
}

void WebView::Reload() {
  private_->webContentsDelegate->WebContents()->GetController().Reload(false);
}

void WebView::LoadURL(const GURL& url) {
  content::NavigationController::LoadURLParams params(url);
  params.transition_type = content::PageTransitionFromInt(
      content::PAGE_TRANSITION_TYPED |
      content::PAGE_TRANSITION_FROM_ADDRESS_BAR);
  private_->webContentsDelegate->WebContents()->
      GetController().LoadURLWithParams(params);
  private_->webContentsDelegate->WebContents()->GetView()->Focus();
}

Evas_Object* WebView::EvasObject() {
  return private_->view_box;
}

WebView* ToWebView(Evas_Object* evas_object)
{
  map<Evas_Object*, WebView*>::iterator found = EvasObjectToWebViewMap().find(evas_object);
  if (found != EvasObjectToWebViewMap().end())
    return found->second;
  return 0;
}

}  // namespace xwalk
