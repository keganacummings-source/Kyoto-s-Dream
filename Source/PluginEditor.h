#pragma once
#include "PluginProcessor.h"
#include "Themes.h"

struct CanvasWidget : public juce::Component
{
    enum class Kind { Dial, Slider, Key, Wave, Stack };

    CanvasWidget(KyotoAudioProcessor& p, juce::ValueTree n);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    KyotoAudioProcessor& proc;
    juce::ValueTree node;
    Kind kind = Kind::Dial;
    juce::Slider slider;
    juce::Label caption;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

class FxBrowser : public juce::Component, private juce::ListBoxModel
{
public:
    FxBrowser(KyotoAudioProcessor& p);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void setTheme(const kt::ThemePalette& t);
    int getSelectedFx() const { return selected; }
    std::function<void(int)> onSelect;

private:
    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent&) override;
    void selectedRowsChanged(int) override;

    KyotoAudioProcessor& proc;
    juce::ListBox list;
    juce::TextEditor search;
    juce::String filter;
    juce::Array<int> filtered;
    int selected = 0;
    kt::ThemePalette theme = kt::kThemes[0];
    void rebuildFilter();
};

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
    void rebuildCanvas();
    void addSeriesStep();
    void reflowSeries();
    juce::Rectangle<int> cellFor(int index, const juce::String& kind) const;
    void saveLocal();
    void publish();
    void login();
    void logout();
    void sendChat();
    void refreshFeed();
    void chatUtility(const juce::String& action);
    void refreshCatalog();
    void deleteCatalogId(const juce::String& id);
    void loadCatalogId(const juce::String& id, const juce::String& name);
    void applyTheme(const juce::String& id);
    void loadWav();
    void saveEffect();
    void publishEffect();
    void refreshEffectBox();
    juce::File sessionFile() const;
    juce::File moduleDir() const;
    juce::File effectDir() const;
    void setLoggedIn(bool on);

    KyotoAudioProcessor& proc;
    int tab = 0;
    bool loggedIn = false;
    bool isAdmin = false;
    float animPhase = 0.f;
    juce::String token, account;
    kt::ThemePalette theme = kt::kThemes[0];

    juce::TextButton shareBtn { "DREAMSHARE" }, chainBtn { "CHAIN" }, fxBtn { "FX BUILDER" }, logoutBtn { "LOG OUT" };
    juce::TextButton chatRefreshBtn { "CHAT" }, threadsBtn { "THREADS" }, socialBtn { "FRIENDS" }, dmBtn { "DM" }, adminDeleteBtn { "REMOVE" }, utilityGoBtn { "GO" };
    juce::TextEditor utilityBox;
    juce::ComboBox utilityActionBox;
    juce::Label status, whoLabel;

    juce::TextEditor userBox, passBox, msgBox, logBox;
    juce::TextButton loginBtn { "LOG IN" }, sendBtn { "SEND" }, feedBtn { "REFRESH" };
    juce::ComboBox themeBox;
    juce::Viewport catalogView;
    juce::Component catalogHolder;

    juce::TextEditor nameBox, effectNameBox;
    juce::ComboBox gridStyleBox, localBox, kindBox, effectBox;
    juce::TextButton addBtn { "ADD NEXT" }, saveBtn { "SAVE" }, upBtn { "UPLOAD" }, wavBtn { "LOAD WAV" };
    juce::TextButton fxAddBtn { "STACK FX" }, fxSaveBtn { "SAVE EFFECT" }, fxUpBtn { "UPLOAD EFFECT" };
    juce::Slider fxAmount;
    juce::Label stackLabel;
    juce::Component panel;
    std::unique_ptr<FxBrowser> fxBrowser;
    juce::OwnedArray<CanvasWidget> widgets;
    juce::ValueTree fxStack { "fxstack" };

    struct CatalogItem { juce::String id, name, face, author; };
    juce::String selectedCatalogId;
    juce::Array<CatalogItem> catalog;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KyotoAudioProcessorEditor)
};
