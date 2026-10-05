#pragma once
#include <JuceHeader.h>

// Single-file theme system. All 42 legacy Dream themes collapsed into
// one palette table so the native UI can switch without loading 42 HTML files.
namespace kt {

struct ThemePalette
{
    const char* id;
    const char* name;
    juce::uint32 bg;       // main background
    juce::uint32 panel;    // panel / card
    juce::uint32 accent;   // primary accent
    juce::uint32 text;     // primary text
    juce::uint32 muted;    // secondary text
    juce::uint32 peg;      // grid peg free
    juce::uint32 pegHot;   // grid peg highlighted for placement
    juce::uint32 knob;     // knob face
    juce::uint32 border;   // subtle border
};

// Compact set of the most distinctive themes from the Site/Themes folder.
// Full list can be expanded later; these cover the visual identity.
inline constexpr ThemePalette kThemes[] = {
    { "trippah",   "Trippah",   0xff0e0c14, 0xff1a1624, 0xffc77dff, 0xfff0e6ff, 0xff8a7aa8, 0xff3a2e4a, 0xffe0a0ff, 0xff2a2238, 0xff4a3a5e },
    { "goonr",     "Goonr",     0xff0a1210, 0xff121c18, 0xff3dffb0, 0xffe0fff0, 0xff6a9a80, 0xff1e3a30, 0xff80ffc0, 0xff1a2a22, 0xff2a4a3a },
    { "abyss",     "Abyss",     0xff05080f, 0xff0c121c, 0xff3a8cff, 0xffd0e4ff, 0xff5a7aaa, 0xff1a2838, 0xff80b0ff, 0xff121a28, 0xff2a3a50 },
    { "amber",     "Amber",     0xff140e08, 0xff221810, 0xffff9a3c, 0xfffff0e0, 0xffa08060, 0xff3a2a18, 0xffffc080, 0xff2a1e12, 0xff4a3a20 },
    { "bloodmoon", "Bloodmoon", 0xff120808, 0xff1e1010, 0xffff4060, 0xffffe0e4, 0xffa06070, 0xff3a1a20, 0xffff80a0, 0xff2a1418, 0xff4a2028 },
    { "cobalt",    "Cobalt",    0xff080c14, 0xff10182a, 0xff4080ff, 0xffe0ecff, 0xff6080b0, 0xff1a2840, 0xff80b0ff, 0xff121c30, 0xff2a3a58 },
    { "ember",     "Ember",     0xff120a06, 0xff1e140c, 0xffff6030, 0xffffece0, 0xffa07050, 0xff3a2418, 0xffffa080, 0xff2a1a10, 0xff4a3020 },
    { "fog",       "Fog",       0xff101418, 0xff1a2028, 0xffa0c0d0, 0xffe8f0f4, 0xff708090, 0xff2a3038, 0xffc0d8e0, 0xff1e242c, 0xff384048 },
    { "graphite",  "Graphite",  0xff101010, 0xff1a1a1a, 0xffb0b0b0, 0xfff0f0f0, 0xff707070, 0xff2a2a2a, 0xffd0d0d0, 0xff1e1e1e, 0xff3a3a3a },
    { "honey",     "Honey",     0xff14100a, 0xff221c12, 0xffffc040, 0xfffff8e0, 0xffa09050, 0xff3a3018, 0xffffe080, 0xff2a2210, 0xff4a3a20 },
    { "ice",       "Ice",       0xff0a1014, 0xff121c24, 0xff80d0ff, 0xffe8f8ff, 0xff60a0c0, 0xff1a3040, 0xffb0e8ff, 0xff142028, 0xff2a4050 },
    { "ink",       "Ink",       0xff08080c, 0xff101018, 0xff6080ff, 0xffe0e4ff, 0xff5060a0, 0xff1a1a30, 0xffa0b0ff, 0xff121220, 0xff2a2a48 },
    { "lagoon",    "Lagoon",    0xff061210, 0xff0c1e1a, 0xff30d0a0, 0xffe0fff4, 0xff50a080, 0xff1a3a30, 0xff80ffc0, 0xff102820, 0xff2a4a3a },
    { "lilac",     "Lilac",     0xff100e14, 0xff1a1622, 0xffc080ff, 0xfff4e8ff, 0xff8070a0, 0xff2a2438, 0xffe0c0ff, 0xff1e1a2a, 0xff3a3048 },
    { "mint",      "Mint",      0xff0a1210, 0xff121c18, 0xff40e0a0, 0xffe0fff0, 0xff60a080, 0xff1a3a2a, 0xff80ffc0, 0xff12281e, 0xff2a4a38 },
    { "neon",      "Neon",      0xff08080c, 0xff101018, 0xff00ffc0, 0xffe0fff8, 0xff40a080, 0xff1a2a28, 0xff80ffe0, 0xff121a1a, 0xff2a3a38 },
    { "pine",      "Pine",      0xff0a100c, 0xff121a14, 0xff40c060, 0xffe0ffe8, 0xff509060, 0xff1a3020, 0xff80e0a0, 0xff122018, 0xff2a4030 },
    { "plum",      "Plum",      0xff10080e, 0xff1a1018, 0xffc040a0, 0xffffe0f0, 0xffa06080, 0xff3a1a30, 0xffff80d0, 0xff24121e, 0xff4a2840 },
    { "rust",      "Rust",      0xff120c08, 0xff1e1610, 0xffe07030, 0xfffff0e0, 0xffa07050, 0xff3a2818, 0xffffa060, 0xff2a1c12, 0xff4a3020 },
    { "steel",     "Steel",     0xff0c1014, 0xff141c24, 0xff80a0c0, 0xffe8f0f8, 0xff6080a0, 0xff1a2838, 0xffb0c8e0, 0xff121a24, 0xff2a3a4a },
    { "void",      "Void",      0xff060608, 0xff0c0c10, 0xffa080ff, 0xfff0e8ff, 0xff7060a0, 0xff1a1830, 0xffc0a0ff, 0xff10101a, 0xff2a2848 },
    { "wine",      "Wine",      0xff10080a, 0xff1a1014, 0xffc04060, 0xffffe0e8, 0xffa06070, 0xff3a1a24, 0xffff80a0, 0xff241218, 0xff4a2030 },
    { "default",   "Default",   0xff12100e, 0xff1c1814, 0xffefe6d6, 0xffefe6d6, 0xff8a8070, 0xff3a3128, 0xffffe0a0, 0xff2a241e, 0xff4a4038 },
};

inline constexpr int kThemeCount = sizeof(kThemes) / sizeof(kThemes[0]);

inline const ThemePalette& themeById(const juce::String& id)
{
    for (int i = 0; i < kThemeCount; ++i)
        if (id.equalsIgnoreCase(kThemes[i].id))
            return kThemes[i];
    return kThemes[kThemeCount - 1]; // default
}

inline juce::Colour c(juce::uint32 argb) { return juce::Colour(argb); }

} // namespace kt
