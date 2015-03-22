// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webui/content_web_ui_controller_factory.h"

#include "content/browser/accessibility/accessibility_ui.h"
#include "content/browser/gpu/gpu_internals_ui.h"
#include "content/browser/indexed_db/indexed_db_internals_ui.h"
#include "content/browser/media/media_internals_ui.h"
#include "content/browser/service_worker/service_worker_internals_ui.h"
#include "content/browser/tracing/tracing_ui.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/common/url_constants.h"
#if defined(USE_AURA) && defined(OS_LINUX)
#include "xwalk/runtime/browser/ui/webui/file_picker/file_picker_ui.h"
#endif

#if defined(ENABLE_WEBRTC)
#include "content/browser/media/webrtc_internals_ui.h"
#endif

namespace content {

WebUI::TypeID ContentWebUIControllerFactory::GetWebUIType(
      BrowserContext* browser_context, const GURL& url) const {
  if (url.host() == kChromeUIWebRTCInternalsHost ||
#if !defined(OS_ANDROID)
      url.host() == kChromeUITracingHost ||
#endif
#if defined(USE_AURA) && defined(OS_LINUX)
	  url.host() == kChromeUIFilePickerHost ||
#endif
      url.host() == kChromeUIGpuHost ||
      url.host() == kChromeUIIndexedDBInternalsHost ||
      url.host() == kChromeUIMediaInternalsHost ||
      url.host() == kChromeUIServiceWorkerInternalsHost ||
      url.host() == kChromeUIAccessibilityHost) {
    return const_cast<ContentWebUIControllerFactory*>(this);
  }
  return WebUI::kNoWebUI;
}

bool ContentWebUIControllerFactory::UseWebUIForURL(
    BrowserContext* browser_context, const GURL& url) const {
  return GetWebUIType(browser_context, url) != WebUI::kNoWebUI;
}

bool ContentWebUIControllerFactory::UseWebUIBindingsForURL(
    BrowserContext* browser_context, const GURL& url) const {
  return UseWebUIForURL(browser_context, url);
}

WebUIController* ContentWebUIControllerFactory::CreateWebUIControllerForURL(
    WebUI* web_ui, const GURL& url) const {
  if (url.host() == kChromeUIGpuHost)
    return new GpuInternalsUI(web_ui);
  if (url.host() == kChromeUIIndexedDBInternalsHost)
    return new IndexedDBInternalsUI(web_ui);
  if (url.host() == kChromeUIMediaInternalsHost)
    return new MediaInternalsUI(web_ui);
  if (url.host() == kChromeUIAccessibilityHost)
    return new AccessibilityUI(web_ui);
  if (url.host() == kChromeUIServiceWorkerInternalsHost)
    return new ServiceWorkerInternalsUI(web_ui);
#if !defined(OS_ANDROID)
  if (url.host() == kChromeUITracingHost)
    return new TracingUI(web_ui);
#endif
#if defined(USE_AURA) && defined(OS_LINUX)
  if (url.host() == kChromeUIFilePickerHost)
    return new ui::FilePickerUI(web_ui);
#endif

#if defined(ENABLE_WEBRTC)
  if (url.host() == kChromeUIWebRTCInternalsHost)
    return new WebRTCInternalsUI(web_ui);
#endif

  return NULL;
}

// static
ContentWebUIControllerFactory* ContentWebUIControllerFactory::GetInstance() {
  return Singleton<ContentWebUIControllerFactory>::get();
}

ContentWebUIControllerFactory::ContentWebUIControllerFactory() {
}

ContentWebUIControllerFactory::~ContentWebUIControllerFactory() {
}

}  // namespace content
