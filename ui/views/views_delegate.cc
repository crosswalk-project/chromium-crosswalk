// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/views_delegate.h"

#include "base/command_line.h"
#include "ui/views/views_touch_selection_controller_factory.h"

namespace views {

ViewsDelegate::ViewsDelegate()
    : views_tsc_factory_(new ViewsTouchEditingControllerFactory),
      should_show_titlebar_(true) {
  ui::TouchEditingControllerFactory::SetInstance(views_tsc_factory_.get());
}

ViewsDelegate::~ViewsDelegate() {
  ui::TouchEditingControllerFactory::SetInstance(NULL);
}

void ViewsDelegate::SaveWindowPlacement(const Widget* widget,
                                        const std::string& window_name,
                                        const gfx::Rect& bounds,
                                        ui::WindowShowState show_state) {
}

bool ViewsDelegate::GetSavedWindowPlacement(
    const Widget* widget,
    const std::string& window_name,
    gfx::Rect* bounds,
    ui::WindowShowState* show_state) const {
  return false;
}

void ViewsDelegate::NotifyAccessibilityEvent(View* view,
                                             ui::AXEvent event_type) {
}

void ViewsDelegate::NotifyMenuItemFocused(const base::string16& menu_name,
                                          const base::string16& menu_item_name,
                                          int item_index,
                                          int item_count,
                                          bool has_submenu) {
}

#if defined(OS_WIN)
HICON ViewsDelegate::GetDefaultWindowIcon() const {
  return NULL;
}

bool ViewsDelegate::IsWindowInMetro(gfx::NativeWindow window) const {
  return false;
}
#elif defined(OS_LINUX) && !defined(OS_CHROMEOS)
gfx::ImageSkia* ViewsDelegate::GetDefaultWindowIcon() const {
  return NULL;
}
#endif

NonClientFrameView* ViewsDelegate::CreateDefaultNonClientFrameView(
    Widget* widget) {
  return NULL;
}

void ViewsDelegate::AddRef() {
}

void ViewsDelegate::ReleaseRef() {
}

content::WebContents* ViewsDelegate::CreateWebContents(
    content::BrowserContext* browser_context,
    content::SiteInstance* site_instance) {
  return NULL;
}

base::TimeDelta ViewsDelegate::GetDefaultTextfieldObscuredRevealDuration() {
  return base::TimeDelta();
}

bool ViewsDelegate::WindowManagerProvidesTitleBar(bool maximized) {
  return false;
}

ui::ContextFactory* ViewsDelegate::GetContextFactory() {
  return NULL;
}

std::string ViewsDelegate::GetApplicationName() {
  base::FilePath program = base::CommandLine::ForCurrentProcess()->GetProgram();
  return program.BaseName().AsUTF8Unsafe();
}

#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
scoped_refptr<base::TaskRunner>
    ViewsDelegate::GetTaskRunnerForAuraLinuxAccessibilityInit() {
  return nullptr;
}
#endif

#if defined(OS_WIN)
int ViewsDelegate::GetAppbarAutohideEdges(HMONITOR monitor,
                                          const base::Closure& callback) {
  return EDGE_BOTTOM;
}
#endif

void ViewsDelegate::SetShouldShowTitleBar(bool show_title_bar) {
  should_show_titlebar_ = show_title_bar;
}

bool ViewsDelegate::ShouldShowTitleBar() const {
  return should_show_titlebar_;
}

}  // namespace views
