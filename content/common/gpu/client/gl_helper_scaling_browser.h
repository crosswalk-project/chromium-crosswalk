// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_GPU_CLIENT_GL_HELPER_SCALING_BROWSER_H_
#define CONTENT_COMMON_GPU_CLIENT_GL_HELPER_SCALING_BROWSER_H_

#include <vector>

#include "content/common/gpu/client/gl_helper.h"
#include "content/common/gpu/client/gl_helper_browser.h"

namespace content {

class ShaderProgramBrowser;
class ScalerImpl;

// Implements GPU texture scaling methods.
// Note that you should probably not use this class directly.
// See gl_helper.cc::CreateScaler instead.
class CONTENT_EXPORT GLHelperScalingBrowser {
 public:
  enum ShaderType {
    SHADER_BILINEAR,
    SHADER_BILINEAR2,
    SHADER_BILINEAR3,
    SHADER_BILINEAR4,
    SHADER_BILINEAR2X2,
    SHADER_BICUBIC_UPSCALE,
    SHADER_BICUBIC_HALF_1D,
    SHADER_PLANAR,
    SHADER_YUV_MRT_PASS1,
    SHADER_YUV_MRT_PASS2,
  };

  // Similar to ScalerInterface, but can generate multiple outputs.
  // Used for YUV conversion in gl_helper.c
  class CONTENT_EXPORT ShaderInterface {
   public:
    ShaderInterface() {}
    virtual ~ShaderInterface() {}
    // Note that the src_texture will have the min/mag filter set to GL_LINEAR
    // and wrap_s/t set to CLAMP_TO_EDGE in this call.
    virtual void Execute(WebKit::WebGLId source_texture,
                         const std::vector<WebKit::WebGLId>& dest_textures) = 0;
  };

  typedef std::pair<ShaderType, bool> ShaderProgramBrowserKeyType;

  GLHelperScalingBrowser(WebKit::WebGraphicsContext3D* context,
                         GLHelperBrowser* helper);
  ~GLHelperScalingBrowser();
  void InitBuffer();

  GLHelperBrowser::ScalerInterface* CreateScaler(
      GLHelperBrowser::ScalerQuality quality,
      gfx::Size src_size,
      gfx::Rect src_subrect,
      const gfx::Size& dst_size,
      bool vertically_flip_texture,
      bool swizzle);

  GLHelperBrowser::ScalerInterface* CreatePlanarScaler(
      const gfx::Size& src_size,
      const gfx::Rect& src_subrect,
      const gfx::Size& dst_size,
      bool vertically_flip_texture,
      const float color_weights[4]);

  ShaderInterface* CreateYuvMrtShader(
      const gfx::Size& src_size,
      const gfx::Rect& src_subrect,
      const gfx::Size& dst_size,
      bool vertically_flip_texture,
      ShaderType shader);

 private:
  // A ScaleOp represents a pass in a scaler pipeline, in one dimension.
  // Note that when quality is GOOD, multiple scaler passes will be
  // combined into one operation for increased performance.
  // Exposed in the header file for testing purposes.
  struct ScaleOp {
    ScaleOp(int factor, bool x, int size)
        : scale_factor(factor), scale_x(x), scale_size(size) {
    }

    // Calculate a set of ScaleOp needed to convert an image of size
    // |src| into an image of size |dst|. If |scale_x| is true, then
    // the calculations are for the X axis of the image, otherwise Y.
    // If |allow3| is true, we can use a SHADER_BILINEAR3 to replace
    // a scale up and scale down with a 3-tap bilinear scale.
    // The calculated ScaleOps are added to |ops|.
    static void AddOps(int src,
                       int dst,
                       bool scale_x,
                       bool allow3,
                       std::deque<ScaleOp>* ops) {
      int num_downscales = 0;
      if (allow3 && dst * 3 >= src && dst * 2 < src) {
        // Technically, this should be a scale up and then a
        // scale down, but it makes the optimization code more
        // complicated.
        ops->push_back(ScaleOp(3, scale_x, dst));
        return;
      }
      while ((dst << num_downscales) < src) {
        num_downscales++;
      }
      if ((dst << num_downscales) != src) {
        ops->push_back(ScaleOp(0, scale_x, dst << num_downscales));
      }
      while (num_downscales) {
        num_downscales--;
        ops->push_back(ScaleOp(2, scale_x, dst << num_downscales));
      }
    }

    // Update |size| to its new size. Before calling this function
    // |size| should be the size of the input image. After calling it,
    // |size| will be the size of the image after this particular
    // scaling operation.
    void UpdateSize(gfx::Size* subrect) {
      if (scale_x) {
        subrect->set_width(scale_size);
      } else {
        subrect->set_height(scale_size);
      }
    }

    // A scale factor of 0 means upscale
    // 2 means 50% scale
    // 3 means 33% scale, etc.
    int scale_factor;
    bool scale_x;  // Otherwise y
    int scale_size;  // Size to scale to.
  };

  // Full specification for a single scaling stage.
  struct ScalerStage {
    ScalerStage(ShaderType shader_,
                gfx::Size src_size_,
                gfx::Rect src_subrect_,
                gfx::Size dst_size_,
                bool scale_x_,
                bool vertically_flip_texture_,
                bool swizzle_);
    ShaderType shader;
    gfx::Size src_size;
    gfx::Rect src_subrect;
    gfx::Size dst_size;
    bool scale_x;
    bool vertically_flip_texture;
    bool swizzle;
  };

  // Compute a vector of scaler stages for a particular
  // set of input/output parameters.
  void ComputeScalerStages(GLHelperBrowser::ScalerQuality quality,
                           const gfx::Size& src_size,
                           const gfx::Rect& src_subrect,
                           const gfx::Size& dst_size,
                           bool vertically_flip_texture,
                           bool swizzle,
                           std::vector<ScalerStage> *scaler_stages);

  // Take two queues of ScaleOp structs and generate a
  // vector of scaler stages. This is the second half of
  // ComputeScalerStages.
  void ConvertScalerOpsToScalerStages(
      GLHelperBrowser::ScalerQuality quality,
      gfx::Size src_size,
      gfx::Rect src_subrect,
      const gfx::Size& dst_size,
      bool vertically_flip_texture,
      bool swizzle,
      std::deque<GLHelperScalingBrowser::ScaleOp>* x_ops,
      std::deque<GLHelperScalingBrowser::ScaleOp>* y_ops,
      std::vector<ScalerStage> *scaler_stages);


  scoped_refptr<ShaderProgramBrowser> GetShaderProgramBrowser(
      ShaderType type, bool swizzle);

  // Interleaved array of 2-dimentional vertex positions (x, y) and
  // 2-dimentional texture coordinates (s, t).
  static const WebKit::WGC3Dfloat kVertexAttributes[];

  WebKit::WebGraphicsContext3D* context_;
  GLHelperBrowser* helper_;

  // The buffer that holds the vertices and the texture coordinates data for
  // drawing a quad.
  ScopedBuffer vertex_attributes_buffer_;

  std::map<ShaderProgramBrowserKeyType,
           scoped_refptr<ShaderProgramBrowser> > shader_programs_;

  friend class ShaderProgramBrowser;
  friend class ScalerImpl;
  DISALLOW_COPY_AND_ASSIGN(GLHelperScalingBrowser);
};


}  // namespace content

#endif  // CONTENT_COMMON_GPU_CLIENT_GL_HELPER_SCALING_H_
