#pragma once
#include <JuceHeader.h>
#include "Themes.h"

// Hardware shells for the Plugin Builder. Slots are normalised inside the face plate.
// The motherboard bay is always occupied first and is the start of the signal chain.
namespace pb
{
enum class SlotKind { Board, Knob, Fader, Key, Screen, Cosmetic };

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
        { SlotKind::Board, 0.03f, 0.58f, 0.30f, 0.34f, "MOTHERBOARD" },
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
        { SlotKind::Board, 0.08f, 0.08f, 0.84f, 0.16f, "MOTHERBOARD" },
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
        { SlotKind::Board, 0.38f, 0.38f, 0.24f, 0.28f, "MOTHERBOARD" },
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
        { SlotKind::Board, 0.08f, 0.62f, 0.84f, 0.28f, "MOTHERBOARD" },
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
        default: return "dial";
    }
}

inline SlotKind styleSlot(const juce::String& style)
{
    if (style == "fader" || style == "hfader" || style == "slider") return SlotKind::Fader;
    if (style == "key") return SlotKind::Key;
    if (style == "screen" || style == "wave") return SlotKind::Screen;
    if (style == "vent" || style == "badge" || style == "rail") return SlotKind::Cosmetic;
    if (style == "board") return SlotKind::Board;
    return SlotKind::Knob;
}

inline bool styleFits(const juce::String& style, SlotKind slot)
{
    return styleSlot(style) == slot;
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

class BuilderCanvas : public juce::Component
{
public:
    int shellIndex = 0;
    bool placing = false;
    juce::String armedStyle;
    kt::ThemePalette theme = kt::kThemes[0];
    std::function<void(int)> onSlot;
    std::function<bool(int)> occupied;
    std::function<juce::Point<float>(int)> anchor;

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(kt::c(theme.panel).withAlpha(0.96f));
        g.fillRoundedRectangle(bounds, 14.f);
        g.setColour(kt::c(theme.border));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 14.f, 1.f);

        const auto& shell = kShells[juce::jlimit(0, kShellCount - 1, shellIndex)];
        g.setColour(kt::c(theme.accent));
        g.setFont(kt::font(theme, 13.f, true));
        g.drawText("PLUGIN BUILDER", 16, 10, 180, 18, juce::Justification::left);
        g.setColour(kt::c(theme.muted));
        g.setFont(kt::font(theme, 10.f));
        g.drawText(juce::String(shell.name) + "  ·  chain starts at the motherboard", 16, 28, getWidth() - 32, 14, juce::Justification::left);

        auto face = faceRect(bounds);
        g.setColour(kt::c(theme.bg).withAlpha(0.88f));
        const float radius = juce::String(shell.silhouette) == "pocket" ? 28.f : juce::String(shell.silhouette) == "tower" ? 8.f : 16.f;
        g.fillRoundedRectangle(face, radius);
        g.setColour(kt::c(theme.accent).withAlpha(0.45f));
        g.drawRoundedRectangle(face, radius, 1.4f);

        for (int i = 1; i < shell.slotCount; ++i)
        {
            if (occupied && occupied(i))
            {
                const int from = 0;
                if (anchor)
                {
                    auto a = anchor(from);
                    auto b = anchor(i);
                    if (a.x > 1.f && b.x > 1.f)
                    {
                        juce::Path wire;
                        wire.startNewSubPath(a);
                        wire.cubicTo(a.x, (a.y + b.y) * 0.5f, b.x, (a.y + b.y) * 0.5f, b.x, b.y);
                        g.setColour(kt::c(theme.accent).withAlpha(0.85f));
                        g.strokePath(wire, juce::PathStrokeType(2.0f));
                        g.setColour(kt::c(theme.pegHot));
                        g.fillEllipse(b.x - 3.f, b.y - 3.f, 6.f, 6.f);
                    }
                }
            }
        }

        for (int i = 0; i < shell.slotCount; ++i)
        {
            const auto& slot = shell.slots[i];
            if (slot.w < 0.02f || slot.h < 0.02f) continue;
            auto r = slotRect(face, slot);
            const bool taken = occupied && occupied(i);
            const bool viable = placing && !taken && styleFits(armedStyle, slot.kind);
            g.setColour(viable ? kt::c(theme.accent).withAlpha(0.28f) : kt::c(theme.panel).withAlpha(taken ? 0.05f : 0.45f));
            g.fillRoundedRectangle(r.reduced(3.f), 8.f);
            g.setColour(viable ? kt::c(theme.accent) : kt::c(theme.border).withAlpha(taken ? 0.25f : 0.7f));
            g.drawRoundedRectangle(r.reduced(3.f), 8.f, viable ? 2.f : 1.f);
            if (!taken)
            {
                g.setColour(viable ? kt::c(theme.text) : kt::c(theme.muted));
                g.setFont(kt::font(theme, 9.f, true));
                g.drawFittedText(slot.name, r.reduced(6.f).toNearestInt(), juce::Justification::centred, 2);
            }
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (!placing || !onSlot) return;
        const auto& shell = kShells[juce::jlimit(0, kShellCount - 1, shellIndex)];
        auto face = faceRect(getLocalBounds().toFloat());
        for (int i = 0; i < shell.slotCount; ++i)
        {
            const auto& slot = shell.slots[i];
            if (slot.w < 0.02f) continue;
            if (occupied && occupied(i)) continue;
            if (!styleFits(armedStyle, slot.kind)) continue;
            if (slotRect(face, slot).contains(e.position))
            {
                onSlot(i);
                return;
            }
        }
    }
};
}
