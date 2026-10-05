#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>
#include <atomic>
#include "BuilderModel.h"

namespace kyoto {
struct FxSlot { int effect=0; float amount=0.65f, tone=0.5f, motion=0.5f, mix=0.65f, shape=0.5f; };

struct InstrumentPreset {
    juce::String id = "custom";
    juce::String name = "Kyoto Init";
    int osc1 = 0, osc2 = 3, osc3 = 1;
    float mix1 = 0.8f, mix2 = 0.35f, mix3 = 0.2f;
    float detune2 = 7.0f, detune3 = -7.0f;
    float attack = 0.01f, decay = 0.18f, sustain = 0.72f, release = 0.25f;
    float cutoff = 0.72f, resonance = 0.15f, drive = 0.0f, noise = 0.0f;
    float lfoRate = 4.0f, lfoDepth = 0.0f;
    int octave = 0;
    std::array<int, 8> fx{};
    std::array<float, 8> fxAmount{};
    std::vector<FxSlot> expertFxChain;
    bool arp = false;
    float arpRate = 8.0f;
    juce::var uiLayout;
    juce::var uiTheme;
};

class InstrumentEngine {
public:
    static constexpr int maxVoices = 32;
    InstrumentEngine();
    void prepare(double sampleRate, int maxBlock);
    void reset();
    void setPreset(const InstrumentPreset& p);
    const InstrumentPreset& preset() const { return current; }
    void render(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);
    void noteOn(int midi, float velocity);
    void noteOff(int midi);
    void allNotesOff();
private:
    struct Voice {
        bool active=false, releasing=false;
        int note=-1;
        float velocity=0.0f;
        double phase[3]{}, age=0.0;
        float env=0.0f;
        int stage=0;
        float releaseStart=0.0f;
    };
    static float osc(float phase, int type);
    static float midiHz(int note);
    float nextEnvelope(Voice& v);
    void renderSample(float& l, float& r);
    double sr=44100.0;
    int maxBlock=1024;
    std::array<Voice,maxVoices> voices{};
    InstrumentPreset current{};
    juce::Random random;
};
}
