// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.icu;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;

import android.content.Context;
import android.text.TextUtils;
import android.text.format.DateFormat;
import android.text.format.Time;

import java.lang.Character;
import java.net.IDN;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.Charset;
import java.nio.charset.CharsetDecoder;
import java.nio.charset.CharsetEncoder;
import java.nio.charset.IllegalCharsetNameException;
import java.nio.charset.MalformedInputException;
import java.nio.charset.UnmappableCharacterException;
import java.nio.charset.UnsupportedCharsetException;
import java.text.BreakIterator;
import java.text.CharacterIterator;
import java.text.Collator;
import java.text.Normalizer;
import java.text.NumberFormat;
import java.text.StringCharacterIterator;
import java.util.HashSet;
import java.util.Locale;
import java.util.MissingResourceException;
import java.util.Set;

/**
 * This class is used to provide alternative for icu functionalities, when not
 * building with ICU.
 */
@JNINamespace("base::android")
public class IcuUtils {
    /**
     * Attempts to convert a Unicode string to an ASCII string using IDN rules.
     * As of May 2014, the underlying Java function IDNA2003.
     * @param src String to convert.
     * @return: String containing only ASCII characters on success, null on
     *                 failure.
     */
    @CalledByNative
    private static String idnToUnicode(String host, String language) {
        try {
            return IDN.toUnicode(host, IDN.USE_STD3_ASCII_RULES);
        } catch (Exception e) {
            return null;
        }
    }

    @CalledByNative
    private static String countryCodeForCurrentTimeZone() {
        Locale locale = Locale.getDefault();
        return locale.getCountry();
    }

    @CalledByNative
    private static String formatInteger(long number) {
        return NumberFormat.getIntegerInstance().format(number);
    }

    @CalledByNative
    private static String formatFloat(double number, int fractional_digits) {
        NumberFormat numberFormatter = NumberFormat.getNumberInstance();
        numberFormatter.setMaximumFractionDigits(fractional_digits);
        return numberFormatter.format(number);
    }

    @CalledByNative
    private static byte[] encodeCodePage(String codepageName, char[] src, int onError) {
        Charset cs = null;
        try {
            cs = Charset.forName(codepageName);
        } catch (IllegalCharsetNameException e) {
             return null;
        } catch (UnsupportedCharsetException e) {
             return null;
        }
        if (!cs.canEncode()) return null;
        CharsetEncoder encoder = cs.newEncoder();
        try {
             ByteBuffer encoded = encoder.encode(CharBuffer.wrap(src));
             return encoded.array();
        } catch (MalformedInputException e) {
             return null;
        } catch (UnmappableCharacterException e) {
             return null;
        } catch (CharacterCodingException e) {
             return null;
        }
    }

    @CalledByNative
    private static char[] decodeCodePage(String codepageName, byte[] src, int onError) {
        Charset cs = null;
        try {
            cs = Charset.forName(codepageName);
        } catch (IllegalCharsetNameException e) {
            return null;
        } catch (UnsupportedCharsetException e) {
            return null;
        }
        CharsetDecoder decoder = cs.newDecoder();
        try {
            CharBuffer utf16 = decoder.decode(ByteBuffer.wrap(src));
            return utf16.array();
        } catch (MalformedInputException e) {
            return null;
        } catch (UnmappableCharacterException e) {
            return null;
        } catch (CharacterCodingException e) {
            return null;
        }
    }

    @CalledByNative
    private static String getConfiguredLocale() {
        Locale locale = Locale.getDefault();
        try {
            String ret = locale.getISO3Language();
            String country = locale.getCountry();
            if (!country.isEmpty()) ret = ret + "-" + country;
            return ret;
        } catch (MissingResourceException e) {
            return "";
        }
    }

    @CalledByNative
    static Locale getLocale(String localeString) {
        if (localeString == null || localeString.isEmpty()) return Locale.getDefault();
        int hythonPos = localeString.indexOf("-");
        if (hythonPos == -1) {
            return new Locale(localeString);
        } else {
            String lang = localeString.substring(0, hythonPos);
            String country = localeString.substring(hythonPos + 1);
            return new Locale(lang, country);
        }
    }

    @CalledByNative
    private static void setDefaultLocale(String localeString) {
        Locale.setDefault(getLocale(localeString));
    }

    @CalledByNative
    private static String formatTimeOfDay(long mills, boolean hour24, boolean keepAmPm) {
        Time time = new Time();
        time.set(mills);
        String format = "";
        if (hour24) format = "%H:%M";
        else format = "%I:%M";
        if (keepAmPm) format += " %P";
        return time.format(format);
    }

    @CalledByNative
    private static String formatDate(long mills, int dateType, boolean appendTimeOfDay) {
        Time time = new Time();
        time.set(mills);
        String format = "";
        // Short type
        if (dateType == 0) format = "%b %e, %Y";
        // numberic type
        else if (dateType == 1) format = "%D";
        // friendly type
        else if (dateType == 2) format = "%A, %B %e, %Y";
        if (appendTimeOfDay) format += " %r";
        return time.format(format);
    }

    @CalledByNative
    private static boolean is24HourFormat(Context context) {
        return DateFormat.is24HourFormat(context);
    }

    private static Set<Integer> illegalCharsetInFilename = null;
    static {
        illegalCharsetInFilename = new HashSet<Integer>();
        for (int illegalChar : new int[]{'"', '*', '/', ':', '<', '>', '?', '\\'}) {
            illegalCharsetInFilename.add(illegalChar);
        }
    }

    @CalledByNative
    private static boolean isCodePointInFilenameIllegal(int codePoint) {
        if (illegalCharsetInFilename.contains(codePoint)) return true;
        if (codePoint >= 0xFDD0 && codePoint <= 0xFDEF) return true;
        int tail = codePoint & 0xFFFF;
        if (tail >= 0xFFFE && tail <= 0xFFFF) {
            int head = codePoint / 0x10000;
            if (head > 0 && head <= 0x10) return true;
        }
        return false;
    }

    @CalledByNative
    private static String cutOffStringForEliding(String input, int max, boolean wordBreak) {
        int index = max;
        if (wordBreak) {
            BreakIterator bi = BreakIterator.getLineInstance();
            bi.setText(input);
            index = bi.preceding(index);
            if (index == BreakIterator.DONE || index == 0) {
                index = max;
            }
        }

        CharacterIterator ci = new StringCharacterIterator(input);
        ci.setIndex(max);
        while(ci.previous() != CharacterIterator.DONE) {
            if (!(Character.isSpaceChar(ci.current()) ||
                    Character.getType(ci.current()) == Character.CONTROL ||
                    Character.getType(ci.current()) == Character.NON_SPACING_MARK)) {
                ci.next();
                break;
            }
        }
        index = ci.getIndex();
        if (ci.previous() == CharacterIterator.DONE) return "";

         return input.substring(0, index);
    }

    @CalledByNative
    private static int collateWithLocale(String locale, String string1, String string2) {
        if (locale == null || locale.isEmpty()) return collate(string1, string2);
        return Collator.getInstance(getLocale(locale)).compare(string1, string2);
    }

    private static int collate(String string1, String string2) {
        return Collator.getInstance().compare(string1, string2);
    }

    @CalledByNative
    private static String normalize(String src) {
        return Normalizer.normalize(src, Normalizer.Form.NFC);
    }

    @CalledByNative
    private static int compareToIgnoreCase(String string1, String string2) {
        return string1.compareToIgnoreCase(string2);
    }

    @CalledByNative
    private static int toLower(int codePoint) {
        return Character.toLowerCase(codePoint);
    }

    @CalledByNative
    private static int toUpper(int codePoint) {
        return Character.toUpperCase(codePoint);
    }

    @CalledByNative
    private static String toLowerString(String src) {
        return src.toLowerCase();
    }

    @CalledByNative
    private static String toUpperString(String src) {
        return src.toUpperCase();
    }

    @CalledByNative
    private static int toTitleCase(int codePoint) {
        return Character.toTitleCase(codePoint);
    }

    @CalledByNative
    private static boolean isArabicChar(int codePoint) {
        return Character.UnicodeBlock.of(codePoint) ==  Character.UnicodeBlock.ARABIC;
    }

    @CalledByNative
    private static boolean isAlphanumeric(int codePoint) {
        return Character.isLetterOrDigit(codePoint);
    }

    @CalledByNative
    private static boolean isSeparatorSpace(int codePoint) {
        return Character.isSpaceChar(codePoint);
    }

    @CalledByNative
    private static boolean isPrintableChar(int codePoint) {
        return TextUtils.isGraphic(CharBuffer.wrap(Character.toChars(codePoint)));
    }

    @CalledByNative
    private static boolean isPunct(int codePoint) {
        int flags = Character.DASH_PUNCTUATION |
                    Character.START_PUNCTUATION |
                    Character.END_PUNCTUATION |
                    Character.CONNECTOR_PUNCTUATION |
                    Character.INITIAL_QUOTE_PUNCTUATION |
                    Character.FINAL_QUOTE_PUNCTUATION |
                    Character.OTHER_PUNCTUATION;

        return (Character.getType(codePoint) & flags) != 0;
    }

    @CalledByNative
    private static boolean isLineSeparator(int codePoint) {
        return (Character.getType(codePoint) & Character.LINE_SEPARATOR) != 0;
    }

    @CalledByNative
    private static int mirroredChar(int codePoint) {
        // AndroidCharacter.getMirror ?
        return codePoint;
    }

    @CalledByNative
    private static int category(int codePoint) {
        int javaType = Character.getType(codePoint);
        // Java Unicode Type consts are in the same sequence as native ICU UCharCategory.
        // The only difference is that java consts don't have number 17.
        if (javaType < 17) return javaType;
        else if (javaType > 17) return javaType - 1;
        else return 0;
    }

    private static final int[] DIRECTIONALITY_MAP = new int[] {
            0, //  DIRECTIONALITY_LEFT_TO_RIGHT, 0
            1, //  DIRECTIONALITY_RIGHT_TO_LEFT, 1
            13, // DIRECTIONALITY_RIGHT_TO_LEFT_ARABIC, 2
            2, //  DIRECTIONALITY_EUROPEAN_NUMBER, 3
            3, //  DIRECTIONALITY_EUROPEAN_NUMBER_SEPARATOR, 4
            4, //  DIRECTIONALITY_EUROPEAN_NUMBER_TERMINATOR, 5
            5, //  DIRECTIONALITY_ARABIC_NUMBER, 6
            6, //  DIRECTIONALITY_COMMON_NUMBER_SEPARATOR, 7
            17, // DIRECTIONALITY_NONSPACING_MARK, 8
            18, // DIRECTIONALITY_BOUNDARY_NEUTRAL 9
            7, //  DIRECTIONALITY_PARAGRAPH_SEPARATOR, 10
            8, //  DIRECTIONALITY_SEGMENT_SEPARATOR, 11
            9, //  DIRECTIONALITY_WHITESPACE, 12
            10, // DIRECTIONALITY_OTHER_NEUTRALS, 13
            11, // DIRECTIONALITY_LEFT_TO_RIGHT_EMBEDDING, 14
            12, // DIRECTIONALITY_LEFT_TO_RIGHT_OVERRIDE, 15
            14, // DIRECTIONALITY_RIGHT_TO_LEFT_EMBEDDING, 16
            15, // DIRECTIONALITY_RIGHT_TO_LEFT_OVERRIDE, 17
            16  // DIRECTIONALITY_POP_DIRECTIONAL_FORMAT, 18
    };

    @CalledByNative
    private static int direction(int codePoint) {
        int javaDirection = Character.getDirectionality(codePoint);
        if (javaDirection == Character.DIRECTIONALITY_UNDEFINED) {
            javaDirection = Character.DIRECTIONALITY_LEFT_TO_RIGHT;
        }
        return DIRECTIONALITY_MAP[javaDirection];
    }

    @CalledByNative
    private static boolean isLower(int codePoint) {
        return Character.isLowerCase(codePoint);
    }

    @CalledByNative
    private static boolean isUpper(int codePoint) {
        return Character.isUpperCase(codePoint);
    }
}
