// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.externalnav;

import android.annotation.TargetApi;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Build;
import android.os.SystemClock;
import android.provider.Browser;
import android.test.InstrumentationTestCase;
import android.test.mock.MockContext;
import android.test.mock.MockPackageManager;
import android.test.suitebuilder.annotation.SmallTest;

import org.chromium.base.CommandLine;
import org.chromium.chrome.browser.Tab;
import org.chromium.chrome.browser.externalnav.ExternalNavigationHandler.OverrideUrlLoadingResult;
import org.chromium.chrome.browser.tab.TabRedirectHandler;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.ui.base.PageTransition;

import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.List;

/**
 * Instrumentation tests for {@link ExternalNavigationHandler}.
 */
public class ExternalNavigationHandlerTest extends InstrumentationTestCase {

    // Expectations
    private static final int IGNORE = 0x0;
    private static final int START_INCOGNITO = 0x1;
    private static final int START_ACTIVITY = 0x2;
    private static final int INTENT_SANITIZATION_EXCEPTION = 0x4;

    private static final int NO_REDIRECT = 0x0;
    private static final int REDIRECT = 0x1;

    private static final String SEARCH_RESULT_URL_FOR_TOM_HANKS =
            "https://www.google.com/search?q=tom+hanks";
    private static final String IMDB_WEBPAGE_FOR_TOM_HANKS = "http://m.imdb.com/name/nm0000158";
    private static final String INTENT_URL_WITH_FALLBACK_URL =
            "intent:///name/nm0000158#Intent;scheme=imdb;package=com.imdb.mobile;"
            + "S." + ExternalNavigationHandler.EXTRA_BROWSER_FALLBACK_URL + "="
            + Uri.encode(IMDB_WEBPAGE_FOR_TOM_HANKS) + ";end";
    private static final String INTENT_URL_WITH_FALLBACK_URL_WITHOUT_PACKAGE_NAME =
            "intent:///name/nm0000158#Intent;scheme=imdb;"
            + "S." + ExternalNavigationHandler.EXTRA_BROWSER_FALLBACK_URL + "="
            + Uri.encode(IMDB_WEBPAGE_FOR_TOM_HANKS) + ";end";
    private static final String SOME_JAVASCRIPT_PAGE = "javascript:window.open(0);";
    private static final String INTENT_URL_WITH_JAVASCRIPT_FALLBACK_URL =
            "intent:///name/nm0000158#Intent;scheme=imdb;package=com.imdb.mobile;"
            + "S." + ExternalNavigationHandler.EXTRA_BROWSER_FALLBACK_URL + "="
            + Uri.encode(SOME_JAVASCRIPT_PAGE) + ";end";
    private static final String IMDB_APP_INTENT_FOR_TOM_HANKS = "imdb:///name/nm0000158";
    private static final String INTENT_URL_WITH_CHAIN_FALLBACK_URL =
            "intent://scan/#Intent;scheme=zxing;"
            + "S." + ExternalNavigationHandler.EXTRA_BROWSER_FALLBACK_URL + "="
            + Uri.encode("http://url.myredirector.com/aaa") + ";end";

    private static final String PLUS_STREAM_URL = "https://plus.google.com/stream";
    private static final String CALENDAR_URL = "http://www.google.com/calendar";
    private static final String KEEP_URL = "http://www.google.com/keep";

    private final TestExternalNavigationDelegate mDelegate;
    private ExternalNavigationHandler mUrlHandler;

    public ExternalNavigationHandlerTest() {
        mDelegate = new TestExternalNavigationDelegate();
        mUrlHandler = new ExternalNavigationHandler(mDelegate);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mDelegate.setContext(getInstrumentation().getContext());
        CommandLine.init(new String[0]);
    }

    @SmallTest
    public void testOrdinaryIncognitoUri() {
        check("http://youtube.com/",
                null, /* referrer */
                true, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.OVERRIDE_WITH_EXTERNAL_INTENT,
                START_INCOGNITO);
    }

    @SmallTest
    public void testChromeReferrer() {
        // http://crbug.com/159153: Don't override http or https URLs from the NTP or bookmarks.
        check("http://youtube.com/",
                "chrome://about", /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);
        check("tel:012345678",
                "chrome://about", /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.OVERRIDE_WITH_EXTERNAL_INTENT,
                START_ACTIVITY);
    }

    @SmallTest
    public void testForwardBackNavigation() {
        // http://crbug.com/164194. We shouldn't show the intent picker on
        // forwards or backwards navigations.
        check("http://youtube.com/",
                null, /* referrer */
                false, /* incognito */
                PageTransition.LINK
                | PageTransition.FORWARD_BACK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);
    }

    @SmallTest
    public void testRedirectFromFormSubmit() {
        // http://crbug.com/181186: We need to show the intent picker when we receive a redirect
        // following a form submit. OAuth of native applications rely on this.
        check("market://1234",
                null, /* referrer */
                false, /* incognito */
                PageTransition.FORM_SUBMIT,
                REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.OVERRIDE_WITH_EXTERNAL_INTENT,
                START_ACTIVITY);
        check("http://youtube.com://",
                null, /* referrer */
                false, /* incognito */
                PageTransition.FORM_SUBMIT,
                REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.OVERRIDE_WITH_EXTERNAL_INTENT,
                START_ACTIVITY);
        // It doesn't make sense to allow intent picker without redirect, since form data
        // is not encoded in the intent (although, in theory, it could be passed in as
        // an extra data in the intent).
        check("http://youtube.com://",
                null, /* referrer */
                false, /* incognito */
                PageTransition.FORM_SUBMIT,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);
    }

    @SmallTest
    public void testIgnore() {
        // About, Content URIs are disabled in Chrome on Android.
        check("about:test",
                null, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);
        check("about:test",
                null, /* referrer */
                true, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);
        check("content:test",
                null, /* referrer */
                true, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);
        check("content:test",
                null, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);
    }

    @SmallTest
    public void testPageTransitionType() {
        // Non-link page transition type are ignored.
        check("http://youtube.com/",
                null, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.OVERRIDE_WITH_EXTERNAL_INTENT,
                START_ACTIVITY);
        check("http://youtube.com/",
                null, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.OVERRIDE_WITH_EXTERNAL_INTENT,
                START_ACTIVITY);
        // http://crbug.com/143118 - Don't show the picker for directly typed URLs, unless
        // the URL results in a redirect.
        check("http://youtube.com/",
                null, /* referrer */
                false, /* incognito */
                PageTransition.TYPED,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);
        // http://crbug.com/162106 - Don't show the picker on reload.
        check("http://youtube.com/",
                null, /* referrer */
                false, /* incognito */
                PageTransition.RELOAD,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);
    }

    @SmallTest
    public void testWtai() {
        // Start the telephone application with the given number.
        check("wtai://wp/mc;0123456789",
                null, /* referrer */
                true, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.OVERRIDE_WITH_EXTERNAL_INTENT,
                START_ACTIVITY | INTENT_SANITIZATION_EXCEPTION);
        // These two cases are currently unimplemented.
        check("wtai://wp/sd;0123456789",
                null, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE | INTENT_SANITIZATION_EXCEPTION);
        check("wtai://wp/ap;0123456789",
                null, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE | INTENT_SANITIZATION_EXCEPTION);
        // Ignore other WTAI urls.
        check("wtai://wp/invalid",
                null, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE | INTENT_SANITIZATION_EXCEPTION);
    }

    @SmallTest
    public void testExternalUri() {
        check("tel:012345678",
                null, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.OVERRIDE_WITH_EXTERNAL_INTENT,
                START_ACTIVITY);
    }

    @SmallTest
    public void testTypedRedirectToExternalProtocol() {
        // http://crbug.com/331571 reverted http://crbug.com/169549
        check("market://1234",
                null, /* referrer */
                false, /* incognito */
                PageTransition.TYPED,
                REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);
        // http://crbug.com/143118
        check("market://1234",
                null, /* referrer */
                false, /* incognito */
                PageTransition.TYPED,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);
    }

    @SmallTest
    public void testIncomingIntentRedirect() {
        int transitionTypeIncomingIntent = PageTransition.LINK
                | PageTransition.FROM_API;
        // http://crbug.com/149218
        check("http://youtube.com/",
                null, /* referrer */
                false, /* incognito */
                transitionTypeIncomingIntent,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);
        // http://crbug.com/170925
        check("http://youtube.com/",
                null, /* referrer */
                false, /* incognito */
                transitionTypeIncomingIntent,
                REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.OVERRIDE_WITH_EXTERNAL_INTENT,
                START_ACTIVITY);
    }

    @SmallTest
    public void testIntentScheme() {
        String url = "intent:wtai://wp/#Intent;action=android.settings.SETTINGS;"
                + "component=package/class;end";
        String urlWithSel = "intent:wtai://wp/#Intent;SEL;action=android.settings.SETTINGS;"
                + "component=package/class;end";

        check(url,
                null, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.OVERRIDE_WITH_EXTERNAL_INTENT,
                START_ACTIVITY);

        // http://crbug.com/370399
        check(urlWithSel,
                null, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.OVERRIDE_WITH_EXTERNAL_INTENT,
                START_ACTIVITY);
    }

    @SmallTest
    public void testYouTubePairingCode() {
        int transitionTypeIncomingIntent = PageTransition.LINK
                | PageTransition.FROM_API;
        String url = "http://m.youtube.com/watch?v=1234&pairingCode=5678";

        // http://crbug/386600 - it makes no sense to switch activities for pairing code URLs.
        check(url,
                null, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);

        check(url,
                null, /* referrer */
                false, /* incognito */
                transitionTypeIncomingIntent,
                REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);
    }

    @SmallTest
    public void testInitialIntent() throws URISyntaxException {
        TabRedirectHandler redirectHandler = new TabRedirectHandler(new TestContext());
        Intent ytIntent = Intent.parseUri("http://youtube.com/", Intent.URI_INTENT_SCHEME);
        Intent fooIntent = Intent.parseUri("http://foo.com/", Intent.URI_INTENT_SCHEME);
        int transTypeLinkFromIntent = PageTransition.LINK
                | PageTransition.FROM_API;

        // Ignore if url is redirected, transition type is IncomingIntent and a new intent doesn't
        // have any new resolver.
        redirectHandler.updateIntent(ytIntent);
        redirectHandler.updateNewUrlLoading(transTypeLinkFromIntent, false, false, 0, 0);
        redirectHandler.updateNewUrlLoading(transTypeLinkFromIntent, true, false, 0, 0);
        check("http://m.youtube.com/",
                null, /* referrer */
                false, /* incognito */
                transTypeLinkFromIntent,
                REDIRECT,
                true,
                false,
                redirectHandler,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);
        // Do not ignore if a new intent has any new resolver.
        redirectHandler.updateIntent(fooIntent);
        redirectHandler.updateNewUrlLoading(transTypeLinkFromIntent, false, false, 0, 0);
        redirectHandler.updateNewUrlLoading(transTypeLinkFromIntent, true, false, 0, 0);
        check("http://m.youtube.com/",
                null, /* referrer */
                false, /* incognito */
                transTypeLinkFromIntent,
                REDIRECT,
                true,
                false,
                redirectHandler,
                OverrideUrlLoadingResult.OVERRIDE_WITH_EXTERNAL_INTENT,
                START_ACTIVITY);
    }

    @SmallTest
    public void testFallbackUrl_IntentResolutionSucceeds() {
        // IMDB app is installed.
        mDelegate.setCanResolveActivity(true);

        check(INTENT_URL_WITH_FALLBACK_URL,
                SEARCH_RESULT_URL_FOR_TOM_HANKS, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.OVERRIDE_WITH_EXTERNAL_INTENT,
                START_ACTIVITY);

        Intent invokedIntent = mDelegate.startActivityIntent;
        assertEquals(IMDB_APP_INTENT_FOR_TOM_HANKS, invokedIntent.getData().toString());
        assertNull("The invoked intent should not have browser_fallback_url\n",
                invokedIntent.getStringExtra(ExternalNavigationHandler.EXTRA_BROWSER_FALLBACK_URL));
        assertNull(mDelegate.getNewUrlAfterClobbering());
        assertNull(mDelegate.getReferrerUrlForClobbering());
    }

    @SmallTest
    public void testFallbackUrl_IntentResolutionFails() {
        // IMDB app isn't installed.
        mDelegate.setCanResolveActivity(false);

        // When intent resolution fails, we should not start an activity, but instead clobber
        // the current tab.
        check(INTENT_URL_WITH_FALLBACK_URL,
                SEARCH_RESULT_URL_FOR_TOM_HANKS, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.OVERRIDE_WITH_CLOBBERING_TAB,
                IGNORE);

        assertNull(mDelegate.startActivityIntent);
        assertEquals(IMDB_WEBPAGE_FOR_TOM_HANKS, mDelegate.getNewUrlAfterClobbering());
        assertEquals(SEARCH_RESULT_URL_FOR_TOM_HANKS, mDelegate.getReferrerUrlForClobbering());
    }

    @SmallTest
    public void testFallbackUrl_IntentResolutionFailsWithoutPackageName() {
        // IMDB app isn't installed.
        mDelegate.setCanResolveActivity(false);

        // Fallback URL should work even when package name isn't given.
        check(INTENT_URL_WITH_FALLBACK_URL_WITHOUT_PACKAGE_NAME,
                SEARCH_RESULT_URL_FOR_TOM_HANKS, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.OVERRIDE_WITH_CLOBBERING_TAB,
                IGNORE);

        assertNull(mDelegate.startActivityIntent);
        assertEquals(IMDB_WEBPAGE_FOR_TOM_HANKS, mDelegate.getNewUrlAfterClobbering());
        assertEquals(SEARCH_RESULT_URL_FOR_TOM_HANKS, mDelegate.getReferrerUrlForClobbering());
    }

    @SmallTest
    public void testFallbackUrl_FallbackShouldNotWarnOnIncognito() {
        // IMDB app isn't installed.
        mDelegate.setCanResolveActivity(false);

        check(INTENT_URL_WITH_FALLBACK_URL,
                SEARCH_RESULT_URL_FOR_TOM_HANKS,
                true, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.OVERRIDE_WITH_CLOBBERING_TAB,
                IGNORE);

        assertNull(mDelegate.startActivityIntent);
        assertEquals(IMDB_WEBPAGE_FOR_TOM_HANKS, mDelegate.getNewUrlAfterClobbering());
        assertEquals(SEARCH_RESULT_URL_FOR_TOM_HANKS, mDelegate.getReferrerUrlForClobbering());
    }

    @SmallTest
    public void testFallbackUrl_IgnoreJavascriptFallbackUrl() {
        // IMDB app isn't installed.
        mDelegate.setCanResolveActivity(false);

        // Will be redirected market since package is given.
        check(INTENT_URL_WITH_JAVASCRIPT_FALLBACK_URL,
                SEARCH_RESULT_URL_FOR_TOM_HANKS,
                true, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.OVERRIDE_WITH_EXTERNAL_INTENT,
                START_ACTIVITY);

        Intent invokedIntent = mDelegate.startActivityIntent;
        assertTrue(invokedIntent.getData().toString().startsWith("market://"));
        assertEquals(null, mDelegate.getNewUrlAfterClobbering());
        assertEquals(null, mDelegate.getReferrerUrlForClobbering());
    }

    @SmallTest
    public void testFallback_UseFallbackUrlForRedirectionFromTypedInUrl() {
        TabRedirectHandler redirectHandler = new TabRedirectHandler(null);

        redirectHandler.updateNewUrlLoading(PageTransition.TYPED, false, false, 0, 0);
        check("http://goo.gl/abcdefg", null, /* referrer */
                false, /* incognito */
                PageTransition.TYPED, NO_REDIRECT, true, false, redirectHandler,
                OverrideUrlLoadingResult.NO_OVERRIDE, IGNORE);

        redirectHandler.updateNewUrlLoading(PageTransition.TYPED, true, false, 0, 0);
        check(INTENT_URL_WITH_FALLBACK_URL_WITHOUT_PACKAGE_NAME, null, /* referrer */
                false, /* incognito */
                PageTransition.TYPED, REDIRECT, true, false, redirectHandler,
                OverrideUrlLoadingResult.OVERRIDE_WITH_CLOBBERING_TAB, IGNORE);

        // Now the user opens a link.
        redirectHandler.updateNewUrlLoading(PageTransition.LINK, false, true, 0, 1);
        check("http://m.youtube.com/", null, /* referrer */
                false, /* incognito */
                PageTransition.LINK, NO_REDIRECT, true, false, redirectHandler,
                OverrideUrlLoadingResult.NO_OVERRIDE, IGNORE);
    }

    @SmallTest
    public void testIgnoreEffectiveRedirectFromIntentFallbackUrl() {
        // We cannot resolve any intent, so fall-back URL will be used.
        mDelegate.setCanResolveActivity(false);

        TabRedirectHandler redirectHandler = new TabRedirectHandler(null);

        redirectHandler.updateNewUrlLoading(PageTransition.LINK, false, true, 0, 0);
        check(INTENT_URL_WITH_CHAIN_FALLBACK_URL,
                null, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                redirectHandler,
                OverrideUrlLoadingResult.OVERRIDE_WITH_CLOBBERING_TAB,
                IGNORE);

        // As a result of intent resolution fallback, we have clobberred the current tab.
        // The fall-back URL was HTTP-schemed, but it was effectively redirected to a new intent
        // URL using javascript. However, we do not allow chained fallback intent, so we do NOT
        // override URL loading here.
        redirectHandler.updateNewUrlLoading(PageTransition.LINK, false, false, 0, 0);
        check(INTENT_URL_WITH_FALLBACK_URL,
                null, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                redirectHandler,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);

        // Now enough time (2 seconds) have passed.
        // New URL loading should not be affected.
        // (The URL happened to be the same as previous one.)
        // TODO(changwan): this is not likely cause flakiness, but it may be better to refactor
        // systemclock or pass the new time as parameter.
        long lastUserInteractionTimeInMillis = SystemClock.elapsedRealtime() + 2 * 1000L;
        redirectHandler.updateNewUrlLoading(
                PageTransition.LINK, false, true, lastUserInteractionTimeInMillis, 1);
        check(INTENT_URL_WITH_FALLBACK_URL,
                null, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                redirectHandler,
                OverrideUrlLoadingResult.OVERRIDE_WITH_CLOBBERING_TAB,
                IGNORE);
    }

    @SmallTest
    public void testIgnoreEffectiveRedirectFromUserTyping() {
        TabRedirectHandler redirectHandler = new TabRedirectHandler(null);

        redirectHandler.updateNewUrlLoading(PageTransition.TYPED, false, false, 0, 0);
        check("http://m.youtube.com/",
                null, /* referrer */
                false, /* incognito */
                PageTransition.TYPED,
                NO_REDIRECT,
                true,
                false,
                redirectHandler,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);

        redirectHandler.updateNewUrlLoading(PageTransition.TYPED, true, false, 0, 0);
        check("http://m.youtube.com/",
                null, /* referrer */
                false, /* incognito */
                PageTransition.TYPED,
                REDIRECT,
                true,
                false,
                redirectHandler,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);

        redirectHandler.updateNewUrlLoading(PageTransition.LINK, false, false, 0, 1);
        check("http://m.youtube.com/",
                null, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                redirectHandler,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);
    }

    @SmallTest
    public void testNavigationFromLinkWithoutUserGesture() {
        TabRedirectHandler redirectHandler = new TabRedirectHandler(null);

        redirectHandler.updateNewUrlLoading(PageTransition.LINK, false, false, 1, 0);
        check("http://m.youtube.com/",
                null, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                redirectHandler,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);

        redirectHandler.updateNewUrlLoading(PageTransition.LINK, true, false, 1, 0);
        check("http://m.youtube.com/",
                null, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                REDIRECT,
                true,
                false,
                redirectHandler,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);

        redirectHandler.updateNewUrlLoading(PageTransition.LINK, false, false, 1, 1);
        check("http://m.youtube.com/",
                null, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                redirectHandler,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);
    }

    @SmallTest
    public void testChromeAppInBackground() {
        mDelegate.setIsChromeAppInForeground(false);
        check("http://youtube.com/",
                null, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);
    }

    @SmallTest
    public void testNotChromeAppInForegroundRequired() {
        mDelegate.setIsChromeAppInForeground(false);
        check("http://youtube.com/",
                null, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                false,
                false,
                null,
                OverrideUrlLoadingResult.OVERRIDE_WITH_EXTERNAL_INTENT,
                START_ACTIVITY);
    }

    @SmallTest
    public void testCreatesIntentsToOpenInNewTab() {
        mUrlHandler = new ExternalNavigationHandler(mDelegate);
        ExternalNavigationParams params = new ExternalNavigationParams.Builder(
                "http://m.youtube.com", false)
                .setOpenInNewTab(true)
                .build();
        OverrideUrlLoadingResult result = mUrlHandler.shouldOverrideUrlLoading(params);
        assertEquals(OverrideUrlLoadingResult.OVERRIDE_WITH_EXTERNAL_INTENT, result);
        assertTrue(mDelegate.startActivityIntent != null);
        assertTrue(mDelegate.startActivityIntent.getBooleanExtra(
                Browser.EXTRA_CREATE_NEW_TAB, false));
    }

    @SmallTest
    public void testCanExternalAppHandleUrl() {
        mDelegate.setCanResolveActivity(true);
        assertTrue(mUrlHandler.canExternalAppHandleUrl("some_app://some_app.com/"));

        mDelegate.setCanResolveActivity(false);
        assertTrue(mUrlHandler.canExternalAppHandleUrl("wtai://wp/mc;0123456789"));
        assertTrue(mUrlHandler.canExternalAppHandleUrl(
                "intent:/#Intent;scheme=no_app;package=com.no_app;end"));
        assertFalse(mUrlHandler.canExternalAppHandleUrl("no_app://no_app.com/"));
    }

    @SmallTest
    public void testPlusAppRefresh() {
        check(PLUS_STREAM_URL,
                PLUS_STREAM_URL,
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);
    }

    @SmallTest
    public void testSameDomainDifferentApps() {
        check(CALENDAR_URL,
                KEEP_URL,
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                false,
                null,
                OverrideUrlLoadingResult.OVERRIDE_WITH_EXTERNAL_INTENT,
                START_ACTIVITY);
    }

    @SmallTest
    public void testBackgroundTabNavigation() {
        check("http://youtube.com/",
                null, /* referrer */
                false, /* incognito */
                PageTransition.LINK,
                NO_REDIRECT,
                true,
                true,
                null,
                OverrideUrlLoadingResult.NO_OVERRIDE,
                IGNORE);
    }

    private static class TestExternalNavigationDelegate implements ExternalNavigationDelegate {
        private Context mContext;

        public void setContext(Context context) {
            mContext = context;
        }

        @Override
        public List<ComponentName> queryIntentActivities(Intent intent) {
            List<ComponentName> list = new ArrayList<ComponentName>();
            if (intent.getDataString().startsWith("http://m.youtube.com")
                    || intent.getDataString().startsWith("http://youtube.com")) {
                list.add(new ComponentName("youtube", "youtube"));
            } else if (intent.getDataString().startsWith(PLUS_STREAM_URL)) {
                list.add(new ComponentName("plus", "plus"));
            } else if (intent.getDataString().startsWith(CALENDAR_URL)) {
                list.add(new ComponentName("calendar", "calendar"));
            } else {
                list.add(new ComponentName("foo", "foo"));
            }

            return list;
        }

        @Override
        public boolean canResolveActivity(Intent intent) {
            return mCanResolveActivity;
        }

        @Override
        public boolean willChromeHandleIntent(Intent intent) {
            return !isSpecializedHandlerAvailable(intent);
        }

        @Override
        public boolean isSpecializedHandlerAvailable(Intent intent) {
            String data = intent.getDataString();
            return (data.startsWith("http://youtube.com")
                    || data.startsWith("http://m.youtube.com")
                    || data.startsWith(CALENDAR_URL)
                    || data.startsWith("wtai://wp/"));
        }

        @Override
        public String getPackageName() {
            return "test";
        }

        @Override
        public void startActivity(Intent intent) {
            startActivityIntent = intent;
        }

        @Override
        public boolean startActivityIfNeeded(Intent intent) {
            // For simplicity, don't distinguish between startActivityIfNeeded and startActivity
            // until a test requires this distinction.
            startActivityIntent = intent;
            return true;
        }

        @Override
        public void startIncognitoIntent(Intent intent) {
            startIncognitoIntentCalled = true;
        }

        @Override
        public OverrideUrlLoadingResult clobberCurrentTab(
                String url, String referrerUrl, Tab tab) {
            mNewUrlAfterClobbering = url;
            mReferrerUrlForClobbering = referrerUrl;
            return OverrideUrlLoadingResult.OVERRIDE_WITH_CLOBBERING_TAB;
        }

        @Override
        public boolean isChromeAppInForeground() {
            return mIsChromeAppInForeground;
        }

        @Override
        public boolean isDocumentMode() {
            return FeatureUtilities.isDocumentMode(mContext);
        }

        public void reset() {
            startActivityIntent = null;
            startIncognitoIntentCalled = false;
        }

        public void setCanResolveActivity(boolean value) {
            mCanResolveActivity = value;
        }

        public String getNewUrlAfterClobbering() {
            return mNewUrlAfterClobbering;
        }

        public String getReferrerUrlForClobbering() {
            return mReferrerUrlForClobbering;
        }

        public void setIsChromeAppInForeground(boolean value) {
            mIsChromeAppInForeground = value;
        }

        public Intent startActivityIntent = null;
        public boolean startIncognitoIntentCalled = false;

        // This should not be reset for every run of check().
        private boolean mCanResolveActivity = true;
        private String mNewUrlAfterClobbering;
        private String mReferrerUrlForClobbering;
        public boolean mIsChromeAppInForeground = true;
    }

    private void checkIntentSanity(Intent intent, String name) {
        assertTrue("The invoked " + name + " doesn't have the BROWSABLE category set\n",
                intent.hasCategory(Intent.CATEGORY_BROWSABLE));
        assertNull("The invoked " + name + " should not have a Component set\n",
                intent.getComponent());
    }

    @TargetApi(Build.VERSION_CODES.ICE_CREAM_SANDWICH_MR1)
    public void check(String url,
                      String referrerUrl,
                      boolean isIncognito,
                      int pageTransition,
                      int isRedirect,
                      boolean chromeAppInForegroundRequired,
                      boolean isBackgroundTabNavigation,
                      TabRedirectHandler redirectHandler,
                      OverrideUrlLoadingResult expectedOverrideResult,
                      int otherExpectation) {
        boolean expectStartIncognito = (otherExpectation & START_INCOGNITO) != 0;
        boolean expectStartActivity = (otherExpectation & START_ACTIVITY) != 0;
        boolean expectSaneIntent = (otherExpectation & INTENT_SANITIZATION_EXCEPTION) == 0;

        mDelegate.reset();

        ExternalNavigationParams params = new ExternalNavigationParams.Builder(url, isIncognito,
                referrerUrl, pageTransition, isRedirect == REDIRECT)
                .setApplicationMustBeInForeground(chromeAppInForegroundRequired)
                .setRedirectHandler(redirectHandler)
                .setIsBackgroundTabNavigation(isBackgroundTabNavigation)
                .build();
        OverrideUrlLoadingResult result = mUrlHandler.shouldOverrideUrlLoading(params);
        boolean startActivityCalled = mDelegate.startActivityIntent != null;

        assertEquals(expectedOverrideResult, result);
        assertEquals(expectStartIncognito, mDelegate.startIncognitoIntentCalled);
        assertEquals(expectStartActivity, startActivityCalled);

        if (startActivityCalled && expectSaneIntent) {
            checkIntentSanity(mDelegate.startActivityIntent, "Intent");
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH_MR1) {
                if (mDelegate.startActivityIntent.getSelector() != null) {
                    checkIntentSanity(mDelegate.startActivityIntent.getSelector(),
                            "Intent's selector");
                }
            }
        }
    }

    private static class TestPackageManager extends MockPackageManager {
        @Override
        public List<ResolveInfo> queryIntentActivities(Intent intent, int flags) {
            List<ResolveInfo> resolves = new ArrayList<ResolveInfo>();
            if (intent.getDataString().startsWith("http://m.youtube.com")
                    ||  intent.getDataString().startsWith("http://youtube.com")) {
                ResolveInfo youTubeApp = new ResolveInfo();
                youTubeApp.activityInfo = new ActivityInfo();
                youTubeApp.activityInfo.packageName = "youtube";
                youTubeApp.activityInfo.name = "youtube";
                resolves.add(youTubeApp);
            } else {
                ResolveInfo fooApp = new ResolveInfo();
                fooApp.activityInfo = new ActivityInfo();
                fooApp.activityInfo.packageName = "foo";
                fooApp.activityInfo.name = "foo";
                resolves.add(fooApp);
            }
            return resolves;
        }
    }

    private static class TestContext extends MockContext {
        @Override
        public PackageManager getPackageManager() {
            return new TestPackageManager();
        }

        @Override
        public String getPackageName() {
            return "test.app.name";
        }

    }
}
