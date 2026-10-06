#pragma once
#include "PluginShells.h"
#include "HardwareInternals.h"
#include <vector>

// Hidden chassis modules, now snapped together by hand.
// Each module opens placement bays and contributes a small hardware colour.
// The mainboard only holds the display — it does not insert an effect.
namespace lg
{
struct Bay
{
    pb::SlotKind kind;
    float x, y, w, h;
    const char* name;
};

struct Module
{
    const char* id;
    const char* name;
    hb::PartKind draw;
    int subtleFx;     // -1 = no effect (the display host)
    float subtleMix;
    const char* blurb;
    Bay bays[3];
    int bayCount;
};

inline const Module kModules[] = {
    { "mainboard", "MAINBOARD", hb::PartKind::Board, -1, 0.f,
      "Holds the display bay. The display is the chain start and has no effect.",
      { { pb::SlotKind::Screen, 0.08f, 0.12f, 0.84f, 0.76f, "DISPLAY" } }, 1 },
    { "power", "POWER CELL", hb::PartKind::Battery, 21, 0.06f,
      "Two knob bays. Adds a subtle Warmth to the chassis.",
      { { pb::SlotKind::Knob, 0.08f, 0.14f, 0.38f, 0.72f, "KNOB" },
        { pb::SlotKind::Knob, 0.54f, 0.14f, 0.38f, 0.72f, "KNOB" } }, 2 },
    { "caps", "CAP BANK", hb::PartKind::Cap, 20, 0.05f,
      "One fader bay. Adds a subtle Glue to the chassis.",
      { { pb::SlotKind::Fader, 0.30f, 0.08f, 0.40f, 0.84f, "FADER" } }, 1 },
    { "ribbon", "BUS RIBBON", hb::PartKind::Ribbon, 4, 0.045f,
      "One effect bay. Adds a subtle Stereo Width to the chassis.",
      { { pb::SlotKind::Fx, 0.10f, 0.18f, 0.80f, 0.64f, "EFFECT" } }, 1 },
    { "fan", "FAN", hb::PartKind::Fan, 30, 0.04f,
      "One vent bay. Adds a subtle Air to the chassis.",
      { { pb::SlotKind::Cosmetic, 0.16f, 0.16f, 0.68f, 0.68f, "VENT" } }, 1 },
    { "port", "I/O PORT", hb::PartKind::Port, 15, 0.035f,
      "One key bay. Adds a subtle Tape colour to the chassis.",
      { { pb::SlotKind::Key, 0.16f, 0.22f, 0.68f, 0.56f, "KEY" } }, 1 },
    { "header", "EXPANSION", hb::PartKind::Header, 55, 0.04f,
      "Knob, fader and effect bays. Adds a subtle Exciter to the chassis.",
      { { pb::SlotKind::Knob, 0.06f, 0.08f, 0.40f, 0.40f, "KNOB" },
        { pb::SlotKind::Fader, 0.52f, 0.08f, 0.40f, 0.84f, "FADER" },
        { pb::SlotKind::Fx, 0.06f, 0.54f, 0.40f, 0.38f, "EFFECT" } }, 3 },
    { "ram", "RAM", hb::PartKind::Ram, 20, 0.03f,
      "One knob bay. Adds a little more Glue to the chassis.",
      { { pb::SlotKind::Knob, 0.18f, 0.14f, 0.64f, 0.72f, "KNOB" } }, 1 },
    { "psu", "PSU", hb::PartKind::Psu, 43, 0.06f,
      "One fader bay. Adds a subtle Soft Clip to the chassis.",
      { { pb::SlotKind::Fader, 0.30f, 0.08f, 0.40f, 0.84f, "FADER" } }, 1 },
    { "drive", "DRIVE", hb::PartKind::Drive, 80, 0.04f,
      "One key bay. Adds a subtle Vinyl colour to the chassis.",
      { { pb::SlotKind::Key, 0.16f, 0.22f, 0.68f, 0.56f, "KEY" } }, 1 },
    { "antenna", "ANTENNA", hb::PartKind::Antenna, 51, 0.035f,
      "One badge bay. Adds a subtle Haas Width to the chassis.",
      { { pb::SlotKind::Cosmetic, 0.22f, 0.18f, 0.56f, 0.64f, "BADGE" } }, 1 }
};

inline constexpr int kModuleCount = 11;

struct Placed
{
    int module = 0;
    int parent = -1;
    int col = 0;
    int row = 0;
    int slotBase = 0;
};

inline void readBricks(const juce::ValueTree& ui, juce::Array<Placed>& out)
{
    out.clearQuick();
    for (int i = 0; i < ui.getNumChildren(); ++i)
    {
        auto child = ui.getChild(i);
        if (! child.hasType("brick")) continue;
        Placed placed;
        placed.module = juce::jlimit(0, kModuleCount - 1, (int) child.getProperty("module", 0));
        placed.parent = (int) child.getProperty("parent", -1);
        placed.col = (int) child.getProperty("col", 0);
        placed.row = (int) child.getProperty("row", 0);
        placed.slotBase = (int) child.getProperty("slotBase", 0);
        out.add(placed);
    }
}

inline void gridSpan(const juce::Array<Placed>& bricks, int& minC, int& minR, int& cols, int& rows)
{
    minC = 0; minR = 0; cols = 1; rows = 1;
    if (bricks.isEmpty()) return;
    int maxC = bricks.getReference(0).col, maxR = bricks.getReference(0).row;
    minC = maxC; minR = maxR;
    for (const auto& brick : bricks)
    {
        minC = juce::jmin(minC, brick.col); maxC = juce::jmax(maxC, brick.col);
        minR = juce::jmin(minR, brick.row); maxR = juce::jmax(maxR, brick.row);
    }
    cols = juce::jmax(1, maxC - minC + 1);
    rows = juce::jmax(1, maxR - minR + 1);
}

inline void cellNorm(const juce::Array<Placed>& bricks, int index, float& x, float& y, float& w, float& h)
{
    int minC, minR, cols, rows;
    gridSpan(bricks, minC, minR, cols, rows);
    const float pad = 0.012f;
    w = 1.f / (float) cols;
    h = 1.f / (float) rows;
    const auto& brick = bricks.getReference(juce::jlimit(0, bricks.size() - 1, index));
    x = (float) (brick.col - minC) * w + pad;
    y = (float) (brick.row - minR) * h + pad;
    w = juce::jmax(0.02f, w - pad * 2.f);
    h = juce::jmax(0.02f, h - pad * 2.f);
}

inline std::vector<pb::Slot> bakeSlots(const juce::Array<Placed>& bricks, int cursor)
{
    int need = juce::jmax(0, cursor);
    for (const auto& brick : bricks)
        need = juce::jmax(need, brick.slotBase + kModules[brick.module].bayCount);
    std::vector<pb::Slot> slots((size_t) need, pb::Slot { pb::SlotKind::Knob, 0.f, 0.f, 0.f, 0.f, "" });
    if (bricks.isEmpty()) return slots;
    int minC, minR, cols, rows;
    gridSpan(bricks, minC, minR, cols, rows);
    const float cellW = 1.f / (float) cols;
    const float cellH = 1.f / (float) rows;
    for (const auto& brick : bricks)
    {
        const auto& module = kModules[brick.module];
        const float ox = (float) (brick.col - minC) * cellW;
        const float oy = (float) (brick.row - minR) * cellH;
        for (int b = 0; b < module.bayCount; ++b)
        {
            const int index = brick.slotBase + b;
            if (index < 0 || index >= (int) slots.size()) continue;
            const auto& bay = module.bays[b];
            slots[(size_t) index] = pb::Slot {
                bay.kind,
                ox + bay.x * cellW,
                oy + bay.y * cellH,
                bay.w * cellW,
                bay.h * cellH,
                bay.name
            };
        }
    }
    return slots;
}

inline bool cellTaken(const juce::Array<Placed>& bricks, int col, int row)
{
    for (const auto& brick : bricks)
        if (brick.col == col && brick.row == row) return true;
    return false;
}
} // namespace lg
