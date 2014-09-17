// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.icu;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;

import java.lang.Character.UnicodeBlock;
import java.text.BreakIterator;
import java.util.Locale;

/**
 * This class is used to bridge BreakIterator for icu functionalities, when not
 * building with ICU.
 */
@JNINamespace("base::android")
public class BreakIteratorBridge {
    BreakIterator m_obj;
    String mText;

    BreakIteratorBridge(BreakIterator obj) {
        m_obj = obj;
    }

    @CalledByNative
    private static BreakIteratorBridge createLineInstance(String locale) {
        Locale l = IcuUtils.getLocale(locale);
        return new BreakIteratorBridge(BreakIterator.getLineInstance(l)); 
    }

    @CalledByNative
    private static BreakIteratorBridge createWordInstance(String locale) {
        Locale l = IcuUtils.getLocale(locale);
        return new BreakIteratorBridge(BreakIterator.getWordInstance(l)); 
    }

    @CalledByNative
    private static BreakIteratorBridge createCharacterInstance(String locale) {
        Locale l = IcuUtils.getLocale(locale);
        return new BreakIteratorBridge(BreakIterator.getCharacterInstance(l)); 
    }

    @CalledByNative
    private static BreakIteratorBridge createSentenceInstance(String locale) {
        Locale l = IcuUtils.getLocale(locale);
        return new BreakIteratorBridge(BreakIterator.getSentenceInstance(l)); 
    }

    @CalledByNative
    void setText(String text) {
        m_obj.setText(text);
        mText = text;
    }

    @CalledByNative
    int first() {
        return m_obj.first();
    }

    @CalledByNative
    int last() {
        return m_obj.last();
    }

    @CalledByNative
    int previous() {
        return m_obj.previous();
    }

    @CalledByNative
    int next() {
        return m_obj.next();
    }

    @CalledByNative
    int current() {
        return m_obj.current();
    }

    @CalledByNative
    int following(int offset) {
        return m_obj.following(offset);
    }

    @CalledByNative
    int preceding(int offset) {
        return m_obj.preceding(offset);
    }

    @CalledByNative
    boolean isBoundary(int offset) {
        return m_obj.isBoundary(offset);
    }

    @CalledByNative
    boolean isWord() {
        int current = m_obj.current();
        int prev = m_obj.previous();
        // Current should always be a boundary, so previous and next make nothing changed.
        assert(current == m_obj.next());
        if (current == BreakIterator.DONE || prev == BreakIterator.DONE) return false;
        String text = mText.substring(prev, current);
        if (text.isEmpty()) return false;
        // BreakIterator will categorize the word, so we only need to detect the first character.
        if (Character.isLetterOrDigit(text.charAt(0))) return true;
        UnicodeBlock type = Character.UnicodeBlock.of(text.charAt(0));
        // In icu, there are categories for ideographic and kana charactors besides letter and number.
        // Referencing third_party/icu/source/common/unicode/ubrk.h
        // So here return true for those characters.
        if (type == UnicodeBlock.CJK_COMPATIBILITY_IDEOGRAPHS ||
                type == UnicodeBlock.CJK_COMPATIBILITY_IDEOGRAPHS_SUPPLEMENT ||
                type == UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS ||
                type == UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A ||
                type == UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B ||
                type == UnicodeBlock.KATAKANA ||
                type == UnicodeBlock.KATAKANA_PHONETIC_EXTENSIONS) {
            return true;
        }
        return false;
    }
}
