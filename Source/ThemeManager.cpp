#include "ThemeManager.h"
#include <cmath>

namespace kyoto
{
    static juce::Colour parseHex(juce::String s, juce::Colour fb)
    {
        s = s.trim();

        if (s.startsWithChar('#'))
        {
            auto raw = s.substring(1);

            if (raw.length() == 6)
                return juce::Colour(
                    (juce::uint32)(
                        0xff000000u |
                        (juce::uint32) raw.getHexValue32()
                    )
                );

            if (raw.length() == 8)
                return juce::Colour(
                    (juce::uint32) raw.getHexValue32()
                );
        }

        return fb;
    }

    ThemeManager::ThemeManager()
    {
        fallback.id = "trippah";
        fallback.name = "TRIPPAH";
        fallback.tag = "classic Dream theme";

        fallback.bg = juce::Colour(0xff0e0608);
        fallback.panel = juce::Colour(0xff1a0c12);
        fallback.panel2 = juce::Colour(0xff12080c);

        fallback.accent = juce::Colour(0xffc04068);
        fallback.accentDim = juce::Colour(0xff7a2844);
        fallback.accentBright = juce::Colour(0xffe07090);
        fallback.accent2 = juce::Colour(0xffa02848);

        fallback.text = juce::Colour(0xfff4e4ea);
        fallback.textDim = juce::Colour(0xff9a7884);
        fallback.border = juce::Colour(0xff341820);

        fallback.glowAlpha = .26f;
        fallback.radius = 6.f;

        all.push_back(fallback);
    }

    float ThemeManager::radiusFromCss(const juce::String& css)
    {
        auto s = css.trim().toLowerCase();

        if (s.endsWithChar('p'))
            s = s.dropLastCharacters(2);

        auto n = s.getFloatValue();

        return juce::jlimit(0.f, 20.f, n);
    }

    juce::Colour ThemeManager::colour(
        const juce::var& v,
        juce::Colour fallback)
    {
        auto s = v.toString();

        if (s.startsWithIgnoreCase("rgba"))
        {
            auto inside =
                s.fromFirstOccurrenceOf(
                    "(",
                    false,
                    false
                ).upToLastOccurrenceOf(
                    ")",
                    false,
                    false
                );

            auto parts =
                juce::StringArray::fromTokens(
                    inside,
                    ",",
                    ""
                );

            if (parts.size() >= 3)
            {
                auto r = juce::jlimit(
                    0,
                    255,
                    parts[0].getIntValue()
                );

                auto g = juce::jlimit(
                    0,
                    255,
                    parts[1].getIntValue()
                );

                auto b = juce::jlimit(
                    0,
                    255,
                    parts[2].getIntValue()
                );

                float a =
                    parts.size() > 3
                        ? parts[3].getFloatValue()
                        : 1.f;

                a = juce::jlimit(
                    0.f,
                    1.f,
                    a
                );

                return juce::Colour(
                    (juce::uint8) r,
                    (juce::uint8) g,
                    (juce::uint8) b,
                    (juce::uint8) (a * 255.f)
                );
            }
        }

        return parseHex(s, fallback);
    }

    bool ThemeManager::loadFromFile(
        const juce::File& file)
    {
        return file.existsAsFile()
            && loadFromJson(
                file.loadFileAsString()
            );
    }

    bool ThemeManager::loadFromJson(
        const juce::String& json)
    {
        auto parsed = juce::JSON::parse(json);

        auto* root =
            parsed.getDynamicObject();

        if (!root)
            return false;

        auto arr =
            root->getProperty("themes");

        auto* themes =
            arr.getArray();

        if (!themes)
            return false;

        std::vector<ThemePalette> next;

        for (auto& item : *themes)
        {
            auto* o =
                item.getDynamicObject();

            if (!o)
                continue;

            ThemePalette t = fallback;

            t.id =
                o->getProperty("id").toString();

            t.name =
                o->getProperty("name").toString();

            t.tag =
                o->getProperty("tag").toString();

            t.scene =
                o->getProperty("scene").toString();

            t.veil =
                o->getProperty("veil").toString();

            auto* vars =
                o->getProperty("vars")
                    .getDynamicObject();

            if (vars)
            {
                t.bg = colour(
                    vars->getProperty("--bg"),
                    t.bg
                );

                t.panel = colour(
                    vars->getProperty("--panel"),
                    t.panel
                );

                t.panel2 = colour(
                    vars->getProperty("--panel-2"),
                    t.panel2
                );

                t.accent = colour(
                    vars->getProperty("--accent"),
                    t.accent
                );

                t.accentDim = colour(
                    vars->getProperty("--accent-dim"),
                    t.accentDim
                );

                t.accentBright = colour(
                    vars->getProperty("--accent-bright"),
                    t.accentBright
                );

                t.accent2 = colour(
                    vars->getProperty("--accent2"),
                    t.accent2
                );

                t.text = colour(
                    vars->getProperty("--text"),
                    t.text
                );

                t.textDim = colour(
                    vars->getProperty("--text-dim"),
                    t.textDim
                );

                t.border = colour(
                    vars->getProperty("--border"),
                    t.border
                );

                t.font =
                    vars->getProperty("--font")
                        .toString();

                auto gl =
                    vars->getProperty("--glow")
                        .toString();

                if (gl.contains("rgba"))
                {
                    t.glowAlpha =
                        juce::jlimit(
                            .05f,
                            .6f,
                            gl.fromLastOccurrenceOf(
                                ",",
                                false,
                                false
                            ).getFloatValue()
                        );
                }

                t.radius =
                    radiusFromCss(
                        vars->getProperty("--radius")
                            .toString()
                    );
            }

            if (t.id.isNotEmpty())
                next.push_back(t);
        }

        if (next.empty())
            return false;

        all = next;

        for (auto& t : all)
        {
            if (t.id == "trippah")
            {
                fallback = t;
                break;
            }
        }

        return true;
    }

    const ThemePalette& ThemeManager::get(
        const juce::String& id) const
    {
        for (auto& t : all)
        {
            if (t.id.equalsIgnoreCase(id))
                return t;
        }

        return fallback;
    }

    ThemePalette ThemeManager::makeCustom(
        const ThemePalette& base,
        const juce::String& name,
        const juce::Colour& accent) const
    {
        auto t = base;

        t.id =
            "custom-" +
            juce::String(
                juce::Random::getSystemRandom()
                    .nextInt(999999)
            );

        t.name =
            name.isNotEmpty()
                ? name
                : "My Dream Theme";

        t.tag = "user-created";

        t.accent = accent;
        t.accentBright =
            accent.brighter(.35f);

        t.accentDim =
            accent.darker(.45f);

        t.accent2 =
            accent.withRotatedHue(.12f);

        t.custom = true;

        return t;
    }

    juce::var ThemeManager::toVar(
        const ThemePalette& t)
    {
        auto* o =
            new juce::DynamicObject();

        o->setProperty("id", t.id);
        o->setProperty("name", t.name);
        o->setProperty("tag", t.tag);
        o->setProperty("scene", t.scene);
        o->setProperty("veil", t.veil);
        o->setProperty("custom", t.custom);

        auto* v =
            new juce::DynamicObject();

        auto hex =
            [](juce::Colour c)
            {
                return "#" +
                    c.toString().substring(2);
            };

        v->setProperty("--bg", hex(t.bg));
        v->setProperty("--panel", hex(t.panel));
        v->setProperty("--panel-2", hex(t.panel2));

        v->setProperty("--accent", hex(t.accent));
        v->setProperty("--accent-dim", hex(t.accentDim));
        v->setProperty("--accent-bright", hex(t.accentBright));
        v->setProperty("--accent2", hex(t.accent2));

        v->setProperty("--text", hex(t.text));
        v->setProperty("--text-dim", hex(t.textDim));
        v->setProperty("--border", hex(t.border));

        v->setProperty(
            "--radius",
            juce::String(t.radius) + "px"
        );

        v->setProperty("--font", t.font);

        o->setProperty(
            "vars",
            juce::var(v)
        );

        return juce::var(o);
    }

    ThemePalette ThemeManager::fromVar(
        const juce::var& v,
        const ThemePalette& fallback)
    {
        auto t = fallback;

        auto* o =
            v.getDynamicObject();

        if (!o)
            return t;

        t.id =
            o->getProperty("id")
                .toString();

        t.name =
            o->getProperty("name")
                .toString();

        t.tag =
            o->getProperty("tag")
                .toString();

        t.custom =
            (bool) o->getProperty("custom");

        auto* vars =
            o->getProperty("vars")
                .getDynamicObject();

        if (vars)
        {
            t.bg = colour(
                vars->getProperty("--bg"),
                t.bg
            );

            t.panel = colour(
                vars->getProperty("--panel"),
                t.panel
            );

            t.panel2 = colour(
                vars->getProperty("--panel-2"),
                t.panel2
            );

            t.accent = colour(
                vars->getProperty("--accent"),
                t.accent
            );

            t.accentDim = colour(
                vars->getProperty("--accent-dim"),
                t.accentDim
            );

            t.accentBright = colour(
                vars->getProperty("--accent-bright"),
                t.accentBright
            );

            t.accent2 = colour(
                vars->getProperty("--accent2"),
                t.accent2
            );

            t.text = colour(
                vars->getProperty("--text"),
                t.text
            );

            t.textDim = colour(
                vars->getProperty("--text-dim"),
                t.textDim
            );

            t.border = colour(
                vars->getProperty("--border"),
                t.border
            );

            t.radius =
                radiusFromCss(
                    vars->getProperty("--radius")
                        .toString()
                );

            t.font =
                vars->getProperty("--font")
                    .toString();
        }

        return t;
    }
}
