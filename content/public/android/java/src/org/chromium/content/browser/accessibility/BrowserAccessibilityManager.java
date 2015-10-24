// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.accessibility;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.Context;
import android.graphics.Rect;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.text.SpannableString;
import android.text.style.URLSpan;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.accessibility.AccessibilityNodeProvider;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.content.browser.ContentViewCore;
import org.chromium.content.browser.RenderCoordinates;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/**
 * Native accessibility for a {@link ContentViewCore}.
 *
 * This class is safe to load on ICS and can be used to run tests, but
 * only the subclass, JellyBeanBrowserAccessibilityManager, actually
 * has a AccessibilityNodeProvider implementation needed for native
 * accessibility.
 */
@JNINamespace("content")
@TargetApi(Build.VERSION_CODES.JELLY_BEAN)
public class BrowserAccessibilityManager {
    private static final String TAG = "BrowserAccessibilityManager";

    // Constants from AccessibilityNodeInfo defined in the L SDK.
    private static final int ACTION_SET_TEXT = 0x200000;
    private static final String ACTION_ARGUMENT_SET_TEXT_CHARSEQUENCE =
            "ACTION_ARGUMENT_SET_TEXT_CHARSEQUENCE";

    private ContentViewCore mContentViewCore;
    private final AccessibilityManager mAccessibilityManager;
    private final RenderCoordinates mRenderCoordinates;
    private long mNativeObj;
    private Rect mAccessibilityFocusRect;
    private boolean mIsHovering;
    private int mLastHoverId = View.NO_ID;
    private int mCurrentRootId;
    private final int[] mTempLocation = new int[2];
    private final ViewGroup mView;
    private boolean mUserHasTouchExplored;
    private boolean mPendingScrollToMakeNodeVisible;
    private boolean mNotifyFrameInfoInitializedCalled;
    private int mSelectionGranularity;
    private int mSelectionStartIndex;
    private int mSelectionEndIndex;
    protected int mAccessibilityFocusId;
    protected boolean mVisible = true;

    /**
     * Create a BrowserAccessibilityManager object, which is owned by the C++
     * BrowserAccessibilityManagerAndroid instance, and connects to the content view.
     * @param nativeBrowserAccessibilityManagerAndroid A pointer to the counterpart native
     *     C++ object that owns this object.
     * @param contentViewCore The content view that this object provides accessibility for.
     */
    @CalledByNative
    private static BrowserAccessibilityManager create(long nativeBrowserAccessibilityManagerAndroid,
            ContentViewCore contentViewCore) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            return new LollipopBrowserAccessibilityManager(
                    nativeBrowserAccessibilityManagerAndroid, contentViewCore);
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            return new JellyBeanBrowserAccessibilityManager(
                    nativeBrowserAccessibilityManagerAndroid, contentViewCore);
        } else {
            return new BrowserAccessibilityManager(
                    nativeBrowserAccessibilityManagerAndroid, contentViewCore);
        }
    }

    protected BrowserAccessibilityManager(long nativeBrowserAccessibilityManagerAndroid,
            ContentViewCore contentViewCore) {
        mNativeObj = nativeBrowserAccessibilityManagerAndroid;
        mContentViewCore = contentViewCore;
        mAccessibilityFocusId = View.NO_ID;
        mIsHovering = false;
        mCurrentRootId = View.NO_ID;
        mView = mContentViewCore.getContainerView();
        mRenderCoordinates = mContentViewCore.getRenderCoordinates();
        mAccessibilityManager =
            (AccessibilityManager) mContentViewCore.getContext()
            .getSystemService(Context.ACCESSIBILITY_SERVICE);

        // This must occur last as it may cause a call to notifyFrameInfoInitialized.
        mContentViewCore.setBrowserAccessibilityManager(this);
    }

    @CalledByNative
    private void onNativeObjectDestroyed() {
        if (mContentViewCore.getBrowserAccessibilityManager() == this) {
            mContentViewCore.setBrowserAccessibilityManager(null);
        }
        mNativeObj = 0;
        mContentViewCore = null;
    }

    /**
     * @return An AccessibilityNodeProvider on JellyBean, and null on previous versions.
     */
    public AccessibilityNodeProvider getAccessibilityNodeProvider() {
        return null;
    }

    /**
     * Set whether the web content made accessible by this class is currently visible.
     * Set it to false if the web view is still on the screen but it's obscured by a
     * dialog or overlay. This will make every virtual view in the web hierarchy report
     * that it's not visible, and not accessibility focusable.
     *
     * @param visible Whether the web content is currently visible and not obscured.
     */
    public void setVisible(boolean visible) {
        if (visible == mVisible) return;

        mVisible = visible;
        mView.sendAccessibilityEvent(AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED);
    }

    /**
     * @see AccessibilityNodeProvider#createAccessibilityNodeInfo(int)
     */
    protected AccessibilityNodeInfo createAccessibilityNodeInfo(int virtualViewId) {
        if (!mAccessibilityManager.isEnabled() || mNativeObj == 0) {
            return null;
        }

        int rootId = nativeGetRootId(mNativeObj);

        if (virtualViewId == View.NO_ID) {
            return createNodeForHost(rootId);
        }

        if (!isFrameInfoInitialized()) {
            return null;
        }

        final AccessibilityNodeInfo info = AccessibilityNodeInfo.obtain(mView);
        info.setPackageName(mContentViewCore.getContext().getPackageName());
        info.setSource(mView, virtualViewId);

        if (virtualViewId == rootId) {
            info.setParent(mView);
        }

        if (nativePopulateAccessibilityNodeInfo(mNativeObj, info, virtualViewId)) {
            return info;
        } else {
            info.recycle();
            return null;
        }
    }

    /**
     * @see AccessibilityNodeProvider#findAccessibilityNodeInfosByText(String, int)
     */
    protected List<AccessibilityNodeInfo> findAccessibilityNodeInfosByText(String text,
            int virtualViewId) {
        return new ArrayList<AccessibilityNodeInfo>();
    }

    protected static boolean isValidMovementGranularity(int granularity) {
        switch (granularity) {
            case AccessibilityNodeInfo.MOVEMENT_GRANULARITY_CHARACTER:
            case AccessibilityNodeInfo.MOVEMENT_GRANULARITY_WORD:
            case AccessibilityNodeInfo.MOVEMENT_GRANULARITY_LINE:
                return true;
        }
        return false;
    }

    /**
     * @see AccessibilityNodeProvider#performAction(int, int, Bundle)
     */
    protected boolean performAction(int virtualViewId, int action, Bundle arguments) {
        // We don't support any actions on the host view or nodes
        // that are not (any longer) in the tree.
        if (!mAccessibilityManager.isEnabled() || mNativeObj == 0
                || !nativeIsNodeValid(mNativeObj, virtualViewId)) {
            return false;
        }

        switch (action) {
            case AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS:
                if (!moveAccessibilityFocusToId(virtualViewId)) return true;

                if (!mIsHovering) {
                    nativeScrollToMakeNodeVisible(
                            mNativeObj, mAccessibilityFocusId);
                } else {
                    mPendingScrollToMakeNodeVisible = true;
                }
                return true;
            case AccessibilityNodeInfo.ACTION_CLEAR_ACCESSIBILITY_FOCUS:
                // ALWAYS respond with TYPE_VIEW_ACCESSIBILITY_FOCUS_CLEARED whether we thought
                // it had focus or not, so that the Android framework cache is correct.
                sendAccessibilityEvent(virtualViewId,
                        AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUS_CLEARED);
                if (mAccessibilityFocusId == virtualViewId) {
                    mAccessibilityFocusId = View.NO_ID;
                    mAccessibilityFocusRect = null;
                }
                return true;
            case AccessibilityNodeInfo.ACTION_CLICK:
                nativeClick(mNativeObj, virtualViewId);
                sendAccessibilityEvent(virtualViewId,
                        AccessibilityEvent.TYPE_VIEW_CLICKED);
                return true;
            case AccessibilityNodeInfo.ACTION_FOCUS:
                nativeFocus(mNativeObj, virtualViewId);
                return true;
            case AccessibilityNodeInfo.ACTION_CLEAR_FOCUS:
                nativeBlur(mNativeObj);
                return true;
            case AccessibilityNodeInfo.ACTION_NEXT_HTML_ELEMENT: {
                if (arguments == null) return false;
                String elementType = arguments.getString(
                        AccessibilityNodeInfo.ACTION_ARGUMENT_HTML_ELEMENT_STRING);
                if (elementType == null) return false;
                elementType = elementType.toUpperCase(Locale.US);
                return jumpToElementType(elementType, true);
            }
            case AccessibilityNodeInfo.ACTION_PREVIOUS_HTML_ELEMENT: {
                if (arguments == null) return false;
                String elementType = arguments.getString(
                        AccessibilityNodeInfo.ACTION_ARGUMENT_HTML_ELEMENT_STRING);
                if (elementType == null) return false;
                elementType = elementType.toUpperCase(Locale.US);
                return jumpToElementType(elementType, false);
            }
            case ACTION_SET_TEXT: {
                if (!nativeIsEditableText(mNativeObj, virtualViewId)) return false;
                if (arguments == null) return false;
                String newText = arguments.getString(
                        ACTION_ARGUMENT_SET_TEXT_CHARSEQUENCE);
                if (newText == null) return false;
                nativeSetTextFieldValue(mNativeObj, virtualViewId, newText);
                // Match Android framework and set the cursor to the end of the text field.
                nativeSetSelection(mNativeObj, virtualViewId, newText.length(), newText.length());
                return true;
            }
            case AccessibilityNodeInfo.ACTION_SET_SELECTION: {
                if (!nativeIsEditableText(mNativeObj, virtualViewId)) return false;
                int selectionStart = 0;
                int selectionEnd = 0;
                if (arguments != null) {
                    selectionStart = arguments.getInt(
                            AccessibilityNodeInfo.ACTION_ARGUMENT_SELECTION_START_INT);
                    selectionEnd = arguments.getInt(
                            AccessibilityNodeInfo.ACTION_ARGUMENT_SELECTION_START_INT);
                }
                nativeSetSelection(mNativeObj, virtualViewId, selectionStart, selectionEnd);
                return true;
            }
            case AccessibilityNodeInfo.ACTION_NEXT_AT_MOVEMENT_GRANULARITY: {
                if (arguments == null) return false;
                int granularity = arguments.getInt(
                        AccessibilityNodeInfo.ACTION_ARGUMENT_MOVEMENT_GRANULARITY_INT);
                boolean extend = arguments.getBoolean(
                        AccessibilityNodeInfo.ACTION_ARGUMENT_EXTEND_SELECTION_BOOLEAN);
                if (!isValidMovementGranularity(granularity)) {
                    return false;
                }
                return nextAtGranularity(granularity, extend);
            }
            case AccessibilityNodeInfo.ACTION_PREVIOUS_AT_MOVEMENT_GRANULARITY: {
                if (arguments == null) return false;
                int granularity = arguments.getInt(
                        AccessibilityNodeInfo.ACTION_ARGUMENT_MOVEMENT_GRANULARITY_INT);
                boolean extend = arguments.getBoolean(
                        AccessibilityNodeInfo.ACTION_ARGUMENT_EXTEND_SELECTION_BOOLEAN);
                if (!isValidMovementGranularity(granularity)) {
                    return false;
                }
                return previousAtGranularity(granularity, extend);
            }
            case AccessibilityNodeInfo.ACTION_SCROLL_FORWARD:
                return scrollForward(virtualViewId);
            case AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD:
                return scrollBackward(virtualViewId);
            default:
                break;
        }
        return false;
    }

    /**
     * @see View#onHoverEvent(MotionEvent)
     */
    public boolean onHoverEvent(MotionEvent event) {
        if (!mAccessibilityManager.isEnabled() || mNativeObj == 0) {
            return false;
        }

        if (event.getAction() == MotionEvent.ACTION_HOVER_EXIT) {
            mIsHovering = false;
            if (mPendingScrollToMakeNodeVisible) {
                nativeScrollToMakeNodeVisible(
                        mNativeObj, mAccessibilityFocusId);
            }
            mPendingScrollToMakeNodeVisible = false;
            return true;
        }

        mIsHovering = true;
        mUserHasTouchExplored = true;
        float x = event.getX();
        float y = event.getY();

        // Convert to CSS coordinates.
        int cssX = (int) (mRenderCoordinates.fromPixToLocalCss(x));
        int cssY = (int) (mRenderCoordinates.fromPixToLocalCss(y));

        // This sends an IPC to the render process to do the hit testing.
        // The response is handled by handleHover.
        nativeHitTest(mNativeObj, cssX, cssY);
        return true;
    }

    /**
     * Called by ContentViewCore to notify us when the frame info is initialized,
     * the first time, since until that point, we can't use mRenderCoordinates to transform
     * web coordinates to screen coordinates.
     */
    public void notifyFrameInfoInitialized() {
        if (mNotifyFrameInfoInitializedCalled) return;

        mNotifyFrameInfoInitializedCalled = true;

        // Invalidate the container view, since the chrome accessibility tree is now
        // ready and listed as the child of the container view.
        mView.sendAccessibilityEvent(AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED);

        // (Re-) focus focused element, since we weren't able to create an
        // AccessibilityNodeInfo for this element before.
        if (mAccessibilityFocusId != View.NO_ID) {
            moveAccessibilityFocusToIdAndRefocusIfNeeded(mAccessibilityFocusId);
        }
    }

    private boolean jumpToElementType(String elementType, boolean forwards) {
        int id = nativeFindElementType(mNativeObj, mAccessibilityFocusId, elementType, forwards);
        if (id == 0) return false;

        moveAccessibilityFocusToId(id);
        return true;
    }

    private void setGranularityAndUpdateSelection(int granularity) {
        if (mSelectionGranularity == 0) {
            mSelectionStartIndex = -1;
            mSelectionEndIndex = -1;
        }
        mSelectionGranularity = granularity;
        if (nativeIsEditableText(mNativeObj, mAccessibilityFocusId)) {
            mSelectionStartIndex = nativeGetEditableTextSelectionStart(
                    mNativeObj, mAccessibilityFocusId);
            mSelectionEndIndex = nativeGetEditableTextSelectionEnd(
                    mNativeObj, mAccessibilityFocusId);
        }
    }

    private boolean nextAtGranularity(int granularity, boolean extendSelection) {
        setGranularityAndUpdateSelection(granularity);
        // This calls finishGranularityMove when it's done.
        return nativeNextAtGranularity(mNativeObj, mSelectionGranularity, extendSelection,
                mAccessibilityFocusId, mSelectionEndIndex);
    }

    private boolean previousAtGranularity(int granularity, boolean extendSelection) {
        setGranularityAndUpdateSelection(granularity);
        // This calls finishGranularityMove when it's done.
        return nativePreviousAtGranularity(mNativeObj, mSelectionGranularity, extendSelection,
                mAccessibilityFocusId, mSelectionEndIndex);
    }

    @CalledByNative
    private void finishGranularityMove(String text, boolean extendSelection,
            int itemStartIndex, int itemEndIndex, boolean forwards) {
        // Prepare to send both a selection and a traversal event in sequence.
        AccessibilityEvent selectionEvent = buildAccessibilityEvent(mAccessibilityFocusId,
                AccessibilityEvent.TYPE_VIEW_TEXT_SELECTION_CHANGED);
        if (selectionEvent == null) return;
        AccessibilityEvent traverseEvent = buildAccessibilityEvent(mAccessibilityFocusId,
                AccessibilityEvent.TYPE_VIEW_TEXT_TRAVERSED_AT_MOVEMENT_GRANULARITY);
        if (traverseEvent == null) {
            selectionEvent.recycle();
            return;
        }

        // Update the cursor or selection based on the traversal. If it's an editable
        // text node, set the real editing cursor too.
        if (forwards) {
            mSelectionEndIndex = itemEndIndex;
        } else {
            mSelectionEndIndex = itemStartIndex;
        }
        if (!extendSelection) {
            mSelectionStartIndex = mSelectionEndIndex;
        }
        if (nativeIsEditableText(mNativeObj, mAccessibilityFocusId)) {
            nativeSetSelection(mNativeObj, mAccessibilityFocusId,
                    mSelectionStartIndex, mSelectionEndIndex);
        }

        // The selection event's "from" and "to" indices are just a cursor at the focus
        // end of the movement, or a selection if extendSelection is true.
        selectionEvent.setFromIndex(mSelectionStartIndex);
        selectionEvent.setToIndex(mSelectionStartIndex);
        selectionEvent.setItemCount(text.length());

        // The traverse event's "from" and "to" indices surround the item (e.g. the word,
        // etc.) with no whitespace.
        traverseEvent.setFromIndex(itemStartIndex);
        traverseEvent.setToIndex(itemEndIndex);
        traverseEvent.setItemCount(text.length());
        traverseEvent.setMovementGranularity(mSelectionGranularity);
        traverseEvent.setContentDescription(text);

        // The traverse event needs to set its associated action that triggered it.
        if (forwards) {
            traverseEvent.setAction(AccessibilityNodeInfo.ACTION_NEXT_AT_MOVEMENT_GRANULARITY);
        } else {
            traverseEvent.setAction(
                    AccessibilityNodeInfo.ACTION_PREVIOUS_AT_MOVEMENT_GRANULARITY);
        }

        mView.requestSendAccessibilityEvent(mView, selectionEvent);
        mView.requestSendAccessibilityEvent(mView, traverseEvent);
    }

    private boolean scrollForward(int virtualViewId) {
        if (nativeIsSlider(mNativeObj, virtualViewId)) {
            return nativeAdjustSlider(mNativeObj, virtualViewId, true);
        } else {
            return nativeScroll(mNativeObj, virtualViewId, ScrollDirection.FORWARD);
        }
    }

    private boolean scrollBackward(int virtualViewId) {
        if (nativeIsSlider(mNativeObj, virtualViewId)) {
            return nativeAdjustSlider(mNativeObj, virtualViewId, false);
        } else {
            return nativeScroll(mNativeObj, virtualViewId, ScrollDirection.BACKWARD);
        }
    }

    private boolean moveAccessibilityFocusToId(int newAccessibilityFocusId) {
        if (newAccessibilityFocusId == mAccessibilityFocusId) return false;

        mAccessibilityFocusId = newAccessibilityFocusId;
        mAccessibilityFocusRect = null;
        mSelectionGranularity = 0;
        mSelectionStartIndex = 0;
        mSelectionEndIndex = 0;

        // Calling nativeSetAccessibilityFocus will asynchronously load inline text boxes for
        // this node and its subtree. If accessibility focus is on anything other than
        // the root, do it - otherwise set it to -1 so we don't load inline text boxes
        // for the whole subtree of the root.
        if (mAccessibilityFocusId == mCurrentRootId) {
            nativeSetAccessibilityFocus(mNativeObj, -1);
        } else {
            nativeSetAccessibilityFocus(mNativeObj, mAccessibilityFocusId);
        }

        sendAccessibilityEvent(mAccessibilityFocusId,
                AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUSED);
        return true;
    }

    private void moveAccessibilityFocusToIdAndRefocusIfNeeded(int newAccessibilityFocusId) {
        // Work around a bug in the Android framework where it doesn't fully update the object
        // with accessibility focus even if you send it a WINDOW_CONTENT_CHANGED. To work around
        // this, clear focus and then set focus again.
        if (newAccessibilityFocusId == mAccessibilityFocusId) {
            sendAccessibilityEvent(newAccessibilityFocusId,
                    AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUS_CLEARED);
            mAccessibilityFocusId = View.NO_ID;
        }

        moveAccessibilityFocusToId(newAccessibilityFocusId);
    }

    private void sendAccessibilityEvent(int virtualViewId, int eventType) {
        // The container view is indicated by a virtualViewId of NO_ID; post these events directly
        // since there's no web-specific information to attach.
        if (virtualViewId == View.NO_ID) {
            mView.sendAccessibilityEvent(eventType);
            return;
        }

        AccessibilityEvent event = buildAccessibilityEvent(virtualViewId, eventType);
        if (event != null) {
            mView.requestSendAccessibilityEvent(mView, event);
        }
    }

    private AccessibilityEvent buildAccessibilityEvent(int virtualViewId, int eventType) {
        // If we don't have any frame info, then the virtual hierarchy
        // doesn't exist in the view of the Android framework, so should
        // never send any events.
        if (!mAccessibilityManager.isEnabled() || mNativeObj == 0
                || !isFrameInfoInitialized()) {
            return null;
        }

        // This is currently needed if we want Android to visually highlight
        // the item that has accessibility focus. In practice, this doesn't seem to slow
        // things down, because it's only called when the accessibility focus moves.
        // TODO(dmazzoni): remove this if/when Android framework fixes bug.
        mView.postInvalidate();

        final AccessibilityEvent event = AccessibilityEvent.obtain(eventType);
        event.setPackageName(mContentViewCore.getContext().getPackageName());
        event.setSource(mView, virtualViewId);
        if (!nativePopulateAccessibilityEvent(mNativeObj, event, virtualViewId, eventType)) {
            event.recycle();
            return null;
        }
        return event;
    }

    private Bundle getOrCreateBundleForAccessibilityEvent(AccessibilityEvent event) {
        Bundle bundle = (Bundle) event.getParcelableData();
        if (bundle == null) {
            bundle = new Bundle();
            event.setParcelableData(bundle);
        }
        return bundle;
    }

    private AccessibilityNodeInfo createNodeForHost(int rootId) {
        // Since we don't want the parent to be focusable, but we can't remove
        // actions from a node, copy over the necessary fields.
        final AccessibilityNodeInfo result = AccessibilityNodeInfo.obtain(mView);
        final AccessibilityNodeInfo source = AccessibilityNodeInfo.obtain(mView);
        mView.onInitializeAccessibilityNodeInfo(source);

        // Copy over parent and screen bounds.
        Rect rect = new Rect();
        source.getBoundsInParent(rect);
        result.setBoundsInParent(rect);
        source.getBoundsInScreen(rect);
        result.setBoundsInScreen(rect);

        // Set up the parent view, if applicable.
        final ViewParent parent = mView.getParentForAccessibility();
        if (parent instanceof View) {
            result.setParent((View) parent);
        }

        // Populate the minimum required fields.
        result.setVisibleToUser(source.isVisibleToUser() && mVisible);
        result.setEnabled(source.isEnabled());
        result.setPackageName(source.getPackageName());
        result.setClassName(source.getClassName());

        // Add the Chrome root node.
        if (isFrameInfoInitialized()) {
            result.addChild(mView, rootId);
        }

        return result;
    }

    /**
     * Returns whether or not the frame info is initialized, meaning we can safely
     * convert web coordinates to screen coordinates. When this is first initialized,
     * notifyFrameInfoInitialized is called - but we shouldn't check whether or not
     * that method was called as a way to determine if frame info is valid because
     * notifyFrameInfoInitialized might not be called at all if mRenderCoordinates
     * gets initialized first.
     */
    private boolean isFrameInfoInitialized() {
        return mRenderCoordinates.getContentWidthCss() != 0.0
                || mRenderCoordinates.getContentHeightCss() != 0.0;
    }

    @CalledByNative
    private void handlePageLoaded(int id) {
        if (mUserHasTouchExplored) return;

        if (mContentViewCore.shouldSetAccessibilityFocusOnPageLoad()) {
            moveAccessibilityFocusToIdAndRefocusIfNeeded(id);
        }
    }

    @CalledByNative
    private void handleFocusChanged(int id) {
        sendAccessibilityEvent(id, AccessibilityEvent.TYPE_VIEW_FOCUSED);
        moveAccessibilityFocusToId(id);
    }

    @CalledByNative
    private void handleCheckStateChanged(int id) {
        sendAccessibilityEvent(id, AccessibilityEvent.TYPE_VIEW_CLICKED);
    }

    @CalledByNative
    private void handleTextSelectionChanged(int id) {
        sendAccessibilityEvent(id, AccessibilityEvent.TYPE_VIEW_TEXT_SELECTION_CHANGED);
    }

    @CalledByNative
    private void handleEditableTextChanged(int id) {
        sendAccessibilityEvent(id, AccessibilityEvent.TYPE_VIEW_TEXT_CHANGED);
    }

    @CalledByNative
    private void handleSliderChanged(int id) {
        sendAccessibilityEvent(id, AccessibilityEvent.TYPE_VIEW_SCROLLED);
    }

    @CalledByNative
    private void handleContentChanged(int id) {
        int rootId = nativeGetRootId(mNativeObj);
        if (rootId != mCurrentRootId) {
            mCurrentRootId = rootId;
            mView.sendAccessibilityEvent(AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED);
        } else {
            sendAccessibilityEvent(id, AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED);
        }
    }

    @CalledByNative
    private void handleNavigate() {
        mAccessibilityFocusId = View.NO_ID;
        mAccessibilityFocusRect = null;
        mUserHasTouchExplored = false;
        // Invalidate the host, since its child is now gone.
        mView.sendAccessibilityEvent(AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED);
    }

    @CalledByNative
    private void handleScrollPositionChanged(int id) {
        sendAccessibilityEvent(id, AccessibilityEvent.TYPE_VIEW_SCROLLED);
    }

    @CalledByNative
    private void handleScrolledToAnchor(int id) {
        moveAccessibilityFocusToId(id);
    }

    @CalledByNative
    private void handleHover(int id) {
        if (mLastHoverId == id) return;

        // Always send the ENTER and then the EXIT event, to match a standard Android View.
        sendAccessibilityEvent(id, AccessibilityEvent.TYPE_VIEW_HOVER_ENTER);
        sendAccessibilityEvent(mLastHoverId, AccessibilityEvent.TYPE_VIEW_HOVER_EXIT);
        mLastHoverId = id;
    }

    @CalledByNative
    private void announceLiveRegionText(String text) {
        mView.announceForAccessibility(text);
    }

    @CalledByNative
    private void setAccessibilityNodeInfoParent(AccessibilityNodeInfo node, int parentId) {
        node.setParent(mView, parentId);
    }

    @CalledByNative
    private void addAccessibilityNodeInfoChild(AccessibilityNodeInfo node, int childId) {
        node.addChild(mView, childId);
    }

    @CalledByNative
    private void setAccessibilityNodeInfoBooleanAttributes(AccessibilityNodeInfo node,
            int virtualViewId,
            boolean checkable, boolean checked, boolean clickable,
            boolean enabled, boolean focusable, boolean focused, boolean password,
            boolean scrollable, boolean selected, boolean visibleToUser) {
        node.setCheckable(checkable);
        node.setChecked(checked);
        node.setClickable(clickable);
        node.setEnabled(enabled);
        node.setFocusable(focusable);
        node.setFocused(focused);
        node.setPassword(password);
        node.setScrollable(scrollable);
        node.setSelected(selected);
        node.setVisibleToUser(visibleToUser && mVisible);

        node.setMovementGranularities(
                AccessibilityNodeInfo.MOVEMENT_GRANULARITY_CHARACTER
                | AccessibilityNodeInfo.MOVEMENT_GRANULARITY_WORD
                | AccessibilityNodeInfo.MOVEMENT_GRANULARITY_LINE);

        if (mAccessibilityFocusId == virtualViewId) {
            node.setAccessibilityFocused(true);
        } else if (mVisible) {
            node.setAccessibilityFocused(false);
        }
    }

    // LollipopBrowserAccessibilityManager overrides this with the non-deprecated APIs.
    @SuppressWarnings("deprecation")
    @CalledByNative
    protected void addAccessibilityNodeInfoActions(AccessibilityNodeInfo node,
            int virtualViewId, boolean canScrollForward, boolean canScrollBackward,
            boolean canScrollUp, boolean canScrollDown, boolean canScrollLeft,
            boolean canScrollRight, boolean clickable, boolean editableText, boolean enabled,
            boolean focusable, boolean focused) {
        node.addAction(AccessibilityNodeInfo.ACTION_NEXT_HTML_ELEMENT);
        node.addAction(AccessibilityNodeInfo.ACTION_PREVIOUS_HTML_ELEMENT);
        node.addAction(AccessibilityNodeInfo.ACTION_NEXT_AT_MOVEMENT_GRANULARITY);
        node.addAction(AccessibilityNodeInfo.ACTION_PREVIOUS_AT_MOVEMENT_GRANULARITY);

        if (editableText && enabled) {
            node.addAction(ACTION_SET_TEXT);
            node.addAction(AccessibilityNodeInfo.ACTION_SET_SELECTION);
        }

        if (canScrollForward) {
            node.addAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD);
        }

        if (canScrollBackward) {
            node.addAction(AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD);
        }

        // TODO(dmazzoni): add custom actions for scrolling up, down,
        // left, and right.

        if (focusable) {
            if (focused) {
                node.addAction(AccessibilityNodeInfo.ACTION_CLEAR_FOCUS);
            } else {
                node.addAction(AccessibilityNodeInfo.ACTION_FOCUS);
            }
        }

        if (mAccessibilityFocusId == virtualViewId) {
            node.addAction(AccessibilityNodeInfo.ACTION_CLEAR_ACCESSIBILITY_FOCUS);
        } else if (mVisible) {
            node.addAction(AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS);
        }

        if (clickable) {
            node.addAction(AccessibilityNodeInfo.ACTION_CLICK);
        }
    }

    @CalledByNative
    private void setAccessibilityNodeInfoClassName(AccessibilityNodeInfo node,
            String className) {
        node.setClassName(className);
    }

    @SuppressLint("NewApi")
    @CalledByNative
    private void setAccessibilityNodeInfoContentDescription(
            AccessibilityNodeInfo node, String contentDescription, boolean annotateAsLink) {
        if (annotateAsLink) {
            SpannableString spannable = new SpannableString(contentDescription);
            spannable.setSpan(new URLSpan(""), 0, spannable.length(), 0);
            node.setContentDescription(spannable);
        } else {
            node.setContentDescription(contentDescription);
        }
    }

    @CalledByNative
    private void setAccessibilityNodeInfoLocation(AccessibilityNodeInfo node,
            final int virtualViewId,
            int absoluteLeft, int absoluteTop, int parentRelativeLeft, int parentRelativeTop,
            int width, int height, boolean isRootNode) {
        // First set the bounds in parent.
        Rect boundsInParent = new Rect(parentRelativeLeft, parentRelativeTop,
                parentRelativeLeft + width, parentRelativeTop + height);
        if (isRootNode) {
            // Offset of the web content relative to the View.
            boundsInParent.offset(0, (int) mRenderCoordinates.getContentOffsetYPix());
        }
        node.setBoundsInParent(boundsInParent);

        // Now set the absolute rect, which requires several transformations.
        Rect rect = new Rect(absoluteLeft, absoluteTop, absoluteLeft + width, absoluteTop + height);

        // Offset by the scroll position.
        rect.offset(-(int) mRenderCoordinates.getScrollX(),
                    -(int) mRenderCoordinates.getScrollY());

        // Convert CSS (web) pixels to Android View pixels
        rect.left = (int) mRenderCoordinates.fromLocalCssToPix(rect.left);
        rect.top = (int) mRenderCoordinates.fromLocalCssToPix(rect.top);
        rect.bottom = (int) mRenderCoordinates.fromLocalCssToPix(rect.bottom);
        rect.right = (int) mRenderCoordinates.fromLocalCssToPix(rect.right);

        // Offset by the location of the web content within the view.
        rect.offset(0,
                    (int) mRenderCoordinates.getContentOffsetYPix());

        // Finally offset by the location of the view within the screen.
        final int[] viewLocation = new int[2];
        mView.getLocationOnScreen(viewLocation);
        rect.offset(viewLocation[0], viewLocation[1]);

        // Clip the node's bounding rect to the viewport bounds.
        int viewportRectTop = viewLocation[1] + (int) mRenderCoordinates.getContentOffsetYPix();
        int viewportRectBottom = viewportRectTop + mContentViewCore.getViewportHeightPix();
        if (rect.top < viewportRectTop) rect.top = viewportRectTop;
        if (rect.bottom > viewportRectBottom) rect.bottom = viewportRectBottom;

        node.setBoundsInScreen(rect);

        // Work around a bug in the Android framework where if the object with accessibility
        // focus moves, the accessibility focus rect is not updated - both the visual highlight,
        // and the location on the screen that's clicked if you double-tap. To work around this,
        // when we know the object with accessibility focus moved, move focus away and then
        // move focus right back to it, which tricks Android into updating its bounds.
        if (virtualViewId == mAccessibilityFocusId && virtualViewId != mCurrentRootId) {
            if (mAccessibilityFocusRect == null) {
                mAccessibilityFocusRect = rect;
            } else if (!mAccessibilityFocusRect.equals(rect)) {
                mAccessibilityFocusRect = rect;
                moveAccessibilityFocusToIdAndRefocusIfNeeded(virtualViewId);
            }
        }
    }

    @CalledByNative
    protected void setAccessibilityNodeInfoLollipopAttributes(AccessibilityNodeInfo node,
            boolean canOpenPopup,
            boolean contentInvalid,
            boolean dismissable,
            boolean multiLine,
            int inputType,
            int liveRegion) {
        // Requires Lollipop or higher.
    }

    @CalledByNative
    protected void setAccessibilityNodeInfoCollectionInfo(AccessibilityNodeInfo node,
            int rowCount, int columnCount, boolean hierarchical) {
        // Requires Lollipop or higher.
    }

    @CalledByNative
    protected void setAccessibilityNodeInfoCollectionItemInfo(AccessibilityNodeInfo node,
            int rowIndex, int rowSpan, int columnIndex, int columnSpan, boolean heading) {
        // Requires Lollipop or higher.
    }

    @CalledByNative
    protected void setAccessibilityNodeInfoRangeInfo(AccessibilityNodeInfo node,
            int rangeType, float min, float max, float current) {
        // Requires Lollipop or higher.
    }

    @CalledByNative
    protected void setAccessibilityNodeInfoViewIdResourceName(
            AccessibilityNodeInfo node, String viewIdResourceName) {
        // Requires Lollipop or higher.
    }

    @CalledByNative
    private void setAccessibilityEventBooleanAttributes(AccessibilityEvent event,
            boolean checked, boolean enabled, boolean password, boolean scrollable) {
        event.setChecked(checked);
        event.setEnabled(enabled);
        event.setPassword(password);
        event.setScrollable(scrollable);
    }

    @CalledByNative
    private void setAccessibilityEventClassName(AccessibilityEvent event, String className) {
        event.setClassName(className);
    }

    @CalledByNative
    private void setAccessibilityEventListAttributes(AccessibilityEvent event,
            int currentItemIndex, int itemCount) {
        event.setCurrentItemIndex(currentItemIndex);
        event.setItemCount(itemCount);
    }

    @CalledByNative
    private void setAccessibilityEventScrollAttributes(AccessibilityEvent event,
            int scrollX, int scrollY, int maxScrollX, int maxScrollY) {
        event.setScrollX(scrollX);
        event.setScrollY(scrollY);
        event.setMaxScrollX(maxScrollX);
        event.setMaxScrollY(maxScrollY);
    }

    @CalledByNative
    private void setAccessibilityEventTextChangedAttrs(AccessibilityEvent event,
            int fromIndex, int addedCount, int removedCount, String beforeText, String text) {
        event.setFromIndex(fromIndex);
        event.setAddedCount(addedCount);
        event.setRemovedCount(removedCount);
        event.setBeforeText(beforeText);
        event.getText().add(text);
    }

    @CalledByNative
    private void setAccessibilityEventSelectionAttrs(AccessibilityEvent event,
            int fromIndex, int toIndex, int itemCount, String text) {
        event.setFromIndex(fromIndex);
        event.setToIndex(toIndex);
        event.setItemCount(itemCount);
        event.getText().add(text);
    }

    @CalledByNative
    protected void setAccessibilityEventLollipopAttributes(AccessibilityEvent event,
            boolean canOpenPopup,
            boolean contentInvalid,
            boolean dismissable,
            boolean multiLine,
            int inputType,
            int liveRegion) {
        // Backwards compatibility for Lollipop AccessibilityNodeInfo fields.
        Bundle bundle = getOrCreateBundleForAccessibilityEvent(event);
        bundle.putBoolean("AccessibilityNodeInfo.canOpenPopup", canOpenPopup);
        bundle.putBoolean("AccessibilityNodeInfo.contentInvalid", contentInvalid);
        bundle.putBoolean("AccessibilityNodeInfo.dismissable", dismissable);
        bundle.putBoolean("AccessibilityNodeInfo.multiLine", multiLine);
        bundle.putInt("AccessibilityNodeInfo.inputType", inputType);
        bundle.putInt("AccessibilityNodeInfo.liveRegion", liveRegion);
    }

    @CalledByNative
    protected void setAccessibilityEventCollectionInfo(AccessibilityEvent event,
            int rowCount, int columnCount, boolean hierarchical) {
        // Backwards compatibility for Lollipop AccessibilityNodeInfo fields.
        Bundle bundle = getOrCreateBundleForAccessibilityEvent(event);
        bundle.putInt("AccessibilityNodeInfo.CollectionInfo.rowCount", rowCount);
        bundle.putInt("AccessibilityNodeInfo.CollectionInfo.columnCount", columnCount);
        bundle.putBoolean("AccessibilityNodeInfo.CollectionInfo.hierarchical", hierarchical);
    }

    @CalledByNative
    protected void setAccessibilityEventHeadingFlag(AccessibilityEvent event,
            boolean heading) {
        // Backwards compatibility for Lollipop AccessibilityNodeInfo fields.
        Bundle bundle = getOrCreateBundleForAccessibilityEvent(event);
        bundle.putBoolean("AccessibilityNodeInfo.CollectionItemInfo.heading", heading);
    }

    @CalledByNative
    protected void setAccessibilityEventCollectionItemInfo(AccessibilityEvent event,
            int rowIndex, int rowSpan, int columnIndex, int columnSpan) {
        // Backwards compatibility for Lollipop AccessibilityNodeInfo fields.
        Bundle bundle = getOrCreateBundleForAccessibilityEvent(event);
        bundle.putInt("AccessibilityNodeInfo.CollectionItemInfo.rowIndex", rowIndex);
        bundle.putInt("AccessibilityNodeInfo.CollectionItemInfo.rowSpan", rowSpan);
        bundle.putInt("AccessibilityNodeInfo.CollectionItemInfo.columnIndex", columnIndex);
        bundle.putInt("AccessibilityNodeInfo.CollectionItemInfo.columnSpan", columnSpan);
    }

    @CalledByNative
    protected void setAccessibilityEventRangeInfo(AccessibilityEvent event,
            int rangeType, float min, float max, float current) {
        // Backwards compatibility for Lollipop AccessibilityNodeInfo fields.
        Bundle bundle = getOrCreateBundleForAccessibilityEvent(event);
        bundle.putInt("AccessibilityNodeInfo.RangeInfo.type", rangeType);
        bundle.putFloat("AccessibilityNodeInfo.RangeInfo.min", min);
        bundle.putFloat("AccessibilityNodeInfo.RangeInfo.max", max);
        bundle.putFloat("AccessibilityNodeInfo.RangeInfo.current", current);
    }

    @CalledByNative
    boolean shouldExposePasswordText() {
        return (Settings.Secure.getInt(
                        mContentViewCore.getContext().getContentResolver(),
                        Settings.Secure.ACCESSIBILITY_SPEAK_PASSWORD, 0) == 1);
    }

    private native int nativeGetRootId(long nativeBrowserAccessibilityManagerAndroid);
    private native boolean nativeIsNodeValid(long nativeBrowserAccessibilityManagerAndroid, int id);
    private native boolean nativeIsEditableText(
            long nativeBrowserAccessibilityManagerAndroid, int id);
    private native int nativeGetEditableTextSelectionStart(
            long nativeBrowserAccessibilityManagerAndroid, int id);
    private native int nativeGetEditableTextSelectionEnd(
            long nativeBrowserAccessibilityManagerAndroid, int id);
    private native void nativeHitTest(long nativeBrowserAccessibilityManagerAndroid, int x, int y);
    private native boolean nativePopulateAccessibilityNodeInfo(
            long nativeBrowserAccessibilityManagerAndroid, AccessibilityNodeInfo info, int id);
    private native boolean nativePopulateAccessibilityEvent(
            long nativeBrowserAccessibilityManagerAndroid, AccessibilityEvent event, int id,
            int eventType);
    private native void nativeClick(long nativeBrowserAccessibilityManagerAndroid, int id);
    private native void nativeFocus(long nativeBrowserAccessibilityManagerAndroid, int id);
    private native void nativeBlur(long nativeBrowserAccessibilityManagerAndroid);
    private native void nativeScrollToMakeNodeVisible(
            long nativeBrowserAccessibilityManagerAndroid, int id);
    private native int nativeFindElementType(long nativeBrowserAccessibilityManagerAndroid,
            int startId, String elementType, boolean forwards);
    private native void nativeSetTextFieldValue(long nativeBrowserAccessibilityManagerAndroid,
            int id, String newValue);
    private native void nativeSetSelection(long nativeBrowserAccessibilityManagerAndroid,
            int id, int start, int end);
    private native boolean nativeNextAtGranularity(long nativeBrowserAccessibilityManagerAndroid,
            int selectionGranularity, boolean extendSelection, int id, int cursorIndex);
    private native boolean nativePreviousAtGranularity(
            long nativeBrowserAccessibilityManagerAndroid,
            int selectionGranularity, boolean extendSelection, int id, int cursorIndex);
    private native boolean nativeAdjustSlider(
            long nativeBrowserAccessibilityManagerAndroid, int id, boolean increment);
    private native void nativeSetAccessibilityFocus(
            long nativeBrowserAccessibilityManagerAndroid, int id);
    private native boolean nativeIsSlider(
            long nativeBrowserAccessibilityManagerAndroid, int id);
    private native boolean nativeScroll(
            long nativeBrowserAccessibilityManagerAndroid, int id, int direction);
}
