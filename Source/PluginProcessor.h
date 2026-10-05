#pragma once
#include <JuceHeader.h>

class KyotoAudioProcessor : public juce::AudioProcessor
{
public:
    KyotoAudioProcessor();
    ~KyotoAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool isFx() const;
    int slotCount() const { return isFx() ? 12 : 8; }

    juce::AudioProcessorValueTreeState apvts;
    juce::ValueTree uiState { "ui" };

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout(bool fx);

private:
    struct Voice
    {
        bool on = false;
        int note = 60;
        float phase = 0.f, sub = 0.f, env = 0.f, vel = 0.8f;
        int stage = 3;
    };
    struct SlotDsp
    {
        float lp[2] {}, hp[2] {}, bp[2] {}, lfo = 0.f;
        std::vector<float> delay[2];
        int w = 0;
    };

    void noteOn(int note, float vel);
    void noteOff(int note);
    float renderVoice(Voice& v);
    float applySlot(int slot, int ch, float x);

    Voice voices[8];
    SlotDsp slotDsp[12];
    double sampleRateHz = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KyotoAudioProcessor)
};
