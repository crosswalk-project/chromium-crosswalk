/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2006, 2007, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Andreas Kling (kling@webkit.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef CSSRule_h
#define CSSRule_h

#include "bindings/core/v8/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "wtf/RefCounted.h"
#include "wtf/text/WTFString.h"

namespace blink {

class CSSParserContext;
class CSSRuleList;
class CSSStyleSheet;
class StyleRuleBase;

class CORE_EXPORT CSSRule : public RefCountedWillBeGarbageCollectedFinalized<CSSRule>, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    virtual ~CSSRule() { }

    enum Type {
        STYLE_RULE = 1,
        CHARSET_RULE = 2,
        IMPORT_RULE = 3,
        MEDIA_RULE = 4,
        FONT_FACE_RULE = 5,
        PAGE_RULE = 6,
        KEYFRAMES_RULE = 7,
        WEBKIT_KEYFRAMES_RULE = KEYFRAMES_RULE,
        KEYFRAME_RULE = 8,
        WEBKIT_KEYFRAME_RULE = KEYFRAME_RULE,
        NAMESPACE_RULE = 10,
        SUPPORTS_RULE = 12,
        VIEWPORT_RULE = 15,
    };

    virtual Type type() const = 0;
    virtual String cssText() const = 0;
    virtual void reattach(StyleRuleBase*) = 0;

    virtual CSSRuleList* cssRules() const { return 0; }

    void setParentStyleSheet(CSSStyleSheet* styleSheet)
    {
        m_parentIsRule = false;
        m_parentStyleSheet = styleSheet;
    }

    void setParentRule(CSSRule* rule)
    {
        m_parentIsRule = true;
        m_parentRule = rule;
    }

    DECLARE_VIRTUAL_TRACE();

    CSSStyleSheet* parentStyleSheet() const
    {
        if (m_parentIsRule)
            return m_parentRule ? m_parentRule->parentStyleSheet() : nullptr;
        return m_parentStyleSheet;
    }

    CSSRule* parentRule() const { return m_parentIsRule ? m_parentRule : nullptr; }

    // NOTE: Just calls notImplemented().
    void setCSSText(const String&);

protected:
    CSSRule(CSSStyleSheet* parent)
        : m_hasCachedSelectorText(false)
        , m_parentIsRule(false)
        , m_parentStyleSheet(parent)
    {
    }

    bool hasCachedSelectorText() const { return m_hasCachedSelectorText; }
    void setHasCachedSelectorText(bool hasCachedSelectorText) const { m_hasCachedSelectorText = hasCachedSelectorText; }

    const CSSParserContext& parserContext() const;

private:
    mutable unsigned char m_hasCachedSelectorText : 1;
    unsigned char m_parentIsRule : 1;

    // These should be Members, but no Members in unions.
    union {
        CSSRule* m_parentRule;
        CSSStyleSheet* m_parentStyleSheet;
    };
};

#define DEFINE_CSS_RULE_TYPE_CASTS(ToType, TYPE_NAME) \
    DEFINE_TYPE_CASTS(ToType, CSSRule, rule, rule->type() == CSSRule::TYPE_NAME, rule.type() == CSSRule::TYPE_NAME)

} // namespace blink

#endif // CSSRule_h
