// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/zygote_host/zygote_host_impl_linux.h"

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/file_util.h"
#include "base/linux_util.h"
#include "base/logging.h"
#include "base/memory/linked_ptr.h"
#include "base/memory/scoped_ptr.h"
#include "base/metrics/histogram.h"
#include "base/path_service.h"
#include "base/posix/eintr_wrapper.h"
#include "base/posix/unix_domain_socket_linux.h"
#include "base/process_util.h"
#include "base/string_number_conversions.h"
#include "base/string_util.h"
#include "base/time.h"
#include "base/utf_string_conversions.h"
#include "content/browser/renderer_host/render_sandbox_host_linux.h"
#include "content/common/zygote_commands_linux.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/result_codes.h"
#include "sandbox/linux/suid/client/setuid_sandbox_client.h"
#include "sandbox/linux/suid/common/sandbox.h"
#include "ui/base/ui_base_switches.h"

#if defined(USE_TCMALLOC)
#include "third_party/tcmalloc/chromium/src/gperftools/heap-profiler.h"
#endif

namespace content {

// static
ZygoteHost* ZygoteHost::GetInstance() {
  return ZygoteHostImpl::GetInstance();
}

ZygoteHostImpl::ZygoteHostImpl()
    : control_fd_(-1),
      pid_(-1),
      init_(false),
      using_suid_sandbox_(false),
      have_read_sandbox_status_word_(false),
      sandbox_status_(0) {}

ZygoteHostImpl::~ZygoteHostImpl() {
  if (init_)
    close(control_fd_);
}

// static
ZygoteHostImpl* ZygoteHostImpl::GetInstance() {
  return Singleton<ZygoteHostImpl>::get();
}

void ZygoteHostImpl::Init(const std::string& sandbox_cmd) {
  DCHECK(!init_);
  init_ = true;

  base::FilePath chrome_path;
  CHECK(PathService::Get(base::FILE_EXE, &chrome_path));
#if defined(TOOLKIT_EFL)
  //TODO(rakuco): make subprocess laucher
  // chrome_path = CommandLine::ForCurrentProcess()->GetSwitchValuePath(switches::kBrowserSubprocessPath);
#endif
  CommandLine cmd_line(chrome_path);

  cmd_line.AppendSwitchASCII(switches::kProcessType, switches::kZygoteProcess);

  int fds[2];
#if defined(OS_FREEBSD) || defined(OS_OPENBSD)
  // The BSDs often don't support SOCK_SEQPACKET yet, so fall back to
  // SOCK_DGRAM if necessary.
  if (socketpair(PF_UNIX, SOCK_SEQPACKET, 0, fds) != 0)
    CHECK(socketpair(PF_UNIX, SOCK_DGRAM, 0, fds) == 0);
#else
  CHECK(socketpair(PF_UNIX, SOCK_SEQPACKET, 0, fds) == 0);
#endif
  base::FileHandleMappingVector fds_to_map;
  fds_to_map.push_back(std::make_pair(fds[1], kZygoteSocketPairFd));

  const CommandLine& browser_command_line = *CommandLine::ForCurrentProcess();
  if (browser_command_line.HasSwitch(switches::kZygoteCmdPrefix)) {
    cmd_line.PrependWrapper(
        browser_command_line.GetSwitchValueNative(switches::kZygoteCmdPrefix));
  }
  // Append any switches from the browser process that need to be forwarded on
  // to the zygote/renderers.
  // Should this list be obtained from browser_render_process_host.cc?
  static const char* kForwardSwitches[] = {
    switches::kAllowSandboxDebugging,
    switches::kLoggingLevel,
    switches::kEnableLogging,  // Support, e.g., --enable-logging=stderr.
    switches::kV,
    switches::kVModule,
    switches::kRegisterPepperPlugins,
    switches::kDisableSeccompFilterSandbox,

    // Zygote process needs to know what resources to have loaded when it
    // becomes a renderer process.
    switches::kForceDeviceScaleFactor,
    switches::kTouchOptimizedUI,

    switches::kNoSandbox,
  };
  cmd_line.CopySwitchesFrom(browser_command_line, kForwardSwitches,
                            arraysize(kForwardSwitches));

  GetContentClient()->browser()->AppendExtraCommandLineSwitches(&cmd_line, -1);

  sandbox_binary_ = sandbox_cmd.c_str();

  if (!sandbox_cmd.empty()) {
    struct stat st;
    if (stat(sandbox_binary_.c_str(), &st) != 0) {
      LOG(FATAL) << "The SUID sandbox helper binary is missing: "
                 << sandbox_binary_ << " Aborting now.";
    }

    if (access(sandbox_binary_.c_str(), X_OK) == 0 &&
        (st.st_uid == 0) &&
        (st.st_mode & S_ISUID) &&
        (st.st_mode & S_IXOTH)) {
      using_suid_sandbox_ = true;
      cmd_line.PrependWrapper(sandbox_binary_);

      scoped_ptr<sandbox::SetuidSandboxClient>
          sandbox_client(sandbox::SetuidSandboxClient::Create());
      sandbox_client->SetupLaunchEnvironment();
    } else {
      LOG(FATAL) << "The SUID sandbox helper binary was found, but is not "
                    "configured correctly. Rather than run without sandboxing "
                    "I'm aborting now. You need to make sure that "
                 << sandbox_binary_ << " is owned by root and has mode 4755.";
    }
  } else {
    LOG(ERROR) << "Running without the SUID sandbox! See "
        "https://code.google.com/p/chromium/wiki/LinuxSUIDSandboxDevelopment "
        "for more information on developing with the sandbox on.";
  }

  // Start up the sandbox host process and get the file descriptor for the
  // renderers to talk to it.
  const int sfd = RenderSandboxHostLinux::GetInstance()->GetRendererSocket();
  fds_to_map.push_back(std::make_pair(sfd, kZygoteRendererSocketFd));

  int dummy_fd = -1;
  if (using_suid_sandbox_) {
    dummy_fd = socket(PF_UNIX, SOCK_DGRAM, 0);
    CHECK(dummy_fd >= 0);
    fds_to_map.push_back(std::make_pair(dummy_fd, kZygoteIdFd));
  }

  base::ProcessHandle process = -1;
  base::LaunchOptions options;
  options.fds_to_remap = &fds_to_map;
  base::LaunchProcess(cmd_line.argv(), options, &process);
  CHECK(process != -1) << "Failed to launch zygote process";

  if (using_suid_sandbox_) {
    // In the SUID sandbox, the real zygote is forked from the sandbox.
    // We need to look for it.
    // But first, wait for the zygote to tell us it's running.
    // The sending code is in content/browser/zygote_main_linux.cc.
    std::vector<int> fds_vec;
    const int kExpectedLength = sizeof(kZygoteHelloMessage);
    char buf[kExpectedLength];
    const ssize_t len = UnixDomainSocket::RecvMsg(fds[0], buf, sizeof(buf),
                                                  &fds_vec);
    CHECK(len == kExpectedLength) << "Incorrect zygote magic length";
    CHECK(0 == strcmp(buf, kZygoteHelloMessage))
        << "Incorrect zygote hello";

    std::string inode_output;
    ino_t inode = 0;
    // Figure out the inode for |dummy_fd|, close |dummy_fd| on our end,
    // and find the zygote process holding |dummy_fd|.
    if (base::FileDescriptorGetInode(&inode, dummy_fd)) {
      close(dummy_fd);
      std::vector<std::string> get_inode_cmdline;
      get_inode_cmdline.push_back(sandbox_binary_);
      get_inode_cmdline.push_back(base::kFindInodeSwitch);
      get_inode_cmdline.push_back(base::Int64ToString(inode));
      CommandLine get_inode_cmd(get_inode_cmdline);
      if (base::GetAppOutput(get_inode_cmd, &inode_output)) {
        base::StringToInt(inode_output, &pid_);
      }
    }
    CHECK(pid_ > 0) << "Did not find zygote process (using sandbox binary "
        << sandbox_binary_ << ")";

    if (process != pid_) {
      // Reap the sandbox.
      base::EnsureProcessGetsReaped(process);
    }
  } else {
    // Not using the SUID sandbox.
    pid_ = process;
  }

  close(fds[1]);
  control_fd_ = fds[0];

  Pickle pickle;
  pickle.WriteInt(kZygoteCommandGetSandboxStatus);
  if (!SendMessage(pickle, NULL))
    LOG(FATAL) << "Cannot communicate with zygote";
  // We don't wait for the reply. We'll read it in ReadReply.
}

bool ZygoteHostImpl::SendMessage(const Pickle& data,
                                 const std::vector<int>* fds) {
  CHECK(data.size() <= kZygoteMaxMessageLength)
      << "Trying to send too-large message to zygote (sending " << data.size()
      << " bytes, max is " << kZygoteMaxMessageLength << ")";
  CHECK(!fds || fds->size() <= UnixDomainSocket::kMaxFileDescriptors)
      << "Trying to send message with too many file descriptors to zygote "
      << "(sending " << fds->size() << ", max is "
      << UnixDomainSocket::kMaxFileDescriptors << ")";

  return UnixDomainSocket::SendMsg(control_fd_,
                                   data.data(), data.size(),
                                   fds ? *fds : std::vector<int>());
}

ssize_t ZygoteHostImpl::ReadReply(void* buf, size_t buf_len) {
  // At startup we send a kZygoteCommandGetSandboxStatus request to the zygote,
  // but don't wait for the reply. Thus, the first time that we read from the
  // zygote, we get the reply to that request.
  if (!have_read_sandbox_status_word_) {
    if (HANDLE_EINTR(read(control_fd_, &sandbox_status_,
                          sizeof(sandbox_status_))) !=
        sizeof(sandbox_status_)) {
      return -1;
    }
    have_read_sandbox_status_word_ = true;
  }

  return HANDLE_EINTR(read(control_fd_, buf, buf_len));
}

pid_t ZygoteHostImpl::ForkRequest(
    const std::vector<std::string>& argv,
    const std::vector<FileDescriptorInfo>& mapping,
    const std::string& process_type) {
  DCHECK(init_);
  Pickle pickle;

  pickle.WriteInt(kZygoteCommandFork);
  pickle.WriteString(process_type);
  pickle.WriteInt(argv.size());
  for (std::vector<std::string>::const_iterator
       i = argv.begin(); i != argv.end(); ++i)
    pickle.WriteString(*i);

  pickle.WriteInt(mapping.size());

  std::vector<int> fds;
  // Scoped pointers cannot be stored in containers, so we have to use a
  // linked_ptr.
  std::vector<linked_ptr<file_util::ScopedFD> > autodelete_fds;
  for (std::vector<FileDescriptorInfo>::const_iterator
       i = mapping.begin(); i != mapping.end(); ++i) {
    pickle.WriteUInt32(i->id);
    fds.push_back(i->fd.fd);
    if (i->fd.auto_close) {
      // Auto-close means we need to close the FDs after they have been passed
      // to the other process.
      linked_ptr<file_util::ScopedFD> ptr(
          new file_util::ScopedFD(&(fds.back())));
      autodelete_fds.push_back(ptr);
    }
  }

  pid_t pid;
  {
    base::AutoLock lock(control_lock_);
    if (!SendMessage(pickle, &fds))
      return base::kNullProcessHandle;

    // Read the reply, which pickles the PID and an optional UMA enumeration.
    static const unsigned kMaxReplyLength = 2048;
    char buf[kMaxReplyLength];
    const ssize_t len = ReadReply(buf, sizeof(buf));

    Pickle reply_pickle(buf, len);
    PickleIterator iter(reply_pickle);
    if (len <= 0 || !reply_pickle.ReadInt(&iter, &pid))
      return base::kNullProcessHandle;

    // If there is a nonempty UMA name string, then there is a UMA
    // enumeration to record.
    std::string uma_name;
    int uma_sample;
    int uma_boundary_value;
    if (reply_pickle.ReadString(&iter, &uma_name) &&
        !uma_name.empty() &&
        reply_pickle.ReadInt(&iter, &uma_sample) &&
        reply_pickle.ReadInt(&iter, &uma_boundary_value)) {
      // We cannot use the UMA_HISTOGRAM_ENUMERATION macro here,
      // because that's only for when the name is the same every time.
      // Here we're using whatever name we got from the other side.
      // But since it's likely that the same one will be used repeatedly
      // (even though it's not guaranteed), we cache it here.
      static base::HistogramBase* uma_histogram;
      if (!uma_histogram || uma_histogram->histogram_name() != uma_name) {
        uma_histogram = base::LinearHistogram::FactoryGet(
            uma_name, 1,
            uma_boundary_value,
            uma_boundary_value + 1,
            base::HistogramBase::kUmaTargetedHistogramFlag);
      }
      uma_histogram->Add(uma_sample);
    }

    if (pid <= 0)
      return base::kNullProcessHandle;
  }

#if !defined(OS_OPENBSD)
  // This is just a starting score for a renderer or extension (the
  // only types of processes that will be started this way).  It will
  // get adjusted as time goes on.  (This is the same value as
  // chrome::kLowestRendererOomScore in chrome/chrome_constants.h, but
  // that's not something we can include here.)
  const int kLowestRendererOomScore = 300;
  AdjustRendererOOMScore(pid, kLowestRendererOomScore);
#endif

  return pid;
}

#if !defined(OS_OPENBSD)
void ZygoteHostImpl::AdjustRendererOOMScore(base::ProcessHandle pid,
                                            int score) {
  // 1) You can't change the oom_score_adj of a non-dumpable process
  //    (EPERM) unless you're root. Because of this, we can't set the
  //    oom_adj from the browser process.
  //
  // 2) We can't set the oom_score_adj before entering the sandbox
  //    because the zygote is in the sandbox and the zygote is as
  //    critical as the browser process. Its oom_adj value shouldn't
  //    be changed.
  //
  // 3) A non-dumpable process can't even change its own oom_score_adj
  //    because it's root owned 0644. The sandboxed processes don't
  //    even have /proc, but one could imagine passing in a descriptor
  //    from outside.
  //
  // So, in the normal case, we use the SUID binary to change it for us.
  // However, Fedora (and other SELinux systems) don't like us touching other
  // process's oom_score_adj (or oom_adj) values
  // (https://bugzilla.redhat.com/show_bug.cgi?id=581256).
  //
  // The offical way to get the SELinux mode is selinux_getenforcemode, but I
  // don't want to add another library to the build as it's sure to cause
  // problems with other, non-SELinux distros.
  //
  // So we just check for files in /selinux. This isn't foolproof, but it's not
  // bad and it's easy.

  static bool selinux;
  static bool selinux_valid = false;

  if (!selinux_valid) {
    const base::FilePath kSelinuxPath("/selinux");
    file_util::FileEnumerator en(kSelinuxPath, false,
                                 file_util::FileEnumerator::FILES);
    bool has_selinux_files = !en.Next().empty();

    selinux = access(kSelinuxPath.value().c_str(), X_OK) == 0 &&
              has_selinux_files;
    selinux_valid = true;
  }

  if (using_suid_sandbox_ && !selinux) {
#if defined(USE_TCMALLOC)
    // If heap profiling is running, these processes are not exiting, at least
    // on ChromeOS. The easiest thing to do is not launch them when profiling.
    // TODO(stevenjb): Investigate further and fix.
    if (IsHeapProfilerRunning())
      return;
#endif
    std::vector<std::string> adj_oom_score_cmdline;
    adj_oom_score_cmdline.push_back(sandbox_binary_);
    adj_oom_score_cmdline.push_back(sandbox::kAdjustOOMScoreSwitch);
    adj_oom_score_cmdline.push_back(base::Int64ToString(pid));
    adj_oom_score_cmdline.push_back(base::IntToString(score));

    base::ProcessHandle sandbox_helper_process;
    if (base::LaunchProcess(adj_oom_score_cmdline, base::LaunchOptions(),
                            &sandbox_helper_process)) {
      base::EnsureProcessGetsReaped(sandbox_helper_process);
    }
  } else if (!using_suid_sandbox_) {
    if (!base::AdjustOOMScore(pid, score))
      PLOG(ERROR) << "Failed to adjust OOM score of renderer with pid " << pid;
  }
}
#endif

void ZygoteHostImpl::AdjustLowMemoryMargin(int64 margin_mb) {
#if defined(OS_CHROMEOS)
  // You can't change the low memory margin unless you're root. Because of this,
  // we can't set the low memory margin from the browser process.
  // So, we use the SUID binary to change it for us.
  if (using_suid_sandbox_) {
#if defined(USE_TCMALLOC)
    // If heap profiling is running, these processes are not exiting, at least
    // on ChromeOS. The easiest thing to do is not launch them when profiling.
    // TODO(stevenjb): Investigate further and fix.
    if (IsHeapProfilerRunning())
      return;
#endif
    std::vector<std::string> adj_low_mem_commandline;
    adj_low_mem_commandline.push_back(sandbox_binary_);
    adj_low_mem_commandline.push_back(sandbox::kAdjustLowMemMarginSwitch);
    adj_low_mem_commandline.push_back(base::Int64ToString(margin_mb));

    base::ProcessHandle sandbox_helper_process;
    if (base::LaunchProcess(adj_low_mem_commandline, base::LaunchOptions(),
                            &sandbox_helper_process)) {
      base::EnsureProcessGetsReaped(sandbox_helper_process);
    } else {
      LOG(ERROR) << "Unable to run suid sandbox to set low memory margin.";
    }
  }
  // Don't adjust memory margin if we're not running with the sandbox: this
  // isn't very common, and not doing it has little impact.
#else
  // Low memory notification is currently only implemented on ChromeOS.
  NOTREACHED() << "AdjustLowMemoryMargin not implemented";
#endif  // defined(OS_CHROMEOS)
}


void ZygoteHostImpl::EnsureProcessTerminated(pid_t process) {
  DCHECK(init_);
  Pickle pickle;

  pickle.WriteInt(kZygoteCommandReap);
  pickle.WriteInt(process);
  if (!SendMessage(pickle, NULL))
    LOG(ERROR) << "Failed to send Reap message to zygote";
}

base::TerminationStatus ZygoteHostImpl::GetTerminationStatus(
    base::ProcessHandle handle,
    bool known_dead,
    int* exit_code) {
  DCHECK(init_);
  Pickle pickle;
  pickle.WriteInt(kZygoteCommandGetTerminationStatus);
  pickle.WriteBool(known_dead);
  pickle.WriteInt(handle);

  // Set this now to handle the early termination cases.
  if (exit_code)
    *exit_code = RESULT_CODE_NORMAL_EXIT;

  static const unsigned kMaxMessageLength = 128;
  char buf[kMaxMessageLength];
  ssize_t len;
  {
    base::AutoLock lock(control_lock_);
    if (!SendMessage(pickle, NULL))
      LOG(ERROR) << "Failed to send GetTerminationStatus message to zygote";
    len = ReadReply(buf, sizeof(buf));
  }

  if (len == -1) {
    LOG(WARNING) << "Error reading message from zygote: " << errno;
    return base::TERMINATION_STATUS_NORMAL_TERMINATION;
  } else if (len == 0) {
    LOG(WARNING) << "Socket closed prematurely.";
    return base::TERMINATION_STATUS_NORMAL_TERMINATION;
  }

  Pickle read_pickle(buf, len);
  int status, tmp_exit_code;
  PickleIterator iter(read_pickle);
  if (!read_pickle.ReadInt(&iter, &status) ||
      !read_pickle.ReadInt(&iter, &tmp_exit_code)) {
    LOG(WARNING) << "Error parsing GetTerminationStatus response from zygote.";
    return base::TERMINATION_STATUS_NORMAL_TERMINATION;
  }

  if (exit_code)
    *exit_code = tmp_exit_code;

  return static_cast<base::TerminationStatus>(status);
}

pid_t ZygoteHostImpl::GetPid() const {
  return pid_;
}

pid_t ZygoteHostImpl::GetSandboxHelperPid() const {
  return RenderSandboxHostLinux::GetInstance()->pid();
}

int ZygoteHostImpl::GetSandboxStatus() const {
  if (have_read_sandbox_status_word_)
    return sandbox_status_;
  return 0;
}

}  // namespace content
