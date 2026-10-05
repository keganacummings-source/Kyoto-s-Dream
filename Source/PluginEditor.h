#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "BuilderModel.h"
#include <memory>
#include <vector>

class KyotoSpxritEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit KyotoSpxritEditor(KyotoSpxritProcessor&); ~KyotoSpxritEditor() override;
    void paint(juce::Graphics&) override; void resized() override;
private:
    KyotoSpxritProcessor& proc;
    enum Page { Home, Selector, Builder, Community, Themes }; Page page=Home;
    juce::TextButton home{"HOME"},selector{"PLUGINS"},builder{"BUILD"},community{"COMMUNITY"},themesPage{"THEMES"};
    juce::TextButton login{"LOGIN / REGISTER"},logout{"LOG OUT"},update{"CHECK UPDATE"},refresh{"REFRESH"};
    juce::TextButton post{"POST"},postWav{"POST WAV"},postImage{"POST IMAGE"};
    juce::TextButton saveModule{"SAVE"},uploadCommunity{"UPLOAD COMMUNITY"},loadCommunity{"LOAD SELECTED"};
    juce::ToggleButton expert{"EXPERT MODE"};
    juce::ComboBox themeBox,variantBox,gridBox,screenBox,elementBox;
    juce::Label title,status,modeLabel,lockLabel,moduleName,themeHint,screenHint;
    juce::TextEditor user,pass,postText,saveName,themeName,themeAccent,elementText;
    juce::ListBox threads,modules,communityList;
    std::unique_ptr<juce::ListBoxModel> threadModel,moduleModel,communityModel;
    struct FxRow;
    std::vector<FxRow*> fxRows;
    std::unique_ptr<juce::Component> fxContent;
    juce::Viewport fxViewport;
    juce::TextButton addFx{"+ ADD FX"};
    juce::Slider oscMix1,oscMix2,oscMix3,detune2,detune3,cutoff,resonance,attack,decay,sustain,release,noise,drive,lfoRate,lfoDepth,octave;
    juce::Slider macroTone,macroPunch,macroSpace,macroMovement,macroWidth;
    juce::TextButton addDial{"+ DIAL"},addSlider{"+ SLIDER"},addScreen{"+ WAVE SCREEN"},addText{"+ TEXT"},deleteElement{"DELETE ELEMENT"};
    juce::ComboBox themeElementBox;
    juce::ToggleButton showGrid{"SHOW GRID"};
    class BuilderCanvas;
    std::unique_ptr<BuilderCanvas> canvas;
    juce::TextButton makeTheme{"MAKE / SAVE CUSTOM THEME"};
    std::vector<kyoto::DreamThread> feed; std::vector<kyoto::CommunityInstrument> communityItems; int online=0;
    bool isFxBuild() const;
    void show(Page); void refreshFeed(); void doLogin(); void postPlain(); void postFile(bool image); void saveBuilder(); void uploadCurrent(); void refreshCommunity(); void loadSelectedCommunity(); void checkUpdate();
    void populateThemes(); void applyThemeSelection(); void createTheme(); void applyVariant(); void setExpert(bool);
    void rebuildFxRows(); void addFxToChain(); void syncFxFromProcessor(); void syncProcessorFromBuilder(); void syncBuilderFromProcessor();
    void addElement(kyoto::BuilderElementType); void removeElement(); void elementChanged(); void selectElement(int); void updateElementText(); void applyGrid(); void applyScreen(); void ensureCustomBuilderTheme();
    void timerCallback() override;
    void style(juce::Button&); void label(juce::Label&,const juce::String&,float=12.0f);
    class FeedModel; class ModuleModel; class CommunityModel;
    juce::String elementTypeName(kyoto::BuilderElementType) const;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KyotoSpxritEditor)
};
