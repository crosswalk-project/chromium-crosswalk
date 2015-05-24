// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBCL_CONFIG_H__
#define WEBCL_CONFIG_H__

#if ENABLE(WEBCL)

#include "bindings/core/v8/ScriptWrappable.h"
#include "bindings/core/v8/ScriptState.h"
#include "platform/Logging.h"
#include <CL/cl.h>
#include <CL/cl_platform.h>
#include <CL/cl_gl.h>
#include <wtf/HashSet.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>
#include <wtf/Vector.h>

typedef cl_int GLint;
typedef cl_uint GLuint;
typedef cl_uint GLenum;

#define MULTI_EXTENSIONS_LENGTH 1024
#define SINGLE_EXTENSION_LENGTH 64

#endif // ENABLE(WEBCL)
#endif // WebCLConfig_H
