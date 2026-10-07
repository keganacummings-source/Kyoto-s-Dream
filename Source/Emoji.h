#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>

namespace kt::emoji
{
enum class Id { None, Heart, Fire, Laugh, Moon, Up, Skull, Eyes, Hundred, Generic };

struct Picker { const char* name; const char* label; Id id; };
inline constexpr Picker kPicker[] = {
    { "heart", "❤  heart", Id::Heart }, { "fire", "🔥  fire", Id::Fire },
    { "laugh", "😂  laugh", Id::Laugh }, { "moon", "🌙  moon", Id::Moon },
    { "100", "💯  100", Id::Hundred }, { "up", "👍  up", Id::Up },
    { "skull", "💀  skull", Id::Skull }, { "eyes", "👀  eyes", Id::Eyes },
    { "joy", "😂  joy", Id::Laugh }, { "sparkles", "✨  sparkles", Id::Generic },
    { "wave", "👋  wave", Id::Generic }, { "star", "★  star", Id::Generic }
};
inline constexpr int kPickerCount = (int) (sizeof(kPicker) / sizeof(kPicker[0]));

inline Id fromName(const juce::String& name)
{
    for (const auto& p : kPicker)
        if (name.equalsIgnoreCase(p.name)) return p.id;
    return Id::None;
}

inline void draw(juce::Graphics& g, Id id, juce::Rectangle<float> r)
{
    const auto c = juce::Colour(0xfff0e6ff);
    const auto accent = juce::Colour(0xffc77dff);
    g.setColour(c);
    const auto cx = r.getCentreX(), cy = r.getCentreY(), s = juce::jmin(r.getWidth(), r.getHeight());

    if (id == Id::Heart)
    {
        juce::Path p;
        p.startNewSubPath(cx, cy + s * .30f);
        p.cubicTo(cx - s*.55f, cy - s*.02f, cx - s*.34f, cy - s*.50f, cx, cy - s*.12f);
        p.cubicTo(cx + s*.34f, cy - s*.50f, cx + s*.55f, cy - s*.02f, cx, cy + s*.30f);
        g.fillPath(p); return;
    }
    if (id == Id::Fire)
    {
        juce::Path p;
        p.startNewSubPath(cx, cy+s*.40f);
        p.cubicTo(cx-s*.42f, cy+s*.15f, cx-s*.28f, cy-s*.16f, cx-s*.02f, cy-s*.46f);
        p.cubicTo(cx+s*.02f, cy-s*.20f, cx+s*.28f, cy-s*.14f, cx+s*.20f, cy+s*.10f);
        p.cubicTo(cx+s*.52f, cy-s*.04f, cx+s*.46f, cy+s*.40f, cx, cy+s*.40f);
        g.fillPath(p); return;
    }
    if (id == Id::Moon)
    {
        g.fillEllipse(r);
        g.setColour(juce::Colours::black.withAlpha(0.72f));
        g.fillEllipse(r.translated(s*.22f, -s*.10f));
        return;
    }
    if (id == Id::Up)
    {
        g.drawRoundedRectangle(r.reduced(s*.16f), s*.12f, 1.4f);
        g.fillRoundedRectangle(r.withSizeKeepingCentre(s*.22f,s*.46f), s*.08f);
        return;
    }
    if (id == Id::Eyes)
    {
        for (int i=0;i<2;++i)
        {
            auto q=r.withSizeKeepingCentre(s*.38f,s*.52f).translated((i==0?-1:1)*s*.18f,0);
            g.drawEllipse(q,1.3f);
            g.fillEllipse(q.withSizeKeepingCentre(s*.10f,s*.16f));
        }
        return;
    }
    if (id == Id::Skull)
    {
        g.fillEllipse(r.reduced(s*.12f,s*.16f));
        g.setColour(juce::Colours::black);
        g.fillEllipse(juce::Rectangle<float>(cx-s*.16f,cy-s*.04f,s*.10f,s*.13f));
        g.fillEllipse(juce::Rectangle<float>(cx+s*.06f,cy-s*.04f,s*.10f,s*.13f));
        g.fillRect(cx-s*.12f,cy+s*.14f,s*.24f,s*.08f); return;
    }
    if (id == Id::Hundred)
    {
        g.setColour(accent); g.drawRoundedRectangle(r.reduced(s*.08f),s*.12f,2.f);
        g.setFont(juce::Font(s*.42f,juce::Font::bold)); g.drawText("100",r,juce::Justification::centred); return;
    }
    if (id == Id::Laugh)
    {
        g.drawEllipse(r.reduced(s*.08f),1.5f);
        g.fillEllipse(cx-s*.18f,cy-s*.08f,s*.10f,s*.12f);
        g.fillEllipse(cx+s*.08f,cy-s*.08f,s*.10f,s*.12f);
        g.drawArc(cx-s*.22f,cy-s*.02f,s*.44f,s*.30f,0.2f,2.9f,1.5f); return;
    }
    g.setColour(accent);
    g.drawEllipse(r.reduced(s*.10f),1.4f);
    g.fillEllipse(r.withSizeKeepingCentre(s*.12f,s*.12f));
}

struct Layout
{
    juce::String text;
    float width = 0.f, height = 0.f, lineHeight = 16.f;
};

inline juce::String cleanShortcodes(const juce::String& input)
{
    auto s = input;
    for (const auto& p : kPicker)
        s = s.replace(":" + juce::String(p.name) + ":", "  ");
    return s;
}

inline Layout layoutText(const juce::String& input, juce::Font font, float maxWidth)
{
    Layout l;
    l.text = input;
    l.lineHeight = juce::jmax(14.f, font.getHeight() * 1.15f);
    juce::AttributedString as;
    as.append(cleanShortcodes(input), font);
    as.setWordWrap(juce::AttributedString::byWord);
    juce::TextLayout tl;
    tl.createLayout(as, juce::jmax(40.f, maxWidth));
    l.width = maxWidth;
    l.height = juce::jmax(l.lineHeight, tl.getHeight());
    return l;
}

inline void paintLayout(juce::Graphics& g, const Layout& l, juce::Point<float> origin,
                        juce::Colour textColour, juce::Font font)
{
    juce::AttributedString as;
    as.append(cleanShortcodes(l.text), font, textColour);
    as.setWordWrap(juce::AttributedString::byWord);
    juce::TextLayout tl;
    tl.createLayout(as, juce::jmax(40.f, l.width));
    tl.draw(g, juce::Rectangle<float>(origin.x, origin.y, l.width, l.height + font.getHeight()));

    // Draw shortcode icons over the blank placeholders. They intentionally use a fixed
    // compact cell so they render even when the host font has no emoji glyphs.
    int searchFrom = 0;
    int cell = 0;
    while (searchFrom < l.text.length())
    {
        int best = -1; Id id = Id::None; int bestLen = 0;
        for (const auto& p : kPicker)
        {
            const auto token = ":" + juce::String(p.name) + ":";
            const int at = l.text.indexOf(searchFrom, token);
            if (at >= 0 && (best < 0 || at < best))
            { best = at; id = p.id; bestLen = token.length(); }
        }
        if (best < 0) break;

        // Approximate horizontal placement. The blank placeholder is intentionally two spaces.
        const auto prefix = cleanShortcodes(l.text.substring(0, best));
        float x = origin.x + std::fmod(font.getStringWidthFloat(prefix),
                                         juce::jmax(1.f, l.width - font.getHeight()));
        float y = origin.y + cell * l.lineHeight;
        auto icon = juce::Rectangle<float>(x, y, font.getHeight(), font.getHeight()).reduced(1.f);
        draw(g, id == Id::None ? Id::Generic : id, icon);
        searchFrom = best + bestLen;
        ++cell;
    }
}
} // namespace kt::emoji
