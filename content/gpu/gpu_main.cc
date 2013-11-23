// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdlib.h>

#if defined(OS_WIN)
#include <dwmapi.h>
#include <windows.h>
#endif

#include "base/debug/trace_event.h"
#include "base/lazy_instance.h"
#include "base/message_loop/message_loop.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/threading/platform_thread.h"
#include "build/build_config.h"
#include "content/child/child_process.h"
#include "content/common/content_constants_internal.h"
#include "content/common/gpu/gpu_config.h"
#include "content/common/gpu/gpu_messages.h"
#include "content/common/sandbox_linux.h"
#include "content/gpu/gpu_child_thread.h"
#include "content/gpu/gpu_process.h"
#include "content/gpu/gpu_watchdog_thread.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/main_function_params.h"
#include "crypto/hmac.h"
#include "gpu/config/gpu_info_collector.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/gl_switches.h"
#include "ui/gl/gpu_switching_manager.h"

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#include "base/win/scoped_com_initializer.h"
#include "content/common/gpu/media/dxva_video_decode_accelerator.h"
#include "sandbox/win/src/sandbox.h"
#elif defined(OS_CHROMEOS) && defined(ARCH_CPU_ARMEL) && defined(USE_X11)
#include "content/common/gpu/media/exynos_video_decode_accelerator.h"
#elif (defined(OS_CHROMEOS) || defined(OS_TIZEN_MOBILE)) &&\
 defined(ARCH_CPU_X86_FAMILY) && defined(USE_X11)
#include "content/common/gpu/media/vaapi_wrapper.h"
#endif

#if defined(USE_X11)
#include "ui/base/x/x11_util.h"
#endif

#if defined(OS_LINUX)
#include "content/public/common/sandbox_init.h"
#endif

const int kGpuTimeout = 10000;

namespace content {

namespace {

bool WarmUpSandbox(const CommandLine& command_line);
#if defined(OS_LINUX)
bool StartSandboxLinux(const gpu::GPUInfo&, GpuWatchdogThread*, bool);
#elif defined(OS_WIN)
bool StartSandboxWindows(const sandbox::SandboxInterfaceInfo*);
#endif

base::LazyInstance<GpuChildThread::DeferredMessages> deferred_messages =
    LAZY_INSTANCE_INITIALIZER;

bool GpuProcessLogMessageHandler(int severity,
                                 const char* file, int line,
                                 size_t message_start,
                                 const std::string& str) {
  std::string header = str.substr(0, message_start);
  std::string message = str.substr(message_start);
  deferred_messages.Get().push(new GpuHostMsg_OnLogMessage(
      severity, header, message));
  return false;
}

}  // namespace anonymous

// Main function for starting the Gpu process.
int GpuMain(const MainFunctionParams& parameters) {
  TRACE_EVENT0("gpu", "GpuMain");
  base::debug::TraceLog::GetInstance()->SetProcessName("GPU Process");
  base::debug::TraceLog::GetInstance()->SetProcessSortIndex(
      kTraceEventGpuProcessSortIndex);

  const CommandLine& command_line = parameters.command_line;
  if (command_line.HasSwitch(switches::kGpuStartupDialog)) {
    ChildProcess::WaitForDebugger("Gpu");
  }

  base::Time start_time = base::Time::Now();

  bool in_browser_process = command_line.HasSwitch(switches::kSingleProcess) ||
                            command_line.HasSwitch(switches::kInProcessGPU);

  if (!in_browser_process) {
#if defined(OS_WIN)
    // Prevent Windows from displaying a modal dialog on failures like not being
    // able to load a DLL.
    SetErrorMode(
        SEM_FAILCRITICALERRORS |
        SEM_NOGPFAULTERRORBOX |
        SEM_NOOPENFILEERRORBOX);
#elif defined(USE_X11)
    ui::SetDefaultX11ErrorHandlers();
#endif

    logging::SetLogMessageHandler(GpuProcessLogMessageHandler);
  }

  if (command_line.HasSwitch(switches::kSupportsDualGpus) &&
      command_line.HasSwitch(switches::kGpuSwitching)) {
    std::string option = command_line.GetSwitchValueASCII(
        switches::kGpuSwitching);
    if (option == switches::kGpuSwitchingOptionNameForceDiscrete)
      ui::GpuSwitchingManager::GetInstance()->ForceUseOfDiscreteGpu();
    else if (option == switches::kGpuSwitchingOptionNameForceIntegrated)
      ui::GpuSwitchingManager::GetInstance()->ForceUseOfIntegratedGpu();
  }

  // Initialization of the OpenGL bindings may fail, in which case we
  // will need to tear down this process. However, we can not do so
  // safely until the IPC channel is set up, because the detection of
  // early return of a child process is implemented using an IPC
  // channel error. If the IPC channel is not fully set up between the
  // browser and GPU process, and the GPU process crashes or exits
  // early, the browser process will never detect it.  For this reason
  // we defer tearing down the GPU process until receiving the
  // GpuMsg_Initialize message from the browser.
  bool dead_on_arrival = false;

  base::MessageLoop::Type message_loop_type = base::MessageLoop::TYPE_IO;
#if defined(OS_WIN)
  // Unless we're running on desktop GL, we don't need a UI message
  // loop, so avoid its use to work around apparent problems with some
  // third-party software.
  if (command_line.HasSwitch(switches::kUseGL) &&
      command_line.GetSwitchValueASCII(switches::kUseGL) ==
          gfx::kGLImplementationDesktopName) {
    message_loop_type = base::MessageLoop::TYPE_UI;
  }
#elif defined(TOOLKIT_GTK)
  message_loop_type = base::MessageLoop::TYPE_GPU;
#elif defined(OS_LINUX)
  message_loop_type = base::MessageLoop::TYPE_DEFAULT;
#endif

  base::MessageLoop main_message_loop(message_loop_type);
  base::PlatformThread::SetName("CrGpuMain");

  // In addition to disabling the watchdog if the command line switch is
  // present, disable the watchdog on valgrind because the code is expected
  // to run slowly in that case.
  bool enable_watchdog =
      !command_line.HasSwitch(switches::kDisableGpuWatchdog) &&
      !RunningOnValgrind();

  // Disable the watchdog in debug builds because they tend to only be run by
  // developers who will not appreciate the watchdog killing the GPU process.
#ifndef NDEBUG
  enable_watchdog = false;
#endif

  bool delayed_watchdog_enable = false;

#if defined(OS_CHROMEOS)
  // Don't start watchdog immediately, to allow developers to switch to VT2 on
  // startup.
  delayed_watchdog_enable = true;
#endif

  scoped_refptr<GpuWatchdogThread> watchdog_thread;

  // Start the GPU watchdog only after anything that is expected to be time
  // consuming has completed, otherwise the process is liable to be aborted.
  if (enable_watchdog && !delayed_watchdog_enable) {
    watchdog_thread = new GpuWatchdogThread(kGpuTimeout);
    watchdog_thread->Start();
  }

  gpu::GPUInfo gpu_info;
  // Get vendor_id, device_id, driver_version from browser process through
  // commandline switches.
  DCHECK(command_line.HasSwitch(switches::kGpuVendorID) &&
         command_line.HasSwitch(switches::kGpuDeviceID) &&
         command_line.HasSwitch(switches::kGpuDriverVersion));
  bool success = base::HexStringToUInt(
      command_line.GetSwitchValueASCII(switches::kGpuVendorID),
      &gpu_info.gpu.vendor_id);
  DCHECK(success);
  success = base::HexStringToUInt(
      command_line.GetSwitchValueASCII(switches::kGpuDeviceID),
      &gpu_info.gpu.device_id);
  DCHECK(success);
  gpu_info.driver_vendor =
      command_line.GetSwitchValueASCII(switches::kGpuDriverVendor);
  gpu_info.driver_version =
      command_line.GetSwitchValueASCII(switches::kGpuDriverVersion);
  GetContentClient()->SetGpuInfo(gpu_info);

  // Warm up resources that don't need access to GPUInfo.
  if (WarmUpSandbox(command_line)) {
#if defined(OS_LINUX)
    bool initialized_sandbox = false;
    bool initialized_gl_context = false;
    bool should_initialize_gl_context = false;
#if defined(OS_CHROMEOS) && defined(ARCH_CPU_ARMEL)
    // On Chrome OS ARM, GPU driver userspace creates threads when initializing
    // a GL context, so start the sandbox early.
    gpu_info.sandboxed = StartSandboxLinux(gpu_info, watchdog_thread.get(),
                                           should_initialize_gl_context);
    initialized_sandbox = true;
#endif
#endif  // defined(OS_LINUX)

    // Load and initialize the GL implementation and locate the GL entry points.
    if (gfx::GLSurface::InitializeOneOff()) {
      // We need to collect GL strings (VENDOR, RENDERER) for blacklisting
      // purposes. However, on Mac we don't actually use them. As documented in
      // crbug.com/222934, due to some driver issues, glGetString could take
      // multiple seconds to finish, which in turn cause the GPU process to
      // crash.
      // By skipping the following code on Mac, we don't really lose anything,
      // because the basic GPU information is passed down from browser process
      // and we already registered them through SetGpuInfo() above.
#if !defined(OS_MACOSX)
      if (!gpu::CollectContextGraphicsInfo(&gpu_info))
        VLOG(1) << "gpu::CollectGraphicsInfo failed";
      GetContentClient()->SetGpuInfo(gpu_info);

#if defined(OS_LINUX)
      initialized_gl_context = true;
#if !defined(OS_CHROMEOS)
      if (gpu_info.gpu.vendor_id == 0x10de &&  // NVIDIA
          gpu_info.driver_vendor == "NVIDIA") {
        base::ThreadRestrictions::AssertIOAllowed();
        if (access("/dev/nvidiactl", R_OK) != 0) {
          VLOG(1) << "NVIDIA device file /dev/nvidiactl access denied";
          dead_on_arrival = true;
        }
      }
#endif  // !defined(OS_CHROMEOS)
#endif  // defined(OS_LINUX)
#endif  // !defined(OS_MACOSX)
    } else {
      VLOG(1) << "gfx::GLSurface::InitializeOneOff failed";
      dead_on_arrival = true;
    }

    if (enable_watchdog && delayed_watchdog_enable) {
      watchdog_thread = new GpuWatchdogThread(kGpuTimeout);
      watchdog_thread->Start();
    }

    // OSMesa is expected to run very slowly, so disable the watchdog in that
    // case.
    if (enable_watchdog &&
        gfx::GetGLImplementation() == gfx::kGLImplementationOSMesaGL) {
      watchdog_thread->Stop();
      watchdog_thread = NULL;
    }

#if defined(OS_LINUX)
    should_initialize_gl_context = !initialized_gl_context &&
                                   !dead_on_arrival;

    if (!initialized_sandbox) {
      gpu_info.sandboxed = StartSandboxLinux(gpu_info, watchdog_thread.get(),
                                             should_initialize_gl_context);
    }
#elif defined(OS_WIN)
    gpu_info.sandboxed = StartSandboxWindows(parameters.sandbox_info);
#endif
  } else {
    dead_on_arrival = true;
  }

  logging::SetLogMessageHandler(NULL);

  GpuProcess gpu_process;

  GpuChildThread* child_thread = new GpuChildThread(watchdog_thread.get(),
                                                    dead_on_arrival,
                                                    gpu_info,
                                                    deferred_messages.Get());
  while (!deferred_messages.Get().empty())
    deferred_messages.Get().pop();

  child_thread->Init(start_time);

  gpu_process.set_main_thread(child_thread);

  if (watchdog_thread)
    watchdog_thread->AddPowerObserver();

  {
    TRACE_EVENT0("gpu", "Run Message Loop");
    main_message_loop.Run();
  }

  child_thread->StopWatchdog();

  return 0;
}

namespace {

#if defined(OS_LINUX)
void CreateDummyGlContext() {
  scoped_refptr<gfx::GLSurface> surface(
      gfx::GLSurface::CreateOffscreenGLSurface(gfx::Size(1, 1)));
  if (!surface.get()) {
    VLOG(1) << "gfx::GLSurface::CreateOffscreenGLSurface failed";
    return;
  }

  // On Linux, this is needed to make sure /dev/nvidiactl has
  // been opened and its descriptor cached.
  scoped_refptr<gfx::GLContext> context(gfx::GLContext::CreateGLContext(
      NULL, surface.get(), gfx::PreferDiscreteGpu));
  if (!context.get()) {
    VLOG(1) << "gfx::GLContext::CreateGLContext failed";
    return;
  }

  // Similarly, this is needed for /dev/nvidia0.
  if (context->MakeCurrent(surface.get())) {
    context->ReleaseCurrent(surface.get());
  } else {
    VLOG(1)  << "gfx::GLContext::MakeCurrent failed";
  }
}
#endif

bool WarmUpSandbox(const CommandLine& command_line) {
  {
    TRACE_EVENT0("gpu", "Warm up rand");
    // Warm up the random subsystem, which needs to be done pre-sandbox on all
    // platforms.
    (void) base::RandUint64();
  }
  {
    TRACE_EVENT0("gpu", "Warm up HMAC");
    // Warm up the crypto subsystem, which needs to done pre-sandbox on all
    // platforms.
    crypto::HMAC hmac(crypto::HMAC::SHA256);
    unsigned char key = '\0';
    if (!hmac.Init(&key, sizeof(key))) {
      LOG(ERROR) << "WarmUpSandbox() failed with crypto::HMAC::Init()";
      return false;
    }
  }

#if defined(OS_CHROMEOS) && defined(ARCH_CPU_ARMEL) && defined(USE_X11)
  ExynosVideoDecodeAccelerator::PreSandboxInitialization();
#elif (defined(OS_CHROMEOS) || defined(OS_TIZEN_MOBILE)) &&\
 defined(ARCH_CPU_X86_FAMILY) && defined(USE_X11)
  VaapiWrapper::PreSandboxInitialization();
#endif

#if defined(OS_WIN)
  {
    TRACE_EVENT0("gpu", "Preload setupapi.dll");
    // Preload this DLL because the sandbox prevents it from loading.
    if (LoadLibrary(L"setupapi.dll") == NULL) {
      LOG(ERROR) << "WarmUpSandbox() failed with loading setupapi.dll";
      return false;
    }
  }

  if (!command_line.HasSwitch(switches::kDisableAcceleratedVideoDecode)) {
    TRACE_EVENT0("gpu", "Initialize DXVA");
    // Initialize H/W video decoding stuff which fails in the sandbox.
    DXVAVideoDecodeAccelerator::PreSandboxInitialization();
  }

  {
    TRACE_EVENT0("gpu", "Warm up DWM");

    // DWM was introduced with Windows Vista. DwmFlush seems to be sufficient
    // to warm it up before lowering the token. DWM is required to present to
    // a window with Vista and later and this allows us to do so with the
    // GPU process sandbox enabled.
    if (base::win::GetVersion() >= base::win::VERSION_VISTA) {
      HMODULE module = LoadLibrary(L"dwmapi.dll");
      if (module) {
        typedef HRESULT (WINAPI *DwmFlushFunc)();
        DwmFlushFunc dwm_flush = reinterpret_cast<DwmFlushFunc>(
            GetProcAddress(module, "DwmFlush"));
        if (dwm_flush)
          dwm_flush();
      }
    }
  }
#endif
  return true;
}

#if defined(OS_LINUX)
void WarmUpSandboxNvidia(const gpu::GPUInfo& gpu_info,
                         bool should_initialize_gl_context) {
  // We special case Optimus since the vendor_id we see may not be Nvidia.
  bool uses_nvidia_driver = (gpu_info.gpu.vendor_id == 0x10de &&  // NVIDIA.
                             gpu_info.driver_vendor == "NVIDIA") ||
                            gpu_info.optimus;
  if (uses_nvidia_driver && should_initialize_gl_context) {
    // We need this on Nvidia to pre-open /dev/nvidiactl and /dev/nvidia0.
    CreateDummyGlContext();
  }
}

bool StartSandboxLinux(const gpu::GPUInfo& gpu_info,
                       GpuWatchdogThread* watchdog_thread,
                       bool should_initialize_gl_context) {
  TRACE_EVENT0("gpu", "Initialize sandbox");

  bool res = false;

  WarmUpSandboxNvidia(gpu_info, should_initialize_gl_context);

  if (watchdog_thread)
    watchdog_thread->Stop();
  // LinuxSandbox::InitializeSandbox() must always be called
  // with only one thread.
  res = LinuxSandbox::InitializeSandbox();
  if (watchdog_thread)
    watchdog_thread->Start();

  return res;
}
#endif  // defined(OS_LINUX)

#if defined(OS_WIN)
bool StartSandboxWindows(const sandbox::SandboxInterfaceInfo* sandbox_info) {
  TRACE_EVENT0("gpu", "Lower token");

  // For Windows, if the target_services interface is not zero, the process
  // is sandboxed and we must call LowerToken() before rendering untrusted
  // content.
  sandbox::TargetServices* target_services = sandbox_info->target_services;
  if (target_services) {
    target_services->LowerToken();
    return true;
  }

  return false;
}
#endif  // defined(OS_WIN)

}  // namespace.

}  // namespace content
