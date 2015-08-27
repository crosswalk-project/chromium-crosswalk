// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/android/browser_media_player_manager.h"

#include "base/android/scoped_java_ref.h"
#include "base/command_line.h"
#include "content/browser/android/content_view_core_impl.h"
#include "content/browser/android/media_players_observer.h"
#include "content/browser/media/android/browser_demuxer_android.h"
#include "content/browser/media/android/media_resource_getter_impl.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/web_contents/web_contents_view_android.h"
#include "content/common/media/media_player_messages_android.h"
#include "content/public/browser/android/content_view_core.h"
#include "content/public/browser/android/external_video_surface_container.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "media/base/android/media_codec_player.h"
#include "media/base/android/media_player_bridge.h"
#include "media/base/android/media_source_player.h"
#include "media/base/android/media_url_interceptor.h"
#include "media/base/media_switches.h"

using media::MediaCodecPlayer;
using media::MediaPlayerAndroid;
using media::MediaPlayerBridge;
using media::MediaPlayerManager;
using media::MediaSourcePlayer;

namespace content {

// Threshold on the number of media players per renderer before we start
// attempting to release inactive media players.
const int kMediaPlayerThreshold = 1;
const int kInvalidMediaPlayerId = -1;

static BrowserMediaPlayerManager::Factory g_factory = NULL;
static media::MediaUrlInterceptor* media_url_interceptor_ = NULL;

// static
void BrowserMediaPlayerManager::RegisterFactory(Factory factory) {
  // TODO(aberent) nullptr test is a temporary fix to simplify upstreaming Cast.
  // Until Cast is fully upstreamed we want the downstream factory to take
  // priority over the upstream factory. The downstream call happens first,
  // so this will ensure that it does.
  if (g_factory == nullptr)
    g_factory = factory;
}

// static
void BrowserMediaPlayerManager::RegisterMediaUrlInterceptor(
    media::MediaUrlInterceptor* media_url_interceptor) {
  media_url_interceptor_ = media_url_interceptor;
}

// static
BrowserMediaPlayerManager* BrowserMediaPlayerManager::Create(
    RenderFrameHost* rfh,
    MediaPlayersObserver* audio_monitor) {
  if (g_factory)
    return g_factory(rfh, audio_monitor);
  return new BrowserMediaPlayerManager(rfh, audio_monitor);
}

ContentViewCore* BrowserMediaPlayerManager::GetContentViewCore() const {
  return ContentViewCoreImpl::FromWebContents(web_contents());
}

MediaPlayerAndroid* BrowserMediaPlayerManager::CreateMediaPlayer(
    const MediaPlayerHostMsg_Initialize_Params& media_player_params,
    bool hide_url_log,
    BrowserDemuxerAndroid* demuxer) {
  switch (media_player_params.type) {
    case MEDIA_PLAYER_TYPE_URL: {
      const std::string user_agent = GetContentClient()->GetUserAgent();
      MediaPlayerBridge* media_player_bridge = new MediaPlayerBridge(
          media_player_params.player_id,
          media_player_params.url,
          media_player_params.first_party_for_cookies,
          user_agent,
          hide_url_log,
          this,
          base::Bind(&BrowserMediaPlayerManager::OnMediaResourcesRequested,
                     weak_ptr_factory_.GetWeakPtr()),
          media_player_params.frame_url,
          media_player_params.allow_credentials);
      ContentViewCoreImpl* content_view_core_impl =
          static_cast<ContentViewCoreImpl*>(ContentViewCore::FromWebContents(
              web_contents_));
      if (!content_view_core_impl) {
        // May reach here due to prerendering. Don't extract the metadata
        // since it is expensive.
        // TODO(qinmin): extract the metadata once the user decided to load
        // the page.
        OnMediaMetadataChanged(
            media_player_params.player_id, base::TimeDelta(), 0, 0, false);
      } else if (!content_view_core_impl->ShouldBlockMediaRequest(
            media_player_params.url)) {
        media_player_bridge->Initialize();
      }
      return media_player_bridge;
    }

    case MEDIA_PLAYER_TYPE_MEDIA_SOURCE: {
      if (base::CommandLine::ForCurrentProcess()->
          HasSwitch(switches::kEnableMediaThreadForMediaPlayback)) {
        return new MediaCodecPlayer(
            media_player_params.player_id,
            weak_ptr_factory_.GetWeakPtr(),
            base::Bind(&BrowserMediaPlayerManager::OnMediaResourcesRequested,
                       weak_ptr_factory_.GetWeakPtr()),
            demuxer->CreateDemuxer(media_player_params.demuxer_client_id),
            media_player_params.frame_url);
      } else {
        return new MediaSourcePlayer(
            media_player_params.player_id,
            this,
            base::Bind(&BrowserMediaPlayerManager::OnMediaResourcesRequested,
                       weak_ptr_factory_.GetWeakPtr()),
            demuxer->CreateDemuxer(media_player_params.demuxer_client_id),
            media_player_params.frame_url);
      }
    }
  }

  NOTREACHED();
  return NULL;
}

BrowserMediaPlayerManager::BrowserMediaPlayerManager(
    RenderFrameHost* render_frame_host,
    MediaPlayersObserver* audio_monitor)
    : render_frame_host_(render_frame_host),
      audio_monitor_(audio_monitor),
      fullscreen_player_id_(kInvalidMediaPlayerId),
      fullscreen_player_is_released_(false),
      web_contents_(WebContents::FromRenderFrameHost(render_frame_host)),
      weak_ptr_factory_(this) {
}

BrowserMediaPlayerManager::~BrowserMediaPlayerManager() {
  // During the tear down process, OnDestroyPlayer() may or may not be called
  // (e.g. the WebContents may be destroyed before the render process). So
  // we cannot DCHECK(players_.empty()) here. Instead, all media players in
  // |players_| will be destroyed here because |player_| is a ScopedVector.

  for (MediaPlayerAndroid* player : players_)
    player->DeleteOnCorrectThread();

  players_.weak_clear();
}

void BrowserMediaPlayerManager::ExitFullscreen(bool release_media_player) {
  if (WebContentsDelegate* delegate = web_contents_->GetDelegate())
    delegate->ExitFullscreenModeForTab(web_contents_);
  if (RenderWidgetHostViewAndroid* view_android =
      static_cast<RenderWidgetHostViewAndroid*>(
          web_contents_->GetRenderWidgetHostView())) {
    view_android->SetOverlayVideoMode(false);
  }

  Send(
      new MediaPlayerMsg_DidExitFullscreen(RoutingID(), fullscreen_player_id_));
  video_view_.reset();
  MediaPlayerAndroid* player = GetFullscreenPlayer();
  fullscreen_player_id_ = kInvalidMediaPlayerId;
  if (!player)
    return;

#if defined(VIDEO_HOLE)
  if (external_video_surface_container_)
    external_video_surface_container_->OnFrameInfoUpdated();
#endif  // defined(VIDEO_HOLE)

  if (release_media_player)
    ReleaseFullscreenPlayer(player);
  else
    player->SetVideoSurface(gfx::ScopedJavaSurface());
}

void BrowserMediaPlayerManager::OnTimeUpdate(
    int player_id,
    base::TimeDelta current_timestamp,
    base::TimeTicks current_time_ticks) {
  Send(new MediaPlayerMsg_MediaTimeUpdate(
      RoutingID(), player_id, current_timestamp, current_time_ticks));
}

void BrowserMediaPlayerManager::SetVideoSurface(
    gfx::ScopedJavaSurface surface) {
  MediaPlayerAndroid* player = GetFullscreenPlayer();
  if (!player)
    return;

  bool empty_surface = surface.IsEmpty();
  player->SetVideoSurface(surface.Pass());
  if (empty_surface)
    return;

  if (RenderWidgetHostViewAndroid* view_android =
      static_cast<RenderWidgetHostViewAndroid*>(
          web_contents_->GetRenderWidgetHostView())) {
    view_android->SetOverlayVideoMode(true);
  }
}

void BrowserMediaPlayerManager::OnMediaMetadataChanged(
    int player_id, base::TimeDelta duration, int width, int height,
    bool success) {
  Send(new MediaPlayerMsg_MediaMetadataChanged(
      RoutingID(), player_id, duration, width, height, success));
  if (fullscreen_player_id_ == player_id)
    video_view_->UpdateMediaMetadata();
}

void BrowserMediaPlayerManager::OnPlaybackComplete(int player_id) {
  Send(new MediaPlayerMsg_MediaPlaybackCompleted(RoutingID(), player_id));
  if (fullscreen_player_id_ == player_id)
    video_view_->OnPlaybackComplete();
}

void BrowserMediaPlayerManager::OnMediaInterrupted(int player_id) {
  // Tell WebKit that the audio should be paused, then release all resources
  Send(new MediaPlayerMsg_MediaPlayerReleased(RoutingID(), player_id));
  OnReleaseResources(player_id);
}

void BrowserMediaPlayerManager::OnBufferingUpdate(
    int player_id, int percentage) {
  Send(new MediaPlayerMsg_MediaBufferingUpdate(
      RoutingID(), player_id, percentage));
  if (fullscreen_player_id_ == player_id)
    video_view_->OnBufferingUpdate(percentage);
}

void BrowserMediaPlayerManager::OnSeekRequest(
    int player_id,
    const base::TimeDelta& time_to_seek) {
  Send(new MediaPlayerMsg_SeekRequest(RoutingID(), player_id, time_to_seek));
}

void BrowserMediaPlayerManager::ReleaseAllMediaPlayers() {
  for (ScopedVector<MediaPlayerAndroid>::iterator it = players_.begin();
      it != players_.end(); ++it) {
    if ((*it)->player_id() == fullscreen_player_id_)
      fullscreen_player_is_released_ = true;
    (*it)->Release();
  }
}

void BrowserMediaPlayerManager::OnSeekComplete(
    int player_id,
    const base::TimeDelta& current_time) {
  Send(new MediaPlayerMsg_SeekCompleted(RoutingID(), player_id, current_time));
}

void BrowserMediaPlayerManager::OnError(int player_id, int error) {
  Send(new MediaPlayerMsg_MediaError(RoutingID(), player_id, error));
  if (fullscreen_player_id_ == player_id)
    video_view_->OnMediaPlayerError(error);
}

void BrowserMediaPlayerManager::OnVideoSizeChanged(
    int player_id, int width, int height) {
  Send(new MediaPlayerMsg_MediaVideoSizeChanged(RoutingID(), player_id,
      width, height));
  if (fullscreen_player_id_ == player_id)
    video_view_->OnVideoSizeChanged(width, height);
}

void BrowserMediaPlayerManager::OnAudibleStateChanged(
    int player_id, bool is_audible) {
  audio_monitor_->OnAudibleStateChanged(
      render_frame_host_, player_id, is_audible);
}

void BrowserMediaPlayerManager::OnWaitingForDecryptionKey(int player_id) {
  Send(new MediaPlayerMsg_WaitingForDecryptionKey(RoutingID(), player_id));
}

media::MediaResourceGetter*
BrowserMediaPlayerManager::GetMediaResourceGetter() {
  if (!media_resource_getter_.get()) {
    RenderProcessHost* host = web_contents()->GetRenderProcessHost();
    BrowserContext* context = host->GetBrowserContext();
    StoragePartition* partition = host->GetStoragePartition();
    storage::FileSystemContext* file_system_context =
        partition ? partition->GetFileSystemContext() : NULL;
    // Eventually this needs to be fixed to pass the correct frame rather
    // than just using the main frame.
    media_resource_getter_.reset(new MediaResourceGetterImpl(
        context,
        file_system_context,
        host->GetID(),
        web_contents()->GetMainFrame()->GetRoutingID()));
  }
  return media_resource_getter_.get();
}

media::MediaUrlInterceptor*
BrowserMediaPlayerManager::GetMediaUrlInterceptor() {
  return media_url_interceptor_;
}

MediaPlayerAndroid* BrowserMediaPlayerManager::GetFullscreenPlayer() {
  return GetPlayer(fullscreen_player_id_);
}

MediaPlayerAndroid* BrowserMediaPlayerManager::GetPlayer(int player_id) {
  for (ScopedVector<MediaPlayerAndroid>::iterator it = players_.begin();
      it != players_.end(); ++it) {
    if ((*it)->player_id() == player_id)
      return *it;
  }
  return NULL;
}

void BrowserMediaPlayerManager::RequestFullScreen(int player_id) {
  if (fullscreen_player_id_ == player_id)
    return;

  if (fullscreen_player_id_ != kInvalidMediaPlayerId) {
    // TODO(qinmin): Determine the correct error code we should report to WMPA.
    OnError(player_id, MediaPlayerAndroid::MEDIA_ERROR_DECODE);
    return;
  }

  Send(new MediaPlayerMsg_RequestFullscreen(RoutingID(), player_id));
}

#if defined(VIDEO_HOLE)
void BrowserMediaPlayerManager::AttachExternalVideoSurface(int player_id,
                                                           jobject surface) {
  MediaPlayerAndroid* player = GetPlayer(player_id);
  if (player) {
    player->SetVideoSurface(
        gfx::ScopedJavaSurface::AcquireExternalSurface(surface));
  }
}

void BrowserMediaPlayerManager::DetachExternalVideoSurface(int player_id) {
  MediaPlayerAndroid* player = GetPlayer(player_id);
  if (player)
    player->SetVideoSurface(gfx::ScopedJavaSurface());
}

void BrowserMediaPlayerManager::OnFrameInfoUpdated() {
  if (fullscreen_player_id_ != kInvalidMediaPlayerId)
    return;

  if (external_video_surface_container_)
    external_video_surface_container_->OnFrameInfoUpdated();
}

void BrowserMediaPlayerManager::OnNotifyExternalSurface(
    int player_id, bool is_request, const gfx::RectF& rect) {
  if (!web_contents_)
    return;

  if (is_request) {
    OnRequestExternalSurface(player_id, rect);
  }
  if (external_video_surface_container_) {
    external_video_surface_container_->OnExternalVideoSurfacePositionChanged(
        player_id, rect);
  }
}

void BrowserMediaPlayerManager::ReleasePlayerOfExternalVideoSurfaceIfNeeded(
    int future_player) {
  int current_player = ExternalVideoSurfaceContainer::kInvalidPlayerId;

  if (external_video_surface_container_)
    current_player = external_video_surface_container_->GetCurrentPlayerId();

  if (current_player == ExternalVideoSurfaceContainer::kInvalidPlayerId)
    return;

  if (current_player != future_player)
    OnMediaInterrupted(current_player);
}

void BrowserMediaPlayerManager::OnRequestExternalSurface(
    int player_id, const gfx::RectF& rect) {
  if (!external_video_surface_container_) {
    ContentBrowserClient* client = GetContentClient()->browser();
    external_video_surface_container_.reset(
        client->OverrideCreateExternalVideoSurfaceContainer(web_contents_));
  }
  // It's safe to use base::Unretained(this), because the callbacks will not
  // be called after running ReleaseExternalVideoSurface().
  if (external_video_surface_container_) {
    // In case we're stealing the external surface from another player.
    ReleasePlayerOfExternalVideoSurfaceIfNeeded(player_id);
    external_video_surface_container_->RequestExternalVideoSurface(
        player_id,
        base::Bind(&BrowserMediaPlayerManager::AttachExternalVideoSurface,
                   base::Unretained(this)),
        base::Bind(&BrowserMediaPlayerManager::DetachExternalVideoSurface,
                   base::Unretained(this)));
  }
}
#endif  // defined(VIDEO_HOLE)

void BrowserMediaPlayerManager::OnEnterFullscreen(int player_id) {
  DCHECK_EQ(fullscreen_player_id_, kInvalidMediaPlayerId);
#if defined(VIDEO_HOLE)
  // If this fullscreen player is started when another player
  // uses the external surface, release that other player.
  ReleasePlayerOfExternalVideoSurfaceIfNeeded(player_id);
  if (external_video_surface_container_)
    external_video_surface_container_->ReleaseExternalVideoSurface(player_id);
#endif  // defined(VIDEO_HOLE)
  if (video_view_.get()) {
    fullscreen_player_id_ = player_id;
    video_view_->OpenVideo();
    return;
  } else if (!ContentVideoView::GetInstance()) {
    // In Android WebView, two ContentViewCores could both try to enter
    // fullscreen video, we just ignore the second one.
    video_view_.reset(new ContentVideoView(this));
    base::android::ScopedJavaLocalRef<jobject> j_content_video_view =
        video_view_->GetJavaObject(base::android::AttachCurrentThread());
    if (!j_content_video_view.is_null()) {
      fullscreen_player_id_ = player_id;
      return;
    }
  }

  // Force the second video to exit fullscreen.
  Send(new MediaPlayerMsg_DidExitFullscreen(RoutingID(), player_id));
  video_view_.reset();
}

void BrowserMediaPlayerManager::OnExitFullscreen(int player_id) {
  if (fullscreen_player_id_ == player_id) {
    MediaPlayerAndroid* player = GetPlayer(player_id);
    if (player)
      player->SetVideoSurface(gfx::ScopedJavaSurface());
    video_view_->OnExitFullscreen();
  }
}

void BrowserMediaPlayerManager::OnInitialize(
    const MediaPlayerHostMsg_Initialize_Params& media_player_params) {
  DCHECK(media_player_params.type != MEDIA_PLAYER_TYPE_MEDIA_SOURCE ||
      media_player_params.demuxer_client_id > 0)
      << "Media source players must have positive demuxer client IDs: "
      << media_player_params.demuxer_client_id;

  RemovePlayer(media_player_params.player_id);

  RenderProcessHostImpl* host = static_cast<RenderProcessHostImpl*>(
      web_contents()->GetRenderProcessHost());
  MediaPlayerAndroid* player =
      CreateMediaPlayer(media_player_params,
                        host->GetBrowserContext()->IsOffTheRecord(),
                        host->browser_demuxer_android().get());

  if (!player)
    return;

  AddPlayer(player);
}

void BrowserMediaPlayerManager::OnStart(int player_id) {
  MediaPlayerAndroid* player = GetPlayer(player_id);
  if (!player)
    return;
  player->Start();
  if (fullscreen_player_id_ == player_id && fullscreen_player_is_released_) {
    video_view_->OpenVideo();
    fullscreen_player_is_released_ = false;
  }
}

void BrowserMediaPlayerManager::OnSeek(
    int player_id,
    const base::TimeDelta& time) {
  MediaPlayerAndroid* player = GetPlayer(player_id);
  if (player)
    player->SeekTo(time);
}

void BrowserMediaPlayerManager::OnPause(
    int player_id,
    bool is_media_related_action) {
  MediaPlayerAndroid* player = GetPlayer(player_id);
  if (player)
    player->Pause(is_media_related_action);
}

void BrowserMediaPlayerManager::OnSetVolume(int player_id, double volume) {
  MediaPlayerAndroid* player = GetPlayer(player_id);
  if (player)
    player->SetVolume(volume);
}

void BrowserMediaPlayerManager::OnSetPoster(int player_id, const GURL& url) {
  // To be overridden by subclasses.
}

void BrowserMediaPlayerManager::OnReleaseResources(int player_id) {
  MediaPlayerAndroid* player = GetPlayer(player_id);
  if (player)
    ReleasePlayer(player);
  if (player_id == fullscreen_player_id_)
    fullscreen_player_is_released_ = true;
}

void BrowserMediaPlayerManager::OnDestroyPlayer(int player_id) {
  RemovePlayer(player_id);
  if (fullscreen_player_id_ == player_id)
    fullscreen_player_id_ = kInvalidMediaPlayerId;
}

void BrowserMediaPlayerManager::OnRequestRemotePlayback(int /* player_id */) {
  // Does nothing if we don't have a remote player
}

void BrowserMediaPlayerManager::OnRequestRemotePlaybackControl(
    int /* player_id */) {
  // Does nothing if we don't have a remote player
}

void BrowserMediaPlayerManager::AddPlayer(MediaPlayerAndroid* player) {
  DCHECK(!GetPlayer(player->player_id()));
  players_.push_back(player);
}

void BrowserMediaPlayerManager::RemovePlayer(int player_id) {
  for (ScopedVector<MediaPlayerAndroid>::iterator it = players_.begin();
      it != players_.end(); ++it) {
    if ((*it)->player_id() == player_id) {
      ReleaseMediaResources(player_id);
      (*it)->DeleteOnCorrectThread();
      players_.weak_erase(it);
      audio_monitor_->RemovePlayer(render_frame_host_, player_id);
      break;
    }
  }
}

scoped_ptr<media::MediaPlayerAndroid> BrowserMediaPlayerManager::SwapPlayer(
      int player_id, media::MediaPlayerAndroid* player) {
  media::MediaPlayerAndroid* previous_player = NULL;
  for (ScopedVector<MediaPlayerAndroid>::iterator it = players_.begin();
      it != players_.end(); ++it) {
    if ((*it)->player_id() == player_id) {
      previous_player = *it;
      ReleaseMediaResources(player_id);
      players_.weak_erase(it);
      players_.push_back(player);
      break;
    }
  }
  return scoped_ptr<media::MediaPlayerAndroid>(previous_player);
}

int BrowserMediaPlayerManager::RoutingID() {
  return render_frame_host_->GetRoutingID();
}

bool BrowserMediaPlayerManager::Send(IPC::Message* msg) {
  return render_frame_host_->Send(msg);
}

void BrowserMediaPlayerManager::ReleaseFullscreenPlayer(
    MediaPlayerAndroid* player) {
  ReleasePlayer(player);
}

void BrowserMediaPlayerManager::OnMediaResourcesRequested(int player_id) {
  int num_active_player = 0;
  ScopedVector<MediaPlayerAndroid>::iterator it;
  for (it = players_.begin(); it != players_.end(); ++it) {
    if (!(*it)->IsPlayerReady())
      continue;

    // The player is already active, ignore it.
    if ((*it)->player_id() == player_id)
      return;
    else
      num_active_player++;
  }

  // Number of active players are less than the threshold, do nothing.
  if (num_active_player < kMediaPlayerThreshold)
    return;

  for (it = players_.begin(); it != players_.end(); ++it) {
    if ((*it)->IsPlayerReady() && !(*it)->IsPlaying() &&
        fullscreen_player_id_ != (*it)->player_id()) {
      ReleasePlayer(*it);
      Send(new MediaPlayerMsg_MediaPlayerReleased(RoutingID(),
                                                  (*it)->player_id()));
    }
  }
}

void BrowserMediaPlayerManager::ReleaseMediaResources(int player_id) {
#if defined(VIDEO_HOLE)
  if (external_video_surface_container_)
    external_video_surface_container_->ReleaseExternalVideoSurface(player_id);
#endif  // defined(VIDEO_HOLE)
}

void BrowserMediaPlayerManager::ReleasePlayer(MediaPlayerAndroid* player) {
  player->Release();
  ReleaseMediaResources(player->player_id());
}

}  // namespace content
