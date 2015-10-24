// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextmenu;

import android.content.Context;
import android.net.MailTo;
import android.os.Build;
import android.text.TextUtils;
import android.view.ContextMenu;
import android.view.MenuInflater;

import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.UrlUtilities;
import org.chromium.chrome.browser.net.spdyproxy.DataReductionProxySettings;
import org.chromium.chrome.browser.preferences.datareduction.DataReductionProxyUma;
import org.chromium.chrome.browser.search_engines.TemplateUrlService;

/**
 * A {@link ContextMenuPopulator} used for showing the default Chrome context menu.
 */
public class ChromeContextMenuPopulator implements ContextMenuPopulator {
    private final ChromeContextMenuItemDelegate mDelegate;
    private MenuInflater mMenuInflater;
    private static final String BLANK_URL = "about:blank";

    static class ContextMenuUma {
        // Note: these values must match the ContextMenuOption enum in histograms.xml.
        static final int ACTION_OPEN_IN_NEW_TAB = 0;
        static final int ACTION_OPEN_IN_INCOGNITO_TAB = 1;
        static final int ACTION_COPY_LINK_ADDRESS = 2;
        static final int ACTION_COPY_EMAIL_ADDRESS = 3;
        static final int ACTION_COPY_LINK_TEXT = 4;
        static final int ACTION_SAVE_LINK = 5;
        static final int ACTION_SAVE_IMAGE = 6;
        static final int ACTION_OPEN_IMAGE = 7;
        static final int ACTION_OPEN_IMAGE_IN_NEW_TAB = 8;
        static final int ACTION_COPY_IMAGE = 9;
        static final int ACTION_COPY_IMAGE_URL = 10;
        static final int ACTION_SEARCH_BY_IMAGE = 11;
        static final int ACTION_LOAD_IMAGES = 12;
        static final int ACTION_LOAD_ORIGINAL_IMAGE = 13;
        static final int ACTION_SAVE_VIDEO = 14;
        static final int NUM_ACTIONS = 15;

        /**
         * Records a histogram entry when the user selects an item from a context menu.
         * @param params The ContextMenuParams describing the current context menu.
         * @param action The action that the user selected (e.g. ACTION_SAVE_IMAGE).
         */
        static void record(ContextMenuParams params, int action) {
            assert action >= 0;
            assert action < NUM_ACTIONS;
            String histogramName;
            if (params.isVideo()) {
                histogramName = "ContextMenu.SelectedOption.Video";
            } else if (params.isImage()) {
                histogramName = params.isAnchor()
                        ? "ContextMenu.SelectedOption.ImageLink"
                        : "ContextMenu.SelectedOption.Image";
            } else {
                assert params.isAnchor();
                histogramName = "ContextMenu.SelectedOption.Link";
            }
            RecordHistogram.recordEnumeratedHistogram(histogramName, action, NUM_ACTIONS);
        }
    }

    /**
     * Builds a {@link ChromeContextMenuPopulator}.
     * @param delegate The {@link ChromeContextMenuItemDelegate} that will be notified with actions
     *                 to perform when menu items are selected.
     */
    public ChromeContextMenuPopulator(ChromeContextMenuItemDelegate delegate) {
        mDelegate = delegate;
    }

    @Override
    public boolean shouldShowContextMenu(ContextMenuParams params) {
        return params != null && (params.isAnchor() || params.isImage() || params.isVideo());
    }

    @Override
    public void buildContextMenu(ContextMenu menu, Context context, ContextMenuParams params) {
        if (!TextUtils.isEmpty(params.getLinkUrl()) && !params.getLinkUrl().equals(BLANK_URL)) {
            setHeaderText(context, menu, params.getLinkUrl());
        } else if (!TextUtils.isEmpty(params.getTitleText())) {
            setHeaderText(context, menu, params.getTitleText());
        }

        if (mMenuInflater == null) mMenuInflater = new MenuInflater(context);

        mMenuInflater.inflate(R.menu.chrome_context_menu, menu);

        menu.setGroupVisible(R.id.contextmenu_group_anchor, params.isAnchor());
        menu.setGroupVisible(R.id.contextmenu_group_image, params.isImage());
        menu.setGroupVisible(R.id.contextmenu_group_video, params.isVideo());

        if (mDelegate.isIncognito() || !mDelegate.isIncognitoSupported()) {
            menu.findItem(R.id.contextmenu_open_in_incognito_tab).setVisible(false);
        }

        if (params.getLinkText().trim().isEmpty()) {
            menu.findItem(R.id.contextmenu_copy_link_text).setVisible(false);
        }

        if (MailTo.isMailTo(params.getLinkUrl())) {
            menu.findItem(R.id.contextmenu_copy_link_address_text).setVisible(false);
        } else {
            menu.findItem(R.id.contextmenu_copy_email_address).setVisible(false);
        }

        menu.findItem(R.id.contextmenu_save_link_as).setVisible(
                UrlUtilities.isDownloadableScheme(params.getLinkUrl()));

        if (params.imageWasFetchedLoFi()
                || !DataReductionProxySettings.getInstance().wasLoFiModeActiveOnMainFrame()
                || !DataReductionProxySettings.getInstance().canUseDataReductionProxy(
                        params.getPageUrl())) {
            menu.findItem(R.id.contextmenu_load_images).setVisible(false);
        } else {
            // Links can have images as backgrounds that aren't recognized here as images. CSS
            // properties can also prevent an image underlying a link from being clickable.
            // When Lo-Fi is active, provide the user with a "Load images" option on links
            // to get the images in these cases.
            DataReductionProxyUma.dataReductionProxyLoFiUIAction(
                    DataReductionProxyUma.ACTION_LOAD_IMAGES_CONTEXT_MENU_SHOWN);
        }

        if (params.isVideo()) {
            menu.findItem(R.id.contextmenu_save_video).setVisible(
                    UrlUtilities.isDownloadableScheme(params.getSrcUrl()));
        } else if (params.isImage() && params.imageWasFetchedLoFi()) {
            DataReductionProxyUma.dataReductionProxyLoFiUIAction(
                    DataReductionProxyUma.ACTION_LOAD_IMAGE_CONTEXT_MENU_SHOWN);
            // All image context menu items other than "Load image," "Open original image in
            // new tab," and "Copy image URL" should be disabled on Lo-Fi images.
            menu.findItem(R.id.contextmenu_save_image).setVisible(false);
            menu.findItem(R.id.contextmenu_open_image).setVisible(false);
            menu.findItem(R.id.contextmenu_open_image_in_new_tab).setVisible(false);
            menu.findItem(R.id.contextmenu_search_by_image).setVisible(false);
            menu.findItem(R.id.contextmenu_copy_image).setVisible(false);
        } else if (params.isImage() && !params.imageWasFetchedLoFi()) {
            menu.findItem(R.id.contextmenu_load_original_image).setVisible(false);

            menu.findItem(R.id.contextmenu_save_image).setVisible(
                    UrlUtilities.isDownloadableScheme(params.getSrcUrl()));

            if (mDelegate.canLoadOriginalImage()) {
                menu.findItem(R.id.contextmenu_open_image_in_new_tab).setVisible(false);
            } else {
                menu.findItem(R.id.contextmenu_open_original_image_in_new_tab).setVisible(false);
            }

            // Avoid showing open image option for same image which is already opened.
            if (mDelegate.getPageUrl().equals(params.getSrcUrl())) {
                menu.findItem(R.id.contextmenu_open_image).setVisible(false);
            }
            final TemplateUrlService templateUrlServiceInstance = TemplateUrlService.getInstance();
            final boolean isSearchByImageAvailable =
                    UrlUtilities.isDownloadableScheme(params.getSrcUrl())
                            && templateUrlServiceInstance.isLoaded()
                            && templateUrlServiceInstance.isSearchByImageAvailable()
                            && templateUrlServiceInstance.getDefaultSearchEngineTemplateUrl()
                                    != null;

            menu.findItem(R.id.contextmenu_search_by_image).setVisible(isSearchByImageAvailable);
            if (isSearchByImageAvailable) {
                menu.findItem(R.id.contextmenu_search_by_image).setTitle(
                        context.getString(R.string.contextmenu_search_web_for_image,
                                TemplateUrlService.getInstance()
                                        .getDefaultSearchEngineTemplateUrl().getShortName()));
            }

            menu.findItem(R.id.contextmenu_copy_image).setVisible(
                    Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN);
        }
    }

    @Override
    public boolean onItemSelected(ContextMenuHelper helper, ContextMenuParams params, int itemId) {
        if (itemId == R.id.contextmenu_open_in_new_tab) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_OPEN_IN_NEW_TAB);
            mDelegate.onOpenInNewTab(params.getLinkUrl(), params.getReferrer());
        } else if (itemId == R.id.contextmenu_open_in_incognito_tab) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_OPEN_IN_INCOGNITO_TAB);
            mDelegate.onOpenInNewIncognitoTab(params.getLinkUrl());
        } else if (itemId == R.id.contextmenu_open_image) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_OPEN_IMAGE);
            mDelegate.onOpenImageUrl(params.getSrcUrl(), params.getReferrer());
        } else if (itemId == R.id.contextmenu_open_image_in_new_tab
                || itemId == R.id.contextmenu_open_original_image_in_new_tab) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_OPEN_IMAGE_IN_NEW_TAB);
            mDelegate.onOpenImageInNewTab(params.getSrcUrl(), params.getReferrer());
        } else if (itemId == R.id.contextmenu_load_images) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_LOAD_IMAGES);
            DataReductionProxyUma.dataReductionProxyLoFiUIAction(
                    DataReductionProxyUma.ACTION_LOAD_IMAGES_CONTEXT_MENU_CLICKED);
            mDelegate.onReloadIgnoringCache();
        } else if (itemId == R.id.contextmenu_load_original_image) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_LOAD_ORIGINAL_IMAGE);
            DataReductionProxyUma.dataReductionProxyLoFiUIAction(
                    DataReductionProxyUma.ACTION_LOAD_IMAGE_CONTEXT_MENU_CLICKED);
            if (!DataReductionProxySettings.getInstance().wasLoFiLoadImageRequestedBefore()) {
                DataReductionProxyUma.dataReductionProxyLoFiUIAction(
                        DataReductionProxyUma.ACTION_LOAD_IMAGE_CONTEXT_MENU_CLICKED_ON_PAGE);
                DataReductionProxySettings.getInstance().setLoFiLoadImageRequested();
            }
            mDelegate.onLoadOriginalImage();
        } else if (itemId == R.id.contextmenu_copy_link_address_text) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_COPY_LINK_ADDRESS);
            mDelegate.onSaveToClipboard(params.getUnfilteredLinkUrl(),
                    ChromeContextMenuItemDelegate.CLIPBOARD_TYPE_LINK_URL);
        } else if (itemId == R.id.contextmenu_copy_email_address) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_COPY_EMAIL_ADDRESS);
            mDelegate.onSaveToClipboard(MailTo.parse(params.getLinkUrl()).getTo(),
                    ChromeContextMenuItemDelegate.CLIPBOARD_TYPE_LINK_URL);
        } else if (itemId == R.id.contextmenu_copy_link_text) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_COPY_LINK_TEXT);
            mDelegate.onSaveToClipboard(
                    params.getLinkText(), ChromeContextMenuItemDelegate.CLIPBOARD_TYPE_LINK_TEXT);
        } else if (itemId == R.id.contextmenu_save_image) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_SAVE_IMAGE);
            if (mDelegate.startDownload(params.getSrcUrl(), false)) {
                helper.startContextMenuDownload(
                        false, mDelegate.isDataReductionProxyEnabledForURL(params.getSrcUrl()));
            }
        } else if (itemId == R.id.contextmenu_save_video) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_SAVE_VIDEO);
            if (mDelegate.startDownload(params.getSrcUrl(), false)) {
                helper.startContextMenuDownload(false, false);
            }
        } else if (itemId == R.id.contextmenu_save_link_as) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_SAVE_LINK);
            if (mDelegate.startDownload(params.getUnfilteredLinkUrl(), true)) {
                helper.startContextMenuDownload(true, false);
            }
        } else if (itemId == R.id.contextmenu_search_by_image) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_SEARCH_BY_IMAGE);
            mDelegate.onSearchByImageInNewTab();
        } else if (itemId == R.id.contextmenu_copy_image) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_COPY_IMAGE);
            mDelegate.onSaveImageToClipboard(params.getSrcUrl());
        } else if (itemId == R.id.contextmenu_copy_image_url) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_COPY_IMAGE_URL);
            mDelegate.onSaveToClipboard(
                    params.getSrcUrl(), ChromeContextMenuItemDelegate.CLIPBOARD_TYPE_IMAGE_URL);
        } else {
            assert false;
        }

        return true;
    }

    private void setHeaderText(Context context, ContextMenu menu, String text) {
        ContextMenuTitleView title = new ContextMenuTitleView(context, text);
        menu.setHeaderView(title);
    }
}
