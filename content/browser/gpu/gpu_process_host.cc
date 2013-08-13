// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/gpu/gpu_process_host.h"

#include "base/base64.h"
#include "base/base_switches.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/debug/trace_event.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/histogram.h"
#include "base/process_util.h"
#include "base/sha1.h"
#include "base/threading/thread.h"
#include "content/browser/browser_child_process_host_impl.h"
#include "content/browser/gpu/gpu_data_manager_impl.h"
#include "content/browser/gpu/gpu_process_host_ui_shim.h"
#include "content/browser/gpu/shader_disk_cache.h"
#include "content/browser/renderer_host/render_widget_helper.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/common/child_process_host_impl.h"
#include "content/common/gpu/gpu_messages.h"
#include "content/common/view_messages.h"
#include "content/gpu/gpu_child_thread.h"
#include "content/gpu/gpu_process.h"
#include "content/port/browser/render_widget_host_view_frame_subscriber.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/result_codes.h"
#include "gpu/command_buffer/common/constants.h"
#include "gpu/command_buffer/service/gpu_switches.h"
#include "ipc/ipc_channel_handle.h"
#include "ipc/ipc_switches.h"
#include "ui/gl/gl_switches.h"


#if defined(OS_WIN)
#include "base/win/windows_version.h"
#include "content/common/sandbox_win.h"
#include "content/public/common/sandboxed_process_launcher_delegate.h"
#include "sandbox/win/src/sandbox_policy.h"
#include "ui/surface/accelerated_surface_win.h"
#endif

namespace content {

bool GpuProcessHost::gpu_enabled_ = true;
bool GpuProcessHost::hardware_gpu_enabled_ = true;

namespace {

enum GPUProcessLifetimeEvent {
  LAUNCHED,
  DIED_FIRST_TIME,
  DIED_SECOND_TIME,
  DIED_THIRD_TIME,
  DIED_FOURTH_TIME,
  GPU_PROCESS_LIFETIME_EVENT_MAX = 100
};

// Indexed by GpuProcessKind. There is one of each kind maximum. This array may
// only be accessed from the IO thread.
GpuProcessHost* g_gpu_process_hosts[GpuProcessHost::GPU_PROCESS_KIND_COUNT];


void SendGpuProcessMessage(GpuProcessHost::GpuProcessKind kind,
                           CauseForGpuLaunch cause,
                           IPC::Message* message) {
  GpuProcessHost* host = GpuProcessHost::Get(kind, cause);
  if (host) {
    host->Send(message);
  } else {
    delete message;
  }
}

void AcceleratedSurfaceBuffersSwappedCompletedForGPU(int host_id,
                                                     int route_id,
                                                     bool alive,
                                                     uint64 surface_handle) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO,
        FROM_HERE,
        base::Bind(&AcceleratedSurfaceBuffersSwappedCompletedForGPU,
                   host_id,
                   route_id,
                   alive,
                   surface_handle));
    return;
  }

  GpuProcessHost* host = GpuProcessHost::FromID(host_id);
  if (host) {
    if (alive) {
      AcceleratedSurfaceMsg_BufferPresented_Params ack_params;
      ack_params.sync_point = 0;
      host->Send(
          new AcceleratedSurfaceMsg_BufferPresented(route_id, ack_params));
    } else {
      host->ForceShutdown();
    }
  }
}

#if defined(OS_WIN)
// This sends a ViewMsg_SwapBuffers_ACK directly to the renderer process
// (RenderWidget).
void AcceleratedSurfaceBuffersSwappedCompletedForRenderer(
    int surface_id,
    base::TimeTicks timebase,
    base::TimeDelta interval) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&AcceleratedSurfaceBuffersSwappedCompletedForRenderer,
                   surface_id, timebase, interval));
    return;
  }

  int render_process_id = 0;
  int render_widget_id = 0;
  if (!GpuSurfaceTracker::Get()->GetRenderWidgetIDForSurface(
      surface_id, &render_process_id, &render_widget_id)) {
    return;
  }
  RenderProcessHost* host = RenderProcessHost::FromID(render_process_id);
  if (!host)
    return;
  RenderWidgetHost* rwh = host->GetRenderWidgetHostByID(render_widget_id);
  if (!rwh)
    return;
  RenderWidgetHostImpl::From(rwh)->AcknowledgeSwapBuffersToRenderer();
  if (interval != base::TimeDelta())
    RenderWidgetHostImpl::From(rwh)->UpdateVSyncParameters(timebase, interval);
}

void AcceleratedSurfaceBuffersSwappedCompleted(int host_id,
                                               int route_id,
                                               int surface_id,
                                               uint64 surface_handle,
                                               bool alive,
                                               base::TimeTicks timebase,
                                               base::TimeDelta interval) {
  AcceleratedSurfaceBuffersSwappedCompletedForGPU(host_id, route_id,
                                                  alive, surface_handle);
  AcceleratedSurfaceBuffersSwappedCompletedForRenderer(surface_id, timebase,
                                                       interval);
}

// NOTE: changes to this class need to be reviewed by the security team.
class GpuSandboxedProcessLauncherDelegate
    : public SandboxedProcessLauncherDelegate {
 public:
  explicit GpuSandboxedProcessLauncherDelegate(CommandLine* cmd_line)
      : cmd_line_(cmd_line) {}
  virtual ~GpuSandboxedProcessLauncherDelegate() {}

  virtual void ShouldSandbox(bool* in_sandbox) OVERRIDE {
    if (cmd_line_->HasSwitch(switches::kDisableGpuSandbox)) {
      *in_sandbox = false;
      DVLOG(1) << "GPU sandbox is disabled";
    }
  }

  virtual void PreSandbox(bool* disable_default_policy,
                          base::FilePath* exposed_dir) OVERRIDE {
    *disable_default_policy = true;
  }

  // For the GPU process we gotten as far as USER_LIMITED. The next level
  // which is USER_RESTRICTED breaks both the DirectX backend and the OpenGL
  // backend. Note that the GPU process is connected to the interactive
  // desktop.
  virtual void PreSpawnTarget(sandbox::TargetPolicy* policy,
                              bool* success) {
    if (base::win::GetVersion() > base::win::VERSION_XP) {
      if (cmd_line_->GetSwitchValueASCII(switches::kUseGL) ==
          gfx::kGLImplementationDesktopName) {
        // Open GL path.
        policy->SetTokenLevel(sandbox::USER_RESTRICTED_SAME_ACCESS,
                              sandbox::USER_LIMITED);
        SetJobLevel(*cmd_line_, sandbox::JOB_UNPROTECTED, 0, policy);
        policy->SetDelayedIntegrityLevel(sandbox::INTEGRITY_LEVEL_LOW);
      } else {
        if (cmd_line_->GetSwitchValueASCII(switches::kUseGL) ==
                gfx::kGLImplementationSwiftShaderName ||
            cmd_line_->HasSwitch(switches::kReduceGpuSandbox) ||
            cmd_line_->HasSwitch(switches::kDisableImageTransportSurface)) {
          // Swiftshader path.
          policy->SetTokenLevel(sandbox::USER_RESTRICTED_SAME_ACCESS,
                                sandbox::USER_LIMITED);
        } else {
          // Angle + DirectX path.
          policy->SetTokenLevel(sandbox::USER_RESTRICTED_SAME_ACCESS,
                                sandbox::USER_RESTRICTED);
          // This is a trick to keep the GPU out of low-integrity processes. It
          // starts at low-integrity for UIPI to work, then drops below
          // low-integrity after warm-up.
          policy->SetDelayedIntegrityLevel(sandbox::INTEGRITY_LEVEL_UNTRUSTED);
        }

        // UI restrictions break when we access Windows from outside our job.
        // However, we don't want a proxy window in this process because it can
        // introduce deadlocks where the renderer blocks on the gpu, which in
        // turn blocks on the browser UI thread. So, instead we forgo a window
        // message pump entirely and just add job restrictions to prevent child
        // processes.
        SetJobLevel(*cmd_line_,
                    sandbox::JOB_LIMITED_USER,
                    JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS |
                    JOB_OBJECT_UILIMIT_DESKTOP |
                    JOB_OBJECT_UILIMIT_EXITWINDOWS |
                    JOB_OBJECT_UILIMIT_DISPLAYSETTINGS,
                    policy);

        policy->SetIntegrityLevel(sandbox::INTEGRITY_LEVEL_LOW);
      }
    } else {
      SetJobLevel(*cmd_line_, sandbox::JOB_UNPROTECTED, 0, policy);
      policy->SetTokenLevel(sandbox::USER_UNPROTECTED,
                            sandbox::USER_LIMITED);
    }

    // Allow the server side of GPU sockets, which are pipes that have
    // the "chrome.gpu" namespace and an arbitrary suffix.
    sandbox::ResultCode result = policy->AddRule(
        sandbox::TargetPolicy::SUBSYS_NAMED_PIPES,
        sandbox::TargetPolicy::NAMEDPIPES_ALLOW_ANY,
        L"\\\\.\\pipe\\chrome.gpu.*");
    if (result != sandbox::SBOX_ALL_OK) {
      *success = false;
      return;
    }

    // Block this DLL even if it is not loaded by the browser process.
    policy->AddDllToUnload(L"cmsetac.dll");

#ifdef USE_AURA
    // GPU also needs to add sections to the browser for aura
    // TODO(jschuh): refactor the GPU channel to remove this. crbug.com/128786
    result = policy->AddRule(sandbox::TargetPolicy::SUBSYS_HANDLES,
                             sandbox::TargetPolicy::HANDLES_DUP_BROKER,
                             L"Section");
    if (result != sandbox::SBOX_ALL_OK) {
      *success = false;
      return;
    }
#endif

    if (cmd_line_->HasSwitch(switches::kEnableLogging)) {
      string16 log_file_path = logging::GetLogFileFullPath();
      if (!log_file_path.empty()) {
        result = policy->AddRule(sandbox::TargetPolicy::SUBSYS_FILES,
                                 sandbox::TargetPolicy::FILES_ALLOW_ANY,
                                 log_file_path.c_str());
        if (result != sandbox::SBOX_ALL_OK) {
          *success = false;
          return;
        }
      }
    }
  }

 private:
  CommandLine* cmd_line_;
};
#endif  // defined(OS_WIN)

}  // anonymous namespace

// This class creates a GPU thread (instead of a GPU process), when running
// with --in-process-gpu or --single-process.
class GpuMainThread : public base::Thread {
 public:
  explicit GpuMainThread(const std::string& channel_id)
      : base::Thread("Chrome_InProcGpuThread"),
        channel_id_(channel_id),
        gpu_process_(NULL) {
  }

  virtual ~GpuMainThread() {
    Stop();
  }

 protected:
  virtual void Init() OVERRIDE {
    gpu_process_ = new GpuProcess();
    // The process object takes ownership of the thread object, so do not
    // save and delete the pointer.
    gpu_process_->set_main_thread(new GpuChildThread(channel_id_));
  }

  virtual void CleanUp() OVERRIDE {
    delete gpu_process_;
  }

 private:
  std::string channel_id_;
  // Deleted in CleanUp() on the gpu thread, so don't use smart pointers.
  GpuProcess* gpu_process_;

  DISALLOW_COPY_AND_ASSIGN(GpuMainThread);
};

// static
bool GpuProcessHost::ValidateHost(GpuProcessHost* host) {
  if (!host)
    return false;

  // The Gpu process is invalid if it's not using SwiftShader, the card is
  // blacklisted, and we can kill it and start over.
  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kSingleProcess) ||
      CommandLine::ForCurrentProcess()->HasSwitch(switches::kInProcessGPU) ||
      (host->valid_ &&
       (host->swiftshader_rendering_ ||
        !GpuDataManagerImpl::GetInstance()->ShouldUseSwiftShader()))) {
    return true;
  }

  host->ForceShutdown();
  return false;
}

// static
GpuProcessHost* GpuProcessHost::Get(GpuProcessKind kind,
                                    CauseForGpuLaunch cause) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  // Don't grant further access to GPU if it is not allowed.
  GpuDataManagerImpl* gpu_data_manager = GpuDataManagerImpl::GetInstance();
  DCHECK(gpu_data_manager);
  if (!gpu_data_manager->GpuAccessAllowed(NULL))
    return NULL;

  if (g_gpu_process_hosts[kind] && ValidateHost(g_gpu_process_hosts[kind]))
    return g_gpu_process_hosts[kind];

  if (cause == CAUSE_FOR_GPU_LAUNCH_NO_LAUNCH)
    return NULL;

  static int last_host_id = 0;
  int host_id;
  host_id = ++last_host_id;

  UMA_HISTOGRAM_ENUMERATION("GPU.GPUProcessLaunchCause",
                            cause,
                            CAUSE_FOR_GPU_LAUNCH_MAX_ENUM);

  GpuProcessHost* host = new GpuProcessHost(host_id, kind);
  if (host->Init())
    return host;

  delete host;
  return NULL;
}

// static
void GpuProcessHost::GetProcessHandles(
    const GpuDataManager::GetGpuProcessHandlesCallback& callback)  {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO,
        FROM_HERE,
        base::Bind(&GpuProcessHost::GetProcessHandles, callback));
    return;
  }
  std::list<base::ProcessHandle> handles;
  for (size_t i = 0; i < arraysize(g_gpu_process_hosts); ++i) {
    GpuProcessHost* host = g_gpu_process_hosts[i];
    if (host && ValidateHost(host))
      handles.push_back(host->process_->GetHandle());
  }
  BrowserThread::PostTask(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(callback, handles));
}

// static
void GpuProcessHost::SendOnIO(GpuProcessKind kind,
                              CauseForGpuLaunch cause,
                              IPC::Message* message) {
  if (!BrowserThread::PostTask(
          BrowserThread::IO, FROM_HERE,
          base::Bind(
              &SendGpuProcessMessage, kind, cause, message))) {
    delete message;
  }
}

// static
GpuProcessHost* GpuProcessHost::FromID(int host_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  for (int i = 0; i < GPU_PROCESS_KIND_COUNT; ++i) {
    GpuProcessHost* host = g_gpu_process_hosts[i];
    if (host && host->host_id_ == host_id && ValidateHost(host))
      return host;
  }

  return NULL;
}

GpuProcessHost::GpuProcessHost(int host_id, GpuProcessKind kind)
    : host_id_(host_id),
      valid_(true),
      in_process_(false),
      swiftshader_rendering_(false),
      kind_(kind),
      process_launched_(false),
      initialized_(false),
      uma_memory_stats_received_(false) {
  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kSingleProcess) ||
      CommandLine::ForCurrentProcess()->HasSwitch(switches::kInProcessGPU)) {
    in_process_ = true;
  }

  // If the 'single GPU process' policy ever changes, we still want to maintain
  // it for 'gpu thread' mode and only create one instance of host and thread.
  DCHECK(!in_process_ || g_gpu_process_hosts[kind] == NULL);

  g_gpu_process_hosts[kind] = this;

  // Post a task to create the corresponding GpuProcessHostUIShim.  The
  // GpuProcessHostUIShim will be destroyed if either the browser exits,
  // in which case it calls GpuProcessHostUIShim::DestroyAll, or the
  // GpuProcessHost is destroyed, which happens when the corresponding GPU
  // process terminates or fails to launch.
  BrowserThread::PostTask(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(base::IgnoreResult(&GpuProcessHostUIShim::Create), host_id));

  process_.reset(new BrowserChildProcessHostImpl(PROCESS_TYPE_GPU, this));
}

GpuProcessHost::~GpuProcessHost() {
  DCHECK(CalledOnValidThread());

  SendOutstandingReplies();

  // Maximum number of times the gpu process is allowed to crash in a session.
  // Once this limit is reached, any request to launch the gpu process will
  // fail.
  const int kGpuMaxCrashCount = 3;

  // Number of times the gpu process has crashed in the current browser session.
  static int gpu_crash_count = 0;
  static int gpu_recent_crash_count = 0;
  static base::Time last_gpu_crash_time;
  static bool crashed_before = false;
  static int swiftshader_crash_count = 0;

  // Ending only acts as a failure if the GPU process was actually started and
  // was intended for actual rendering (and not just checking caps or other
  // options).
  if (process_launched_ && kind_ == GPU_PROCESS_KIND_SANDBOXED) {
    if (swiftshader_rendering_) {
      UMA_HISTOGRAM_ENUMERATION("GPU.SwiftShaderLifetimeEvents",
                                DIED_FIRST_TIME + swiftshader_crash_count,
                                GPU_PROCESS_LIFETIME_EVENT_MAX);

      if (++swiftshader_crash_count >= kGpuMaxCrashCount) {
        // SwiftShader is too unstable to use. Disable it for current session.
        gpu_enabled_ = false;
      }
    } else {
      ++gpu_crash_count;
      UMA_HISTOGRAM_ENUMERATION("GPU.GPUProcessLifetimeEvents",
                                std::min(DIED_FIRST_TIME + gpu_crash_count,
                                         GPU_PROCESS_LIFETIME_EVENT_MAX - 1),
                                GPU_PROCESS_LIFETIME_EVENT_MAX);

      // Allow about 1 GPU crash per hour to be removed from the crash count,
      // so very occasional crashes won't eventually add up and prevent the
      // GPU process from launching.
      ++gpu_recent_crash_count;
      base::Time current_time = base::Time::Now();
      if (crashed_before) {
        int hours_different = (current_time - last_gpu_crash_time).InHours();
        gpu_recent_crash_count =
            std::max(0, gpu_recent_crash_count - hours_different);
      }

      crashed_before = true;
      last_gpu_crash_time = current_time;

      if (gpu_recent_crash_count >= kGpuMaxCrashCount ||
          !initialized_) {
#if !defined(OS_CHROMEOS)
        // The gpu process is too unstable to use. Disable it for current
        // session.
        hardware_gpu_enabled_ = false;
        GpuDataManagerImpl::GetInstance()->DisableHardwareAcceleration();
#endif
      }
    }
  }

  int exit_code;
  base::TerminationStatus status = process_->GetTerminationStatus(&exit_code);
  UMA_HISTOGRAM_ENUMERATION("GPU.GPUProcessTerminationStatus",
                            status,
                            base::TERMINATION_STATUS_MAX_ENUM);

  UMA_HISTOGRAM_COUNTS_100("GPU.AtExitSurfaceCount",
                           GpuSurfaceTracker::Get()->GetSurfaceCount());
  UMA_HISTOGRAM_BOOLEAN("GPU.AtExitReceivedMemoryStats",
                        uma_memory_stats_received_);

  if (uma_memory_stats_received_) {
    UMA_HISTOGRAM_COUNTS_100("GPU.AtExitManagedMemoryClientCount",
                             uma_memory_stats_.client_count);
    UMA_HISTOGRAM_COUNTS_100("GPU.AtExitContextGroupCount",
                             uma_memory_stats_.context_group_count);
    UMA_HISTOGRAM_CUSTOM_COUNTS(
        "GPU.AtExitMBytesAllocated",
        uma_memory_stats_.bytes_allocated_current / 1024 / 1024, 1, 2000, 50);
    UMA_HISTOGRAM_CUSTOM_COUNTS(
        "GPU.AtExitMBytesAllocatedMax",
        uma_memory_stats_.bytes_allocated_max / 1024 / 1024, 1, 2000, 50);
    UMA_HISTOGRAM_CUSTOM_COUNTS(
        "GPU.AtExitMBytesLimit",
        uma_memory_stats_.bytes_limit / 1024 / 1024, 1, 2000, 50);
  }

  if (status == base::TERMINATION_STATUS_NORMAL_TERMINATION ||
      status == base::TERMINATION_STATUS_ABNORMAL_TERMINATION) {
    UMA_HISTOGRAM_ENUMERATION("GPU.GPUProcessExitCode",
                              exit_code,
                              RESULT_CODE_LAST_CODE);
  }

  // In case we never started, clean up.
  while (!queued_messages_.empty()) {
    delete queued_messages_.front();
    queued_messages_.pop();
  }

  // This is only called on the IO thread so no race against the constructor
  // for another GpuProcessHost.
  if (g_gpu_process_hosts[kind_] == this)
    g_gpu_process_hosts[kind_] = NULL;

  // If there are any remaining offscreen contexts at the point the
  // GPU process exits, assume something went wrong, and block their
  // URLs from accessing client 3D APIs without prompting.
  BlockLiveOffscreenContexts();

  std::string message;
  switch (status) {
    case base::TERMINATION_STATUS_NORMAL_TERMINATION:
      message = "The GPU process exited normally. Everything is okay.";
      break;
    case base::TERMINATION_STATUS_ABNORMAL_TERMINATION:
      message = base::StringPrintf(
          "The GPU process exited with code %d.",
          exit_code);
      break;
    case base::TERMINATION_STATUS_PROCESS_WAS_KILLED:
      message = "You killed the GPU process! Why?";
      break;
    case base::TERMINATION_STATUS_PROCESS_CRASHED:
      message = "The GPU process crashed!";
      break;
    default:
      break;
  }

  BrowserThread::PostTask(BrowserThread::UI,
                          FROM_HERE,
                          base::Bind(&GpuProcessHostUIShim::Destroy,
                                     host_id_,
                                     message));
}

bool GpuProcessHost::Init() {
  init_start_time_ = base::TimeTicks::Now();

  TRACE_EVENT_INSTANT0("gpu", "LaunchGpuProcess", TRACE_EVENT_SCOPE_THREAD);

  std::string channel_id = process_->GetHost()->CreateChannel();
  if (channel_id.empty())
    return false;

  if (in_process_) {
    CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kDisableGpuWatchdog);

    in_process_gpu_thread_.reset(new GpuMainThread(channel_id));
    in_process_gpu_thread_->Start();

    OnProcessLaunched();  // Fake a callback that the process is ready.
  } else if (!LaunchGpuProcess(channel_id)) {
    return false;
  }

  if (!Send(new GpuMsg_Initialize()))
    return false;

  return true;
}

void GpuProcessHost::RouteOnUIThread(const IPC::Message& message) {
  BrowserThread::PostTask(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(&RouteToGpuProcessHostUIShimTask, host_id_, message));
}

bool GpuProcessHost::Send(IPC::Message* msg) {
  DCHECK(CalledOnValidThread());
  if (process_->GetHost()->IsChannelOpening()) {
    queued_messages_.push(msg);
    return true;
  }

  bool result = process_->Send(msg);
  if (!result) {
    // Channel is hosed, but we may not get destroyed for a while. Send
    // outstanding channel creation failures now so that the caller can restart
    // with a new process/channel without waiting.
    SendOutstandingReplies();
  }
  return result;
}

void GpuProcessHost::AddFilter(IPC::ChannelProxy::MessageFilter* filter) {
  DCHECK(CalledOnValidThread());
  process_->GetHost()->AddFilter(filter);
}

bool GpuProcessHost::OnMessageReceived(const IPC::Message& message) {
  DCHECK(CalledOnValidThread());
  IPC_BEGIN_MESSAGE_MAP(GpuProcessHost, message)
    IPC_MESSAGE_HANDLER(GpuHostMsg_Initialized, OnInitialized)
    IPC_MESSAGE_HANDLER(GpuHostMsg_ChannelEstablished, OnChannelEstablished)
    IPC_MESSAGE_HANDLER(GpuHostMsg_CommandBufferCreated, OnCommandBufferCreated)
    IPC_MESSAGE_HANDLER(GpuHostMsg_DestroyCommandBuffer, OnDestroyCommandBuffer)
    IPC_MESSAGE_HANDLER(GpuHostMsg_ImageCreated, OnImageCreated)
    IPC_MESSAGE_HANDLER(GpuHostMsg_DidCreateOffscreenContext,
                        OnDidCreateOffscreenContext)
    IPC_MESSAGE_HANDLER(GpuHostMsg_DidLoseContext, OnDidLoseContext)
    IPC_MESSAGE_HANDLER(GpuHostMsg_DidDestroyOffscreenContext,
                        OnDidDestroyOffscreenContext)
    IPC_MESSAGE_HANDLER(GpuHostMsg_GpuMemoryUmaStats,
                        OnGpuMemoryUmaStatsReceived)
#if defined(OS_MACOSX)
    IPC_MESSAGE_HANDLER(GpuHostMsg_AcceleratedSurfaceBuffersSwapped,
                        OnAcceleratedSurfaceBuffersSwapped)
#endif
#if defined(OS_WIN)
    IPC_MESSAGE_HANDLER(GpuHostMsg_AcceleratedSurfaceBuffersSwapped,
                        OnAcceleratedSurfaceBuffersSwapped)
    IPC_MESSAGE_HANDLER(GpuHostMsg_AcceleratedSurfacePostSubBuffer,
                        OnAcceleratedSurfacePostSubBuffer)
    IPC_MESSAGE_HANDLER(GpuHostMsg_AcceleratedSurfaceSuspend,
                        OnAcceleratedSurfaceSuspend)
    IPC_MESSAGE_HANDLER(GpuHostMsg_AcceleratedSurfaceRelease,
                        OnAcceleratedSurfaceRelease)
#endif
    IPC_MESSAGE_HANDLER(GpuHostMsg_DestroyChannel,
                        OnDestroyChannel)
    IPC_MESSAGE_HANDLER(GpuHostMsg_CacheShader,
                        OnCacheShader)

    IPC_MESSAGE_UNHANDLED(RouteOnUIThread(message))
  IPC_END_MESSAGE_MAP()

  return true;
}

void GpuProcessHost::OnChannelConnected(int32 peer_pid) {
  TRACE_EVENT0("gpu", "GpuProcessHost::OnChannelConnected");

  while (!queued_messages_.empty()) {
    Send(queued_messages_.front());
    queued_messages_.pop();
  }
}

void GpuProcessHost::EstablishGpuChannel(
    int client_id,
    bool share_context,
    const EstablishChannelCallback& callback) {
  DCHECK(CalledOnValidThread());
  TRACE_EVENT0("gpu", "GpuProcessHost::EstablishGpuChannel");

  // If GPU features are already blacklisted, no need to establish the channel.
  if (!GpuDataManagerImpl::GetInstance()->GpuAccessAllowed(NULL)) {
    callback.Run(IPC::ChannelHandle(), GPUInfo());
    return;
  }

  if (Send(new GpuMsg_EstablishChannel(client_id, share_context))) {
    channel_requests_.push(callback);
  } else {
    callback.Run(IPC::ChannelHandle(), GPUInfo());
  }

  if (!CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableGpuShaderDiskCache)) {
    CreateChannelCache(client_id, gpu::kDefaultMaxProgramCacheMemoryBytes);
  }
}

void GpuProcessHost::CreateViewCommandBuffer(
    const gfx::GLSurfaceHandle& compositing_surface,
    int surface_id,
    int client_id,
    const GPUCreateCommandBufferConfig& init_params,
    const CreateCommandBufferCallback& callback) {
  TRACE_EVENT0("gpu", "GpuProcessHost::CreateViewCommandBuffer");

  DCHECK(CalledOnValidThread());

  if (!compositing_surface.is_null() &&
      Send(new GpuMsg_CreateViewCommandBuffer(
          compositing_surface, surface_id, client_id, init_params))) {
    create_command_buffer_requests_.push(callback);
    surface_refs_.insert(std::make_pair(surface_id,
        GpuSurfaceTracker::GetInstance()->GetSurfaceRefForSurface(surface_id)));
  } else {
    callback.Run(MSG_ROUTING_NONE);
  }
}

void GpuProcessHost::CreateImage(gfx::PluginWindowHandle window,
                                 int client_id,
                                 int image_id,
                                 const CreateImageCallback& callback) {
  TRACE_EVENT0("gpu", "GpuProcessHost::CreateImage");

  DCHECK(CalledOnValidThread());

  if (Send(new GpuMsg_CreateImage(window, client_id, image_id))) {
    create_image_requests_.push(callback);
  } else {
    callback.Run(gfx::Size());
  }
}

void GpuProcessHost::DeleteImage(int client_id,
                                 int image_id,
                                 int sync_point) {
  TRACE_EVENT0("gpu", "GpuProcessHost::DeleteImage");

  DCHECK(CalledOnValidThread());

  Send(new GpuMsg_DeleteImage(client_id, image_id, sync_point));
}

void GpuProcessHost::OnInitialized(bool result) {
  UMA_HISTOGRAM_BOOLEAN("GPU.GPUProcessInitialized", result);
  initialized_ = result;
}

void GpuProcessHost::OnChannelEstablished(
    const IPC::ChannelHandle& channel_handle) {
  TRACE_EVENT0("gpu", "GpuProcessHost::OnChannelEstablished");

  if (channel_requests_.empty()) {
    // This happens when GPU process is compromised.
    RouteOnUIThread(GpuHostMsg_OnLogMessage(
        logging::LOG_WARNING,
        "WARNING",
        "Received a ChannelEstablished message but no requests in queue."));
    return;
  }
  EstablishChannelCallback callback = channel_requests_.front();
  channel_requests_.pop();

  // Currently if any of the GPU features are blacklisted, we don't establish a
  // GPU channel.
  if (!channel_handle.name.empty() &&
      !GpuDataManagerImpl::GetInstance()->GpuAccessAllowed(NULL)) {
    Send(new GpuMsg_CloseChannel(channel_handle));
    callback.Run(IPC::ChannelHandle(), GPUInfo());
    RouteOnUIThread(GpuHostMsg_OnLogMessage(
        logging::LOG_WARNING,
        "WARNING",
        "Hardware acceleration is unavailable."));
    return;
  }

  callback.Run(channel_handle,
               GpuDataManagerImpl::GetInstance()->GetGPUInfo());
}

void GpuProcessHost::OnCommandBufferCreated(const int32 route_id) {
  TRACE_EVENT0("gpu", "GpuProcessHost::OnCommandBufferCreated");

  if (create_command_buffer_requests_.empty())
    return;

  CreateCommandBufferCallback callback =
      create_command_buffer_requests_.front();
  create_command_buffer_requests_.pop();
  callback.Run(route_id);
}

void GpuProcessHost::OnDestroyCommandBuffer(int32 surface_id) {
  TRACE_EVENT0("gpu", "GpuProcessHost::OnDestroyCommandBuffer");
  SurfaceRefMap::iterator it = surface_refs_.find(surface_id);
  if (it != surface_refs_.end()) {
    surface_refs_.erase(it);
  }
}

void GpuProcessHost::OnImageCreated(const gfx::Size size) {
  TRACE_EVENT0("gpu", "GpuProcessHost::OnImageCreated");

  if (create_image_requests_.empty())
    return;

  CreateImageCallback callback = create_image_requests_.front();
  create_image_requests_.pop();
  callback.Run(size);
}

void GpuProcessHost::OnDidCreateOffscreenContext(const GURL& url) {
  urls_with_live_offscreen_contexts_.insert(url);
}

void GpuProcessHost::OnDidLoseContext(bool offscreen,
                                      gpu::error::ContextLostReason reason,
                                      const GURL& url) {
  // TODO(kbr): would be nice to see the "offscreen" flag too.
  TRACE_EVENT2("gpu", "GpuProcessHost::OnDidLoseContext",
               "reason", reason,
               "url",
               url.possibly_invalid_spec());

  if (!offscreen || url.is_empty()) {
    // Assume that the loss of the compositor's or accelerated canvas'
    // context is a serious event and blame the loss on all live
    // offscreen contexts. This more robustly handles situations where
    // the GPU process may not actually detect the context loss in the
    // offscreen context.
    BlockLiveOffscreenContexts();
    return;
  }

  GpuDataManagerImpl::DomainGuilt guilt;
  switch (reason) {
    case gpu::error::kGuilty:
      guilt = GpuDataManagerImpl::DOMAIN_GUILT_KNOWN;
      break;
    case gpu::error::kUnknown:
      guilt = GpuDataManagerImpl::DOMAIN_GUILT_UNKNOWN;
      break;
    case gpu::error::kInnocent:
      return;
    default:
      NOTREACHED();
      return;
  }

  GpuDataManagerImpl::GetInstance()->BlockDomainFrom3DAPIs(url, guilt);
}

void GpuProcessHost::OnDidDestroyOffscreenContext(const GURL& url) {
  urls_with_live_offscreen_contexts_.erase(url);
}

void GpuProcessHost::OnGpuMemoryUmaStatsReceived(
    const GPUMemoryUmaStats& stats) {
  TRACE_EVENT0("gpu", "GpuProcessHost::OnGpuMemoryUmaStatsReceived");
  uma_memory_stats_received_ = true;
  uma_memory_stats_ = stats;
}

#if defined(OS_MACOSX)
void GpuProcessHost::OnAcceleratedSurfaceBuffersSwapped(
    const GpuHostMsg_AcceleratedSurfaceBuffersSwapped_Params& params) {
  TRACE_EVENT0("gpu", "GpuProcessHost::OnAcceleratedSurfaceBuffersSwapped");

  gfx::GLSurfaceHandle surface_handle =
      GpuSurfaceTracker::Get()->GetSurfaceHandle(params.surface_id);
  // Compositor window is always gfx::kNullPluginWindow.
  // TODO(jbates) http://crbug.com/105344 This will be removed when there are no
  // plugin windows.
  if (surface_handle.handle != gfx::kNullPluginWindow ||
      surface_handle.transport_type == gfx::TEXTURE_TRANSPORT) {
    RouteOnUIThread(GpuHostMsg_AcceleratedSurfaceBuffersSwapped(params));
    return;
  }

  base::ScopedClosureRunner scoped_completion_runner(
      base::Bind(&AcceleratedSurfaceBuffersSwappedCompletedForGPU,
                 host_id_, params.route_id,
                 true /* alive */, params.surface_handle));

  int render_process_id = 0;
  int render_widget_id = 0;
  if (!GpuSurfaceTracker::Get()->GetRenderWidgetIDForSurface(
      params.surface_id, &render_process_id, &render_widget_id)) {
    return;
  }
  RenderWidgetHelper* helper =
      RenderWidgetHelper::FromProcessHostID(render_process_id);
  if (!helper)
    return;

  // Pass the SwapBuffers on to the RenderWidgetHelper to wake up the UI thread
  // if the browser is waiting for a new frame. Otherwise the RenderWidgetHelper
  // will forward to the RenderWidgetHostView via RenderProcessHostImpl and
  // RenderWidgetHostImpl.
  scoped_completion_runner.Release();
  helper->DidReceiveBackingStoreMsg(ViewHostMsg_CompositorSurfaceBuffersSwapped(
      render_widget_id,
      params.surface_id,
      params.surface_handle,
      params.route_id,
      params.size,
      host_id_));
}
#endif  // OS_MACOSX

#if defined(OS_WIN)
void GpuProcessHost::OnAcceleratedSurfaceBuffersSwapped(
    const GpuHostMsg_AcceleratedSurfaceBuffersSwapped_Params& params) {
  TRACE_EVENT0("gpu", "GpuProcessHost::OnAcceleratedSurfaceBuffersSwapped");

  base::ScopedClosureRunner scoped_completion_runner(
      base::Bind(&AcceleratedSurfaceBuffersSwappedCompleted,
          host_id_, params.route_id, params.surface_id, params.surface_handle,
          true, base::TimeTicks(), base::TimeDelta()));

  gfx::GLSurfaceHandle handle =
      GpuSurfaceTracker::Get()->GetSurfaceHandle(params.surface_id);

  if (handle.is_null())
    return;

  if (handle.transport_type == gfx::TEXTURE_TRANSPORT) {
    TRACE_EVENT1("gpu", "SurfaceIDNotFound_RoutingToUI",
                 "surface_id", params.surface_id);
    // This is a content area swap, send it on to the UI thread.
    scoped_completion_runner.Release();
    RouteOnUIThread(GpuHostMsg_AcceleratedSurfaceBuffersSwapped(params));
    return;
  }

  // Otherwise it's the UI swap.

  scoped_refptr<AcceleratedPresenter> presenter(
      AcceleratedPresenter::GetForWindow(handle.handle));
  if (!presenter) {
    TRACE_EVENT1("gpu",
                 "EarlyOut_NativeWindowNotFound",
                 "handle",
                 handle.handle);
    return;
  }

  scoped_completion_runner.Release();
  presenter->AsyncPresentAndAcknowledge(
      params.size,
      params.surface_handle,
      base::Bind(&AcceleratedSurfaceBuffersSwappedCompleted,
                 host_id_,
                 params.route_id,
                 params.surface_id,
                 params.surface_handle));

  FrameSubscriberMap::iterator it = frame_subscribers_.find(params.surface_id);
  if (it != frame_subscribers_.end() && it->second) {
    const base::Time present_time = base::Time::Now();
    scoped_refptr<media::VideoFrame> target_frame;
    RenderWidgetHostViewFrameSubscriber::DeliverFrameCallback copy_callback;
    if (it->second->ShouldCaptureFrame(present_time,
                                       &target_frame, &copy_callback)) {
      // It is a potential improvement to do the copy in present, but we use a
      // simpler approach for now.
      presenter->AsyncCopyToVideoFrame(
          gfx::Rect(params.size), target_frame,
          base::Bind(copy_callback, present_time));
    }
  }
}

void GpuProcessHost::OnAcceleratedSurfacePostSubBuffer(
    const GpuHostMsg_AcceleratedSurfacePostSubBuffer_Params& params) {
  TRACE_EVENT0("gpu", "GpuProcessHost::OnAcceleratedSurfacePostSubBuffer");

  NOTIMPLEMENTED();
}

void GpuProcessHost::OnAcceleratedSurfaceSuspend(int32 surface_id) {
  TRACE_EVENT0("gpu", "GpuProcessHost::OnAcceleratedSurfaceSuspend");

  gfx::PluginWindowHandle handle =
      GpuSurfaceTracker::Get()->GetSurfaceHandle(surface_id).handle;

  if (!handle) {
#if defined(USE_AURA)
    RouteOnUIThread(GpuHostMsg_AcceleratedSurfaceSuspend(surface_id));
#endif
    return;
  }

  scoped_refptr<AcceleratedPresenter> presenter(
      AcceleratedPresenter::GetForWindow(handle));
  if (!presenter)
    return;

  presenter->Suspend();
}

void GpuProcessHost::OnAcceleratedSurfaceRelease(
    const GpuHostMsg_AcceleratedSurfaceRelease_Params& params) {
  TRACE_EVENT0("gpu", "GpuProcessHost::OnAcceleratedSurfaceRelease");

  gfx::PluginWindowHandle handle =
      GpuSurfaceTracker::Get()->GetSurfaceHandle(params.surface_id).handle;
  if (!handle) {
#if defined(USE_AURA)
    RouteOnUIThread(GpuHostMsg_AcceleratedSurfaceRelease(params));
    return;
#endif
  }

  scoped_refptr<AcceleratedPresenter> presenter(
      AcceleratedPresenter::GetForWindow(handle));
  if (!presenter)
    return;

  presenter->ReleaseSurface();
}

#endif  // OS_WIN

void GpuProcessHost::OnProcessLaunched() {
  UMA_HISTOGRAM_TIMES("GPU.GPUProcessLaunchTime",
                      base::TimeTicks::Now() - init_start_time_);
}

void GpuProcessHost::OnProcessCrashed(int exit_code) {
  SendOutstandingReplies();
  GpuDataManagerImpl::GetInstance()->ProcessCrashed(
      process_->GetTerminationStatus(NULL));
}

GpuProcessHost::GpuProcessKind GpuProcessHost::kind() {
  return kind_;
}

void GpuProcessHost::ForceShutdown() {
  // This is only called on the IO thread so no race against the constructor
  // for another GpuProcessHost.
  if (g_gpu_process_hosts[kind_] == this)
    g_gpu_process_hosts[kind_] = NULL;

  process_->ForceShutdown();
}

void GpuProcessHost::BeginFrameSubscription(
    int surface_id,
    base::WeakPtr<RenderWidgetHostViewFrameSubscriber> subscriber) {
  frame_subscribers_[surface_id] = subscriber;
}

void GpuProcessHost::EndFrameSubscription(int surface_id) {
  frame_subscribers_.erase(surface_id);
}

bool GpuProcessHost::LaunchGpuProcess(const std::string& channel_id) {
  if (!(gpu_enabled_ &&
      GpuDataManagerImpl::GetInstance()->ShouldUseSwiftShader()) &&
      !hardware_gpu_enabled_) {
    SendOutstandingReplies();
    return false;
  }

  const CommandLine& browser_command_line = *CommandLine::ForCurrentProcess();

  CommandLine::StringType gpu_launcher =
      browser_command_line.GetSwitchValueNative(switches::kGpuLauncher);

#if defined(OS_LINUX)
  int child_flags = gpu_launcher.empty() ? ChildProcessHost::CHILD_ALLOW_SELF :
                                           ChildProcessHost::CHILD_NORMAL;
#else
  int child_flags = ChildProcessHost::CHILD_NORMAL;
#endif

  base::FilePath exe_path = ChildProcessHost::GetChildPath(child_flags);
  if (exe_path.empty())
    return false;

  CommandLine* cmd_line = new CommandLine(exe_path);
  cmd_line->AppendSwitchASCII(switches::kProcessType, switches::kGpuProcess);
  cmd_line->AppendSwitchASCII(switches::kProcessChannelID, channel_id);

  if (kind_ == GPU_PROCESS_KIND_UNSANDBOXED)
    cmd_line->AppendSwitch(switches::kDisableGpuSandbox);

  // Propagate relevant command line switches.
  static const char* const kSwitchNames[] = {
    switches::kDisableAcceleratedVideoDecode,
    switches::kDisableBreakpad,
    switches::kDisableGLMultisampling,
    switches::kDisableGpuSandbox,
    switches::kDisableGpuWatchdog,
    switches::kDisableImageTransportSurface,
    switches::kDisableLogging,
    switches::kDisableSeccompFilterSandbox,
    switches::kEnableGpuSandbox,
    switches::kEnableLogging,
    switches::kEnableVirtualGLContexts,
    switches::kGpuStartupDialog,
    switches::kLoggingLevel,
    switches::kNoSandbox,
    switches::kReduceGpuSandbox,
    switches::kTestGLLib,
    switches::kTraceStartup,
    switches::kUseExynosVda,
    switches::kV,
    switches::kVModule,
#if defined(OS_MACOSX)
    switches::kEnableSandboxLogging,
#endif
#if defined(USE_AURA)
    switches::kUIPrioritizeInGpuProcess,
#endif
  };
  cmd_line->CopySwitchesFrom(browser_command_line, kSwitchNames,
                             arraysize(kSwitchNames));
  cmd_line->CopySwitchesFrom(
      browser_command_line, switches::kGpuSwitches, switches::kNumGpuSwitches);
  cmd_line->CopySwitchesFrom(
      browser_command_line, switches::kGLSwitchesCopiedFromGpuProcessHost,
      switches::kGLSwitchesCopiedFromGpuProcessHostNumSwitches);

  GetContentClient()->browser()->AppendExtraCommandLineSwitches(
      cmd_line, process_->GetData().id);

  GpuDataManagerImpl::GetInstance()->AppendGpuCommandLine(cmd_line);

  if (cmd_line->HasSwitch(switches::kUseGL)) {
    swiftshader_rendering_ =
        (cmd_line->GetSwitchValueASCII(switches::kUseGL) == "swiftshader");
  }

  UMA_HISTOGRAM_BOOLEAN("GPU.GPU.GPUProcessSoftwareRendering",
                        swiftshader_rendering_);

  // If specified, prepend a launcher program to the command line.
  if (!gpu_launcher.empty())
    cmd_line->PrependWrapper(gpu_launcher);

  process_->Launch(
#if defined(OS_WIN)
      new GpuSandboxedProcessLauncherDelegate(cmd_line),
#elif defined(OS_POSIX)
      false,
      base::EnvironmentVector(),
#endif
      cmd_line);
  process_launched_ = true;

  UMA_HISTOGRAM_ENUMERATION("GPU.GPUProcessLifetimeEvents",
                            LAUNCHED, GPU_PROCESS_LIFETIME_EVENT_MAX);
  return true;
}

void GpuProcessHost::SendOutstandingReplies() {
  valid_ = false;
  // First send empty channel handles for all EstablishChannel requests.
  while (!channel_requests_.empty()) {
    EstablishChannelCallback callback = channel_requests_.front();
    channel_requests_.pop();
    callback.Run(IPC::ChannelHandle(), GPUInfo());
  }

  while (!create_command_buffer_requests_.empty()) {
    CreateCommandBufferCallback callback =
        create_command_buffer_requests_.front();
    create_command_buffer_requests_.pop();
    callback.Run(MSG_ROUTING_NONE);
  }
}

void GpuProcessHost::BlockLiveOffscreenContexts() {
  for (std::multiset<GURL>::iterator iter =
           urls_with_live_offscreen_contexts_.begin();
       iter != urls_with_live_offscreen_contexts_.end(); ++iter) {
    GpuDataManagerImpl::GetInstance()->BlockDomainFrom3DAPIs(
        *iter, GpuDataManagerImpl::DOMAIN_GUILT_UNKNOWN);
  }
}

std::string GpuProcessHost::GetShaderPrefixKey() {
  if (shader_prefix_key_.empty()) {
    GPUInfo info = GpuDataManagerImpl::GetInstance()->GetGPUInfo();

    std::string in_str = GetContentClient()->GetProduct() + "-" +
        info.gl_vendor + "-" + info.gl_renderer + "-" +
        info.driver_version + "-" + info.driver_vendor;

    base::Base64Encode(base::SHA1HashString(in_str), &shader_prefix_key_);
  }

  return shader_prefix_key_;
}

void GpuProcessHost::LoadedShader(const std::string& key,
                                  const std::string& data) {
  std::string prefix = GetShaderPrefixKey();
  if (!key.compare(0, prefix.length(), prefix))
    Send(new GpuMsg_LoadedShader(data));
}

void GpuProcessHost::CreateChannelCache(int32 client_id, size_t cache_size) {
  TRACE_EVENT0("gpu", "GpuProcessHost::CreateChannelCache");

  scoped_refptr<ShaderDiskCache> cache =
      ShaderCacheFactory::GetInstance()->Get(client_id);
  if (!cache)
    return;

  cache->set_max_cache_size(cache_size);
  cache->set_host_id(host_id_);

  client_id_to_shader_cache_[client_id] = cache;
}

void GpuProcessHost::OnDestroyChannel(int32 client_id) {
  TRACE_EVENT0("gpu", "GpuProcessHost::OnDestroyChannel");
  client_id_to_shader_cache_.erase(client_id);
}

void GpuProcessHost::OnCacheShader(int32 client_id,
                                   const std::string& key,
                                   const std::string& shader) {
  TRACE_EVENT0("gpu", "GpuProcessHost::OnCacheShader");
  ClientIdToShaderCacheMap::iterator iter =
      client_id_to_shader_cache_.find(client_id);
  // If the cache doesn't exist then this is an off the record profile.
  if (iter == client_id_to_shader_cache_.end())
    return;
  iter->second->Cache(GetShaderPrefixKey() + ":" + key, shader);
}

}  // namespace content
