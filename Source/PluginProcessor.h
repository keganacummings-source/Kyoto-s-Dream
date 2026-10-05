#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DspEngine.h"
#include "FeatureNames.h"
#include "InstrumentEngine.h"
#include "DreamAPI.h"
#include "ThemeManager.h"
#include <array>
#include <atomic>
#include <map>
#include <string>
#include <vector>

class KyotoSpxritProcessor final : public juce::AudioProcessor {
public:
    KyotoSpxritProcessor(); ~KyotoSpxritProcessor() override = default;
    void prepareToPlay(double,int) override; void releaseResources() override; void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override; juce::AudioProcessorEditor* createEditor() override; bool hasEditor() const override{return true;}
    const juce::String getName() const override;
    bool acceptsMidi() const override { return true; } bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override{return 2.0;} int getNumPrograms() override{return 1;} int getCurrentProgram() override{return 0;} void setCurrentProgram(int) override{} const juce::String getProgramName(int) override{return {};} void changeProgramName(int,const juce::String&) override{}
    void getStateInformation(juce::MemoryBlock&) override; void setStateInformation(const void*,int) override;

    juce::AudioProcessorValueTreeState state;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParams();

    std::vector<juce::String> installedModules() const;
    void addModule(const juce::String& id);
    void saveModule(const juce::String& name);
    bool loadModule(const juce::String& id);
    bool hasModules() const;
    juce::String activeModule() const;
    void setActiveModule(const juce::String&);
    kyoto::InstrumentPreset getInstrumentPreset() const;
    void setInstrumentPreset(const kyoto::InstrumentPreset&);
    void savePreset(const juce::String&name);

    int fxIndex(int slot) const; void setFxIndex(int slot,int idx);
    float fxParam(int slot,int which) const; void setFxParam(int slot,int which,float value);
    std::vector<kyoto::FxSlot> fxChain() const; void setFxChain(const std::vector<kyoto::FxSlot>&); void addFxSlot(int effect=0); void removeFxSlot(size_t index); void setFxChainSlot(size_t index,const kyoto::FxSlot&);

    kyoto::DreamAPI& api(){return dreamApi;} const juce::String& token()const{return dreamToken;} const juce::String& user()const{return dreamUser;} bool loggedIn()const{return dreamToken.isNotEmpty();}
    void setSession(const kyoto::DreamSession&s); void logout();

    const kyoto::ThemePalette& theme() const { return activeTheme; }
    const std::vector<kyoto::ThemePalette>& themes() const { return themeManager.themes(); }
    void setTheme(const juce::String& id);
    juce::String themeId() const { return dreamTheme; }
    void setUiVariant(kyoto::UiVariant v); kyoto::UiVariant uiVariant() const { return uiMode; }
    const kyoto::BuilderLayout& uiLayout() const { return builderLayout; } void setUiLayout(const kyoto::BuilderLayout& l) { builderLayout=l; }
    void setExpertMode(bool b) { expertMode=b; } bool isExpertMode() const { return expertMode; }
    void addCustomTheme(const kyoto::ThemePalette& t);
    const std::vector<kyoto::ThemePalette>& customThemes() const { return userThemes; }

    static constexpr int fxSlots=8;
    static constexpr const char* version="0.2.0";
private:
    static constexpr bool isFxBuild =
    #if defined(KYOTO_IS_FX)
        true;
    #else
        false;
    #endif
    dm::DspEngine fx; kyoto::InstrumentEngine synth; kyoto::DreamAPI dreamApi; kyoto::ThemeManager themeManager;
    kyoto::ThemePalette activeTheme; kyoto::BuilderLayout builderLayout=kyoto::defaultLayout(); kyoto::UiVariant uiMode=kyoto::UiVariant::Classic; bool expertMode=false; std::vector<kyoto::ThemePalette> userThemes;
    std::map<std::string,kyoto::InstrumentPreset> modulePresets;
    std::array<std::atomic<float>*,dm::DspEngine::effectCount> enabled{}; std::array<std::atomic<float>*,dm::DspEngine::effectCount> amount{},tone{},motion{},mix{},shape{};
    std::array<std::atomic<float>*,fxSlots> fxSelect{};
    std::vector<kyoto::FxSlot> expertFxChain;
    juce::StringArray modules; juce::String currentModule=""; juce::String dreamToken,dreamUser,dreamRole,dreamTheme;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KyotoSpxritProcessor)
};
