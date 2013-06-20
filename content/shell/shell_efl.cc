// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/shell.h"

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Eina.h>
#include <Elementary.h>
#include <Evas.h>

#include "base/logging.h"
#include "base/strings/string_piece.h"
#include "base/utf_string_conversions.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_view.h"
#include "content/public/common/renderer_preferences.h"
#include "content/shell/shell_browser_context.h"
#include "content/shell/shell_content_browser_client.h"

namespace content {

void Shell::PlatformInitialize(const gfx::Size& default_window_size) {
  elm_init(0, NULL);  // We're not interested in argc and argv here.
  elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
}

void Shell::PlatformCleanUp() {
}

void Shell::PlatformEnableUIControl(UIControl control, bool is_enabled) {
  if (headless_)
    return;
}

void Shell::PlatformSetAddressBarURL(const GURL& url) {
  if (headless_)
    return;
}

void Shell::PlatformSetIsLoading(bool loading) {
  if (headless_)
    return;
}

void Shell::PlatformCreateWindow(int width, int height) {
  SizeTo(width, height);

  if (headless_)
    return;

  main_window_ = elm_win_add(NULL, "Content Shell", ELM_WIN_BASIC);

  elm_win_title_set(main_window_, "Content Shell");
  elm_win_autodel_set(main_window_, true);

  evas_object_resize(main_window_, width, height);
  evas_object_event_callback_add(main_window_, EVAS_CALLBACK_DEL,
                                 OnMainWindowDel, this);

  Evas_Object* rect = evas_object_rectangle_add(
      evas_object_evas_get(main_window_));
  evas_object_color_set(rect, 255, 0, 0, 255);
  evas_object_size_hint_weight_set(rect, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  elm_win_resize_object_add(main_window_, rect);
  evas_object_show(rect);

  evas_object_show(main_window_);
}

void Shell::PlatformSetContents() {
  if (headless_)
    return;

  WebContentsView* content_view = web_contents_->GetView();
  // gtk_container_add(GTK_CONTAINER(vbox_), content_view->GetNativeView());
}

void Shell::SizeTo(int width, int height) {
  content_width_ = width;
  content_height_ = height;
  if (web_contents_) {
    // gtk_widget_set_size_request(web_contents_->GetView()->GetNativeView(),
    //                             width, height);
  }
}

void Shell::PlatformResizeSubViews() {
  SizeTo(content_width_, content_height_);
}

void Shell::Close() {
  if (headless_) {
    delete this;
    return;
  }

  // gtk_widget_destroy(GTK_WIDGET(window_));
}

void Shell::PlatformSetTitle(const string16& title) {
  if (headless_)
    return;

  std::string title_utf8 = UTF16ToUTF8(title);
  elm_win_title_set(main_window_, title_utf8.c_str());
}

void Shell::OnMainWindowDel(void* data, Evas* evas, Evas_Object* object,
                            void* event_info) {
  Shell* shell = static_cast<Shell*>(data);
  delete shell;
}

}  // namespace content
