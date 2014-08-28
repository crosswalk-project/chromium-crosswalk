// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/services/native_viewport/platform_viewport.h"

#include "ui/events/event.h"
#include "ui/events/platform/platform_event_dispatcher.h"
#include "ui/events/platform/platform_event_source.h"
#include "ui/ozone/public/cursor_factory_ozone.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ui/ozone/public/surface_factory_ozone.h"
#include "ui/platform_window/platform_window.h"
#include "ui/platform_window/platform_window_delegate.h"

namespace mojo {

// TODO(spang): Deduplicate with PlatformViewportX11.. but there's a hack
// in there that prevents this.
class PlatformViewportOzone : public PlatformViewport,
                              public ui::PlatformWindowDelegate {
 public:
  explicit PlatformViewportOzone(Delegate* delegate) : delegate_(delegate) {
    ui::OzonePlatform::InitializeForUI();
  }

  virtual ~PlatformViewportOzone() {
    // Destroy the platform-window while |this| is still alive.
    platform_window_.reset();
  }

 private:
  // Overridden from PlatformViewport:
  virtual void Init(const gfx::Rect& bounds) OVERRIDE {
    platform_window_ =
        ui::OzonePlatform::GetInstance()->CreatePlatformWindow(this, bounds);
  }

  virtual void Show() OVERRIDE { platform_window_->Show(); }

  virtual void Hide() OVERRIDE { platform_window_->Hide(); }

  virtual void Close() OVERRIDE { platform_window_->Close(); }

  virtual gfx::Size GetSize() OVERRIDE {
    return platform_window_->GetBounds().size();
  }

  virtual void SetBounds(const gfx::Rect& bounds) OVERRIDE {
    platform_window_->SetBounds(bounds);
  }

  virtual void SetCapture() OVERRIDE { platform_window_->SetCapture(); }

  virtual void ReleaseCapture() OVERRIDE { platform_window_->ReleaseCapture(); }

  // ui::PlatformWindowDelegate:
  virtual void OnBoundsChanged(const gfx::Rect& new_bounds) OVERRIDE {
    delegate_->OnBoundsChanged(new_bounds);
  }

  virtual void OnDamageRect(const gfx::Rect& damaged_region) OVERRIDE {}

  virtual void DispatchEvent(ui::Event* event) OVERRIDE {
    delegate_->OnEvent(event);
  }

  virtual void OnCloseRequest() OVERRIDE { platform_window_->Close(); }

  virtual void OnClosed() OVERRIDE { delegate_->OnDestroyed(); }

  virtual void OnWindowStateChanged(ui::PlatformWindowState state) OVERRIDE {}

  virtual void OnLostCapture() OVERRIDE {}

  virtual void OnAcceleratedWidgetAvailable(
      gfx::AcceleratedWidget widget) OVERRIDE {
    delegate_->OnAcceleratedWidgetAvailable(widget);
  }

  virtual void OnActivationChanged(bool active) OVERRIDE {}

  scoped_ptr<ui::PlatformWindow> platform_window_;
  Delegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(PlatformViewportOzone);
};

// static
scoped_ptr<PlatformViewport> PlatformViewport::Create(Delegate* delegate) {
  return scoped_ptr<PlatformViewport>(
      new PlatformViewportOzone(delegate)).Pass();
}

}  // namespace mojo
