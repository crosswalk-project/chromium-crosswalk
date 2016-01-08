// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VISITEDLINK_BROWSER_VISITEDLINK_EVENT_LISTENER_H_
#define COMPONENTS_VISITEDLINK_BROWSER_VISITEDLINK_EVENT_LISTENER_H_

#include <map>

#include "base/memory/linked_ptr.h"
#include "base/timer/timer.h"
#include "components/visitedlink/browser/visitedlink_master.h"
#ifndef DISABLE_NOTIFICATIONS
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#endif

namespace base {
class SharedMemory;
}

namespace content {
class BrowserContext;
}

namespace visitedlink {

class VisitedLinkUpdater;

// VisitedLinkEventListener broadcasts link coloring database updates to all
// processes. It also coalesces the updates to avoid excessive broadcasting of
// messages to the renderers.
#ifndef DISABLE_NOTIFICATIONS
class VisitedLinkEventListener : public VisitedLinkMaster::Listener,
                                 public content::NotificationObserver {
#else
class VisitedLinkEventListener : public VisitedLinkMaster::Listener {
#endif
 public:
  VisitedLinkEventListener(VisitedLinkMaster* master,
                           content::BrowserContext* browser_context);
  ~VisitedLinkEventListener() override;

  void NewTable(base::SharedMemory* table_memory) override;
  void Add(VisitedLinkMaster::Fingerprint fingerprint) override;
  void Reset() override;

 private:
  void CommitVisitedLinks();

#ifndef DISABLE_NOTIFICATIONS
  // content::NotificationObserver implementation.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;
#endif
  base::OneShotTimer<VisitedLinkEventListener> coalesce_timer_;
  VisitedLinkCommon::Fingerprints pending_visited_links_;

#ifndef DISABLE_NOTIFICATIONS
  content::NotificationRegistrar registrar_;
#endif

  // Map between renderer child ids and their VisitedLinkUpdater.
  typedef std::map<int, linked_ptr<VisitedLinkUpdater> > Updaters;
  Updaters updaters_;

  VisitedLinkMaster* master_;

  // Used to filter RENDERER_PROCESS_CREATED notifications to renderers that
  // belong to this BrowserContext.
  content::BrowserContext* browser_context_;

  DISALLOW_COPY_AND_ASSIGN(VisitedLinkEventListener);
};

}  // namespace visitedlink

#endif  // COMPONENTS_VISITEDLINK_BROWSER_VISITEDLINK_EVENT_LISTENER_H_
