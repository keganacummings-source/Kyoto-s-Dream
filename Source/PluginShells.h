#pragma once
#include <JuceHeader.h>
#include "Themes.h"
#include "HardwareInternals.h"
#include <vector>

// Hardware shells and the builder face.
// The chain starts at the display, not at a motherboard effect.
// Wires run from the part you click to the part you place next.
// Lego templates (TemplateModules.h) can replace the fixed shell bays.
namespace pb
{
enum class SlotKind { Board, Knob, Fader, Key, Screen, Cosmetic, Fx };

struct Slot
{
    SlotKind kind;
    float x, y, w, h;
    const char* name;
};

struct Shell
{
    const char* id;
    const char* name;
    int hiddenFx;
    float hiddenMix;
    const char* silhouette;
    Slot slots[10];
    int slotCount;
};

inline const Shell kShells[] = {
    { "console", "Console Deck", 21, 0.10f, "console", {
        { SlotKind::Board, 0.03f, 0.58f, 0.30f, 0.34f, "DISPLAY" },
        { SlotKind::Screen, 0.04f, 0.08f, 0.46f, 0.34f, "SCREEN" },
        { SlotKind::Knob, 0.54f, 0.08f, 0.14f, 0.28f, "KNOB A" },
        { SlotKind::Knob, 0.70f, 0.08f, 0.14f, 0.28f, "KNOB B" },
        { SlotKind::Knob, 0.84f, 0.08f, 0.13f, 0.28f, "KNOB C" },
        { SlotKind::Fader, 0.54f, 0.42f, 0.12f, 0.50f, "FADER A" },
        { SlotKind::Fader, 0.68f, 0.42f, 0.12f, 0.50f, "FADER B" },
        { SlotKind::Key, 0.82f, 0.46f, 0.15f, 0.18f, "KEY" },
        { SlotKind::Cosmetic, 0.36f, 0.62f, 0.14f, 0.28f, "VENT" },
        { SlotKind::Cosmetic, 0.82f, 0.70f, 0.15f, 0.22f, "BADGE" }
    }, 10 },
    { "tower", "Tower Rack", 48, 0.09f, "tower", {
        { SlotKind::Board, 0.08f, 0.08f, 0.84f, 0.16f, "DISPLAY" },
        { SlotKind::Screen, 0.10f, 0.28f, 0.80f, 0.16f, "SCREEN" },
        { SlotKind::Knob, 0.10f, 0.48f, 0.24f, 0.20f, "KNOB A" },
        { SlotKind::Knob, 0.38f, 0.48f, 0.24f, 0.20f, "KNOB B" },
        { SlotKind::Knob, 0.66f, 0.48f, 0.24f, 0.20f, "KNOB C" },
        { SlotKind::Fader, 0.10f, 0.72f, 0.16f, 0.24f, "FADER A" },
        { SlotKind::Fader, 0.30f, 0.72f, 0.16f, 0.24f, "FADER B" },
        { SlotKind::Key, 0.52f, 0.74f, 0.18f, 0.18f, "KEY" },
        { SlotKind::Cosmetic, 0.74f, 0.74f, 0.16f, 0.18f, "RAIL" },
        { SlotKind::Cosmetic, 0.08f, 0.28f, 0.0f, 0.0f, "UNUSED" }
    }, 9 },
    { "desk", "Desk Wing", 53, 0.08f, "desk", {
        { SlotKind::Board, 0.38f, 0.38f, 0.24f, 0.28f, "DISPLAY" },
        { SlotKind::Screen, 0.34f, 0.06f, 0.32f, 0.26f, "SCREEN" },
        { SlotKind::Knob, 0.06f, 0.10f, 0.12f, 0.24f, "KNOB A" },
        { SlotKind::Knob, 0.20f, 0.10f, 0.12f, 0.24f, "KNOB B" },
        { SlotKind::Knob, 0.70f, 0.10f, 0.12f, 0.24f, "KNOB C" },
        { SlotKind::Fader, 0.06f, 0.46f, 0.12f, 0.46f, "FADER A" },
        { SlotKind::Fader, 0.82f, 0.46f, 0.12f, 0.46f, "FADER B" },
        { SlotKind::Key, 0.70f, 0.42f, 0.12f, 0.18f, "KEY" },
        { SlotKind::Cosmetic, 0.20f, 0.62f, 0.14f, 0.26f, "VENT" },
        { SlotKind::Cosmetic, 0.68f, 0.68f, 0.12f, 0.22f, "BADGE" }
    }, 10 },
    { "pocket", "Pocket Unit", 15, 0.12f, "pocket", {
        { SlotKind::Board, 0.08f, 0.62f, 0.84f, 0.28f, "DISPLAY" },
        { SlotKind::Screen, 0.12f, 0.08f, 0.76f, 0.24f, "SCREEN" },
        { SlotKind::Knob, 0.10f, 0.38f, 0.22f, 0.20f, "KNOB A" },
        { SlotKind::Knob, 0.39f, 0.38f, 0.22f, 0.20f, "KNOB B" },
        { SlotKind::Fader, 0.68f, 0.36f, 0.20f, 0.22f, "FADER" },
        { SlotKind::Key, 0.68f, 0.36f, 0.0f, 0.0f, "UNUSED" },
        { SlotKind::Cosmetic, 0.10f, 0.38f, 0.0f, 0.0f, "UNUSED" },
        { SlotKind::Cosmetic, 0.39f, 0.62f, 0.0f, 0.0f, "UNUSED" },
        { SlotKind::Key, 0.68f, 0.62f, 0.20f, 0.16f, "KEY" },
        { SlotKind::Cosmetic, 0.10f, 0.62f, 0.22f, 0.0f, "UNUSED" }
    }, 5 }
};

inline constexpr int kShellCount = 4;

inline juce::String styleToken(int kindId)
{
    switch (kindId)
    {
        case 1: return "dial";
        case 2: return "pointer";
        case 3: return "fader";
        case 4: return "hfader";
        case 5: return "key";
        case 6: return "screen";
        case 7: return "vent";
        case 8: return "badge";
        case 9: return "rail";
        case 10: return "effect";
        default: return "dial";
    }
}

inline SlotKind styleSlot(const juce::String& style)
{
    if (style == "fader" || style == "hfader" || style == "slider") return SlotKind::Fader;
    if (style == "key") return SlotKind::Key;
    if (style == "screen" || style == "wave" || style == "display") return SlotKind::Screen;
    if (style == "vent" || style == "badge" || style == "rail") return SlotKind::Cosmetic;
    if (style == "board") return SlotKind::Board;
    if (style == "effect" || style == "fx") return SlotKind::Fx;
    return SlotKind::Knob;
}

inline bool styleFits(const juce::String& style, SlotKind slot)
{
    const auto want = styleSlot(style);
    if (want == slot) return true;
    // Effects can sit in an effect bay or any control bay so a small template still chains.
    if (want == SlotKind::Fx && (slot == SlotKind::Fx || slot == SlotKind::Knob || slot == SlotKind::Fader)) return true;
    // The display takes the old motherboard bay as well as a screen bay.
    if (want == SlotKind::Screen && slot == SlotKind::Board) return true;
    return false;
}

inline int cosmeticHiddenFx(const juce::String& style)
{
    if (style == "vent") return 32;
    if (style == "badge") return 14;
    if (style == "rail") return 4;
    return -1;
}

inline juce::Rectangle<float> faceRect(juce::Rectangle<float> panel)
{
    return panel.reduced(16.f, 50.f);
}

inline juce::Rectangle<float> slotRect(juce::Rectangle<float> face, const Slot& slot)
{
    return { face.getX() + slot.x * face.getWidth(), face.getY() + slot.y * face.getHeight(),
             slot.w * face.getWidth(), slot.h * face.getHeight() };
}

inline float shellRadius(const Shell& shell)
{
    const auto s = juce::String(shell.silhouette);
    return s == "pocket" ? 28.f : s == "tower" ? 8.f : 16.f;
}

class BuilderCanvas : public juce::Component
{
public:
    struct BrickGlyph
    {
        hb::HardwarePart part;
        float x = 0.f, y = 0.f, w = 0.f, h = 0.f;
        bool selected = false;
        int index = 0;
    };

    int shellIndex = 0;
    bool placing = false;
    bool templateMode = false;
    juce::String armedStyle;
    juce::String subtitle { "Chain starts at the display" };
    kt::ThemePalette theme = kt::kThemes[0];
    std::vector<Slot> bays;
    std::vector<BrickGlyph> glyphs;
    std::function<void(int)> onSlot;
    std::function<void(int)> onBrick;
    std::function<void(int)> onPickLink;
    std::function<bool(int)> occupied;
    std::function<juce::Point<float>(int)> anchor;
    std::function<int(int)> parentOf;
    int hoverSlot = -1;
    int linkSlot = -1;
    float phase = 0.f;

    int bayCount() const { return (int) bays.size(); }

    const Slot* bayPtr(int index) const
    {
        if (index < 0 || index >= bayCount()) return nullptr;
        return &bays[(size_t) index];
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        const auto& shell = kShells[juce::jlimit(0, kShellCount - 1, shellIndex)];
        const auto accent = kt::c(theme.accent);

        g.setColour(kt::c(theme.panel).withAlpha(0.96f));
        g.fillRoundedRectangle(bounds, 14.f);
        g.setColour(kt::c(theme.border));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 14.f, 1.f);

        g.setColour(accent);
        g.setFont(kt::font(theme, 15.f, true));
        g.drawText(templateMode ? "TEMPLATE" : "PLUGIN BUILDER", 18, 12, 260, 18, juce::Justification::left);
        g.setColour(kt::c(theme.muted));
        g.setFont(kt::font(theme, 11.5f));
        g.drawText(subtitle, 18, 31, getWidth() - 36, 15, juce::Justification::left);

        auto face = faceRect(bounds);
        const float radius = shellRadius(shell);
        g.setColour(kt::c(theme.bg).withAlpha(0.88f));
        g.fillRoundedRectangle(face, radius);
        g.setColour(accent.withAlpha(0.45f));
        g.drawRoundedRectangle(face, radius, 1.4f);

        g.setColour(kt::c(theme.muted).withAlpha(0.65f));
        for (auto p : { juce::Point<float>(face.getX() + 10.f, face.getY() + 10.f),
                        juce::Point<float>(face.getRight() - 10.f, face.getY() + 10.f),
                        juce::Point<float>(face.getX() + 10.f, face.getBottom() - 10.f),
                        juce::Point<float>(face.getRight() - 10.f, face.getBottom() - 10.f) })
            g.fillEllipse(p.x - 2.5f, p.y - 2.5f, 5.f, 5.f);

        for (const auto& glyph : glyphs)
        {
            auto r = juce::Rectangle<float>(face.getX() + glyph.x * face.getWidth(),
                                            face.getY() + glyph.y * face.getHeight(),
                                            glyph.w * face.getWidth(), glyph.h * face.getHeight());
            hb::drawHardwarePart(g, theme, glyph.part, r, phase, glyph.selected);
            if (glyph.selected)
            {
                g.setColour(accent.withAlpha(0.9f));
                g.drawRoundedRectangle(r.reduced(1.f), 6.f, 2.f);
            }
        }

        for (int i = 0; i < bayCount(); ++i)
        {
            const auto* slot = bayPtr(i);
            if (slot == nullptr || slot->w < 0.02f || slot->h < 0.02f) continue;
            if (!(occupied && occupied(i) && anchor)) continue;
            const int parent = parentOf ? parentOf(i) : -1;
            if (parent < 0) continue;
            auto a = anchor(parent);
            auto b = anchor(i);
            if (a.x <= 1.f || b.x <= 1.f) continue;
            juce::Path wire;
            wire.startNewSubPath(a);
            wire.cubicTo(a.x, (a.y + b.y) * 0.5f, b.x, (a.y + b.y) * 0.5f, b.x, b.y);
            g.setColour(accent.withAlpha(0.9f));
            g.strokePath(wire, juce::PathStrokeType(2.2f));
            g.setColour(kt::c(theme.pegHot));
            g.fillEllipse(a.x - 3.f, a.y - 3.f, 6.f, 6.f);
            g.fillEllipse(b.x - 3.f, b.y - 3.f, 6.f, 6.f);
        }

        for (int i = 0; i < bayCount(); ++i)
        {
            const auto* slot = bayPtr(i);
            if (slot == nullptr || slot->w < 0.02f || slot->h < 0.02f) continue;
            auto r = slotRect(face, *slot).reduced(3.f);
            const bool taken = occupied && occupied(i);
            const bool fits = placing && ! taken && styleFits(armedStyle, slot->kind);
            const bool hovered = placing && i == hoverSlot && fits;
            const bool linked = i == linkSlot && taken;

            g.setColour(fits ? accent.withAlpha(hovered ? 0.38f : 0.26f)
                             : linked ? accent.withAlpha(0.22f)
                                      : kt::c(theme.panel).withAlpha(taken ? 0.05f : 0.42f));
            g.fillRoundedRectangle(r, 8.f);
            g.setColour(fits || linked ? accent : kt::c(theme.border).withAlpha(taken ? 0.25f : 0.7f));
            g.drawRoundedRectangle(r, 8.f, fits || linked ? (hovered || linked ? 2.4f : 1.8f) : 1.f);

            const float pegR = juce::jmin(3.4f, r.getWidth() * 0.12f);
            const float inset = juce::jmax(7.f, juce::jmin(r.getWidth(), r.getHeight()) * 0.14f);
            for (auto c : { juce::Point<float>(r.getX() + inset, r.getY() + inset),
                            juce::Point<float>(r.getRight() - inset, r.getY() + inset),
                            juce::Point<float>(r.getX() + inset, r.getBottom() - inset),
                            juce::Point<float>(r.getRight() - inset, r.getBottom() - inset) })
            {
                if (taken)
                {
                    g.setColour(accent.withAlpha(0.35f));
                    g.fillEllipse(c.x - pegR * 0.7f, c.y - pegR * 0.7f, pegR * 1.4f, pegR * 1.4f);
                }
                else
                {
                    g.setColour(fits ? kt::c(theme.pegHot) : kt::c(theme.peg).withAlpha(0.85f));
                    g.fillEllipse(c.x - pegR, c.y - pegR, pegR * 2.f, pegR * 2.f);
                }
            }

            if (! taken)
            {
                g.setColour(fits ? kt::c(theme.text) : kt::c(theme.muted));
                g.setFont(kt::font(theme, 10.5f, true));
                const juce::String name = slot->name != nullptr ? juce::String(slot->name) : juce::String();
                g.drawFittedText(name, r.reduced(6.f).toNearestInt(), juce::Justification::centred, 2);
            }
        }
    }

    int brickAt(juce::Point<float> pos) const
    {
        auto face = faceRect(getLocalBounds().toFloat());
        for (int i = (int) glyphs.size() - 1; i >= 0; --i)
        {
            const auto& glyph = glyphs[(size_t) i];
            auto r = juce::Rectangle<float>(face.getX() + glyph.x * face.getWidth(),
                                            face.getY() + glyph.y * face.getHeight(),
                                            glyph.w * face.getWidth(), glyph.h * face.getHeight());
            if (r.contains(pos)) return glyph.index;
        }
        return -1;
    }

    int slotAt(juce::Point<float> pos, bool emptyOnly) const
    {
        auto face = faceRect(getLocalBounds().toFloat());
        for (int i = 0; i < bayCount(); ++i)
        {
            const auto* slot = bayPtr(i);
            if (slot == nullptr || slot->w < 0.02f) continue;
            const bool taken = occupied && occupied(i);
            if (emptyOnly)
            {
                if (! placing || taken) continue;
                if (! styleFits(armedStyle, slot->kind)) continue;
            }
            else if (! taken) continue;
            if (slotRect(face, *slot).contains(pos)) return i;
        }
        return -1;
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        const int hit = placing ? slotAt(e.position, true) : -1;
        if (hit != hoverSlot) { hoverSlot = hit; repaint(); }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (hoverSlot != -1) { hoverSlot = -1; repaint(); }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (templateMode)
        {
            const int brick = brickAt(e.position);
            if (brick >= 0 && onBrick) onBrick(brick);
            return;
        }
        const int occupiedHit = slotAt(e.position, false);
        if (occupiedHit >= 0 && onPickLink)
        {
            onPickLink(occupiedHit);
            return;
        }
        if (! placing || ! onSlot) return;
        const int empty = slotAt(e.position, true);
        if (empty >= 0) onSlot(empty);
    }
};
}
