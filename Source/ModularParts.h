#pragma once
#include "Themes.h"

// ============================================================================
//  MODULAR PARTS - COSMETIC ONLY
//  Every placed part can carry a SKIN (visual family) and a LOOK note. Modules have NO effect on sound or
//  control behaviour: only effects the user places affect audio. Data lives here; the editor stores "skin" + "quirk"
//  on each placed widget ValueTree node, so skins/quirks serialize with builds
//  for free. Future batches just add rows to the tables below.
// ============================================================================

namespace kt
{

// ---- Skins: "stock" follows the active theme; the rest recolour the part so
// machines can mix families. accentHex/panelHex null = inherit the theme.
struct PartSkin { const char* id; const char* name; const char* accentHex; const char* panelHex; };

inline constexpr PartSkin kPartSkins[] = {
    { "stock",  "Stock",  nullptr,   nullptr   },
    { "copper", "Copper", "#b87333", "#241811" },
    { "mint",   "Mint",   "#69d1a0", "#10241c" },
    { "violet", "Violet", "#9a6cf0", "#191330" },
    { "amber",  "Amber",  "#e0a020", "#291f0a" },
    { "ice",    "Ice",    "#7ec8f0", "#0e2130" },
    { "crimson","Crimson","#e04050", "#2a0e12" },
    { "jade",   "Jade",   "#30c080", "#0e2418" },
    { "gold",   "Gold",   "#ffd040", "#2a2208" },
    { "plasma", "Plasma", "#ff40c0", "#1a0a1a" },
    { "toxic",  "Toxic",  "#a0ff20", "#142a0a" },
    { "cobalt", "Cobalt", "#4080ff", "#0a1428" }
};
inline constexpr int kPartSkinCount = 12;

inline const PartSkin* partSkinById(const juce::String& id)
{
    for (int i = 0; i < kPartSkinCount; ++i)
        if (id == kPartSkins[i].id) return &kPartSkins[i];
    return nullptr;
}

// ---- Modular pieces, each with a special quirk.
// Batch 1: cosmetic skins. Batch 2: functional quirks (timer, randomizer, sequencer,
// lfo, s&h, logic, switch, router) that modulate live values when plugged into a part.
struct ModPiece { const char* id; const char* name; const char* quirk; const char* skin; };

inline constexpr ModPiece kModPieces[] = {
    // Batch 1 - cosmetic
    { "snap",  "Snap Dial",    "Cosmetic: mint stepped-dial look",      "mint"   },
    { "lens",  "Lens Knob",    "Cosmetic: copper precision-knob look", "copper" },
    { "flip",  "Invert Fader", "Cosmetic: violet fader look",    "violet" },
    { "warp",  "Warp Dial",    "Cosmetic: amber dial look",            "amber"  },
    { "pulse", "Pulse Key",    "Cosmetic: ice key look",                 "ice"    },
    { "ghost", "Ghost Screen", "Cosmetic: stock screen look",    "stock"  },
    // Batch 2 - functional modulators
    { "timer",     "Timer",      "Cycles a value up and down at a set rate. Plug into any knob to auto-sweep it.", "jade" },
    { "randomizer", "Randomizer", "Randomly jumps a value within a range at a set interval. Plug into any knob for live chaos.", "plasma" },
    { "sequencer", "Sequencer",  "Steps through 8 values in order. Plug into a knob for rhythmic patterns.", "gold" },
    { "lfo",       "LFO",        "Smooth sine wave modulation at audio or sub rate. Plug into any knob.", "cobalt" },
    { "samplehold","S&H",        "Samples a value at random intervals and holds it. Plug into a knob.", "toxic" },
    { "logic",     "Logic Gate", "AND/OR/NOT gate for combining two mod sources. Plug into two knobs.", "crimson" },
    { "switch",    "Switch",     "Toggles between two connected values on key press. Plug into two knobs.", "copper" },
    { "router",    "Router",     "Sends a value to one of four outputs based on a control. Plug into up to four knobs.", "ice" },
    { "clock",     "Clock",      "Master tempo source. Plug into timers, sequencers and LFOs to sync them.", "gold" },
    { "envelope",  "Envelope",   "ADSR envelope follower. Plug into a knob to make it respond to audio level.", "violet" }
};
inline constexpr int kModPieceCount = 16;

// ---- Placeable control params: the FX options a dial/fader can be bound to
// when it is placed. Token matches the APVTS suffixes (s01amt, s01mot ...).
struct ControlParam { const char* token; const char* name; };

inline constexpr ControlParam kControlParams[] = {
    { "amt",  "Amount" },
    { "tone", "Tone"   },
    { "mot",  "Motion" },
    { "mix",  "Mix %"  },
    { "shp",  "Shape"  }
};
inline constexpr int kControlParamCount = 5;

// ---- Modulator settings: describes how a functional quirk modulates its target.
struct ModSettings
{
    float rate = 0.5f;       // speed of modulation (0..1)
    float depth = 1.0f;     // how much it modulates (0..1)
    float minRange = 0.0f;  // minimum value for randomizer/sequencer
    float maxRange = 1.0f;  // maximum value for randomizer/sequencer
    int steps = 8;           // steps for sequencer
    bool bipolar = true;     // true = centered around 0.5, false = 0..1
};

} // namespace kt
