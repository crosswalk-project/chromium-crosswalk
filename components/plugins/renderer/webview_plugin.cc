// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/plugins/renderer/webview_plugin.h"

#include "base/message_loop/message_loop.h"
#include "base/metrics/histogram.h"
#include "base/numerics/safe_conversions.h"
#include "content/public/common/web_preferences.h"
#include "content/public/renderer/render_view.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/WebKit/public/platform/WebSize.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/platform/WebURLRequest.h"
#include "third_party/WebKit/public/platform/WebURLResponse.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebInputEvent.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebPluginContainer.h"
#include "third_party/WebKit/public/web/WebView.h"

using blink::WebCanvas;
using blink::WebCursorInfo;
using blink::WebDragData;
using blink::WebDragOperationsMask;
using blink::WebImage;
using blink::WebInputEvent;
using blink::WebLocalFrame;
using blink::WebMouseEvent;
using blink::WebPlugin;
using blink::WebPluginContainer;
using blink::WebPoint;
using blink::WebRect;
using blink::WebSize;
using blink::WebString;
using blink::WebURLError;
using blink::WebURLRequest;
using blink::WebURLResponse;
using blink::WebVector;
using blink::WebView;
using content::WebPreferences;

WebViewPlugin::WebViewPlugin(WebViewPlugin::Delegate* delegate,
                             const WebPreferences& preferences)
    : delegate_(delegate),
      container_(NULL),
      web_view_(WebView::create(this)),
      finished_loading_(false),
      focused_(false) {
  // ApplyWebPreferences before making a WebLocalFrame so that the frame sees a
  // consistent view of our preferences.
  content::RenderView::ApplyWebPreferences(preferences, web_view_);
  web_frame_ = WebLocalFrame::create(this);
  web_view_->setMainFrame(web_frame_);
}

// static
WebViewPlugin* WebViewPlugin::Create(WebViewPlugin::Delegate* delegate,
                                     const WebPreferences& preferences,
                                     const std::string& html_data,
                                     const GURL& url) {
  WebViewPlugin* plugin = new WebViewPlugin(delegate, preferences);
  plugin->web_view()->mainFrame()->loadHTMLString(html_data, url);
  return plugin;
}

WebViewPlugin::~WebViewPlugin() {
  web_view_->close();
  web_frame_->close();
}

void WebViewPlugin::ReplayReceivedData(WebPlugin* plugin) {
  if (!response_.isNull()) {
    plugin->didReceiveResponse(response_);
    size_t total_bytes = 0;
    for (std::list<std::string>::iterator it = data_.begin(); it != data_.end();
         ++it) {
      plugin->didReceiveData(
          it->c_str(), base::checked_cast<int, size_t>(it->length()));
      total_bytes += it->length();
    }
    UMA_HISTOGRAM_MEMORY_KB(
        "PluginDocument.Memory",
        (base::checked_cast<int, size_t>(total_bytes / 1024)));
    UMA_HISTOGRAM_COUNTS(
        "PluginDocument.NumChunks",
        (base::checked_cast<int, size_t>(data_.size())));
  }
  // We need to transfer the |focused_| to new plugin after it loaded.
  if (focused_) {
    plugin->updateFocus(true, blink::WebFocusTypeNone);
  }
  if (finished_loading_) {
    plugin->didFinishLoading();
  }
  if (error_) {
    plugin->didFailLoading(*error_);
  }
}

void WebViewPlugin::RestoreTitleText() {
  if (container_)
    container_->element().setAttribute("title", old_title_);
}

WebPluginContainer* WebViewPlugin::container() const { return container_; }

bool WebViewPlugin::initialize(WebPluginContainer* container) {
  container_ = container;
  if (container_) {
    // We must call layout again here to ensure that the container is laid
    // out before we next try to paint it, which is a requirement of the
    // document life cycle in Blink. In most cases, needsLayout is set by
    // scheduleAnimation, but due to timers controlling widget update,
    // scheduleAnimation may be invoked before this initialize call (which
    // comes through the widget update process). It doesn't hurt to mark
    // for layout again, and it does help us in the race-condition situation.
    container_->setNeedsLayout();

    old_title_ = container_->element().getAttribute("title");

    // Propagate device scale to inner webview to load the correct resource
    // when images have a "srcset" attribute.
    web_view_->setDeviceScaleFactor(container_->deviceScaleFactor());
  }
  return true;
}

void WebViewPlugin::destroy() {
  if (delegate_) {
    delegate_->PluginDestroyed();
    delegate_ = NULL;
  }
  container_ = NULL;
  base::MessageLoop::current()->DeleteSoon(FROM_HERE, this);
}

v8::Local<v8::Object> WebViewPlugin::v8ScriptableObject(v8::Isolate* isolate) {
  if (!delegate_)
    return v8::Local<v8::Object>();

  return delegate_->GetV8ScriptableObject(isolate);
}

void WebViewPlugin::layoutIfNeeded() {
  web_view_->layout();
}

void WebViewPlugin::paint(WebCanvas* canvas, const WebRect& rect) {
  gfx::Rect paint_rect = gfx::IntersectRects(rect_, rect);
  if (paint_rect.IsEmpty())
    return;

  paint_rect.Offset(-rect_.x(), -rect_.y());

  canvas->save();
  canvas->translate(SkIntToScalar(rect_.x()), SkIntToScalar(rect_.y()));

  // Apply inverse device scale factor, as the outer webview has already
  // applied it, and the inner webview will apply it again.
  SkScalar inverse_scale =
      SkFloatToScalar(1.0 / container_->deviceScaleFactor());
  canvas->scale(inverse_scale, inverse_scale);

  web_view_->paint(canvas, paint_rect);

  canvas->restore();
}

// Coordinates are relative to the containing window.
void WebViewPlugin::updateGeometry(const WebRect& window_rect,
                                   const WebRect& clip_rect,
                                   const WebRect& unobscured_rect,
                                   const WebVector<WebRect>& cut_outs_rects,
                                   bool is_visible) {
  if (static_cast<gfx::Rect>(window_rect) != rect_) {
    rect_ = window_rect;
    WebSize newSize(window_rect.width, window_rect.height);
    web_view_->resize(newSize);
  }
}

void WebViewPlugin::updateFocus(bool focused, blink::WebFocusType focus_type) {
  focused_ = focused;
}

bool WebViewPlugin::acceptsInputEvents() { return true; }

bool WebViewPlugin::handleInputEvent(const WebInputEvent& event,
                                     WebCursorInfo& cursor) {
  // For tap events, don't handle them. They will be converted to
  // mouse events later and passed to here.
  if (event.type == WebInputEvent::GestureTap)
    return false;

  // For LongPress events we return false, since otherwise the context menu will
  // be suppressed. https://crbug.com/482842
  if (event.type == WebInputEvent::GestureLongPress)
    return false;

  if (event.type == WebInputEvent::ContextMenu) {
    if (delegate_) {
      const WebMouseEvent& mouse_event =
          reinterpret_cast<const WebMouseEvent&>(event);
      delegate_->ShowContextMenu(mouse_event);
    }
    return true;
  }
  current_cursor_ = cursor;
  bool handled = web_view_->handleInputEvent(event);
  cursor = current_cursor_;

  return handled;
}

void WebViewPlugin::didReceiveResponse(const WebURLResponse& response) {
  DCHECK(response_.isNull());
  response_ = response;
}

void WebViewPlugin::didReceiveData(const char* data, int data_length) {
  data_.push_back(std::string(data, data_length));
}

void WebViewPlugin::didFinishLoading() {
  DCHECK(!finished_loading_);
  finished_loading_ = true;
}

void WebViewPlugin::didFailLoading(const WebURLError& error) {
  DCHECK(!error_.get());
  error_.reset(new WebURLError(error));
}

bool WebViewPlugin::acceptsLoadDrops() { return false; }

void WebViewPlugin::setToolTipText(const WebString& text,
                                   blink::WebTextDirection hint) {
  if (container_)
    container_->element().setAttribute("title", text);
}

void WebViewPlugin::startDragging(WebLocalFrame*,
                                  const WebDragData&,
                                  WebDragOperationsMask,
                                  const WebImage&,
                                  const WebPoint&) {
  // Immediately stop dragging.
  web_view_->dragSourceSystemDragEnded();
}

bool WebViewPlugin::allowsBrokenNullLayerTreeView() const {
  return true;
}

void WebViewPlugin::didInvalidateRect(const WebRect& rect) {
  if (container_)
    container_->invalidateRect(rect);
}

void WebViewPlugin::didUpdateLayoutSize(const WebSize&) {
  if (container_)
    container_->setNeedsLayout();
}

void WebViewPlugin::didChangeCursor(const WebCursorInfo& cursor) {
  current_cursor_ = cursor;
}

void WebViewPlugin::scheduleAnimation() {
  if (container_)
    container_->setNeedsLayout();
}

void WebViewPlugin::didClearWindowObject(WebLocalFrame* frame) {
  if (delegate_)
    delegate_->BindWebFrame(frame);
}

void WebViewPlugin::didReceiveResponse(WebLocalFrame* frame,
                                       unsigned identifier,
                                       const WebURLResponse& response) {
  WebFrameClient::didReceiveResponse(frame, identifier, response);
}
