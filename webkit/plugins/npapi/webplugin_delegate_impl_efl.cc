// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/plugins/npapi/webplugin_delegate_impl.h"

#include <string>
#include <vector>

#include "base/metrics/stats_counters.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"
#include "ui/gfx/blit.h"
#include "webkit/plugins/npapi/plugin_instance.h"
#include "webkit/plugins/npapi/webplugin.h"
#include "webkit/plugins/plugin_constants.h"

#include "third_party/npapi/bindings/npapi_x11.h"

using WebKit::WebCursorInfo;
using WebKit::WebKeyboardEvent;
using WebKit::WebInputEvent;
using WebKit::WebMouseEvent;

namespace webkit {
namespace npapi {

WebPluginDelegateImpl::WebPluginDelegateImpl(
    PluginInstance* instance) {
}

WebPluginDelegateImpl::~WebPluginDelegateImpl() {
}

bool WebPluginDelegateImpl::PlatformInitialize() {
  return true;
}

void WebPluginDelegateImpl::PlatformDestroyInstance() {
  // Nothing to do here.
}

void WebPluginDelegateImpl::Paint(WebKit::WebCanvas* canvas,
                                  const gfx::Rect& rect) {
}

bool WebPluginDelegateImpl::WindowedCreatePlugin() {
  return true;
}

void WebPluginDelegateImpl::WindowedDestroyWindow() {
}

bool WebPluginDelegateImpl::WindowedReposition(
    const gfx::Rect& window_rect,
    const gfx::Rect& clip_rect) {
  return true;
}

void WebPluginDelegateImpl::WindowedSetWindow() {
}

void WebPluginDelegateImpl::WindowlessUpdateGeometry(
    const gfx::Rect& window_rect,
    const gfx::Rect& clip_rect) {
}

void WebPluginDelegateImpl::WindowlessPaint(cairo_t* context,
                                            const gfx::Rect& damage_rect) {
}

void WebPluginDelegateImpl::WindowlessSetWindow() {
}

bool WebPluginDelegateImpl::PlatformSetPluginHasFocus(bool focused) {
  return true;
}

bool WebPluginDelegateImpl::PlatformHandleInputEvent(
    const WebInputEvent& event, WebCursorInfo* cursor_info) {
  return true;
}

}  // namespace npapi
}  // namespace webkit
