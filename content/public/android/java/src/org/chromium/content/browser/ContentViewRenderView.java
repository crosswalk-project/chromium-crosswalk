// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.SurfaceTexture;
import android.os.Build;
import android.os.Handler;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.TextureView;
import android.view.TextureView.SurfaceTextureListener;
import android.widget.FrameLayout;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;
import org.chromium.base.ObserverList;
import org.chromium.base.ObserverList.RewindableIterator;
import org.chromium.base.TraceEvent;
import org.chromium.ui.base.WindowAndroid;

/***
 * This view is used by a ContentView to render its content.
 * Call {@link #setCurrentContentView(ContentView)} with the contentView that should be displayed.
 * Note that only one ContentView can be shown at a time.
 */
@JNINamespace("content")
public class ContentViewRenderView extends FrameLayout {
    private static final int MAX_SWAP_BUFFER_COUNT = 2;

    // The native side of this object.
    private long mNativeContentViewRenderView;
    private final SurfaceHolder.Callback mSurfaceCallback;

    private final SurfaceView mSurfaceView;
    private final VSyncAdapter mVSyncAdapter;

    // Enum for the type of compositing surface:
    //   SURFACE_VIEW - Use SurfaceView as compositing surface which
    //                  has a bit performance advantage
    //   TEXTURE_VIEW - Use TextureView as compositing surface which
    //                  supports animation on the View
    public enum CompositingSurfaceType { SURFACE_VIEW, TEXTURE_VIEW };

    // The stuff for TextureView usage. It is not a good practice to mix 2 different
    // implementations into one single class. However, for the sake of reducing the
    // effort of rebasing maintanence in future, here we avoid heavily changes in
    // this class.
    private TextureView mTextureView;
    private Surface mSurface;
    private CompositingSurfaceType mCompositingSurfaceType;

    private int mPendingRenders;
    private int mPendingSwapBuffers;
    private boolean mNeedToRender;

    private ContentView mCurrentContentView;

    // The listener which will be triggered when below two conditions become valid.
    // 1. The view has been initialized and ready to draw content to the screen.
    // 2. The compositor finished compositing and the OpenGL buffers has been swapped.
    //    Which means the view has been updated with visually non-empty content.
    // This listener will be triggered only once after registered.
    private FirstRenderedFrameListener mFirstRenderedFrameListener;
    private boolean mFirstFrameReceived;

    private final Runnable mRenderRunnable = new Runnable() {
        @Override
        public void run() {
            render();
        }
    };

    public interface FirstRenderedFrameListener{
        public void onFirstFrameReceived();
    }

    // Initialize the TextureView for rendering ContentView and configure the callback
    // listeners.
    private void initTextureView(Context context) {
        mTextureView = new TextureView(context);
        mTextureView.setBackgroundColor(Color.WHITE);

        mTextureView.setSurfaceTextureListener(new SurfaceTextureListener() {
            @Override
            public void onSurfaceTextureAvailable(SurfaceTexture surfaceTexture,
                    int width, int height) {
                assert mNativeContentViewRenderView != 0;

                mSurface = new Surface(surfaceTexture);
                nativeSurfaceCreated(mNativeContentViewRenderView);
                // Force to trigger the compositor to start working.
                onSurfaceTextureSizeChanged(surfaceTexture, width, height);

                mPendingSwapBuffers = 0;
                mPendingRenders = 0;
                onReadyToRender();
            }

            @Override
            public void onSurfaceTextureSizeChanged(SurfaceTexture surfaceTexture,
                    int width, int height) {
                assert mNativeContentViewRenderView != 0 && mSurface != null;
                assert surfaceTexture == mTextureView.getSurfaceTexture();
                assert mSurface != null;

                // Here we hard-code the pixel format since the native part requires
                // the format parameter to decide if the compositing surface should be
                // replaced with a new one when the format is changed.
                //
                // If TextureView is used, the surface won't be possible to changed,
                // so that the format is also not changed. There is no special reason
                // to use RGBA_8888 value since the native part won't use its real
                // value to do something for drawing.
                //
                // TODO(hmin): Figure out how to get pixel format from SurfaceTexture.
                int format = PixelFormat.RGBA_8888;
                nativeSurfaceChanged(mNativeContentViewRenderView,
                        format, width, height, mSurface);
                if (mCurrentContentView != null) {
                    mCurrentContentView.getContentViewCore().onPhysicalBackingSizeChanged(
                            width, height);
                }
            }

            @Override
            public boolean onSurfaceTextureDestroyed(SurfaceTexture surfaceTexture) {
                assert mNativeContentViewRenderView != 0;
                nativeSurfaceDestroyed(mNativeContentViewRenderView);

                // Release the underlying surface to make it invalid.
                mSurface.release();
                mSurface = null;
                return true;
            }

            @Override
            public void onSurfaceTextureUpdated(SurfaceTexture surfaceTexture) {
                // Do nothing since the SurfaceTexture won't be updated via updateTexImage().
            }
        });
    }

    public ContentViewRenderView(Context context, WindowAndroid rootWindow) {
        this(context, rootWindow, CompositingSurfaceType.SURFACE_VIEW);
    }

    /**
     * Constructs a new ContentViewRenderView that should be can to a view hierarchy.
     * Native code should add/remove the layers to be rendered through the ContentViewLayerRenderer.
     * @param context The context used to create this.
     * @param useTextureView True if TextureView is used as compositing target surface,
     *                       otherwise SurfaceView is used.
     */
    public ContentViewRenderView(Context context, WindowAndroid rootWindow,
                CompositingSurfaceType surfaceType) {
        super(context);
        assert rootWindow != null;
        mNativeContentViewRenderView = nativeInit(rootWindow.getNativePointer());
        assert mNativeContentViewRenderView != 0;

        mCompositingSurfaceType = surfaceType;
        if (surfaceType == CompositingSurfaceType.TEXTURE_VIEW) {
            initTextureView(context);

            mVSyncAdapter = new VSyncAdapter(getContext());
            addView(mTextureView,
                    new FrameLayout.LayoutParams(
                            FrameLayout.LayoutParams.MATCH_PARENT,
                            FrameLayout.LayoutParams.MATCH_PARENT));

            // Avoid compiler warning.
            mSurfaceView = null;
            mSurfaceCallback = null;
            return;
        }

        mSurfaceView = createSurfaceView(getContext());
        mSurfaceView.setZOrderMediaOverlay(true);
        mSurfaceCallback = new SurfaceHolder.Callback() {
            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                assert mNativeContentViewRenderView != 0;
                nativeSurfaceChanged(mNativeContentViewRenderView,
                        format, width, height, holder.getSurface());
                if (mCurrentContentView != null) {
                    mCurrentContentView.getContentViewCore().onPhysicalBackingSizeChanged(
                            width, height);
                }
            }

            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                setSurfaceViewBackgroundColor(Color.WHITE);

                assert mNativeContentViewRenderView != 0;
                nativeSurfaceCreated(mNativeContentViewRenderView);

                mPendingSwapBuffers = 0;
                mPendingRenders = 0;

                onReadyToRender();
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                assert mNativeContentViewRenderView != 0;
                nativeSurfaceDestroyed(mNativeContentViewRenderView);
            }
        };
        mSurfaceView.getHolder().addCallback(mSurfaceCallback);

        mVSyncAdapter = new VSyncAdapter(getContext());
        addView(mSurfaceView,
                new FrameLayout.LayoutParams(
                        FrameLayout.LayoutParams.MATCH_PARENT,
                        FrameLayout.LayoutParams.MATCH_PARENT));
    }

    private class VSyncAdapter implements VSyncManager.Provider, VSyncMonitor.Listener {
        private final VSyncMonitor mVSyncMonitor;
        private boolean mVSyncNotificationEnabled;
        private VSyncManager.Listener mVSyncListener;
        private final ObserverList<VSyncManager.Listener> mCurrentVSyncListeners;
        private final RewindableIterator<VSyncManager.Listener> mCurrentVSyncListenersIterator;

        // The VSyncMonitor gives the timebase for the actual vsync, but we don't want render until
        // we have had a chance for input events to propagate to the compositor thread. This takes
        // 3 ms typically, so we adjust the vsync timestamps forward by a bit to give input events a
        // chance to arrive.
        private static final long INPUT_EVENT_LAG_FROM_VSYNC_MICROSECONDS = 3200;

        VSyncAdapter(Context context) {
            mVSyncMonitor = new VSyncMonitor(context, this);
            mCurrentVSyncListeners = new ObserverList<VSyncManager.Listener>();
            mCurrentVSyncListenersIterator = mCurrentVSyncListeners.rewindableIterator();
        }

        @Override
        public void onVSync(VSyncMonitor monitor, long vsyncTimeMicros) {
            if (mNeedToRender) {
                if (mPendingSwapBuffers + mPendingRenders <= MAX_SWAP_BUFFER_COUNT) {
                    mNeedToRender = false;
                    mPendingRenders++;
                    render();
                } else {
                    TraceEvent.instant("ContentViewRenderView:bail");
                }
            }

            if (mVSyncListener != null) {
                if (mVSyncNotificationEnabled) {
                    for (mCurrentVSyncListenersIterator.rewind();
                            mCurrentVSyncListenersIterator.hasNext();) {
                        mCurrentVSyncListenersIterator.next().onVSync(vsyncTimeMicros);
                    }
                    mVSyncMonitor.requestUpdate();
                } else {
                    // Compensate for input event lag. Input events are delivered immediately on
                    // pre-JB releases, so this adjustment is only done for later versions.
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
                        vsyncTimeMicros += INPUT_EVENT_LAG_FROM_VSYNC_MICROSECONDS;
                    }
                    mVSyncListener.updateVSync(vsyncTimeMicros,
                            mVSyncMonitor.getVSyncPeriodInMicroseconds());
                }
            }
        }

        @Override
        public void registerVSyncListener(VSyncManager.Listener listener) {
            if (!mVSyncNotificationEnabled) mVSyncMonitor.requestUpdate();
            mCurrentVSyncListeners.addObserver(listener);
            mVSyncNotificationEnabled = true;
        }

        @Override
        public void unregisterVSyncListener(VSyncManager.Listener listener) {
            mCurrentVSyncListeners.removeObserver(listener);
            if (mCurrentVSyncListeners.isEmpty()) {
                mVSyncNotificationEnabled = false;
            }
        }

        void setVSyncListener(VSyncManager.Listener listener) {
            mVSyncListener = listener;
            if (mVSyncListener != null) mVSyncMonitor.requestUpdate();
        }

        void requestUpdate() {
            mVSyncMonitor.requestUpdate();
        }
    }

    /**
     * Sets the background color of the surface view.  This method is necessary because the
     * background color of ContentViewRenderView itself is covered by the background of
     * SurfaceView.
     * @param color The color of the background.
     */
    public void setSurfaceViewBackgroundColor(int color) {
        if (mSurfaceView != null) {
            mSurfaceView.setBackgroundColor(color);
        }
    }

    /**
     * Should be called when the ContentViewRenderView is not needed anymore so its associated
     * native resource can be freed.
     */
    public void destroy() {
        if (mCompositingSurfaceType == CompositingSurfaceType.TEXTURE_VIEW) {
            mTextureView.setSurfaceTextureListener(null);
            if (mSurface != null) {
                mSurface.release();
                mSurface = null;
            }
        } else {
            mSurfaceView.getHolder().removeCallback(mSurfaceCallback);
        }

        nativeDestroy(mNativeContentViewRenderView);
        mNativeContentViewRenderView = 0;
    }

    /**
     * Makes the passed ContentView the one displayed by this ContentViewRenderView.
     */
    public void setCurrentContentView(ContentView contentView) {
        assert mNativeContentViewRenderView != 0;
        mCurrentContentView = contentView;

        ContentViewCore contentViewCore =
                contentView != null ? contentView.getContentViewCore() : null;

        nativeSetCurrentContentView(mNativeContentViewRenderView,
                contentViewCore != null ? contentViewCore.getNativeContentViewCore() : 0);

        if (contentViewCore != null) {
            contentViewCore.onPhysicalBackingSizeChanged(getWidth(), getHeight());
            mVSyncAdapter.setVSyncListener(contentViewCore.getVSyncListener(mVSyncAdapter));
        }
    }

    public void registerFirstRenderedFrameListener(FirstRenderedFrameListener listener) {
        mFirstRenderedFrameListener = listener;
        if (mFirstFrameReceived && mFirstRenderedFrameListener != null) {
            mFirstRenderedFrameListener.onFirstFrameReceived();
        }
    }

    /**
     * This method should be subclassed to provide actions to be performed once the view is ready to
     * render.
     */
    protected void onReadyToRender() {
    }

    /**
     * This method could be subclassed optionally to provide a custom SurfaceView object to
     * this ContentViewRenderView.
     * @param context The context used to create the SurfaceView object.
     * @return The created SurfaceView object.
     */
    protected SurfaceView createSurfaceView(Context context) {
        return new SurfaceView(context) {
            @Override
            public void onDraw(Canvas canvas) {
                // We only need to draw to software canvases, which are used for taking screenshots.
                if (canvas.isHardwareAccelerated()) return;
                Bitmap bitmap = Bitmap.createBitmap(getWidth(), getHeight(),
                        Bitmap.Config.ARGB_8888);
                if (nativeCompositeToBitmap(mNativeContentViewRenderView, bitmap)) {
                    canvas.drawBitmap(bitmap, 0, 0, null);
                }
            }
        };
    }

    /**
     * @return whether the surface view is initialized and ready to render.
     */
    public boolean isInitialized() {
        return mSurfaceView.getHolder().getSurface() != null || mSurface != null;
    }

    /**
     * Enter or leave overlay video mode.
     * @param enabled Whether overlay mode is enabled.
     */
    public void setOverlayVideoMode(boolean enabled) {
        if (mCompositingSurfaceType == CompositingSurfaceType.TEXTURE_VIEW) {
            nativeSetOverlayVideoMode(mNativeContentViewRenderView, enabled);
            return;
        }

        int format = enabled ? PixelFormat.TRANSLUCENT : PixelFormat.OPAQUE;
        mSurfaceView.getHolder().setFormat(format);
        nativeSetOverlayVideoMode(mNativeContentViewRenderView, enabled);
    }

    @CalledByNative
    private void requestRender() {
        ContentViewCore contentViewCore = mCurrentContentView != null ?
                mCurrentContentView.getContentViewCore() : null;

        boolean rendererHasFrame =
                contentViewCore != null && contentViewCore.consumePendingRendererFrame();

        if (rendererHasFrame && mPendingSwapBuffers + mPendingRenders < MAX_SWAP_BUFFER_COUNT) {
            TraceEvent.instant("requestRender:now");
            mNeedToRender = false;
            mPendingRenders++;

            // The handler can be null if we are detached from the window.  Calling
            // {@link View#post(Runnable)} properly handles this case, but we lose the front of
            // queue behavior.  That is okay for this edge case.
            Handler handler = getHandler();
            if (handler != null) {
                handler.postAtFrontOfQueue(mRenderRunnable);
            } else {
                post(mRenderRunnable);
            }
            mVSyncAdapter.requestUpdate();
        } else if (mPendingRenders <= 0) {
            assert mPendingRenders == 0;
            TraceEvent.instant("requestRender:later");
            mNeedToRender = true;
            mVSyncAdapter.requestUpdate();
        }
    }

    @CalledByNative
    private void onSwapBuffersCompleted() {
        TraceEvent.instant("onSwapBuffersCompleted");

        if (!mFirstFrameReceived && mCurrentContentView != null &&
                mCurrentContentView.getContentViewCore().isReady()) {
            mFirstFrameReceived = true;
            if (mFirstRenderedFrameListener != null) {
                mFirstRenderedFrameListener.onFirstFrameReceived();
            }
        }

        if (mPendingSwapBuffers == MAX_SWAP_BUFFER_COUNT && mNeedToRender) requestRender();
        if (mPendingSwapBuffers > 0) mPendingSwapBuffers--;
    }

    private void render() {
        if (mPendingRenders > 0) mPendingRenders--;

        // Waiting for the content view contents to be ready avoids compositing
        // when the surface texture is still empty.
        if (mCurrentContentView == null) return;
        ContentViewCore contentViewCore = mCurrentContentView.getContentViewCore();
        if (contentViewCore == null || !contentViewCore.isReady()) {
            return;
        }

        boolean didDraw = nativeComposite(mNativeContentViewRenderView);
        if (didDraw) {
            mPendingSwapBuffers++;
            // Ignore if TextureView is used.
            if (mCompositingSurfaceType == CompositingSurfaceType.TEXTURE_VIEW) return;
            if (mSurfaceView.getBackground() != null) {
                post(new Runnable() {
                    @Override
                    public void run() {
                        mSurfaceView.setBackgroundResource(0);
                    }
                });
            }
        }
    }

    private native long nativeInit(long rootWindowNativePointer);
    private native void nativeDestroy(long nativeContentViewRenderView);
    private native void nativeSetCurrentContentView(long nativeContentViewRenderView,
            long nativeContentView);
    private native void nativeSurfaceCreated(long nativeContentViewRenderView);
    private native void nativeSurfaceDestroyed(long nativeContentViewRenderView);
    private native void nativeSurfaceChanged(long nativeContentViewRenderView,
            int format, int width, int height, Surface surface);
    private native boolean nativeComposite(long nativeContentViewRenderView);
    private native boolean nativeCompositeToBitmap(long nativeContentViewRenderView, Bitmap bitmap);
    private native void nativeSetOverlayVideoMode(long nativeContentViewRenderView,
            boolean enabled);
}
