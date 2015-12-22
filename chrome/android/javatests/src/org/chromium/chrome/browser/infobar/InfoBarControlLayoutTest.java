// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import android.content.Context;
import android.test.InstrumentationTestCase;
import android.test.UiThreadTest;
import android.test.suitebuilder.annotation.SmallTest;
import android.view.View;
import android.view.View.MeasureSpec;
import android.view.ViewGroup.LayoutParams;

import org.chromium.chrome.R;

/**
 * Tests for InfoBarControlLayout.  This suite doesn't check for specific details, like margins
 * paddings, and instead focuses on whether controls are placed correctly.
 */
public class InfoBarControlLayoutTest extends InstrumentationTestCase {
    private static final int SWITCH_ID_1 = 1;
    private static final int SWITCH_ID_2 = 2;
    private static final int SWITCH_ID_3 = 3;
    private static final int SWITCH_ID_4 = 4;
    private static final int SWITCH_ID_5 = 5;

    private int mMaxInfoBarWidth;
    private Context mContext;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mContext = getInstrumentation().getTargetContext();
        mContext.setTheme(R.style.MainTheme);
        mMaxInfoBarWidth = mContext.getResources().getDimensionPixelSize(R.dimen.infobar_max_width);
    }

    /**
     * A small control on the last line takes up the full width.
     */
    @SmallTest
    @UiThreadTest
    public void testOneSmallControlTakesFullWidth() {
        InfoBarControlLayout layout = new InfoBarControlLayout(mContext);
        layout.setLayoutParams(
                new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
        View smallSwitch = layout.addSwitch(0, "A", SWITCH_ID_1, false);

        // Trigger the measurement algorithm.
        int parentWidthSpec =
                MeasureSpec.makeMeasureSpec(mMaxInfoBarWidth, MeasureSpec.AT_MOST);
        int parentHeightSpec = MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED);
        layout.measure(parentWidthSpec, parentHeightSpec);

        // Small control takes the full width of the layout because it's put on its own line.
        InfoBarControlLayout.ControlLayoutParams params =
                InfoBarControlLayout.getControlLayoutParams(smallSwitch);
        assertEquals(0, params.top);
        assertEquals(0, params.start);
        assertEquals(2, params.columnsRequired);
        assertEquals(mMaxInfoBarWidth, smallSwitch.getMeasuredWidth());
    }

    /**
     * Tests the layout algorithm on a set of five controls, the second of which is a huge control
     * and takes up the whole line.  The other smaller controls try to pack themselves as tightly
     * as possible, strecthing out if necessary for aesthetics, resulting in a layout like this:
     *
     * -------------------------
     * | A (small)             |
     * -------------------------
     * | B (big)               |
     * -------------------------
     * | C (small) | D (small) |
     * -------------------------
     * | E (small)             |
     * -------------------------
     */
    @SmallTest
    @UiThreadTest
    public void testComplexSwitchLayout() {
        // Add five controls to the layout.
        InfoBarControlLayout layout = new InfoBarControlLayout(mContext);
        layout.setLayoutParams(
                new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));

        View switch1 = layout.addSwitch(0, "A", SWITCH_ID_1, false);
        View switch2 = layout.addSwitch(0, "B", SWITCH_ID_2, false);
        View switch3 = layout.addSwitch(0, "C", SWITCH_ID_3, false);
        View switch4 = layout.addSwitch(0, "D", SWITCH_ID_4, false);
        View switch5 = layout.addSwitch(0, "E", SWITCH_ID_4, false);

        // Make the second control require the full layout width.
        switch2.setMinimumWidth(mMaxInfoBarWidth);

        // Trigger the measurement algorithm.
        int parentWidthSpec =
                MeasureSpec.makeMeasureSpec(mMaxInfoBarWidth, MeasureSpec.AT_MOST);
        int parentHeightSpec = MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED);
        layout.measure(parentWidthSpec, parentHeightSpec);

        // Small control takes the full width of the layout because the next one doesn't fit.
        assertEquals(0, layout.getControlLayoutParams(switch1).top);
        assertEquals(0, layout.getControlLayoutParams(switch1).start);
        assertEquals(2, layout.getControlLayoutParams(switch1).columnsRequired);
        assertEquals(mMaxInfoBarWidth, switch1.getMeasuredWidth());

        // Big control gets shunted onto the next row and takes up the whole space.
        assertTrue(layout.getControlLayoutParams(switch2).top > switch1.getMeasuredHeight());
        assertEquals(0, layout.getControlLayoutParams(switch2).start);
        assertEquals(2, layout.getControlLayoutParams(switch2).columnsRequired);
        assertEquals(mMaxInfoBarWidth, switch2.getMeasuredWidth());

        // Small control gets placed onto the next line and takes only half the width.
        int bottomOfSwitch2 =
                layout.getControlLayoutParams(switch2).top + switch2.getMeasuredHeight();
        assertTrue(layout.getControlLayoutParams(switch3).top > bottomOfSwitch2);
        assertEquals(0, layout.getControlLayoutParams(switch3).start);
        assertEquals(1, layout.getControlLayoutParams(switch3).columnsRequired);
        assertTrue(switch3.getMeasuredWidth() < mMaxInfoBarWidth);

        // Small control gets placed next to the previous small control.
        assertEquals(layout.getControlLayoutParams(switch3).top,
                layout.getControlLayoutParams(switch4).top);
        assertTrue(layout.getControlLayoutParams(switch4).start > switch3.getMeasuredWidth());
        assertEquals(1, layout.getControlLayoutParams(switch4).columnsRequired);
        assertTrue(switch4.getMeasuredWidth() < mMaxInfoBarWidth);

        // Last small control has no room left and gets put on its own line, taking the full width.
        int bottomOfSwitch4 =
                layout.getControlLayoutParams(switch4).top + switch4.getMeasuredHeight();
        assertTrue(layout.getControlLayoutParams(switch5).top > bottomOfSwitch4);
        assertEquals(0, layout.getControlLayoutParams(switch5).start);
        assertEquals(2, layout.getControlLayoutParams(switch5).columnsRequired);
        assertEquals(mMaxInfoBarWidth, switch5.getMeasuredWidth());
    }
}
