// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/gpu/media/gpu_video_decode_accelerator.h"

#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/message_loop/message_loop_proxy.h"
#include "base/stl_util.h"

#include "content/common/gpu/gpu_channel.h"
#include "content/common/gpu/gpu_messages.h"
#include "content/public/common/content_switches.h"
#include "gpu/command_buffer/common/command_buffer.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_message_utils.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_surface_egl.h"

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#include "content/common/gpu/media/dxva_video_decode_accelerator.h"
#elif defined(OS_CHROMEOS) && defined(ARCH_CPU_ARMEL) && defined(USE_X11)
#include "content/common/gpu/media/exynos_video_decode_accelerator.h"
#elif defined(OS_CHROMEOS) && defined(ARCH_CPU_X86_FAMILY) && defined(USE_X11)
#include "ui/gl/gl_context_glx.h"
#include "content/common/gpu/media/vaapi_video_decode_accelerator.h"
#elif defined(OS_TIZEN_MOBILE) && defined(ARCH_CPU_X86_FAMILY)
#include "content/common/gpu/media/vaapi_video_decode_accelerator_tizen.h"
#elif defined(OS_ANDROID)
#include "content/common/gpu/media/android_video_decode_accelerator.h"
#endif

#include "gpu/command_buffer/service/texture_manager.h"
#include "ui/gfx/size.h"

using gpu::gles2::TextureManager;

namespace content {

static bool MakeDecoderContextCurrent(
    const base::WeakPtr<GpuCommandBufferStub> stub) {
  if (!stub.get()) {
    DLOG(ERROR) << "Stub is gone; won't MakeCurrent().";
    return false;
  }

  if (!stub->decoder()->MakeCurrent()) {
    DLOG(ERROR) << "Failed to MakeCurrent()";
    return false;
  }

  return true;
}

class GpuVideoDecodeAccelerator::MessageFilter
    : public IPC::ChannelProxy::MessageFilter {
 public:
  MessageFilter(GpuVideoDecodeAccelerator* owner, int32 host_route_id)
      : owner_(owner), host_route_id_(host_route_id) {}

  virtual void OnChannelError() OVERRIDE { channel_ = NULL; }

  virtual void OnChannelClosing() OVERRIDE { channel_ = NULL; }

  virtual void OnFilterAdded(IPC::Channel* channel) OVERRIDE {
    channel_ = channel;
  }

  virtual void OnFilterRemoved() OVERRIDE {
    // This will delete |owner_| and |this|.
    owner_->OnFilterRemoved();
  }

  virtual bool OnMessageReceived(const IPC::Message& msg) OVERRIDE {
    if (msg.routing_id() != host_route_id_)
      return false;

    IPC_BEGIN_MESSAGE_MAP(MessageFilter, msg)
      IPC_MESSAGE_FORWARD(AcceleratedVideoDecoderMsg_Decode, owner_,
                          GpuVideoDecodeAccelerator::OnDecode)
      IPC_MESSAGE_UNHANDLED(return false;)
    IPC_END_MESSAGE_MAP()
    return true;
  }

  bool SendOnIOThread(IPC::Message* message) {
    DCHECK(!message->is_sync());
    if (!channel_) {
      delete message;
      return false;
    }
    return channel_->Send(message);
  }

 protected:
  virtual ~MessageFilter() {}

 private:
  GpuVideoDecodeAccelerator* owner_;
  int32 host_route_id_;
  // The channel to which this filter was added.
  IPC::Channel* channel_;
};

GpuVideoDecodeAccelerator::GpuVideoDecodeAccelerator(
    int32 host_route_id,
    GpuCommandBufferStub* stub,
    const scoped_refptr<base::MessageLoopProxy>& io_message_loop)
    : init_done_msg_(NULL),
      host_route_id_(host_route_id),
      stub_(stub),
      texture_target_(0),
      io_message_loop_(io_message_loop),
      weak_factory_for_io_(this) {
  DCHECK(stub_);
  stub_->AddDestructionObserver(this);
  stub_->channel()->AddRoute(host_route_id_, this);
  child_message_loop_ = base::MessageLoopProxy::current();
  make_context_current_ =
      base::Bind(&MakeDecoderContextCurrent, stub_->AsWeakPtr());
}

GpuVideoDecodeAccelerator::~GpuVideoDecodeAccelerator() {
  if (video_decode_accelerator_)
    video_decode_accelerator_.release()->Destroy();
}

bool GpuVideoDecodeAccelerator::OnMessageReceived(const IPC::Message& msg) {
  DCHECK(stub_);
  if (!video_decode_accelerator_)
    return false;
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(GpuVideoDecodeAccelerator, msg)
    IPC_MESSAGE_HANDLER(AcceleratedVideoDecoderMsg_Decode, OnDecode)
    IPC_MESSAGE_HANDLER(AcceleratedVideoDecoderMsg_AssignPictureBuffers,
                        OnAssignPictureBuffers)
    IPC_MESSAGE_HANDLER(AcceleratedVideoDecoderMsg_ReusePictureBuffer,
                        OnReusePictureBuffer)
    IPC_MESSAGE_HANDLER(AcceleratedVideoDecoderMsg_Flush, OnFlush)
    IPC_MESSAGE_HANDLER(AcceleratedVideoDecoderMsg_Reset, OnReset)
    IPC_MESSAGE_HANDLER(AcceleratedVideoDecoderMsg_Destroy, OnDestroy)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void GpuVideoDecodeAccelerator::ProvidePictureBuffers(
    uint32 requested_num_of_buffers,
    const gfx::Size& dimensions,
    uint32 texture_target) {
  if (!Send(new AcceleratedVideoDecoderHostMsg_ProvidePictureBuffers(
          host_route_id_, requested_num_of_buffers, dimensions,
          texture_target))) {
    DLOG(ERROR) << "Send(AcceleratedVideoDecoderHostMsg_ProvidePictureBuffers) "
                << "failed";
  }
  texture_target_ = texture_target;
}

void GpuVideoDecodeAccelerator::DismissPictureBuffer(
    int32 picture_buffer_id) {
  // Notify client that picture buffer is now unused.
  if (!Send(new AcceleratedVideoDecoderHostMsg_DismissPictureBuffer(
          host_route_id_, picture_buffer_id))) {
    DLOG(ERROR) << "Send(AcceleratedVideoDecoderHostMsg_DismissPictureBuffer) "
                << "failed";
  }
}

void GpuVideoDecodeAccelerator::PictureReady(
    const media::Picture& picture) {
  if (!Send(new AcceleratedVideoDecoderHostMsg_PictureReady(
          host_route_id_,
          picture.picture_buffer_id(),
          picture.bitstream_buffer_id()))) {
    DLOG(ERROR) << "Send(AcceleratedVideoDecoderHostMsg_PictureReady) failed";
  }
}

void GpuVideoDecodeAccelerator::NotifyError(
    media::VideoDecodeAccelerator::Error error) {
  if (init_done_msg_) {
    // If we get an error while we're initializing, NotifyInitializeDone won't
    // be called, so we need to send the reply (with an error) here.
    GpuCommandBufferMsg_CreateVideoDecoder::WriteReplyParams(
        init_done_msg_, -1);
    if (!Send(init_done_msg_))
      DLOG(ERROR) << "Send(init_done_msg_) failed";
    init_done_msg_ = NULL;
    return;
  }
  if (!Send(new AcceleratedVideoDecoderHostMsg_ErrorNotification(
          host_route_id_, error))) {
    DLOG(ERROR) << "Send(AcceleratedVideoDecoderHostMsg_ErrorNotification) "
                << "failed";
  }
}

void GpuVideoDecodeAccelerator::Initialize(
    const media::VideoCodecProfile profile,
    IPC::Message* init_done_msg) {
  DCHECK(stub_);
  DCHECK(!video_decode_accelerator_.get());
  DCHECK(!init_done_msg_);
  DCHECK(init_done_msg);
  init_done_msg_ = init_done_msg;

#if !defined(OS_WIN)
  // Ensure we will be able to get a GL context at all before initializing
  // non-Windows VDAs.
  if (!make_context_current_.Run()) {
    NotifyError(media::VideoDecodeAccelerator::PLATFORM_FAILURE);
    return;
  }
#endif

#if defined(OS_WIN)
  if (base::win::GetVersion() < base::win::VERSION_WIN7) {
    NOTIMPLEMENTED() << "HW video decode acceleration not available.";
    NotifyError(media::VideoDecodeAccelerator::PLATFORM_FAILURE);
    return;
  }
  DLOG(INFO) << "Initializing DXVA HW decoder for windows.";
  video_decode_accelerator_.reset(new DXVAVideoDecodeAccelerator(
      this, make_context_current_));
#elif defined(OS_CHROMEOS) && defined(ARCH_CPU_ARMEL) && defined(USE_X11)
  video_decode_accelerator_.reset(new ExynosVideoDecodeAccelerator(
      gfx::GLSurfaceEGL::GetHardwareDisplay(),
      stub_->decoder()->GetGLContext()->GetHandle(),
      this,
      weak_factory_for_io_.GetWeakPtr(),
      make_context_current_,
      io_message_loop_));
#elif defined(OS_CHROMEOS) && defined(ARCH_CPU_X86_FAMILY) && defined(USE_X11)
  gfx::GLContextGLX* glx_context =
      static_cast<gfx::GLContextGLX*>(stub_->decoder()->GetGLContext());
  GLXContext glx_context_handle =
      static_cast<GLXContext>(glx_context->GetHandle());
  video_decode_accelerator_.reset(new VaapiVideoDecodeAccelerator(
      glx_context->display(), glx_context_handle, this,
      make_context_current_));
#elif defined(OS_TIZEN_MOBILE) && defined(ARCH_CPU_X86_FAMILY)
  video_decode_accelerator_.reset(new VaapiVideoDecodeAccelerator(
      gfx::GLSurfaceEGL::GetHardwareDisplay(),
      stub_->decoder()->GetGLContext()->GetHandle(),
      this,
      make_context_current_));
#elif defined(OS_ANDROID)
  video_decode_accelerator_.reset(new AndroidVideoDecodeAccelerator(
      this,
      stub_->decoder()->AsWeakPtr(),
      make_context_current_));
#else
  NOTIMPLEMENTED() << "HW video decode acceleration not available.";
  NotifyError(media::VideoDecodeAccelerator::PLATFORM_FAILURE);
  return;
#endif

  if (video_decode_accelerator_->CanDecodeOnIOThread()) {
    filter_ = new MessageFilter(this, host_route_id_);
    stub_->channel()->AddFilter(filter_.get());
  }

  if (!video_decode_accelerator_->Initialize(profile))
    NotifyError(media::VideoDecodeAccelerator::PLATFORM_FAILURE);
}

// Runs on IO thread if video_decode_accelerator_->CanDecodeOnIOThread() is
// true, otherwise on the main thread.
void GpuVideoDecodeAccelerator::OnDecode(
    base::SharedMemoryHandle handle, int32 id, uint32 size) {
  DCHECK(video_decode_accelerator_.get());
  if (id < 0) {
    DLOG(FATAL) << "BitstreamBuffer id " << id << " out of range";
    if (child_message_loop_->BelongsToCurrentThread()) {
      NotifyError(media::VideoDecodeAccelerator::INVALID_ARGUMENT);
    } else {
      child_message_loop_->PostTask(
          FROM_HERE,
          base::Bind(&GpuVideoDecodeAccelerator::NotifyError,
                     base::Unretained(this),
                     media::VideoDecodeAccelerator::INVALID_ARGUMENT));
    }
    return;
  }
  video_decode_accelerator_->Decode(media::BitstreamBuffer(id, handle, size));
}

void GpuVideoDecodeAccelerator::OnAssignPictureBuffers(
      const std::vector<int32>& buffer_ids,
      const std::vector<uint32>& texture_ids,
      const std::vector<gfx::Size>& sizes) {
  DCHECK(stub_);
  if (buffer_ids.size() != texture_ids.size() ||
      buffer_ids.size() != sizes.size()) {
    NotifyError(media::VideoDecodeAccelerator::INVALID_ARGUMENT);
    return;
  }

  gpu::gles2::GLES2Decoder* command_decoder = stub_->decoder();
  gpu::gles2::TextureManager* texture_manager =
      command_decoder->GetContextGroup()->texture_manager();

  std::vector<media::PictureBuffer> buffers;
  for (uint32 i = 0; i < buffer_ids.size(); ++i) {
    if (buffer_ids[i] < 0) {
      DLOG(FATAL) << "Buffer id " << buffer_ids[i] << " out of range";
      NotifyError(media::VideoDecodeAccelerator::INVALID_ARGUMENT);
      return;
    }
    gpu::gles2::TextureRef* texture_ref = texture_manager->GetTexture(
        texture_ids[i]);
    if (!texture_ref) {
      DLOG(FATAL) << "Failed to find texture id " << texture_ids[i];
      NotifyError(media::VideoDecodeAccelerator::INVALID_ARGUMENT);
      return;
    }
    gpu::gles2::Texture* info = texture_ref->texture();
    if (info->target() != texture_target_) {
      DLOG(FATAL) << "Texture target mismatch for texture id "
                  << texture_ids[i];
      NotifyError(media::VideoDecodeAccelerator::INVALID_ARGUMENT);
      return;
    }
    // GL_TEXTURE_EXTERNAL_OES textures have their dimensions defined by the
    // underlying EGLImage.
    if (texture_target_ != GL_TEXTURE_EXTERNAL_OES) {
      GLsizei width = 0, height = 0;
      info->GetLevelSize(texture_target_, 0, &width, &height);
      if (width != sizes[i].width() || height != sizes[i].height()) {
        DLOG(FATAL) << "Size mismatch for texture id " << texture_ids[i];
        NotifyError(media::VideoDecodeAccelerator::INVALID_ARGUMENT);
        return;
      }
    }
    if (!texture_manager->ClearRenderableLevels(command_decoder, texture_ref)) {
      DLOG(FATAL) << "Failed to Clear texture id " << texture_ids[i];
      NotifyError(media::VideoDecodeAccelerator::PLATFORM_FAILURE);
      return;
    }
    uint32 service_texture_id;
    if (!command_decoder->GetServiceTextureId(
            texture_ids[i], &service_texture_id)) {
      DLOG(FATAL) << "Failed to translate texture!";
      NotifyError(media::VideoDecodeAccelerator::PLATFORM_FAILURE);
      return;
    }
    buffers.push_back(media::PictureBuffer(
        buffer_ids[i], sizes[i], service_texture_id));
  }
  video_decode_accelerator_->AssignPictureBuffers(buffers);
}

void GpuVideoDecodeAccelerator::OnReusePictureBuffer(
    int32 picture_buffer_id) {
  DCHECK(video_decode_accelerator_.get());
  video_decode_accelerator_->ReusePictureBuffer(picture_buffer_id);
}

void GpuVideoDecodeAccelerator::OnFlush() {
  DCHECK(video_decode_accelerator_.get());
  video_decode_accelerator_->Flush();
}

void GpuVideoDecodeAccelerator::OnReset() {
  DCHECK(video_decode_accelerator_.get());
  video_decode_accelerator_->Reset();
}

void GpuVideoDecodeAccelerator::OnDestroy() {
  DCHECK(video_decode_accelerator_.get());
  DCHECK(stub_);
  stub_->channel()->RemoveRoute(host_route_id_);
  stub_->RemoveDestructionObserver(this);
  if (filter_.get()) {
    // Remove the filter first because the member variables can be accessed on
    // IO thread. When filter is removed, OnFilterRemoved will delete |this|.
    stub_->channel()->RemoveFilter(filter_.get());
  } else {
    delete this;
  }
}

void GpuVideoDecodeAccelerator::OnFilterRemoved() {
  // We're destroying; cancel all callbacks.
  weak_factory_for_io_.InvalidateWeakPtrs();
  child_message_loop_->DeleteSoon(FROM_HERE, this);
}

void GpuVideoDecodeAccelerator::NotifyEndOfBitstreamBuffer(
    int32 bitstream_buffer_id) {
  if (!Send(new AcceleratedVideoDecoderHostMsg_BitstreamBufferProcessed(
          host_route_id_, bitstream_buffer_id))) {
    DLOG(ERROR)
        << "Send(AcceleratedVideoDecoderHostMsg_BitstreamBufferProcessed) "
        << "failed";
  }
}

void GpuVideoDecodeAccelerator::NotifyInitializeDone() {
  GpuCommandBufferMsg_CreateVideoDecoder::WriteReplyParams(
      init_done_msg_, host_route_id_);
  if (!Send(init_done_msg_))
    DLOG(ERROR) << "Send(init_done_msg_) failed";
  init_done_msg_ = NULL;
}

void GpuVideoDecodeAccelerator::NotifyFlushDone() {
  if (!Send(new AcceleratedVideoDecoderHostMsg_FlushDone(host_route_id_)))
    DLOG(ERROR) << "Send(AcceleratedVideoDecoderHostMsg_FlushDone) failed";
}

void GpuVideoDecodeAccelerator::NotifyResetDone() {
  if (!Send(new AcceleratedVideoDecoderHostMsg_ResetDone(host_route_id_)))
    DLOG(ERROR) << "Send(AcceleratedVideoDecoderHostMsg_ResetDone) failed";
}

void GpuVideoDecodeAccelerator::OnWillDestroyStub() { OnDestroy(); }

bool GpuVideoDecodeAccelerator::Send(IPC::Message* message) {
  DCHECK(stub_);
  if (filter_.get() && io_message_loop_->BelongsToCurrentThread())
    return filter_->SendOnIOThread(message);
  DCHECK(child_message_loop_->BelongsToCurrentThread());
  return stub_->channel()->Send(message);
}

}  // namespace content
