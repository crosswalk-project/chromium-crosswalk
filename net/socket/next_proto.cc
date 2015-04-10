// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/next_proto.h"

namespace net {

NextProtoVector NextProtosHttpOnly() {
  NextProtoVector next_protos;
  next_protos.push_back(kProtoHTTP11);
  return next_protos;
}

NextProtoVector NextProtosDefaults() {
  NextProtoVector next_protos;
  next_protos.push_back(kProtoHTTP11);
  next_protos.push_back(kProtoSPDY31);
  next_protos.push_back(kProtoSPDY4_14);
  return next_protos;
}

NextProtoVector NextProtosWithSpdyAndQuic(bool spdy_enabled,
                                          bool quic_enabled) {
  NextProtoVector next_protos;
  next_protos.push_back(kProtoHTTP11);
#if !defined(DISABLE_QUIC_SUPPORT)
  if (quic_enabled)
    next_protos.push_back(kProtoQUIC1SPDY3);
#endif
  if (spdy_enabled) {
    next_protos.push_back(kProtoSPDY31);
    next_protos.push_back(kProtoSPDY4_14);
  }
  return next_protos;
}

NextProtoVector NextProtosSpdy31() {
  NextProtoVector next_protos;
  next_protos.push_back(kProtoHTTP11);
#if !defined(DISABLE_QUIC_SUPPORT)
  next_protos.push_back(kProtoQUIC1SPDY3);
#endif
  next_protos.push_back(kProtoSPDY31);
  return next_protos;
}

NextProtoVector NextProtosSpdy4Http2() {
  NextProtoVector next_protos;
  next_protos.push_back(kProtoHTTP11);
#if !defined(DISABLE_QUIC_SUPPORT)
  next_protos.push_back(kProtoQUIC1SPDY3);
#endif
  next_protos.push_back(kProtoSPDY31);
  next_protos.push_back(kProtoSPDY4_14);
  return next_protos;
}

}  // namespace net
