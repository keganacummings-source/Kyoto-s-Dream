#pragma once
#include "PluginProcessor.h"

class KyotoAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit KyotoAudioProcessorEditor(KyotoAudioProcessor&);
    ~KyotoAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void showTab(int tab);
    void rebuildKnobs();
    void addEffect();
    void saveLocal();
    void publish();
    void login();
    void sendChat();
    void refreshFeed();
    void refreshCatalog();
    void loadNamed(const juce::String& name);
    juce::File sessionFile() const;
    juce::File moduleDir() const;

    KyotoAudioProcessor& proc;
    int tab = 0;
    juce::String token, account;
    juce::TextButton shareBtn { "DREAMSHARE" }, buildBtn { "BUILD" }, saveBtn { "SAVE" }, upBtn { "UPLOAD" };
    juce::TextButton loginBtn { "LOGIN" }, sendBtn { "SEND" }, addBtn { "ADD FX" }, feedBtn { "FEED" }, catBtn { "CATALOG" };
    juce::TextEditor userBox, passBox, msgBox, nameBox, logBox;
    juce::ComboBox gridBox, fxBox, localBox, remoteBox;
    juce::Label status;
    juce::Component panel;

    struct Knob : juce::Component
    {
        Knob(KyotoAudioProcessor&, juce::ValueTree);
        void resized() override;
        void mouseDrag(const juce::MouseEvent&) override;
        KyotoAudioProcessor& proc;
        juce::ValueTree node;
        juce::Slider slider;
        juce::Label caption;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    juce::OwnedArray<Knob> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KyotoAudioProcessorEditor)
};
