#pragma once
#include <JuceHeader.h>
#include "Themes.h"
#include <array>
#include <cmath>
#include <cstring>
#include <algorithm>

// Hardware shells for the Plugin Builder. Slots are normalised inside the face plate.
// The motherboard bay is always occupied first and is the start of the signal chain.
// Each shell also carries its own internal hardware (see HardwareInternals.h) so the
// selected template forms a distinct, special-looking machine.
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
    Slot slots[24];
    int slotCount;
    float corner = -1.f;
    int trim = 0;
    int screenStyle = 0;
    int internals = -1;
    bool mirrorInternals = false;
    float tint = 0.f;
    int bezel = 0;
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

inline constexpr int kGenIndex = kShellCount;
inline constexpr int kShellChoices = kShellCount + 1;

struct GeneratedShell
{
    Shell shell {};
    std::array<std::array<char, 20>, 24> names {};
    std::array<char, 48> title {};
    std::array<char, 16> id {};
    std::array<char, 16> silhouette {};
    juce::uint32 seed = 0;
    int serial = 0;
    bool ready = false;
};

inline GeneratedShell& generated()
{
    static GeneratedShell g;
    return g;
}

inline const Shell& shellAt(int index)
{
    if (index >= kGenIndex && generated().ready)
        return generated().shell;
    return kShells[juce::jlimit(0, kShellCount - 1, index)];
}

inline int clampShellIndex(int index)
{
    return (index >= kGenIndex && generated().ready)
        ? kGenIndex : juce::jlimit(0, kShellCount - 1, index);
}
inline int shellInternalIndex(int index)
{
    if (index >= kGenIndex && generated().ready)
        return juce::jlimit(0, 3, generated().shell.internals);
    return juce::jlimit(0, 3, index);
}

inline int templateSignature(juce::uint32 seed)
{
    juce::Random r((juce::int64) seed);
    return r.nextInt(6) * 8 + r.nextInt(5);
}

inline juce::uint32 freshSeed(int previousSignature)
{
    juce::Random r((juce::int64) juce::Time::getMillisecondCounterHiRes());
    auto seed = (juce::uint32) r.nextInt();
    for (int i = 0; i < 64 && previousSignature >= 0 && templateSignature(seed) == previousSignature; ++i)
        seed = (juce::uint32) r.nextInt();
    return seed;
}

inline void copyText(std::array<char, 48>& dst, const juce::String& value)
{
    std::memset(dst.data(), 0, dst.size());
    std::strncpy(dst.data(), value.substring(0, (int) dst.size() - 1).toRawUTF8(), dst.size() - 1);
}

inline void copyText(std::array<char, 16>& dst, const juce::String& value)
{
    std::memset(dst.data(), 0, dst.size());
    std::strncpy(dst.data(), value.substring(0, (int) dst.size() - 1).toRawUTF8(), dst.size() - 1);
}

inline void copyName(GeneratedShell& g, int i, const juce::String& value)
{
    if (i < 0 || i >= (int) g.names.size()) return;
    std::memset(g.names[(size_t) i].data(), 0, g.names[(size_t) i].size());
    std::strncpy(g.names[(size_t) i].data(), value.substring(0, 18).toRawUTF8(), g.names[(size_t) i].size() - 1);
    g.shell.slots[i].name = g.names[(size_t) i].data();
}

inline void setGeneratedSlot(GeneratedShell& g, int i, SlotKind kind,
                             float x, float y, float w, float h, const juce::String& name)
{
    if (i < 0 || i >= 24) return;
    g.shell.slots[i] = { kind, x, y, w, h, nullptr };
    copyName(g, i, name);
}

// Generates a complete machine: one motherboard, one screen, and 6-20 total bays.
// Layout archetype + screen style are seed-derived so a new build is visibly different.
inline void designShell(juce::uint32 seed, int serial = 1)
{
    auto& g = generated();
    g = {};
    g.seed = seed;
    g.serial = serial;
    juce::Random r((juce::int64) seed);
    const int archetype = r.nextInt(6);
    const int screenStyle = r.nextInt(5);
    const int desired = 6 + r.nextInt(15); // 6..20
    const char* silhouettes[] = { "console", "tower", "desk", "pocket", "console", "tower" };
    copyText(g.silhouette, silhouettes[archetype]);
    copyText(g.id, "gen");
    copyText(g.title, "Dream Machine " + juce::String(serial));

    g.shell.id = g.id.data();
    g.shell.name = g.title.data();
    g.shell.hiddenFx = r.nextInt(32);
    g.shell.hiddenMix = 0.055f + r.nextFloat() * 0.09f;
    g.shell.silhouette = g.silhouette.data();
    g.shell.slotCount = desired;

    // The first two bays are always unique functional anchors.
    setGeneratedSlot(g, 0, SlotKind::Board, 0.04f, 0.68f, 0.40f, 0.24f, "MOTHERBOARD");
    setGeneratedSlot(g, 1, SlotKind::Screen, 0.05f, 0.07f, 0.58f, 0.27f, "SCREEN");

    const int extras = desired - 2;
    const int cols = archetype == 1 ? 2 : 4;
    const float top = 0.39f, bottom = 0.93f, gap = 0.018f;
    const int rows = (extras + cols - 1) / cols;
    const float cw = (0.92f - gap * (cols - 1)) / (float) cols;
    const float rh = (bottom - top - gap * (rows - 1)) / (float) rows;
    for (int n = 0; n < extras; ++n)
    {
        const int i = n + 2;
        const int col = n % cols, row = n / cols;
        const float x = 0.04f + col * (cw + gap);
        const float y = top + row * (rh + gap);
        SlotKind kind;
        const int pick = r.nextInt(100);
        if (pick < 28) kind = SlotKind::Knob;
        else if (pick < 48) kind = SlotKind::Fader;
        else if (pick < 64) kind = SlotKind::Key;
        else if (pick < 78) kind = SlotKind::Cosmetic;
        else kind = SlotKind::Knob;
        const float w = kind == SlotKind::Fader ? cw * 0.72f : cw;
        const float h = kind == SlotKind::Fader ? rh : rh * 0.92f;
        const char* kindName = kind == SlotKind::Fader ? "FADER" :
                               kind == SlotKind::Key ? "KEY" :
                               kind == SlotKind::Cosmetic ? "DETAIL" : "KNOB";
        setGeneratedSlot(g, i, kind, x, y, w, h, juce::String(kindName) + " " + juce::String(i - 1));
    }
    g.shell.corner = 8.f + r.nextFloat() * 22.f;
    g.shell.trim = r.nextInt(7);
    g.shell.screenStyle = screenStyle;
    g.shell.internals = r.nextInt(4);
    g.shell.mirrorInternals = r.nextBool();
    g.shell.tint = (r.nextFloat() - 0.5f) * 0.10f;
    g.shell.bezel = r.nextInt(3);
    g.ready = true;
}

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
    // Sound and toggle buttons can sit in any bay except the motherboard.
    if (style == "sound" || style == "button") return slot != SlotKind::Board;
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

inline float shellRadius(const Shell& shell)
{
    if (shell.corner >= 0.f) return shell.corner;
    const auto s = juce::String(shell.silhouette);
    return s == "pocket" ? 28.f : s == "tower" ? 8.f : 16.f;
}

class BuilderCanvas : public juce::Component
{
public:
    int shellIndex = 0;
    bool placing = false;
    juce::String armedStyle;
    kt::ThemePalette theme = kt::kThemes[0];
    std::function<void(int)> onSlot;
    std::function<void(int, juce::Point<int>)> onRightClick;
    std::function<bool(int)> occupied;
    std::function<juce::Point<float>(int)> anchor;
    std::function<int(int)> parentOf;
    std::function<int()> highlightedBay;
    std::function<void()> onBackgroundClick;
    std::function<void()> onResized;
    int hoverSlot = -1;
    void resized() override { if (onResized) onResized(); }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        const auto& shell = shellAt(shellIndex);
        const auto accent = kt::c(theme.accent);

        g.setColour(kt::c(theme.panel).withAlpha(0.96f));
        g.fillRoundedRectangle(bounds, 14.f);
        g.setColour(kt::c(theme.border));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 14.f, 1.f);

        // Clean header: template name on one line, chain hint below it.
        g.setColour(accent);
        g.setFont(kt::font(theme, 15.f, true));
        g.drawText("PLUGIN BUILDER", 18, 12, 220, 18, juce::Justification::left);
        g.setColour(kt::c(theme.muted));
        g.setFont(kt::font(theme, 11.5f));
        g.drawText(juce::String(shell.name) + "  -  chain starts at the motherboard", 18, 31, getWidth() - 36, 15, juce::Justification::left);

        auto face = faceRect(bounds);
        const float radius = shellRadius(shell);

        // Faceplate with a per-template tint so each hardware reads special.
        g.setColour(kt::c(theme.bg).withAlpha(0.88f));
        g.fillRoundedRectangle(face, radius);
        g.setColour(accent.withAlpha(0.45f));
        g.drawRoundedRectangle(face, radius, 1.4f);

        // Shell trim: corner screws, LED strip and a name badge - like real hardware.
        g.setColour(kt::c(theme.muted).withAlpha(0.65f));
        for (auto p : { juce::Point<float>(face.getX() + 10.f, face.getY() + 10.f),
                        juce::Point<float>(face.getRight() - 10.f, face.getY() + 10.f),
                        juce::Point<float>(face.getX() + 10.f, face.getBottom() - 10.f),
                        juce::Point<float>(face.getRight() - 10.f, face.getBottom() - 10.f) })
        {
            g.fillEllipse(p.x - 2.5f, p.y - 2.5f, 5.f, 5.f);
        }
        for (int i = 0; i < 3; ++i)
        {
            g.setColour(accent.withAlpha(i == 0 ? 0.9f : 0.3f));
            g.fillRect(face.getRight() - 34.f + (float) i * 9.f, face.getY() + 8.f, 5.f, 5.f);
        }
        g.setColour(accent);
        g.setFont(kt::font(theme, 9.f, true));
        g.drawText(juce::String(shell.name).toUpperCase(), (int) face.getRight() - 130, (int) face.getBottom() - 22, 120, 13, juce::Justification::centredRight);
        g.setColour(accent.withAlpha(0.5f));
        g.drawLine(face.getRight() - 130, face.getBottom() - 9.f, face.getRight() - 20.f, face.getBottom() - 9.f, 1.2f);

        // Bay wiring follows the explicit part-to-part links. The selected part is the hot node.
        const int hot = highlightedBay ? highlightedBay() : -1;
        for (int i = 1; i < shell.slotCount; ++i)
        {
            if (occupied && occupied(i) && anchor)
            {
                int from = parentOf ? parentOf(i) : 0;
                if (from < 0 || from >= shell.slotCount || from == i || ! occupied(from)) from = 0;
                auto a = anchor(from);
                auto b = anchor(i);
                if (a.x > 1.f && b.x > 1.f)
                {
                    const bool glow = i == hot || from == hot;
                    juce::Path wire;
                    wire.startNewSubPath(a);
                    wire.cubicTo(a.x, (a.y + b.y) * 0.5f, b.x, (a.y + b.y) * 0.5f, b.x, b.y);
                    g.setColour(accent.withAlpha(glow ? 1.0f : 0.78f));
                    g.strokePath(wire, juce::PathStrokeType(glow ? 3.0f : 2.0f));
                    g.setColour(kt::c(theme.pegHot));
                    g.fillEllipse(b.x - 3.f, b.y - 3.f, 6.f, 6.f);
                    g.fillEllipse(a.x - 2.5f, a.y - 2.5f, 5.f, 5.f);
                }
            }
        }

        for (int i = 0; i < shell.slotCount; ++i)
        {
            const auto& slot = shell.slots[i];
            if (slot.w < 0.02f || slot.h < 0.02f) continue;
            auto r = slotRect(face, slot).reduced(3.f);
            const bool taken = occupied && occupied(i);
            const bool fits = placing && ! taken && styleFits(armedStyle, slot.kind);
            const bool hovered = placing && i == hoverSlot && fits;

            // Bay plate.
            g.setColour(fits ? accent.withAlpha(hovered ? 0.38f : 0.26f) : kt::c(theme.panel).withAlpha(taken ? 0.05f : 0.42f));
            g.fillRoundedRectangle(r, 8.f);
            g.setColour(fits ? accent : kt::c(theme.border).withAlpha(taken ? 0.25f : 0.7f));
            g.drawRoundedRectangle(r, 8.f, fits ? (hovered ? 2.4f : 1.8f) : 1.f);

            // Lego studs: pegs at the four corners click parts into the bay.
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
                    if (fits)
                    {
                        g.setColour(kt::c(theme.pegHot).withAlpha(0.35f));
                        g.drawEllipse(c.x - pegR - 2.f, c.y - pegR - 2.f, pegR * 2.f + 4.f, pegR * 2.f + 4.f, 1.f);
                    }
                }
            }

            if (! taken)
            {
                g.setColour(fits ? kt::c(theme.text) : kt::c(theme.muted));
                g.setFont(kt::font(theme, 10.5f, true));
                g.drawFittedText(slot.name, r.reduced(6.f).toNearestInt(), juce::Justification::centred, 2);
            }
        }
    }

    int slotAt(juce::Point<float> pos) const
    {
        if (! placing) return -1;
        const auto& shell = shellAt(shellIndex);
        auto face = faceRect(getLocalBounds().toFloat());
        for (int i = 0; i < shell.slotCount; ++i)
        {
            const auto& slot = shell.slots[i];
            if (slot.w < 0.02f) continue;
            if (occupied && occupied(i)) continue;
            if (! styleFits(armedStyle, slot.kind)) continue;
            if (slotRect(face, slot).contains(pos)) return i;
        }
        return -1;
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        const int hit = slotAt(e.position);
        if (hit != hoverSlot) { hoverSlot = hit; repaint(); }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (hoverSlot != -1) { hoverSlot = -1; repaint(); }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            const auto& shell = shellAt(shellIndex);
            auto face = faceRect(getLocalBounds().toFloat());
            int hit = -1;
            for (int i = 0; i < shell.slotCount; ++i)
                if (slotRect(face, shell.slots[i]).contains(e.position)) { hit = i; break; }
            if (onRightClick) onRightClick(hit, e.getScreenPosition());
            return;
        }
        if (! placing || ! onSlot)
        {
            if (onBackgroundClick) onBackgroundClick();
            return;
        }
        const auto& shell = shellAt(shellIndex);
        auto face = faceRect(getLocalBounds().toFloat());
        for (int i = 0; i < shell.slotCount; ++i)
        {
            const auto& slot = shell.slots[i];
            if (slot.w < 0.02f) continue;
            if (occupied && occupied(i)) continue;
            if (! styleFits(armedStyle, slot.kind)) continue;
            if (slotRect(face, slot).contains(e.position))
            {
                onSlot(i);
                return;
            }
        }
    }
};
}