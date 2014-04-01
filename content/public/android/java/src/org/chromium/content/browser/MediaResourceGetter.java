// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.Context;
import android.content.pm.PackageManager;
import android.media.MediaMetadataRetriever;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Log;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;
import org.chromium.base.PathUtils;

import java.io.File;
import java.util.HashMap;

/**
 * Java counterpart of android MediaResourceGetter.
 */
@JNINamespace("content")
class MediaResourceGetter {

    private static final String TAG = "MediaResourceGetter";

    private static class MediaMetadata {
        private final int mDurationInMilliseconds;
        private final int mWidth;
        private final int mHeight;
        private final boolean mSuccess;

        private MediaMetadata(int durationInMilliseconds, int width, int height, boolean success) {
            mDurationInMilliseconds = durationInMilliseconds;
            mWidth = width;
            mHeight = height;
            mSuccess = success;
        }

        @CalledByNative("MediaMetadata")
        private int getDurationInMilliseconds() { return mDurationInMilliseconds; }

        @CalledByNative("MediaMetadata")
        private int getWidth() { return mWidth; }

        @CalledByNative("MediaMetadata")
        private int getHeight() { return mHeight; }

        @CalledByNative("MediaMetadata")
        private boolean isSuccess() { return mSuccess; }
    }

    @CalledByNative
    private static MediaMetadata extractMediaMetadata(Context context, String url, String cookies,
            String userAgent) {
        int durationInMilliseconds = 0;
        int width = 0;
        int height = 0;
        boolean success = false;
        if ("GT-I9100".contentEquals(android.os.Build.MODEL)
                && android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.JELLY_BEAN) {
            return new MediaMetadata(0, 0, 0, success);
        }
        // TODO(qinmin): use ConnectionTypeObserver to listen to the network type change.
        ConnectivityManager mConnectivityManager =
                (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        if (mConnectivityManager != null) {
            if (context.checkCallingOrSelfPermission(
                    android.Manifest.permission.ACCESS_NETWORK_STATE) !=
                    PackageManager.PERMISSION_GRANTED) {
                return new MediaMetadata(0, 0, 0, false);
            }

            NetworkInfo info = mConnectivityManager.getActiveNetworkInfo();
            if (info == null) {
                return new MediaMetadata(durationInMilliseconds, width, height, success);
            }
            switch (info.getType()) {
                case ConnectivityManager.TYPE_ETHERNET:
                case ConnectivityManager.TYPE_WIFI:
                    break;
                case ConnectivityManager.TYPE_WIMAX:
                case ConnectivityManager.TYPE_MOBILE:
                default:
                    return new MediaMetadata(durationInMilliseconds, width, height, success);
            }
        }

        MediaMetadataRetriever retriever = new MediaMetadataRetriever();
        try {
            Uri uri = Uri.parse(url);
            String scheme = uri.getScheme();
            // Keep app uri have the same behavior with file asset uri.
            if (scheme.equals("app")) {
                return new MediaMetadata(durationInMilliseconds, width, height, success);
            }
            if (scheme == null || scheme.equals("file")) {
                File file = new File(uri.getPath());
                String path = file.getAbsolutePath();
                if (file.exists() && (path.startsWith("/mnt/sdcard/") ||
                        path.startsWith("/sdcard/") ||
                        path.startsWith(PathUtils.getExternalStorageDirectory()) ||
                        path.startsWith(context.getCacheDir().getAbsolutePath()))) {
                    retriever.setDataSource(path);
                } else {
                    return new MediaMetadata(durationInMilliseconds, width, height, success);
                }
            } else {
                HashMap<String, String> headersMap = new HashMap<String, String>();
                if (!TextUtils.isEmpty(cookies)) {
                    headersMap.put("Cookie", cookies);
                }
                if (!TextUtils.isEmpty(userAgent)) {
                    headersMap.put("User-Agent", userAgent);
                }
                retriever.setDataSource(url, headersMap);
            }
            String duration = retriever.extractMetadata(
                    MediaMetadataRetriever.METADATA_KEY_DURATION);
            String videoWidth = retriever.extractMetadata(
                    MediaMetadataRetriever.METADATA_KEY_VIDEO_WIDTH);
            String videoHeight = retriever.extractMetadata(
                    MediaMetadataRetriever.METADATA_KEY_VIDEO_HEIGHT);
            if (duration == null || videoWidth == null || videoHeight == null) {
                return new MediaMetadata(durationInMilliseconds, width, height, success);
            }
            durationInMilliseconds = Integer.parseInt(duration);
            width = Integer.parseInt(videoWidth);
            height = Integer.parseInt(videoHeight);
            success = true;
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "Invalid url: " + e);
        } catch (RuntimeException e) {
            Log.e(TAG, "Invalid url: " + e);
        }
        return new MediaMetadata(durationInMilliseconds, width, height, success);
    }
}
