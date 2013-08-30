// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_GPU_CLIENT_GL_HELPER_BROWSER_H_
#define CONTENT_COMMON_GPU_CLIENT_GL_HELPER_BROWSER_H_

#include "base/atomicops.h"
#include "base/basictypes.h"
#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "content/common/content_export.h"
#include "content/common/gpu/client/webgraphicscontext3d_command_buffer_browser_impl.h"

namespace gfx {
class Rect;
class Size;
}

namespace media {
class VideoFrame;
};

class SkRegion;

namespace content {

class GLHelperScalingBrowser;
class ReadbackYUVInterfaceBrowser;

// Provides higher level operations on top of the WebKit::WebGraphicsContext3D
// interfaces.
class CONTENT_EXPORT GLHelperBrowser {
 public:
  explicit GLHelperBrowser(WebKit::WebGraphicsContext3D* context);
  ~GLHelperBrowser();

  enum ScalerQuality {
    // Bilinear single pass, fastest possible.
    SCALER_QUALITY_FAST = 1,

    // Bilinear upscale + N * 50% bilinear downscales.
    // This is still fast enough for most purposes and
    // Image quality is nearly as good as the BEST option.
    SCALER_QUALITY_GOOD = 2,

    // Bicubic upscale + N * 50% bicubic downscales.
    // Produces very good quality scaled images, but it's
    // 2-8x slower than the "GOOD" quality, so it's not always
    // worth it.
    SCALER_QUALITY_BEST = 3,
  };


  // Copies the block of pixels specified with |src_subrect| from |src_texture|,
  // scales it to |dst_size|, and writes it into |out|.
  // |src_size| is the size of |src_texture|. The result is of format GL_BGRA
  // and is potentially flipped vertically to make it a correct image
  // representation.  |callback| is invoked with the copy result when the copy
  // operation has completed.
  // Note that the src_texture will have the min/mag filter set to GL_LINEAR
  // and wrap_s/t set to CLAMP_TO_EDGE in this call.
  void CropScaleReadbackAndCleanTexture(
      WebKit::WebGLId src_texture,
      const gfx::Size& src_size,
      const gfx::Rect& src_subrect,
      const gfx::Size& dst_size,
      unsigned char* out,
      const base::Callback<void(bool)>& callback);

  // Copies the texture data out of |texture| into |out|.  |size| is the
  // size of the texture.  No post processing is applied to the pixels.  The
  // texture is assumed to have a format of GL_RGBA with a pixel type of
  // GL_UNSIGNED_BYTE.  This is a blocking call that calls glReadPixels on this
  // current context.
  void ReadbackTextureSync(WebKit::WebGLId texture,
                           const gfx::Rect& src_rect,
                           unsigned char* out);

  // Creates a copy of the specified texture. |size| is the size of the texture.
  // Note that the src_texture will have the min/mag filter set to GL_LINEAR
  // and wrap_s/t set to CLAMP_TO_EDGE in this call.
  WebKit::WebGLId CopyTexture(WebKit::WebGLId texture,
                              const gfx::Size& size);

  // Creates a scaled copy of the specified texture. |src_size| is the size of
  // the texture and |dst_size| is the size of the resulting copy.
  // Note that the src_texture will have the min/mag filter set to GL_LINEAR
  // and wrap_s/t set to CLAMP_TO_EDGE in this call.
  WebKit::WebGLId CopyAndScaleTexture(
      WebKit::WebGLId texture,
      const gfx::Size& src_size,
      const gfx::Size& dst_size,
      bool vertically_flip_texture,
      ScalerQuality quality);

  // Returns the shader compiled from the source.
  WebKit::WebGLId CompileShaderFromSource(const WebKit::WGC3Dchar* source,
                                          WebKit::WGC3Denum type);

  // Copies all pixels from |previous_texture| into |texture| that are
  // inside the region covered by |old_damage| but not part of |new_damage|.
  void CopySubBufferDamage(WebKit::WebGLId texture,
                           WebKit::WebGLId previous_texture,
                           const SkRegion& new_damage,
                           const SkRegion& old_damage);

  // Simply creates a texture.
  WebKit::WebGLId CreateTexture();

  // Resizes the texture's size to |size|.
  void ResizeTexture(WebKit::WebGLId texture, const gfx::Size& size);

  // Copies the framebuffer data given in |rect| to |texture|.
  void CopyTextureSubImage(WebKit::WebGLId texture, const gfx::Rect& rect);

  // Copies the all framebuffer data to |texture|. |size| specifies the
  // size of the framebuffer.
  void CopyTextureFullImage(WebKit::WebGLId texture, const gfx::Size& size);

  // A scaler will cache all intermediate textures and programs
  // needed to scale from a specified size to a destination size.
  // If the source or destination sizes changes, you must create
  // a new scaler.
  class CONTENT_EXPORT ScalerInterface {
   public:
    ScalerInterface() {}
    virtual ~ScalerInterface() {}

    // Note that the src_texture will have the min/mag filter set to GL_LINEAR
    // and wrap_s/t set to CLAMP_TO_EDGE in this call.
    virtual void Scale(WebKit::WebGLId source_texture,
                       WebKit::WebGLId dest_texture) = 0;
    virtual const gfx::Size& SrcSize() = 0;
    virtual const gfx::Rect& SrcSubrect() = 0;
    virtual const gfx::Size& DstSize() = 0;
  };

  // Note that the quality may be adjusted down if texture
  // allocations fail or hardware doesn't support the requtested
  // quality. Note that ScalerQuality enum is arranged in
  // numerical order for simplicity.
  ScalerInterface* CreateScaler(ScalerQuality quality,
                                const gfx::Size& src_size,
                                const gfx::Rect& src_subrect,
                                const gfx::Size& dst_size,
                                bool vertically_flip_texture,
                                bool swizzle);

  // Create a readback pipeline that will scale a subsection of the source
  // texture, then convert it to YUV422 planar form and then read back that.
  // This reduces the amount of memory read from GPU to CPU memory by a factor
  // 2.6, which can be quite handy since readbacks have very limited speed
  // on some platforms. All values in |dst_size| and |dst_subrect| must be
  // a multiple of two. If |use_mrt| is true, the pipeline will try to optimize
  // the YUV conversion using the multi-render-target extension. |use_mrt|
  // should only be set to false for testing.
  ReadbackYUVInterfaceBrowser* CreateReadbackPipelineYUV(
      ScalerQuality quality,
      const gfx::Size& src_size,
      const gfx::Rect& src_subrect,
      const gfx::Size& dst_size,
      const gfx::Rect& dst_subrect,
      bool flip_vertically,
      bool use_mrt);

  // Returns the maximum number of draw buffers available,
  // 0 if GL_EXT_draw_buffers is not available.
  WebKit::WGC3Dint MaxDrawBuffers();

 protected:
  class CopyTextureToImpl;

  // Creates |copy_texture_to_impl_| if NULL.
  void InitCopyTextToImpl();
  // Creates |scaler_impl_| if NULL.
  void InitScalerImpl();

  WebKit::WebGraphicsContext3D* context_;
  scoped_ptr<CopyTextureToImpl> copy_texture_to_impl_;
  scoped_ptr<GLHelperScalingBrowser> scaler_impl_;

  DISALLOW_COPY_AND_ASSIGN(GLHelperBrowser);
};

// Similar to a ScalerInterface, a yuv readback pipeline will
// cache a scaler and all intermediate textures and frame buffers
// needed to scale, crop, letterbox and read back a texture from
// the GPU into CPU-accessible RAM. A single readback pipeline
// can handle multiple outstanding readbacks at the same time, but
// if the source or destination sizes change, you'll need to create
// a new readback pipeline.
class CONTENT_EXPORT ReadbackYUVInterfaceBrowser {
public:
  ReadbackYUVInterfaceBrowser() {}
  virtual ~ReadbackYUVInterfaceBrowser() {}

  // Note that |target| must use YV12 format.
  virtual void ReadbackYUV(
      WebKit::WebGLId texture,
      media::VideoFrame* target,
      const base::Callback<void(bool)>& callback) = 0;
  virtual GLHelperBrowser::ScalerInterface* scaler() = 0;
};

}  // namespace content

#endif  // CONTENT_COMMON_GPU_CLIENT_GL_HELPER_BROWSER_H_
