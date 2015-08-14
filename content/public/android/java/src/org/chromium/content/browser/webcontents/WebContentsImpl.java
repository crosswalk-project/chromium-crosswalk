// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.webcontents;

import android.graphics.Color;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;
import org.chromium.content_public.browser.AccessibilitySnapshotCallback;
import org.chromium.content_public.browser.AccessibilitySnapshotNode;
import org.chromium.content_public.browser.JavaScriptCallback;
import org.chromium.content_public.browser.NavigationController;
import org.chromium.content_public.browser.NavigationTransitionDelegate;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsObserver;
import org.chromium.ui.accessibility.AXTextStyle;

/**
 * The WebContentsImpl Java wrapper to allow communicating with the native WebContentsImpl
 * object.
 */
@JNINamespace("content")
//TODO(tedchoc): Remove the package restriction once this class moves to a non-public content
//               package whose visibility will be enforced via DEPS.
/* package */ class WebContentsImpl implements WebContents {

    private long mNativeWebContentsAndroid;
    private NavigationController mNavigationController;

    // Lazily created proxy observer for handling all Java-based WebContentsObservers.
    private WebContentsObserverProxy mObserverProxy;

    private NavigationTransitionDelegate mNavigationTransitionDelegate = null;

    private WebContentsImpl(
            long nativeWebContentsAndroid, NavigationController navigationController) {
        mNativeWebContentsAndroid = nativeWebContentsAndroid;
        mNavigationController = navigationController;
    }

    @CalledByNative
    private static WebContentsImpl create(
            long nativeWebContentsAndroid, NavigationController navigationController) {
        return new WebContentsImpl(nativeWebContentsAndroid, navigationController);
    }

    @CalledByNative
    private void clearNativePtr() {
        mNativeWebContentsAndroid = 0;
        mNavigationController = null;
        if (mObserverProxy != null) {
            mObserverProxy.destroy();
            mObserverProxy = null;
        }
    }

    @CalledByNative
    private long getNativePointer() {
        return mNativeWebContentsAndroid;
    }

    @Override
    public void destroy() {
        if (mNativeWebContentsAndroid != 0) nativeDestroyWebContents(mNativeWebContentsAndroid);
    }

    @Override
    public NavigationController getNavigationController() {
        return mNavigationController;
    }

    @Override
    public String getTitle() {
        return nativeGetTitle(mNativeWebContentsAndroid);
    }

    @Override
    public String getVisibleUrl() {
        return nativeGetVisibleURL(mNativeWebContentsAndroid);
    }

    @Override
    public boolean isLoading() {
        return nativeIsLoading(mNativeWebContentsAndroid);
    }

    @Override
    public boolean isLoadingToDifferentDocument() {
        return nativeIsLoadingToDifferentDocument(mNativeWebContentsAndroid);
    }

    @Override
    public void stop() {
        nativeStop(mNativeWebContentsAndroid);
    }

    @Override
    public void insertCSS(String css) {
        if (mNativeWebContentsAndroid == 0) return;
        nativeInsertCSS(mNativeWebContentsAndroid, css);
    }

    @Override
    public void onHide() {
        nativeOnHide(mNativeWebContentsAndroid);
    }

    @Override
    public void onShow() {
        nativeOnShow(mNativeWebContentsAndroid);
    }

    @Override
    public void releaseMediaPlayers() {
        nativeReleaseMediaPlayers(mNativeWebContentsAndroid);
    }

    @Override
    public int getBackgroundColor() {
        return nativeGetBackgroundColor(mNativeWebContentsAndroid);
    }

    @Override
    public void addStyleSheetByURL(String url) {
        nativeAddStyleSheetByURL(mNativeWebContentsAndroid, url);
    }

    @Override
    public void showInterstitialPage(
            String url, long interstitialPageDelegateAndroid) {
        nativeShowInterstitialPage(mNativeWebContentsAndroid, url, interstitialPageDelegateAndroid);
    }

    @Override
    public boolean isShowingInterstitialPage() {
        return nativeIsShowingInterstitialPage(mNativeWebContentsAndroid);
    }

    @Override
    public boolean isReady() {
        return nativeIsRenderWidgetHostViewReady(mNativeWebContentsAndroid);
    }

    @Override
    public void exitFullscreen() {
        nativeExitFullscreen(mNativeWebContentsAndroid);
    }

    @Override
    public void updateTopControlsState(boolean enableHiding, boolean enableShowing,
            boolean animate) {
        nativeUpdateTopControlsState(mNativeWebContentsAndroid, enableHiding,
                enableShowing, animate);
    }

    @Override
    public void showImeIfNeeded() {
        nativeShowImeIfNeeded(mNativeWebContentsAndroid);
    }

    @Override
    public void scrollFocusedEditableNodeIntoView() {
        // The native side keeps track of whether the zoom and scroll actually occurred. It is
        // more efficient to do it this way and sometimes fire an unnecessary message rather
        // than synchronize with the renderer and always have an additional message.
        nativeScrollFocusedEditableNodeIntoView(mNativeWebContentsAndroid);
    }

    @Override
    public void selectWordAroundCaret() {
        nativeSelectWordAroundCaret(mNativeWebContentsAndroid);
    }

    @Override
    public String getUrl() {
        return nativeGetURL(mNativeWebContentsAndroid);
    }

    @Override
    public String getLastCommittedUrl() {
        return nativeGetLastCommittedURL(mNativeWebContentsAndroid);
    }

    @Override
    public boolean isIncognito() {
        return nativeIsIncognito(mNativeWebContentsAndroid);
    }

    @Override
    public void resumeResponseDeferredAtStart() {
        nativeResumeResponseDeferredAtStart(mNativeWebContentsAndroid);
    }

    @Override
    public void resumeLoadingCreatedWebContents() {
        nativeResumeLoadingCreatedWebContents(mNativeWebContentsAndroid);
    }

    @Override
    public void setHasPendingNavigationTransitionForTesting() {
        nativeSetHasPendingNavigationTransitionForTesting(mNativeWebContentsAndroid);
    }

    @Override
    public void setNavigationTransitionDelegate(NavigationTransitionDelegate delegate) {
        mNavigationTransitionDelegate = delegate;
    }

    /**
     * Inserts the provided markup sandboxed into the frame.
     */
    @Override
    public void setupTransitionView(String markup) {
        nativeSetupTransitionView(mNativeWebContentsAndroid, markup);
    }

    /**
     * Hides transition elements specified by the selector, and activates any
     * exiting-transition stylesheets.
     */
    @Override
    public void beginExitTransition(String cssSelector, boolean exitToNativeApp) {
        nativeBeginExitTransition(mNativeWebContentsAndroid, cssSelector, exitToNativeApp);
    }

    /**
     * Revert the effect of exit transition.
     */
    @Override
    public void revertExitTransition() {
        nativeRevertExitTransition(mNativeWebContentsAndroid);
    }

    /**
     * Hide transition elements.
     */
    public void hideTransitionElements(String cssSelector) {
        nativeHideTransitionElements(mNativeWebContentsAndroid, cssSelector);
    }

    /**
     * Show transition elements.
     */
    public void showTransitionElements(String cssSelector) {
        nativeShowTransitionElements(mNativeWebContentsAndroid, cssSelector);
    }

    /**
     * Clear the navigation transition data.
     */
    @Override
    public void clearNavigationTransitionData() {
        nativeClearNavigationTransitionData(mNativeWebContentsAndroid);
    }

    /**
     * Fetch transition elements.
     */
    @Override
    public void fetchTransitionElements(String url) {
        nativeFetchTransitionElements(mNativeWebContentsAndroid, url);
    }

    @CalledByNative
    private void didDeferAfterResponseStarted(String markup, String cssSelector,
            String enteringColor) {
        if (mNavigationTransitionDelegate != null) {
            mNavigationTransitionDelegate.didDeferAfterResponseStarted(markup,
                    cssSelector, enteringColor);
        }
    }

    @CalledByNative
    private boolean willHandleDeferAfterResponseStarted() {
        if (mNavigationTransitionDelegate == null) return false;
        return mNavigationTransitionDelegate.willHandleDeferAfterResponseStarted();
    }

    @CalledByNative
    private void addEnteringStylesheetToTransition(String stylesheet) {
        if (mNavigationTransitionDelegate != null) {
            mNavigationTransitionDelegate.addEnteringStylesheetToTransition(stylesheet);
        }
    }

    @CalledByNative
    private void didStartNavigationTransitionForFrame(long frameId) {
        if (mNavigationTransitionDelegate != null) {
            mNavigationTransitionDelegate.didStartNavigationTransitionForFrame(frameId);
        }
    }

    @CalledByNative
    private void addNavigationTransitionElements(String name, int x, int y, int width, int height) {
        if (mNavigationTransitionDelegate != null) {
            mNavigationTransitionDelegate.addNavigationTransitionElements(
                    name, x, y, width, height);
        }
    }

    @CalledByNative
    private void onTransitionElementsFetched(String cssSelector) {
        if (mNavigationTransitionDelegate != null) {
            mNavigationTransitionDelegate.onTransitionElementsFetched(cssSelector);
        }
    }

    @Override
    public void evaluateJavaScript(String script, JavaScriptCallback callback) {
        nativeEvaluateJavaScript(mNativeWebContentsAndroid, script, callback);
    }

    @Override
    public void addMessageToDevToolsConsole(int level, String message) {
        nativeAddMessageToDevToolsConsole(mNativeWebContentsAndroid, level, message);
    }

    @Override
    public boolean hasAccessedInitialDocument() {
        return nativeHasAccessedInitialDocument(mNativeWebContentsAndroid);
    }

    @CalledByNative
    private static void onEvaluateJavaScriptResult(
            String jsonResult, JavaScriptCallback callback) {
        callback.handleJavaScriptResult(jsonResult);
    }

    @Override
    public int getThemeColor(int defaultColor) {
        int color = nativeGetThemeColor(mNativeWebContentsAndroid);
        if (color == Color.TRANSPARENT) return defaultColor;

        return (color | 0xFF000000);
    }

    @Override
    public void requestAccessibilitySnapshot(AccessibilitySnapshotCallback callback,
            float offsetY, float scrollX) {
        nativeRequestAccessibilitySnapshot(mNativeWebContentsAndroid, callback,
                offsetY, scrollX);
    }

    // root node can be null if parsing fails.
    @CalledByNative
    private static void onAccessibilitySnapshot(AccessibilitySnapshotNode root,
            AccessibilitySnapshotCallback callback) {
        callback.onAccessibilitySnapshot(root);
    }

    @CalledByNative
    private static void addAccessibilityNodeAsChild(AccessibilitySnapshotNode parent,
            AccessibilitySnapshotNode child) {
        parent.addChild(child);
    }

    @CalledByNative
    private static AccessibilitySnapshotNode createAccessibilitySnapshotNode(int x,
            int y, int scrollX, int scrollY, int width, int height, String text,
            int color, int bgcolor, float size, int textStyle, String className) {

        AccessibilitySnapshotNode node = new AccessibilitySnapshotNode(x, y, scrollX,
                scrollY, width, height, text, className);
        // if size is smaller than 0, then style information does not exist.
        if (size >= 0.0) {
            boolean bold = (textStyle & AXTextStyle.text_style_bold) > 0;
            boolean italic = (textStyle & AXTextStyle.text_style_italic) > 0;
            boolean underline = (textStyle & AXTextStyle.text_style_underline) > 0;
            boolean lineThrough = (textStyle & AXTextStyle.text_style_line_through) > 0;
            node.setStyle(color, bgcolor, size, bold, italic, underline, lineThrough);
        }
        return node;
    }

    @Override
    public void addObserver(WebContentsObserver observer) {
        assert mNativeWebContentsAndroid != 0;
        if (mObserverProxy == null) mObserverProxy = new WebContentsObserverProxy(this);
        mObserverProxy.addObserver(observer);
    }

    @Override
    public void removeObserver(WebContentsObserver observer) {
        if (mObserverProxy == null) return;
        mObserverProxy.removeObserver(observer);
    }

    // This is static to avoid exposing a public destroy method on the native side of this class.
    private static native void nativeDestroyWebContents(long webContentsAndroidPtr);

    private native String nativeGetTitle(long nativeWebContentsAndroid);
    private native String nativeGetVisibleURL(long nativeWebContentsAndroid);
    private native boolean nativeIsLoading(long nativeWebContentsAndroid);
    private native boolean nativeIsLoadingToDifferentDocument(long nativeWebContentsAndroid);
    private native void nativeStop(long nativeWebContentsAndroid);
    private native void nativeInsertCSS(long nativeWebContentsAndroid, String css);
    private native void nativeOnHide(long nativeWebContentsAndroid);
    private native void nativeOnShow(long nativeWebContentsAndroid);
    private native void nativeReleaseMediaPlayers(long nativeWebContentsAndroid);
    private native int nativeGetBackgroundColor(long nativeWebContentsAndroid);
    private native void nativeAddStyleSheetByURL(long nativeWebContentsAndroid,
            String url);
    private native void nativeShowInterstitialPage(long nativeWebContentsAndroid,
            String url, long nativeInterstitialPageDelegateAndroid);
    private native boolean nativeIsShowingInterstitialPage(long nativeWebContentsAndroid);
    private native boolean nativeIsRenderWidgetHostViewReady(long nativeWebContentsAndroid);
    private native void nativeExitFullscreen(long nativeWebContentsAndroid);
    private native void nativeUpdateTopControlsState(long nativeWebContentsAndroid,
            boolean enableHiding, boolean enableShowing, boolean animate);
    private native void nativeShowImeIfNeeded(long nativeWebContentsAndroid);
    private native void nativeScrollFocusedEditableNodeIntoView(long nativeWebContentsAndroid);
    private native void nativeSelectWordAroundCaret(long nativeWebContentsAndroid);
    private native String nativeGetURL(long nativeWebContentsAndroid);
    private native String nativeGetLastCommittedURL(long nativeWebContentsAndroid);
    private native boolean nativeIsIncognito(long nativeWebContentsAndroid);
    private native void nativeResumeResponseDeferredAtStart(long nativeWebContentsAndroid);
    private native void nativeResumeLoadingCreatedWebContents(long nativeWebContentsAndroid);
    private native void nativeSetHasPendingNavigationTransitionForTesting(
            long nativeWebContentsAndroid);
    private native void nativeSetupTransitionView(long nativeWebContentsAndroid,
            String markup);
    private native void nativeBeginExitTransition(long nativeWebContentsAndroid,
            String cssSelector, boolean exitToNativeApp);
    private native void nativeRevertExitTransition(long nativeWebContentsAndroid);
    private native void nativeHideTransitionElements(long nativeWebContentsAndroid,
            String cssSelector);
    private native void nativeShowTransitionElements(long nativeWebContentsAndroid,
            String cssSelector);
    private native void nativeClearNavigationTransitionData(long nativeWebContentsAndroid);
    private native void nativeFetchTransitionElements(long nativeWebContentsAndroid, String url);
    private native void nativeEvaluateJavaScript(long nativeWebContentsAndroid,
            String script, JavaScriptCallback callback);
    private native void nativeAddMessageToDevToolsConsole(
            long nativeWebContentsAndroid, int level, String message);
    private native boolean nativeHasAccessedInitialDocument(
            long nativeWebContentsAndroid);
    private native int nativeGetThemeColor(long nativeWebContentsAndroid);
    private native void nativeRequestAccessibilitySnapshot(long nativeWebContentsAndroid,
            AccessibilitySnapshotCallback callback, float offsetY, float scrollX);
}
