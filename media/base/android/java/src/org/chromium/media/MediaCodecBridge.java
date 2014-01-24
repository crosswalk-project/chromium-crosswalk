// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.media;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaCrypto;
import android.media.MediaFormat;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;

/**
 * A wrapper of the MediaCodec class to facilitate exception capturing and
 * audio rendering.
 */
@JNINamespace("media")
class MediaCodecBridge {
    private static final String TAG = "MediaCodecBridge";

    // Error code for MediaCodecBridge. Keep this value in sync with
    // MediaCodecStatus in media_codec_bridge.h.
    private static final int MEDIA_CODEC_OK = 0;
    private static final int MEDIA_CODEC_DEQUEUE_INPUT_AGAIN_LATER = 1;
    private static final int MEDIA_CODEC_DEQUEUE_OUTPUT_AGAIN_LATER = 2;
    private static final int MEDIA_CODEC_OUTPUT_BUFFERS_CHANGED = 3;
    private static final int MEDIA_CODEC_OUTPUT_FORMAT_CHANGED = 4;
    private static final int MEDIA_CODEC_INPUT_END_OF_STREAM = 5;
    private static final int MEDIA_CODEC_OUTPUT_END_OF_STREAM = 6;
    private static final int MEDIA_CODEC_NO_KEY = 7;
    private static final int MEDIA_CODEC_STOPPED = 8;
    private static final int MEDIA_CODEC_ERROR = 9;

    // Codec direction.  Keep this in sync with media_codec_bridge.h.
    private static final int MEDIA_CODEC_DECODER = 0;
    private static final int MEDIA_CODEC_ENCODER = 1;

    // After a flush(), dequeueOutputBuffer() can often produce empty presentation timestamps
    // for several frames. As a result, the player may find that the time does not increase
    // after decoding a frame. To detect this, we check whether the presentation timestamp from
    // dequeueOutputBuffer() is larger than input_timestamp - MAX_PRESENTATION_TIMESTAMP_SHIFT_US
    // after a flush. And we set the presentation timestamp from dequeueOutputBuffer() to be
    // non-decreasing for the remaining frames.
    private static final long MAX_PRESENTATION_TIMESTAMP_SHIFT_US = 100000;

    private ByteBuffer[] mInputBuffers;
    private ByteBuffer[] mOutputBuffers;

    private MediaCodec mMediaCodec;
    private AudioTrack mAudioTrack;
    private boolean mFlushed;
    private long mLastPresentationTimeUs;

    private static class DequeueInputResult {
        private final int mStatus;
        private final int mIndex;

        private DequeueInputResult(int status, int index) {
            mStatus = status;
            mIndex = index;
        }

        @CalledByNative("DequeueInputResult")
        private int status() { return mStatus; }

        @CalledByNative("DequeueInputResult")
        private int index() { return mIndex; }
    }

    /**
     * This class represents supported android codec information.
     */
    private static class CodecInfo {
        private final String mCodecType;  // e.g. "video/x-vnd.on2.vp8".
        private final String mCodecName;  // e.g. "OMX.google.vp8.decoder".
        private final int mDirection;

        private CodecInfo(String codecType, String codecName,
                          int direction) {
            mCodecType = codecType;
            mCodecName = codecName;
            mDirection = direction;
        }

        @CalledByNative("CodecInfo")
        private String codecType() { return mCodecType; }

        @CalledByNative("CodecInfo")
        private String codecName() { return mCodecName; }

        @CalledByNative("CodecInfo")
        private int direction() { return mDirection; }
    }

    private static class DequeueOutputResult {
        private final int mStatus;
        private final int mIndex;
        private final int mFlags;
        private final int mOffset;
        private final long mPresentationTimeMicroseconds;
        private final int mNumBytes;

        private DequeueOutputResult(int status, int index, int flags, int offset,
                long presentationTimeMicroseconds, int numBytes) {
            mStatus = status;
            mIndex = index;
            mFlags = flags;
            mOffset = offset;
            mPresentationTimeMicroseconds = presentationTimeMicroseconds;
            mNumBytes = numBytes;
        }

        @CalledByNative("DequeueOutputResult")
        private int status() { return mStatus; }

        @CalledByNative("DequeueOutputResult")
        private int index() { return mIndex; }

        @CalledByNative("DequeueOutputResult")
        private int flags() { return mFlags; }

        @CalledByNative("DequeueOutputResult")
        private int offset() { return mOffset; }

        @CalledByNative("DequeueOutputResult")
        private long presentationTimeMicroseconds() { return mPresentationTimeMicroseconds; }

        @CalledByNative("DequeueOutputResult")
        private int numBytes() { return mNumBytes; }
    }

    /**
     * Get a list of supported android codec mimes.
     */
    @CalledByNative
    private static CodecInfo[] getCodecsInfo() {
        // Return the first (highest-priority) codec for each MIME type.
        Map<String, CodecInfo> encoderInfoMap = new HashMap<String, CodecInfo>();
        Map<String, CodecInfo> decoderInfoMap = new HashMap<String, CodecInfo>();
        int count = MediaCodecList.getCodecCount();
        for (int i = 0; i < count; ++i) {
            MediaCodecInfo info = MediaCodecList.getCodecInfoAt(i);
            int direction =
                info.isEncoder() ? MEDIA_CODEC_ENCODER : MEDIA_CODEC_DECODER;
            String codecString = info.getName();
            String[] supportedTypes = info.getSupportedTypes();
            for (int j = 0; j < supportedTypes.length; ++j) {
                Map<String, CodecInfo> map = info.isEncoder() ? encoderInfoMap : decoderInfoMap;
                if (!map.containsKey(supportedTypes[j])) {
                    map.put(supportedTypes[j], new CodecInfo(
                        supportedTypes[j], codecString, direction));
                }
            }
        }
        ArrayList<CodecInfo> codecInfos = new ArrayList<CodecInfo>(
            decoderInfoMap.size() + encoderInfoMap.size());
        codecInfos.addAll(encoderInfoMap.values());
        codecInfos.addAll(decoderInfoMap.values());
        return codecInfos.toArray(new CodecInfo[codecInfos.size()]);
    }

    private static String getSecureDecoderNameForMime(String mime) {
        int count = MediaCodecList.getCodecCount();
        for (int i = 0; i < count; ++i) {
            MediaCodecInfo info = MediaCodecList.getCodecInfoAt(i);
            if (info.isEncoder()) {
                continue;
            }

            String[] supportedTypes = info.getSupportedTypes();
            for (int j = 0; j < supportedTypes.length; ++j) {
                if (supportedTypes[j].equalsIgnoreCase(mime)) {
                    return info.getName() + ".secure";
                }
            }
        }

        return null;
    }

    private MediaCodecBridge(MediaCodec mediaCodec) {
        assert(mediaCodec != null);
        mMediaCodec = mediaCodec;
        mLastPresentationTimeUs = 0;
        mFlushed = true;
    }

    @CalledByNative
    private static MediaCodecBridge create(String mime, boolean isSecure, int direction) {
        // Creation of ".secure" codecs sometimes crash instead of throwing exceptions
        // on pre-JBMR2 devices.
        if (isSecure && Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN_MR2) {
            return null;
        }
        MediaCodec mediaCodec = null;
        try {
            // |isSecure| only applies to video decoders.
            if (mime.startsWith("video") && isSecure && direction == MEDIA_CODEC_DECODER) {
                mediaCodec = MediaCodec.createByCodecName(getSecureDecoderNameForMime(mime));
            } else {
                if (direction == MEDIA_CODEC_ENCODER) {
                    mediaCodec = MediaCodec.createEncoderByType(mime);
                } else {
                    mediaCodec = MediaCodec.createDecoderByType(mime);
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "Failed to create MediaCodec: " +  mime + ", isSecure: "
                    + isSecure + ", direction: " + direction, e);
        }

        if (mediaCodec == null) {
            return null;
        }

        return new MediaCodecBridge(mediaCodec);
    }

    @CalledByNative
    private void release() {
        String name = mMediaCodec.getName();
        mMediaCodec.stop();
        mMediaCodec.release();
        mMediaCodec = null;
        if (mAudioTrack != null) {
            mAudioTrack.release();
        }
    }

    @CalledByNative
    private boolean start() {
        try {
            mMediaCodec.start();
            mInputBuffers = mMediaCodec.getInputBuffers();
        } catch (IllegalStateException e) {
            Log.e(TAG, "Cannot start the media codec", e);
            return false;
        }
        return true;
    }

    @CalledByNative
    private DequeueInputResult dequeueInputBuffer(long timeoutUs) {
        int status = MEDIA_CODEC_ERROR;
        int index = -1;
        try {
            int index_or_status = mMediaCodec.dequeueInputBuffer(timeoutUs);
            if (index_or_status >= 0) { // index!
                status = MEDIA_CODEC_OK;
                index = index_or_status;
            } else if (index_or_status == MediaCodec.INFO_TRY_AGAIN_LATER) {
                Log.e(TAG, "dequeueInputBuffer: MediaCodec.INFO_TRY_AGAIN_LATER");
                status = MEDIA_CODEC_DEQUEUE_INPUT_AGAIN_LATER;
            } else {
                Log.e(TAG, "Unexpected index_or_status: " + index_or_status);
                assert(false);
            }
        } catch(Exception e) {
            Log.e(TAG, "Failed to dequeue input buffer", e);
        }
        return new DequeueInputResult(status, index);
    }

    @CalledByNative
    private int flush() {
        try {
            mFlushed = true;
            if (mAudioTrack != null) {
                mAudioTrack.flush();
            }
            mMediaCodec.flush();
        } catch(IllegalStateException e) {
            Log.e(TAG, "Failed to flush MediaCodec", e);
            return MEDIA_CODEC_ERROR;
        }
        return MEDIA_CODEC_OK;
    }

    @CalledByNative
    private void stop() {
        mMediaCodec.stop();
        if (mAudioTrack != null) {
            mAudioTrack.pause();
        }
    }

    @CalledByNative
    private int getOutputHeight() {
        return mMediaCodec.getOutputFormat().getInteger(MediaFormat.KEY_HEIGHT);
    }

    @CalledByNative
    private int getOutputWidth() {
        return mMediaCodec.getOutputFormat().getInteger(MediaFormat.KEY_WIDTH);
    }

    @CalledByNative
    private ByteBuffer getInputBuffer(int index) {
        return mInputBuffers[index];
    }

    @CalledByNative
    private ByteBuffer getOutputBuffer(int index) {
        return mOutputBuffers[index];
    }

    @CalledByNative
    private int getInputBuffersCount() {
      return mInputBuffers.length;
    }

    @CalledByNative
    private int getOutputBuffersCount() {
      return mOutputBuffers != null ? mOutputBuffers.length : -1;
    }

    @CalledByNative
    private int getOutputBuffersCapacity() {
      return mOutputBuffers != null ? mOutputBuffers[0].capacity() : -1;
    }

    @CalledByNative
    private boolean getOutputBuffers() {
        try {
            mOutputBuffers = mMediaCodec.getOutputBuffers();
        } catch (IllegalStateException e) {
            Log.e(TAG, "Cannot get output buffers", e);
            return false;
        }
        return true;
    }

    @CalledByNative
    private int queueInputBuffer(
            int index, int offset, int size, long presentationTimeUs, int flags) {
        resetLastPresentationTimeIfNeeded(presentationTimeUs);
        try {
            mMediaCodec.queueInputBuffer(index, offset, size, presentationTimeUs, flags);
        } catch(Exception e) {
            Log.e(TAG, "Failed to queue input buffer", e);
            return MEDIA_CODEC_ERROR;
        }
        return MEDIA_CODEC_OK;
    }

    @CalledByNative
    private void setVideoBitrate(int bps) {
        Bundle b = new Bundle();
        b.putInt(MediaCodec.PARAMETER_KEY_VIDEO_BITRATE, bps);
        mMediaCodec.setParameters(b);
    }

    @CalledByNative
    private void requestKeyFrameSoon() {
        Bundle b = new Bundle();
        b.putInt(MediaCodec.PARAMETER_KEY_REQUEST_SYNC_FRAME, 0);
        mMediaCodec.setParameters(b);
    }

    @CalledByNative
    private int queueSecureInputBuffer(
            int index, int offset, byte[] iv, byte[] keyId, int[] numBytesOfClearData,
            int[] numBytesOfEncryptedData, int numSubSamples, long presentationTimeUs) {
        resetLastPresentationTimeIfNeeded(presentationTimeUs);
        try {
            MediaCodec.CryptoInfo cryptoInfo = new MediaCodec.CryptoInfo();
            cryptoInfo.set(numSubSamples, numBytesOfClearData, numBytesOfEncryptedData,
                    keyId, iv, MediaCodec.CRYPTO_MODE_AES_CTR);
            mMediaCodec.queueSecureInputBuffer(index, offset, cryptoInfo, presentationTimeUs, 0);
        } catch (MediaCodec.CryptoException e) {
            Log.e(TAG, "Failed to queue secure input buffer", e);
            if (e.getErrorCode() == MediaCodec.CryptoException.ERROR_NO_KEY) {
                Log.e(TAG, "MediaCodec.CryptoException.ERROR_NO_KEY");
                return MEDIA_CODEC_NO_KEY;
            }
            Log.e(TAG, "MediaCodec.CryptoException with error code " + e.getErrorCode());
            return MEDIA_CODEC_ERROR;
        } catch(IllegalStateException e) {
            Log.e(TAG, "Failed to queue secure input buffer", e);
            return MEDIA_CODEC_ERROR;
        }
        return MEDIA_CODEC_OK;
    }

    @CalledByNative
    private void releaseOutputBuffer(int index, boolean render) {
        mMediaCodec.releaseOutputBuffer(index, render);
    }

    @CalledByNative
    private DequeueOutputResult dequeueOutputBuffer(long timeoutUs) {
        MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
        int status = MEDIA_CODEC_ERROR;
        int index = -1;
        try {
            int index_or_status = mMediaCodec.dequeueOutputBuffer(info, timeoutUs);
            if (info.presentationTimeUs < mLastPresentationTimeUs) {
                // TODO(qinmin): return a special code through DequeueOutputResult
                // to notify the native code the the frame has a wrong presentation
                // timestamp and should be skipped.
                info.presentationTimeUs = mLastPresentationTimeUs;
            }
            mLastPresentationTimeUs = info.presentationTimeUs;

            if (index_or_status >= 0) { // index!
                status = MEDIA_CODEC_OK;
                index = index_or_status;
            } else if (index_or_status == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
                status = MEDIA_CODEC_OUTPUT_BUFFERS_CHANGED;
            } else if (index_or_status == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                status = MEDIA_CODEC_OUTPUT_FORMAT_CHANGED;
            } else if (index_or_status == MediaCodec.INFO_TRY_AGAIN_LATER) {
                status = MEDIA_CODEC_DEQUEUE_OUTPUT_AGAIN_LATER;
            } else {
                Log.e(TAG, "Unexpected index_or_status: " + index_or_status);
                assert(false);
            }
        } catch (IllegalStateException e) {
            Log.e(TAG, "Failed to dequeue output buffer", e);
        }

        return new DequeueOutputResult(
                status, index, info.flags, info.offset, info.presentationTimeUs, info.size);
    }

    @CalledByNative
    private boolean configureVideo(MediaFormat format, Surface surface, MediaCrypto crypto,
            int flags) {
        try {
            mMediaCodec.configure(format, surface, crypto, flags);
            return true;
        } catch (IllegalStateException e) {
            Log.e(TAG, "Cannot configure the video codec", e);
        }
        return false;
    }

    @CalledByNative
    private static MediaFormat createAudioFormat(String mime, int SampleRate, int ChannelCount) {
        return MediaFormat.createAudioFormat(mime, SampleRate, ChannelCount);
    }

    @CalledByNative
    private static MediaFormat createVideoDecoderFormat(String mime, int width, int height) {
        return MediaFormat.createVideoFormat(mime, width, height);
    }

    @CalledByNative
    private static MediaFormat createVideoEncoderFormat(String mime, int width, int height,
            int bitRate, int frameRate, int iFrameInterval, int colorFormat) {
        MediaFormat format = MediaFormat.createVideoFormat(mime, width, height);
        format.setInteger(MediaFormat.KEY_BIT_RATE, bitRate);
        format.setInteger(MediaFormat.KEY_FRAME_RATE, frameRate);
        format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, iFrameInterval);
        format.setInteger(MediaFormat.KEY_COLOR_FORMAT, colorFormat);
        return format;
    }

    @CalledByNative
    private static void setCodecSpecificData(MediaFormat format, int index, byte[] bytes) {
        String name = null;
        if (index == 0) {
            name = "csd-0";
        } else if (index == 1) {
            name = "csd-1";
        }
        if (name != null) {
            format.setByteBuffer(name, ByteBuffer.wrap(bytes));
        }
    }

    @CalledByNative
    private static void setFrameHasADTSHeader(MediaFormat format) {
        format.setInteger(MediaFormat.KEY_IS_ADTS, 1);
    }

    @CalledByNative
    private boolean configureAudio(MediaFormat format, MediaCrypto crypto, int flags,
            boolean playAudio) {
        try {
            mMediaCodec.configure(format, null, crypto, flags);
            if (playAudio) {
                int sampleRate = format.getInteger(MediaFormat.KEY_SAMPLE_RATE);
                int channelCount = format.getInteger(MediaFormat.KEY_CHANNEL_COUNT);
                int channelConfig = (channelCount == 1) ? AudioFormat.CHANNEL_OUT_MONO :
                        AudioFormat.CHANNEL_OUT_STEREO;
                // Using 16bit PCM for output. Keep this value in sync with
                // kBytesPerAudioOutputSample in media_codec_bridge.cc.
                int minBufferSize = AudioTrack.getMinBufferSize(sampleRate, channelConfig,
                        AudioFormat.ENCODING_PCM_16BIT);
                mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate, channelConfig,
                        AudioFormat.ENCODING_PCM_16BIT, minBufferSize, AudioTrack.MODE_STREAM);
            }
            return true;
        } catch (IllegalStateException e) {
            Log.e(TAG, "Cannot configure the audio codec", e);
        }
        return false;
    }

    @CalledByNative
    private void playOutputBuffer(byte[] buf) {
        if (mAudioTrack != null) {
            if (AudioTrack.PLAYSTATE_PLAYING != mAudioTrack.getPlayState()) {
                mAudioTrack.play();
            }
            int size = mAudioTrack.write(buf, 0, buf.length);
            if (buf.length != size) {
                Log.i(TAG, "Failed to send all data to audio output, expected size: " +
                        buf.length + ", actual size: " + size);
            }
        }
    }

    @CalledByNative
    private void setVolume(double volume) {
        if (mAudioTrack != null) {
            mAudioTrack.setStereoVolume((float) volume, (float) volume);
        }
    }

    private void resetLastPresentationTimeIfNeeded(long presentationTimeUs) {
        if (mFlushed) {
            mLastPresentationTimeUs =
                    Math.max(presentationTimeUs - MAX_PRESENTATION_TIMESTAMP_SHIFT_US, 0);
            mFlushed = false;
        }
    }
}
