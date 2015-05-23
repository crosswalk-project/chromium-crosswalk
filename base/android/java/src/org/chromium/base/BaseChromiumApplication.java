// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import android.app.Application;
import android.content.Context;

/**
 * Basic application functionality that should be shared among all browser applications.
 */
public class BaseChromiumApplication extends Application {

    @Override
    public void onCreate() {
        super.onCreate();
        ApplicationStatusManager.init(this);
    }

    /** Initializes the {@link CommandLine}. */
    public void initCommandLine() {}

    /**
     * This must only be called for contexts whose application is a subclass of
     * {@link BaseChromiumApplication}.
     */
    @VisibleForTesting
    public static void initCommandLine(Context context) {
        ((BaseChromiumApplication) context.getApplicationContext()).initCommandLine();
    };
}
