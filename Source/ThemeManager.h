#pragma once
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <vector>

namespace kyoto {
struct ThemePalette {
    juce::String id, name, tag, font, scene, veil;
    juce::Colour bg{0xff0a0a0a}, panel{0xff141414}, panel2{0xff0e0e0e};
    juce::Colour accent{0xffe62020}, accentDim{0xffa01818}, accentBright{0xffff5555}, accent2{0xffff3344};
    juce::Colour text{0xfff4f4f4}, textDim{0xff9a9a9a}, border{0xff2c2c2c};
    float glowAlpha = 0.22f;
    float radius = 0.0f;
    bool custom = false;
};

enum class UiVariant { Classic = 0, Compact, Glass, Terminal };

class ThemeManager {
public:
    ThemeManager();
    bool loadFromFile(const juce::File& file);
    bool loadFromJson(const juce::String& json);
    const std::vector<ThemePalette>& themes() const { return all; }
    const ThemePalette& get(const juce::String& id) const;
    ThemePalette makeCustom(const ThemePalette& base, const juce::String& name, const juce::Colour& accent) const;
    static float radiusFromCss(const juce::String& css);
    static juce::Colour colour(const juce::var& v, juce::Colour fallback);
    static juce::var toVar(const ThemePalette& t);
    static ThemePalette fromVar(const juce::var& v, const ThemePalette& fallback);
private:
    std::vector<ThemePalette> all;
    ThemePalette fallback;
};
}
