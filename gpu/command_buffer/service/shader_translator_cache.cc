// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <GLES2/gl2.h>

#include "gpu/command_buffer/service/shader_translator_cache.h"

namespace gpu {
namespace gles2 {

ShaderTranslatorCache::ShaderTranslatorCache() {
}

ShaderTranslatorCache::~ShaderTranslatorCache() {
#if !defined(DISABLE_ANGLE_ON_ANDROID)
  DCHECK(cache_.empty());
#endif
}

void ShaderTranslatorCache::OnDestruct(ShaderTranslator* translator) {
#if !defined(DISABLE_ANGLE_ON_ANDROID)
  Cache::iterator it = cache_.begin();
  while (it != cache_.end()) {
    if (it->second == translator) {
      cache_.erase(it);
      return;
    }
    it++;
  }
#endif
}

#if defined(DISABLE_ANGLE_ON_ANDROID)
scoped_refptr<ShaderTranslator> ShaderTranslatorCache::GetTranslator() {
  return new ShaderTranslator();
}
#else
scoped_refptr<ShaderTranslator> ShaderTranslatorCache::GetTranslator(
    sh::GLenum shader_type,
    ShShaderSpec shader_spec,
    const ShBuiltInResources* resources,
    ShaderTranslatorInterface::GlslImplementationType
        glsl_implementation_type,
    ShCompileOptions driver_bug_workarounds) {
  ShaderTranslatorInitParams params(shader_type,
                                    shader_spec,
                                    *resources,
                                    glsl_implementation_type,
                                    driver_bug_workarounds);

  Cache::iterator it = cache_.find(params);
  if (it != cache_.end())
    return it->second;

  ShaderTranslator* translator = new ShaderTranslator();
  if (translator->Init(shader_type, shader_spec, resources,
                       glsl_implementation_type,
                       driver_bug_workarounds)) {
    cache_[params] = translator;
    translator->AddDestructionObserver(this);
    return translator;
  } else {
    return NULL;
  }
}
#endif

}  // namespace gles2
}  // namespace gpu
