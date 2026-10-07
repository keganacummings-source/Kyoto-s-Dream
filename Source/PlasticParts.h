#pragma once
#include "Themes.h"
#include "ModularParts.h"
#include <cmath>

// Cartoonish plastic part graphics. Cheap to draw (fills + a few ellipses/gradients).
// Size tiers adapt palette emphasis so small bays stay readable and large ones get more detail.
namespace kt {
namespace plastic {

enum class SizeTier { Tiny, Small, Medium, Large };

inline SizeTier tierOf(float w, float h)
{
    const float m = juce::jmin(w, h);
    if (m < 36.f) return SizeTier::Tiny;
    if (m < 56.f) return SizeTier::Small;
    if (m < 88.f) return SizeTier::Medium;
    return SizeTier::Large;
}

struct Cols
{
    juce::Colour shell, shellHi, shellLo, accent, accentSoft, peg, ink, rim;
};

inline Cols colsFor(const ThemePalette& t, const juce::String& skinId, SizeTier tier)
{
    Cols c;
    c.shell = kt::c(t.panel);
    c.shellHi = c.shell.brighter(0.22f);
    c.shellLo = c.shell.darker(0.18f);
    c.accent = kt::c(t.accent);
    c.accentSoft = c.accent.withAlpha(0.55f);
    c.peg = kt::c(t.peg);
    c.ink = kt::c(t.text);
    c.rim = kt::c(t.border);

    if (auto* skin = partSkinById(skinId))
    {
        if (skin->accentHex != nullptr) c.accent = juce::Colour::fromString(juce::String(skin->accentHex));
        if (skin->panelHex != nullptr) c.shell = juce::Colour::fromString(juce::String(skin->panelHex));
        c.shellHi = c.shell.brighter(0.25f);
        c.shellLo = c.shell.darker(0.2f);
        c.accentSoft = c.accent.withAlpha(0.6f);
    }

    // Resize palette shift: larger parts lean into accent plastic; tiny stay muted.
    switch (tier)
    {
        case SizeTier::Tiny:
            c.shell = c.shell.interpolatedWith(c.peg, 0.15f);
            c.accent = c.accent.withMultipliedSaturation(0.85f);
            break;
        case SizeTier::Small:
            break;
        case SizeTier::Medium:
            c.shell = c.shell.interpolatedWith(c.accent, 0.08f);
            break;
        case SizeTier::Large:
            c.shell = c.shell.interpolatedWith(c.accent, 0.14f);
            c.shellHi = c.shellHi.interpolatedWith(juce::Colours::white, 0.12f);
            c.accent = c.accent.brighter(0.06f);
            break;
    }
    return c;
}

// Soft plastic body: rounded rect + top highlight + bottom shade (no blur, cheap).
inline void fillPlasticBody(juce::Graphics& g, juce::Rectangle<float> r, float rad, const Cols& c, bool selected)
{
    g.setColour(c.shellLo);
    g.fillRoundedRectangle(r.translated(0.f, 1.2f), rad);
    g.setColour(c.shell);
    g.fillRoundedRectangle(r, rad);
    // Top gloss strip (non-destructive)
    auto gloss = r.withHeight(juce::jmax(3.f, r.getHeight() * 0.28f)).reduced(1.5f, 0.f);
    g.setColour(c.shellHi.withAlpha(0.55f));
    g.fillRoundedRectangle(gloss, rad * 0.9f);
    g.setColour(selected ? c.accent.withAlpha(0.95f) : c.rim.withAlpha(0.75f));
    g.drawRoundedRectangle(r, rad, selected ? 2.f : 1.1f);
}

inline void drawDial(juce::Graphics& g, juce::Rectangle<float> bounds, const ThemePalette& t,
                     const juce::String& skinId, float value01, bool selected, int styleVariant)
{
    const auto tier = tierOf(bounds.getWidth(), bounds.getHeight());
    const auto c = colsFor(t, skinId, tier);
    const float rad = juce::jlimit(6.f, 16.f, juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.18f);
    fillPlasticBody(g, bounds, rad, c, selected);

    auto face = bounds.reduced(bounds.getWidth() * 0.14f, bounds.getHeight() * 0.18f);
    const float d = juce::jmin(face.getWidth(), face.getHeight());
    auto knob = juce::Rectangle<float>(face.getCentreX() - d * 0.5f, face.getCentreY() - d * 0.5f, d, d);

    // Plastic bezel ring
    g.setColour(c.peg.darker(0.1f));
    g.fillEllipse(knob.expanded(2.5f));
    g.setColour(c.shellHi);
    g.fillEllipse(knob);
    g.setColour(c.shellLo);
    g.drawEllipse(knob, 1.2f);

    // Style variants (unique silhouettes, still cheap)
    const float ang = -2.4f + 4.8f * juce::jlimit(0.f, 1.f, value01);
    const float cx = knob.getCentreX(), cy = knob.getCentreY();
    switch (styleVariant % 5)
    {
        case 0: // pointer knob
        {
            juce::Path p;
            p.addTriangle(cx + std::cos(ang) * d * 0.38f, cy + std::sin(ang) * d * 0.38f,
                          cx + std::cos(ang + 2.5f) * d * 0.12f, cy + std::sin(ang + 2.5f) * d * 0.12f,
                          cx + std::cos(ang - 2.5f) * d * 0.12f, cy + std::sin(ang - 2.5f) * d * 0.12f);
            g.setColour(c.accent);
            g.fillPath(p);
            break;
        }
        case 1: // arc window
        {
            juce::Path arc;
            arc.addCentredArc(cx, cy, d * 0.32f, d * 0.32f, 0.f, -2.4f, ang, true);
            g.setColour(c.accent.withAlpha(0.85f));
            g.strokePath(arc, juce::PathStrokeType(juce::jmax(2.f, d * 0.08f)));
            g.setColour(c.accent);
            g.fillEllipse(cx - d * 0.07f, cy - d * 0.07f, d * 0.14f, d * 0.14f);
            break;
        }
        case 2: // dual ring
            g.setColour(c.accent.withAlpha(0.35f));
            g.drawEllipse(knob.reduced(d * 0.12f), 2.f);
            g.setColour(c.accent);
            g.fillEllipse(cx + std::cos(ang) * d * 0.28f - 3.f, cy + std::sin(ang) * d * 0.28f - 3.f, 6.f, 6.f);
            break;
        case 3: // flat top plastic
            g.setColour(c.shellLo.withAlpha(0.5f));
            g.fillEllipse(knob.reduced(d * 0.18f));
            g.setColour(c.accent);
            g.drawLine(cx, cy, cx + std::cos(ang) * d * 0.34f, cy + std::sin(ang) * d * 0.34f, 2.4f);
            break;
        default: // gem cap
            g.setColour(c.accent.withAlpha(0.4f));
            g.fillEllipse(knob.reduced(d * 0.22f));
            g.setColour(c.accent);
            g.fillEllipse(cx - d * 0.08f, cy - d * 0.08f, d * 0.16f, d * 0.16f);
            break;
    }

    if (tier >= SizeTier::Medium)
    {
        // Specular dot
        g.setColour(juce::Colours::white.withAlpha(0.35f));
        g.fillEllipse(knob.getX() + d * 0.2f, knob.getY() + d * 0.18f, d * 0.16f, d * 0.1f);
    }
}

inline void drawFader(juce::Graphics& g, juce::Rectangle<float> bounds, const ThemePalette& t,
                      const juce::String& skinId, float value01, bool selected, bool horizontal)
{
    const auto tier = tierOf(bounds.getWidth(), bounds.getHeight());
    const auto c = colsFor(t, skinId, tier);
    const float rad = 8.f;
    fillPlasticBody(g, bounds, rad, c, selected);

    auto track = bounds.reduced(bounds.getWidth() * 0.28f, bounds.getHeight() * 0.14f);
    if (horizontal) track = bounds.reduced(bounds.getWidth() * 0.12f, bounds.getHeight() * 0.32f);

    g.setColour(c.peg.darker(0.15f));
    g.fillRoundedRectangle(track, 4.f);
    g.setColour(c.accent.withAlpha(0.75f));
    auto fill = track;
    if (horizontal)
        fill.setWidth(track.getWidth() * juce::jlimit(0.f, 1.f, value01));
    else
    {
        const float h = track.getHeight() * juce::jlimit(0.f, 1.f, value01);
        fill = { track.getX(), track.getBottom() - h, track.getWidth(), h };
    }
    g.fillRoundedRectangle(fill, 4.f);

    // Cap
    juce::Rectangle<float> cap;
    if (horizontal)
        cap = { track.getX() + track.getWidth() * value01 - 7.f, track.getCentreY() - 10.f, 14.f, 20.f };
    else
        cap = { track.getCentreX() - 10.f, track.getBottom() - track.getHeight() * value01 - 7.f, 20.f, 14.f };
    g.setColour(c.shellHi);
    g.fillRoundedRectangle(cap, 3.f);
    g.setColour(c.accent);
    g.drawRoundedRectangle(cap, 3.f, 1.2f);
    if (tier >= SizeTier::Medium)
    {
        g.setColour(c.rim.withAlpha(0.5f));
        if (horizontal) g.drawLine(cap.getX() + 4.f, cap.getCentreY(), cap.getRight() - 4.f, cap.getCentreY(), 1.f);
        else g.drawLine(cap.getCentreX(), cap.getY() + 4.f, cap.getCentreX(), cap.getBottom() - 4.f, 1.f);
    }
}

inline void drawKey(juce::Graphics& g, juce::Rectangle<float> bounds, const ThemePalette& t,
                    const juce::String& skinId, bool down, bool selected, const juce::String& label)
{
    const auto tier = tierOf(bounds.getWidth(), bounds.getHeight());
    const auto c = colsFor(t, skinId, tier);
    const float rad = juce::jlimit(6.f, 14.f, juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.2f);
    auto r = bounds;
    if (down) r.translate(0.f, 1.5f);
    fillPlasticBody(g, r, rad, c, selected);
    auto pad = r.reduced(6.f);
    g.setColour(down ? c.accent : c.shellHi.interpolatedWith(c.accent, 0.15f));
    g.fillRoundedRectangle(pad, rad - 2.f);
    g.setColour(c.accent.withAlpha(0.5f));
    g.drawRoundedRectangle(pad, rad - 2.f, 1.2f);
    g.setColour(c.ink);
    g.setFont(kt::font(t, juce::jlimit(9.f, 14.f, pad.getHeight() * 0.28f), true));
    g.drawFittedText(label, pad.toNearestInt(), juce::Justification::centred, 2);
}

inline void drawButton(juce::Graphics& g, juce::Rectangle<float> bounds, const ThemePalette& t,
                       const juce::String& skinId, bool on, bool selected, const juce::String& label)
{
    const auto tier = tierOf(bounds.getWidth(), bounds.getHeight());
    const auto c = colsFor(t, skinId, tier);
    fillPlasticBody(g, bounds, 10.f, c, selected);
    auto led = juce::Rectangle<float>(bounds.getRight() - 16.f, bounds.getY() + 8.f, 8.f, 8.f);
    g.setColour(on ? c.accent : c.peg);
    g.fillEllipse(led);
    if (on && tier >= SizeTier::Small)
    {
        g.setColour(c.accent.withAlpha(0.35f));
        g.drawEllipse(led.expanded(3.f), 1.5f);
    }
    g.setColour(c.ink);
    g.setFont(kt::font(t, 11.f, true));
    g.drawFittedText(label, bounds.reduced(8.f, 6.f).toNearestInt(), juce::Justification::centredLeft, 2);
}

inline void drawCosmetic(juce::Graphics& g, juce::Rectangle<float> bounds, const ThemePalette& t,
                         const juce::String& skinId, const juce::String& style, bool selected, const juce::String& label)
{
    const auto tier = tierOf(bounds.getWidth(), bounds.getHeight());
    const auto c = colsFor(t, skinId, tier);
    fillPlasticBody(g, bounds, 8.f, c, selected);
    g.setColour(c.accent.withAlpha(0.55f));
    if (style == "vent")
    {
        const int lines = tier >= SizeTier::Large ? 6 : (tier >= SizeTier::Medium ? 5 : 3);
        for (int i = 0; i < lines; ++i)
        {
            const float y = bounds.getY() + 10.f + i * (bounds.getHeight() - 24.f) / juce::jmax(1, lines - 1);
            g.drawLine(bounds.getX() + 10.f, y, bounds.getRight() - 10.f, y, 2.f);
        }
    }
    else if (style == "rail")
    {
        auto rail = bounds.reduced(bounds.getWidth() * 0.38f, 8.f);
        g.setColour(c.shellLo);
        g.fillRoundedRectangle(rail, 4.f);
        g.setColour(c.accent.withAlpha(0.7f));
        g.fillRoundedRectangle(rail.reduced(3.f), 3.f);
    }
    else if (style == "badge")
    {
        g.setColour(c.accent.withAlpha(0.25f));
        g.fillRoundedRectangle(bounds.reduced(8.f), 6.f);
        g.setColour(c.accent);
        g.drawRoundedRectangle(bounds.reduced(8.f), 6.f, 1.5f);
    }
    else
    {
        g.fillEllipse(bounds.reduced(juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.22f));
    }
    g.setColour(c.ink.withAlpha(0.9f));
    g.setFont(kt::font(t, 10.f, true));
    g.drawText(label, bounds.removeFromBottom(15.f).toNearestInt(), juce::Justification::centred);
}

inline void drawWaveFrame(juce::Graphics& g, juce::Rectangle<float> bounds, const ThemePalette& t,
                          const juce::String& skinId, bool selected, bool stack)
{
    const auto tier = tierOf(bounds.getWidth(), bounds.getHeight());
    const auto c = colsFor(t, skinId, tier);
    fillPlasticBody(g, bounds, 10.f, c, selected);
    auto screen = bounds.reduced(7.f);
    g.setColour(kt::c(t.bg).darker(0.05f));
    g.fillRoundedRectangle(screen, 6.f);
    g.setColour(c.accent.withAlpha(0.35f));
    g.drawRoundedRectangle(screen, 6.f, 1.2f);
    g.setColour(c.accent);
    g.setFont(kt::font(t, 10.f, true));
    g.drawText(stack ? "STACKED FX" : "WAV", screen.removeFromBottom(16.f).toNearestInt(), juce::Justification::centred);
}

// Stable style variant from node id/label so each part looks unique without storing extra state.
inline int styleVariantFor(const juce::ValueTree& node)
{
    const auto s = node.getProperty("label").toString() + node.getProperty("skin").toString()
                 + node.getProperty("slot").toString() + node.getProperty("shellSlot").toString();
    int h = 0;
    for (auto ch : s)
        h = 31 * h + (int) ch;
    return std::abs(h);
}

} // namespace plastic
} // namespace kt
