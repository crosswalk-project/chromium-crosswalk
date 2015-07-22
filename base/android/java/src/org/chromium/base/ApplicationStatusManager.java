// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import android.app.Activity;
import android.app.Application;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.Window;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

/**
 * Basic application functionality that should be shared among all browser applications.
 */
public class ApplicationStatusManager {
    /**
     * Interface to be implemented by listeners for window focus events.
     */
    public interface WindowFocusChangedListener {
        /**
         * Called when the window focus changes for {@code activity}.
         * @param activity The {@link Activity} that has a window focus changed event.
         * @param hasFocus Whether or not {@code activity} gained or lost focus.
         */
        public void onWindowFocusChanged(Activity activity, boolean hasFocus);
    }

    private static ObserverList<WindowFocusChangedListener> sWindowFocusListeners =
            new ObserverList<WindowFocusChangedListener>();

    /**
     * Intercepts calls to an existing Window.Callback. Most invocations are passed on directly
     * to the composed Window.Callback but enables intercepting/manipulating others.
     *
     * This is used to relay window focus changes throughout the app and remedy a bug in the
     * appcompat library.
     */
    private static class WindowCallbackProxy implements InvocationHandler {
        private final Window.Callback mCallback;
        private final Activity mActivity;

        public WindowCallbackProxy(Activity activity, Window.Callback callback) {
            mCallback = callback;
            mActivity = activity;
        }

        @Override
        public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
            if (method.getName().equals("onWindowFocusChanged") && args.length == 1
                    && args[0] instanceof Boolean) {
                onWindowFocusChanged((boolean) args[0]);
                return null;
            } else if (method.getName().equals("dispatchKeyEvent") && args.length == 1
                    && args[0] instanceof KeyEvent) {
                return dispatchKeyEvent((KeyEvent) args[0]);
            } else {
                return method.invoke(mCallback, args);
            }
        }

        public void onWindowFocusChanged(boolean hasFocus) {
            mCallback.onWindowFocusChanged(hasFocus);

            for (WindowFocusChangedListener listener : sWindowFocusListeners) {
                listener.onWindowFocusChanged(mActivity, hasFocus);
            }
        }

        public boolean dispatchKeyEvent(KeyEvent event) {
            // TODO(aurimas): remove this once AppCompatDelegateImpl no longer steals
            // KEYCODE_MENU. (see b/20529185)
            if (event.getKeyCode() == KeyEvent.KEYCODE_MENU && mActivity.dispatchKeyEvent(event)) {
                return true;
            }
            return mCallback.dispatchKeyEvent(event);
        }
    }

    public static void init(Application app) {
        ApplicationStatus.initialize(app);

        app.registerActivityLifecycleCallbacks(new Application.ActivityLifecycleCallbacks() {
            @Override
            public void onActivityCreated(final Activity activity, Bundle savedInstanceState) {
                setWindowFocusChangedCallback(activity);
            }

            @Override
            public void onActivityDestroyed(Activity activity) {
                assert Proxy.isProxyClass(activity.getWindow().getCallback().getClass());
            }

            @Override
            public void onActivityPaused(Activity activity) {
                assert Proxy.isProxyClass(activity.getWindow().getCallback().getClass());
            }

            @Override
            public void onActivityResumed(Activity activity) {
                assert Proxy.isProxyClass(activity.getWindow().getCallback().getClass());
            }

            @Override
            public void onActivitySaveInstanceState(Activity activity, Bundle outState) {
                assert Proxy.isProxyClass(activity.getWindow().getCallback().getClass());
            }

            @Override
            public void onActivityStarted(Activity activity) {
                assert Proxy.isProxyClass(activity.getWindow().getCallback().getClass());
            }

            @Override
            public void onActivityStopped(Activity activity) {
                assert Proxy.isProxyClass(activity.getWindow().getCallback().getClass());
            }
        });
    }

    /**
     * Registers a listener to receive window focus updates on activities in this application.
     * @param listener Listener to receive window focus events.
     */
    public static void registerWindowFocusChangedListener(WindowFocusChangedListener listener) {
        sWindowFocusListeners.addObserver(listener);
    }

    /**
     * Unregisters a listener from receiving window focus updates on activities in this application.
     * @param listener Listener that doesn't want to receive window focus events.
     */
    public static void unregisterWindowFocusChangedListener(WindowFocusChangedListener listener) {
        sWindowFocusListeners.removeObserver(listener);
    }

    /**
     * When ApplicationStatus initialized after application started, the onActivityCreated(),
     * onActivityStarted() and onActivityResumed() callbacks will be missed.
     * This function will give the chance to simulate these three callbacks.
     */
    public static void informActivityStarted(final Activity activity) {
        setWindowFocusChangedCallback(activity);
        ApplicationStatus.informActivityStarted(activity);
    }

    private static void setWindowFocusChangedCallback(final Activity activity) {
        Window.Callback callback = activity.getWindow().getCallback();
        activity.getWindow().setCallback(
            (Window.Callback) Proxy.newProxyInstance(Window.Callback.class.getClassLoader(),
                                                     new Class[] {Window.Callback.class},
                                                     new WindowCallbackProxy(activity, callback)));
    }
}
