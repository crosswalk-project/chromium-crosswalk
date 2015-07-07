// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.remote;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.support.v7.media.MediaRouter;

/**
 * An implementation of {@link LockScreenTransportControl} targeting platforms with an API greater
 * than 15. Extends {@link LockScreenTransportControlV14}, adding support for remote volume control.
 */
@TargetApi(Build.VERSION_CODES.JELLY_BEAN)
class LockScreenTransportControlV16 extends LockScreenTransportControlV14 {

    private final MediaRouter mMediaRouter;

    LockScreenTransportControlV16(Context context) {
        super(context);
        mMediaRouter = MediaRouter.getInstance(context);
    }

    @Override
    protected void register() {
        super.register();
        mMediaRouter.addRemoteControlClient(getRemoteControlClient());
    }

    @Override
    protected void unregister() {
        mMediaRouter.removeRemoteControlClient(getRemoteControlClient());
        mRemoteControlClient.editMetadata(true).apply();
        mRemoteControlClient.setTransportControlFlags(0);
        mAudioManager.abandonAudioFocus(mAudioFocusListener);
        mAudioManager.unregisterMediaButtonEventReceiver(mMediaEventReceiver);
        mAudioManager.unregisterRemoteControlClient(mRemoteControlClient);
        mRemoteControlClient = null;
    }

    protected void updatePlaybackState(int state) {
        if (mRemoteControlClient != null) mRemoteControlClient.setPlaybackState(state);
    }

    protected int getTransportControlFlags() {
        return android.media.RemoteControlClient.FLAG_KEY_MEDIA_PLAY_PAUSE
                | android.media.RemoteControlClient.FLAG_KEY_MEDIA_STOP;
    }

    @Override
    public void onRouteSelected(String name, MediaRouteController mediaRouteController) {
        setScreenName(name);
    }

    @Override
    public void onRouteUnselected(MediaRouteController mediaRouteController) {
        hide();
    }

    @Override
    public void onPrepared(MediaRouteController mediaRouteController) {
    }

    @Override
    public void onError(int error, String errorMessage) {
        // Stop the session for all errors
        hide();
    }

    @Override
    public void onDurationUpdated(long durationMillis) {
        RemoteVideoInfo videoInfo = new RemoteVideoInfo(getVideoInfo());
        videoInfo.durationMillis = durationMillis;
        setVideoInfo(videoInfo);
    }

    @Override
    public void onPositionChanged(long positionMillis) {
        RemoteVideoInfo videoInfo = new RemoteVideoInfo(getVideoInfo());
        videoInfo.currentTimeMillis = positionMillis;
        setVideoInfo(videoInfo);
    }

    @Override
    public void onTitleChanged(String title) {
        RemoteVideoInfo videoInfo = new RemoteVideoInfo(getVideoInfo());
        videoInfo.title = title;
        setVideoInfo(videoInfo);
    }

    @Override
    protected boolean isPlaying() {
        return mIsPlaying;
    }
}
