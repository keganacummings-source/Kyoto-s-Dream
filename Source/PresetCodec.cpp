#include "PresetCodec.h"

namespace kyoto
{
    juce::var presetToVar(const InstrumentPreset& p)
    {
        auto* o = new juce::DynamicObject();

        o->setProperty("id", p.id);
        o->setProperty("name", p.name);

        o->setProperty("osc1", p.osc1);
        o->setProperty("osc2", p.osc2);
        o->setProperty("osc3", p.osc3);

        o->setProperty("mix1", p.mix1);
        o->setProperty("mix2", p.mix2);
        o->setProperty("mix3", p.mix3);

        o->setProperty("detune2", p.detune2);
        o->setProperty("detune3", p.detune3);

        o->setProperty("attack", p.attack);
        o->setProperty("decay", p.decay);
        o->setProperty("sustain", p.sustain);
        o->setProperty("release", p.release);

        o->setProperty("cutoff", p.cutoff);
        o->setProperty("resonance", p.resonance);

        o->setProperty("drive", p.drive);
        o->setProperty("noise", p.noise);

        o->setProperty("lfoRate", p.lfoRate);
        o->setProperty("lfoDepth", p.lfoDepth);

        o->setProperty("octave", p.octave);
        o->setProperty("arp", p.arp);
        o->setProperty("arpRate", p.arpRate);

        juce::Array<juce::var> fx;
        juce::Array<juce::var> amt;

        for (int i = 0; i < 8; ++i)
        {
            fx.add(p.fx[i]);
            amt.add(p.fxAmount[i]);
        }

        o->setProperty("fx", juce::var(fx));
        o->setProperty("fxAmount", juce::var(amt));

        if (!p.uiLayout.isVoid())
            o->setProperty("uiLayout", p.uiLayout);

        if (!p.uiTheme.isVoid())
            o->setProperty("uiTheme", p.uiTheme);

        juce::Array<juce::var> chain;

        for (const auto& f : p.expertFxChain)
        {
            auto* x = new juce::DynamicObject();

            x->setProperty("effect", f.effect);
            x->setProperty("amount", f.amount);
            x->setProperty("tone", f.tone);
            x->setProperty("motion", f.motion);
            x->setProperty("mix", f.mix);
            x->setProperty("shape", f.shape);

            chain.add(juce::var(x));
        }

        o->setProperty("expertFxChain", juce::var(chain));

        return juce::var(o);
    }

    bool presetFromVar(
        const juce::var& v,
        InstrumentPreset& p)
    {
        auto* o = v.getDynamicObject();

        if (!o)
            return false;

        auto getFloat =
            [&](const char* key, float fallback)
            {
                if (!o->hasProperty(key))
                    return fallback;

                return static_cast<float>(
                    o->getProperty(key));
            };

        auto getInt =
            [&](const char* key, int fallback)
            {
                if (!o->hasProperty(key))
                    return fallback;

                return static_cast<int>(
                    o->getProperty(key));
            };

        auto getBool =
            [&](const char* key, bool fallback)
            {
                if (!o->hasProperty(key))
                    return fallback;

                return static_cast<bool>(
                    o->getProperty(key));
            };

        p.id =
            o->getProperty("id").toString();

        p.name =
            o->getProperty("name").toString();

        p.osc1 = getInt("osc1", p.osc1);
        p.osc2 = getInt("osc2", p.osc2);
        p.osc3 = getInt("osc3", p.osc3);

        p.mix1 = getFloat("mix1", p.mix1);
        p.mix2 = getFloat("mix2", p.mix2);
        p.mix3 = getFloat("mix3", p.mix3);

        p.detune2 = getFloat("detune2", p.detune2);
        p.detune3 = getFloat("detune3", p.detune3);

        p.attack =
            getFloat("attack", p.attack);

        p.decay =
            getFloat("decay", p.decay);

        p.sustain =
            getFloat("sustain", p.sustain);

        p.release =
            getFloat("release", p.release);

        p.cutoff =
            getFloat("cutoff", p.cutoff);

        p.resonance =
            getFloat("resonance", p.resonance);

        p.drive =
            getFloat("drive", p.drive);

        p.noise =
            getFloat("noise", p.noise);

        p.lfoRate =
            getFloat("lfoRate", p.lfoRate);

        p.lfoDepth =
            getFloat("lfoDepth", p.lfoDepth);

        p.octave =
            getInt("octave", p.octave);

        p.arp =
            getBool("arp", p.arp);

        p.arpRate =
            getFloat("arpRate", p.arpRate);

        if (auto* a =
                o->getProperty("fx").getArray())
        {
            for (int i = 0;
                 i < 8 && i < a->size();
                 ++i)
            {
                p.fx[i] =
                    static_cast<int>(
                        a->getUnchecked(i));
            }
        }

        if (auto* a =
                o->getProperty("fxAmount").getArray())
        {
            for (int i = 0;
                 i < 8 && i < a->size();
                 ++i)
            {
                p.fxAmount[i] =
                    static_cast<float>(
                        a->getUnchecked(i));
            }
        }

        if (o->hasProperty("uiLayout"))
            p.uiLayout =
                o->getProperty("uiLayout");

        if (o->hasProperty("uiTheme"))
            p.uiTheme =
                o->getProperty("uiTheme");

        p.expertFxChain.clear();

        if (auto* a =
                o->getProperty("expertFxChain").getArray())
        {
            for (auto& item : *a)
            {
                if (auto* x =
                        item.getDynamicObject())
                {
                    FxSlot f;

                    f.effect =
                        x->hasProperty("effect")
                            ? static_cast<int>(
                                  x->getProperty("effect"))
                            : 0;

                    f.amount =
                        x->hasProperty("amount")
                            ? static_cast<float>(
                                  x->getProperty("amount"))
                            : f.amount;

                    f.tone =
                        x->hasProperty("tone")
                            ? static_cast<float>(
                                  x->getProperty("tone"))
                            : f.tone;

                    f.motion =
                        x->hasProperty("motion")
                            ? static_cast<float>(
                                  x->getProperty("motion"))
                            : f.motion;

                    f.mix =
                        x->hasProperty("mix")
                            ? static_cast<float>(
                                  x->getProperty("mix"))
                            : f.mix;

                    f.shape =
                        x->hasProperty("shape")
                            ? static_cast<float>(
                                  x->getProperty("shape"))
                            : f.shape;

                    p.expertFxChain.push_back(f);
                }
            }
        }

        return true;
    }
}
