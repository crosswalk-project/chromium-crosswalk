// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_VIEWS_DELEGATE_H_
#define UI_VIEWS_VIEWS_DELEGATE_H_

#include <string>

#if defined(OS_WIN)
#include <windows.h>
#endif

#include "base/strings/string16.h"
#include "ui/base/accessibility/accessibility_types.h"
#include "ui/base/ui_base_types.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/views_export.h"
#include "ui/views/widget/widget.h"

namespace base {
class TimeDelta;
}

namespace content {
class WebContents;
class BrowserContext;
class SiteInstance;
}

namespace gfx {
class ImageSkia;
class Rect;
}

namespace views {

class NativeWidget;
class NonClientFrameView;
class ViewsTouchSelectionControllerFactory;
class View;
class Widget;
namespace internal {
class NativeWidgetDelegate;
}

// ViewsDelegate is an interface implemented by an object using the views
// framework. It is used to obtain various high level application utilities
// and perform some actions such as window placement saving.
//
// The embedding app must set views_delegate to assign its ViewsDelegate
// implementation.
class VIEWS_EXPORT ViewsDelegate {
 public:
  // The active ViewsDelegate used by the views system.
  static ViewsDelegate* views_delegate;

  ViewsDelegate();

  virtual ~ViewsDelegate();

  // Saves the position, size and "show" state for the window with the
  // specified name.
  virtual void SaveWindowPlacement(const Widget* widget,
                                   const std::string& window_name,
                                   const gfx::Rect& bounds,
                                   ui::WindowShowState show_state) = 0;

  // Retrieves the saved position and size and "show" state for the window with
  // the specified name.
  virtual bool GetSavedWindowPlacement(
      const Widget* widget,
      const std::string& window_name,
      gfx::Rect* bounds,
      ui::WindowShowState* show_state) const = 0;

  virtual void NotifyAccessibilityEvent(
      View* view,
      ui::AccessibilityTypes::Event event_type) = 0;

  // For accessibility, notify the delegate that a menu item was focused
  // so that alternate feedback (speech / magnified text) can be provided.
  virtual void NotifyMenuItemFocused(const base::string16& menu_name,
                                     const base::string16& menu_item_name,
                                     int item_index,
                                     int item_count,
                                     bool has_submenu) = 0;

#if defined(OS_WIN)
  // Retrieves the default window icon to use for windows if none is specified.
  virtual HICON GetDefaultWindowIcon() const = 0;
  // Returns true if the window passed in is in the Windows 8 metro
  // environment.
  virtual bool IsWindowInMetro(gfx::NativeWindow window) const = 0;
#elif defined(OS_LINUX) && !defined(OS_CHROMEOS)
  virtual gfx::ImageSkia* GetDefaultWindowIcon() const = 0;
#endif

  // Creates a default NonClientFrameView to be used for windows that don't
  // specify their own. If this function returns NULL, the
  // views::CustomFrameView type will be used.
  virtual NonClientFrameView* CreateDefaultNonClientFrameView(
      Widget* widget) = 0;

  // AddRef/ReleaseRef are invoked while a menu is visible. They are used to
  // ensure we don't attempt to exit while a menu is showing.
  virtual void AddRef() = 0;
  virtual void ReleaseRef() = 0;

  // Creates a web contents. This will return NULL unless overriden.
  virtual content::WebContents* CreateWebContents(
      content::BrowserContext* browser_context,
      content::SiteInstance* site_instance) = 0;

  // Gives the platform a chance to modify the properties of a Widget.
  virtual void OnBeforeWidgetInit(Widget::InitParams* params,
                                  internal::NativeWidgetDelegate* delegate) = 0;

  // Returns the default obscured text reveal duration.
  virtual base::TimeDelta GetDefaultTextfieldObscuredRevealDuration() = 0;

  // Returns true if the operating system's window manager will always provide a
  // title bar with caption buttons (ignoring the setting to
  // |remove_standard_frame| in InitParams). If |maximized|, this applies to
  // maximized windows; otherwise to restored windows.
  virtual bool WindowManagerProvidesTitleBar(bool maximized);

  virtual void SetShowTitleBar(bool show_title_bar) { should_show_titlebar_ = show_title_bar; }
  virtual bool ShouldShowTitleBar() const { return  should_show_titlebar_; }
 private:
  scoped_ptr<ViewsTouchSelectionControllerFactory> views_tsc_factory_;
  // Set to ture if the window should have the title bar.
  bool should_show_titlebar_;
};

}  // namespace views

#endif  // UI_VIEWS_VIEWS_DELEGATE_H_
