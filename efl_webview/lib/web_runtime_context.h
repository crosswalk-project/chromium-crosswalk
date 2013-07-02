// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_WEB_RUNTIME_CONTEXT_H_
#define XWALK_WEB_RUNTIME_CONTEXT_H_

#include "base/basictypes.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/run_loop.h"

namespace content {
class BrowserContext;
}

namespace xwalk {

class WebRuntimeContext : public base::RefCounted<WebRuntimeContext> {
 public:
  static scoped_refptr<WebRuntimeContext> current();
  content::BrowserContext* BrowserContext();

 private:
  friend class base::RefCounted<WebRuntimeContext>;
  WebRuntimeContext();
  ~WebRuntimeContext();

  scoped_ptr<base::RunLoop> run_loop_;
  scoped_ptr<content::BrowserContext> browser_context_;

  DISALLOW_COPY_AND_ASSIGN(WebRuntimeContext);
};

}  // namespace xwalk

#endif  // XWALK_WEB_RUNTIME_CONTEXT_H_
