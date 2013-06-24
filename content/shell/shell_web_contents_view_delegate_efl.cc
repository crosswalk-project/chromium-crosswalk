// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/shell_web_contents_view_delegate.h"

#include "base/command_line.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_view.h"
#include "content/public/common/context_menu_params.h"
#include "content/shell/shell.h"
#include "content/shell/shell_browser_context.h"
#include "content/shell/shell_browser_main_parts.h"
#include "content/shell/shell_content_browser_client.h"
#include "content/shell/shell_devtools_frontend.h"
#include "content/shell/shell_switches.h"
#include "content/shell/shell_web_contents_view_delegate_creator.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebContextMenuData.h"
#include "ui/base/gtk/focus_store_gtk.h"
#include "ui/base/gtk/gtk_floating_container.h"

using WebKit::WebContextMenuData;

namespace content {

WebContentsViewDelegate* CreateShellWebContentsViewDelegate(
    WebContents* web_contents) {
  return new ShellWebContentsViewDelegate(web_contents);
}

ShellWebContentsViewDelegate::ShellWebContentsViewDelegate(
    WebContents* web_contents)
    : web_contents_(web_contents) {
}

ShellWebContentsViewDelegate::~ShellWebContentsViewDelegate() {
}

void ShellWebContentsViewDelegate::ShowContextMenu(
    const ContextMenuParams& params,
    ContextMenuSourceType type) {
  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kDumpRenderTree))
    return;
}

WebDragDestDelegate* ShellWebContentsViewDelegate::GetDragDestDelegate() {
  return NULL;
}

void ShellWebContentsViewDelegate::Initialize(GtkWidget* expanded_container,
                                              ui::FocusStoreGtk* focus_store) {
//  expanded_container_ = expanded_container;
//
//  gtk_container_add(GTK_CONTAINER(floating_.get()), expanded_container_);
//  gtk_widget_show(floating_.get());
}

gfx::NativeView ShellWebContentsViewDelegate::GetNativeView() const {
  // return floating_.get();
  return 0;
}

void ShellWebContentsViewDelegate::Focus() {
}

gboolean ShellWebContentsViewDelegate::OnNativeViewFocusEvent(
    GtkWidget* widget,
    GtkDirectionType type,
    gboolean* return_value) {
  return false;
}


}  // namespace content
