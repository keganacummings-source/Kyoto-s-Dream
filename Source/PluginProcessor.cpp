#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FxCatalog.h"
#include <cmath>

namespace
{
juce::String slotId(int i, const char* tail)
{
    return "s" + juce::String(i + 1).paddedLeft('0', 2) + tail;
}
juce::String slotName(int i, const char* label)
{
    return "Slot " + juce::String(i + 1).paddedLeft('0', 2) + " " + label;
}
float param(juce::AudioProcessorValueTreeState& tree, const juce::String& id)
{
    if (auto* p = tree.getRawParameterValue(id))
        return p->load();
    return 0.f;
}
}

KyotoAudioProcessor::KyotoAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout(isFx()))
{
    uiState.setProperty("grid", 0, nullptr);
    uiState.setProperty("free", 0, nullptr);
    uiState.setProperty("name", "untitled", nullptr);
    uiState.setProperty("theme", "trippah", nullptr);
}

KyotoAudioProcessor::~KyotoAudioProcessor() = default;

bool KyotoAudioProcessor::isFx() const
{
#if KYOTO_IS_FX
    return true;
#else
    return false;
#endif
}

const juce::String KyotoAudioProcessor::getName() const
{
    return isFx() ? "KYOTRIPPAH FX" : "KYOTO";
}

bool KyotoAudioProcessor::acceptsMidi() const { return ! isFx(); }

juce::AudioProcessorValueTreeState::ParameterLayout KyotoAudioProcessor::createLayout(bool fx)
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    if (! fx)
    {
        layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID { "osc", 1 }, "Oscillator", 0, 2, 0));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "cutoff", 1 }, "Cutoff", 0.f, 1.f, 0.62f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "res", 1 }, "Resonance", 0.f, 1.f, 0.18f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "attack", 1 }, "Attack", 0.f, 1.f, 0.05f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "decay", 1 }, "Decay", 0.f, 1.f, 0.28f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "sustain", 1 }, "Sustain", 0.f, 1.f, 0.65f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "release", 1 }, "Release", 0.f, 1.f, 0.35f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "sub", 1 }, "Sub Oscillator", 0.f, 1.f, 0.22f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "noise", 1 }, "Noise", 0.f, 1.f, 0.f));
    }
    const int n = fx ? 12 : 8;
    for (int i = 0; i < n; ++i)
    {
        layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { slotId(i, "on"), 1 }, slotName(i, "On"), false));
        layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID { slotId(i, "type"), 1 }, slotName(i, "Type"), 0, kt::kFxCount - 1, 0));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { slotId(i, "amt"), 1 }, slotName(i, "Amount"), 0.f, 1.f, 0.45f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { slotId(i, "tone"), 1 }, slotName(i, "Tone"), 0.f, 1.f, 0.5f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { slotId(i, "mot"), 1 }, slotName(i, "Motion"), 0.f, 1.f, 0.35f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { slotId(i, "mix"), 1 }, slotName(i, "Mix"), 0.f, 1.f, 0.4f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { slotId(i, "shp"), 1 }, slotName(i, "Shape"), 0.f, 1.f, 0.5f));
    }
    return layout;
}

void KyotoAudioProcessor::prepareToPlay(double sampleRate, int)
{
    sampleRateHz = sampleRate > 0.0 ? sampleRate : 44100.0;
    const int delayN = juce::jmax(64, (int) sampleRateHz);
    for (auto& s : slotDsp)
    {
        s = {};
        s.delay[0].assign((size_t) delayN, 0.f);
        s.delay[1].assign((size_t) delayN, 0.f);
    }
    for (auto& v : voices)
        v = {};
}

void KyotoAudioProcessor::releaseResources() {}

bool KyotoAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    const auto in = layouts.getMainInputChannelSet();
    if (in.isDisabled())
        return true;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void KyotoAudioProcessor::noteOn(int note, float vel)
{
    Voice* slot = nullptr;
    for (auto& v : voices)
        if (! v.on || v.note == note)
            slot = &v;
    if (slot == nullptr)
        slot = &voices[0];
    slot->on = true;
    slot->note = note;
    slot->vel = juce::jlimit(0.05f, 1.f, vel);
    slot->stage = 0;
    slot->env = 0.f;
    slot->phase = 0.f;
    slot->sub = 0.f;
}

void KyotoAudioProcessor::noteOff(int note)
{
    for (auto& v : voices)
        if (v.on && v.note == note && v.stage < 3)
            v.stage = 3;
}

float KyotoAudioProcessor::renderVoice(Voice& v)
{
    if (! v.on)
        return 0.f;
    const float atk = 0.001f + param(apvts, "attack") * 0.8f;
    const float dec = 0.01f + param(apvts, "decay") * 1.2f;
    const float sus = param(apvts, "sustain");
    const float rel = 0.02f + param(apvts, "release") * 1.5f;
    const float dt = 1.f / (float) sampleRateHz;
    if (v.stage == 0)
    {
        v.env += dt / atk;
        if (v.env >= 1.f) { v.env = 1.f; v.stage = 1; }
    }
    else if (v.stage == 1)
    {
        v.env -= dt / dec * (1.f - sus);
        if (v.env <= sus) { v.env = sus; v.stage = 2; }
    }
    else if (v.stage == 3)
    {
        v.env -= dt / rel;
        if (v.env <= 0.f) { v.env = 0.f; v.on = false; return 0.f; }
    }
    const float freq = 440.f * std::pow(2.f, (v.note - 69) / 12.f);
    const float inc = freq / (float) sampleRateHz;
    v.phase += inc;
    if (v.phase >= 1.f) v.phase -= 1.f;
    v.sub += inc * 0.5f;
    if (v.sub >= 1.f) v.sub -= 1.f;
    const int osc = (int) param(apvts, "osc");
    float wave = v.phase * 2.f - 1.f;
    if (osc == 1) wave = v.phase < 0.5f ? 1.f : -1.f;
    if (osc == 2) wave = std::sin(v.phase * juce::MathConstants<float>::twoPi);
    const float sub = std::sin(v.sub * juce::MathConstants<float>::twoPi) * param(apvts, "sub");
    const float noise = (juce::Random::getSystemRandom().nextFloat() * 2.f - 1.f) * param(apvts, "noise");
    return (wave * 0.35f + sub * 0.25f + noise * 0.15f) * v.env * v.vel;
}

float KyotoAudioProcessor::applySlot(int slot, int ch, float x)
{
    if (param(apvts, slotId(slot, "on")) < 0.5f)
        return x;
    const int type = juce::jlimit(0, kt::kFxCount - 1, (int) std::lround(param(apvts, slotId(slot, "type"))));
    const int fam = kt::kFx[type].family;
    const float amount = param(apvts, slotId(slot, "amt"));
    const float tone = param(apvts, slotId(slot, "tone"));
    const float motion = param(apvts, slotId(slot, "mot"));
    const float mix = param(apvts, slotId(slot, "mix"));
    const float shape = param(apvts, slotId(slot, "shp"));
    auto& d = slotDsp[slot];
    float wet = x;
    if (fam == 4)
    {
        const float c = 0.02f + tone * 0.4f;
        d.lp[ch] += c * (x - d.lp[ch]);
        d.hp[ch] += c * (x - d.hp[ch]);
        const float hp = x - d.hp[ch];
        wet = (type % 2 == 0) ? d.lp[ch] : hp;
    }
    else if (fam == 0 || fam == 1)
    {
        const int n = (int) d.delay[ch].size();
        const int taps = juce::jmax(1, (int) ((0.03f + motion * (fam == 1 ? 0.45f : 0.25f)) * (float) sampleRateHz));
        const int r = (d.w + n - (taps % n)) % n;
        const float fb = juce::jlimit(0.f, 0.82f, amount * (fam == 1 ? 0.75f : 0.65f));
        wet = d.delay[ch][(size_t) r];
        d.delay[ch][(size_t) d.w] = x + wet * fb;
        if (ch == 0)
            d.w = (d.w + 1) % n;
    }
    else if (fam == 2 || fam == 3)
    {
        d.lfo += (0.05f + motion * 6.f) / (float) sampleRateHz;
        if (d.lfo > 1.f) d.lfo -= 1.f;
        const float l = std::sin(d.lfo * juce::MathConstants<float>::twoPi);
        if (fam == 2)
            wet = ch == 0 ? x * (1.f + l * amount * 0.5f) : x * (1.f - l * amount * 0.5f);
        else
        {
            const float c = 0.05f + tone * 0.3f;
            d.bp[ch] += c * ((x * (0.5f + 0.5f * l)) - d.bp[ch]);
            wet = d.bp[ch];
        }
    }
    else if (fam == 6)
    {
        const float thr = 0.15f + (1.f - amount) * 0.8f;
        const float over = std::abs(x) - thr;
        wet = over > 0.f ? std::copysign(thr + over / (1.f + shape * 6.f), x) : x;
    }
    else
    {
        const float k = 1.f + amount * (2.f + shape * 10.f);
        wet = std::tanh(x * k);
        d.lp[ch] += (0.05f + tone * 0.4f) * (wet - d.lp[ch]);
        wet = d.lp[ch];
    }
    float y = x * (1.f - mix * 0.85f) + wet * (0.15f + mix * 0.85f);
    if (! std::isfinite(y))
        y = 0.f;
    return y;
}

void KyotoAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    if (! isFx())
    {
        for (const auto meta : midi)
        {
            const auto msg = meta.getMessage();
            if (msg.isNoteOn()) noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
            else if (msg.isNoteOff()) noteOff(msg.getNoteNumber());
        }
    }
    const int nIn = getTotalNumInputChannels();
    const int nOut = getTotalNumOutputChannels();
    const int n = buffer.getNumSamples();
    for (int i = 0; i < n; ++i)
    {
        float synth = 0.f;
        if (! isFx())
        {
            for (auto& v : voices)
                synth += renderVoice(v);
            const float cut = 0.02f + param(apvts, "cutoff") * 0.5f;
            slotDsp[0].hp[0] += cut * (synth - slotDsp[0].hp[0]);
            synth = slotDsp[0].hp[0];
        }
        for (int ch = 0; ch < nOut; ++ch)
        {
            float x = synth;
            if (ch < nIn)
                x += buffer.getReadPointer(ch)[i];
            for (int s = 0; s < slotCount(); ++s)
                x = applySlot(s, juce::jmin(ch, 1), x);
            if (std::abs(x) > 0.98f)
                x = std::copysign(0.98f, x);
            buffer.getWritePointer(ch)[i] = x;
        }
    }
}

void KyotoAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
    {
        if (auto ui = uiState.createXml())
            xml->addChildElement(ui.release());
        copyXmlToBinary(*xml, dest);
    }
}

void KyotoAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
    {
        auto tree = juce::ValueTree::fromXml(*xml);
        auto ui = tree.getChildWithName("ui");
        if (ui.isValid())
        {
            uiState = ui;
            tree.removeChild(ui, nullptr);
        }
        apvts.replaceState(tree);
    }
}

juce::AudioProcessorEditor* KyotoAudioProcessor::createEditor()
{
    return new KyotoAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KyotoAudioProcessor();
}
