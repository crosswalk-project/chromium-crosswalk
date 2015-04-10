// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Multiply-included file, hence no include guard.

#include "content/common/child_process_messages.h"

#include "content/common/accessibility_messages.h"
#include "content/common/appcache_messages.h"
#include "content/common/bluetooth/bluetooth_messages.h"
#include "content/common/browser_plugin/browser_plugin_messages.h"
#include "content/common/cache_storage/cache_storage_messages.h"
#include "content/common/cc_messages.h"
#include "content/common/clipboard_messages.h"
#include "content/common/database_messages.h"
#include "content/common/device_sensors/device_light_messages.h"
#include "content/common/device_sensors/device_motion_messages.h"
#include "content/common/device_sensors/device_orientation_messages.h"
#include "content/common/devtools_messages.h"
#include "content/common/dom_storage/dom_storage_messages.h"
#include "content/common/drag_messages.h"
#include "content/common/drag_traits.h"
#include "content/common/file_utilities_messages.h"
#include "content/common/fileapi/file_system_messages.h"
#include "content/common/fileapi/webblob_messages.h"
#include "content/common/frame_messages.h"
#include "content/common/gamepad_messages.h"
#include "content/common/geofencing_messages.h"
#include "content/common/gpu/gpu_messages.h"
#include "content/common/image_messages.h"
#include "content/common/indexed_db/indexed_db_messages.h"
#include "content/common/input_messages.h"
#include "content/common/manifest_manager_messages.h"
#include "content/common/media/aec_dump_messages.h"
#include "content/common/media/audio_messages.h"
// TODO(xhwang): Move this to a new ifdef block.
#include "content/common/media/cdm_messages.h"
#include "content/common/media/media_stream_messages.h"
#include "content/common/media/media_stream_track_metrics_host_messages.h"
#include "content/common/media/midi_messages.h"
#include "content/common/media/peer_connection_tracker_messages.h"
#include "content/common/media/video_capture_messages.h"
#include "content/common/media/webrtc_identity_messages.h"
#include "content/common/memory_benchmark_messages.h"
#include "content/common/message_port_messages.h"
#include "content/common/mime_registry_messages.h"
#include "content/common/mojo/mojo_messages.h"
#include "content/common/navigator_connect_messages.h"
#include "content/common/pepper_messages.h"
#include "content/common/platform_notification_messages.h"
#include "content/common/plugin_process_messages.h"
#include "content/common/power_monitor_messages.h"
#include "content/common/push_messaging_messages.h"
#include "content/common/quota_messages.h"
#include "content/common/resource_messages.h"
#include "content/common/screen_orientation_messages.h"
#include "content/common/service_worker/embedded_worker_messages.h"
#include "content/common/service_worker/service_worker_messages.h"
#include "content/common/speech_recognition_messages.h"
#include "content/common/text_input_client_messages.h"
#include "content/common/utility_messages.h"
#include "content/common/view_messages.h"
#include "content/common/websocket_messages.h"
#include "content/common/worker_messages.h"

#if defined(ENABLE_WEBRTC)
#include "content/common/p2p_messages.h"
#endif

#if defined(OS_ANDROID)
#include "content/common/gin_java_bridge_messages.h"
#include "content/common/media/media_player_messages_android.h"
#endif  // defined(OS_ANDROID)

#if defined(OS_TIZEN) && defined(ENABLE_MURPHY)
#include "xwalk/tizen/common/media/media_player_messages.h"
#endif
