#pragma once
#include "PluginProcessor.h"
#include "Themes.h"

// ---------------------------------------------------------------------------
// Visual widget that lives on the Builder canvas (dial / slider / key / wave)
// ---------------------------------------------------------------------------
struct CanvasWidget : public juce::Component
{
    enum class Kind { Dial, Slider, Key, Wave };

    CanvasWidget(KyotoAudioProcessor& p, juce::ValueTree n);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    KyotoAudioProcessor& proc;
    juce::ValueTree node;
    Kind kind = Kind::Dial;
    juce::Slider slider;
    juce::Label caption;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    bool dragging = false;
};

// ---------------------------------------------------------------------------
// Scrollable FX browser with hover-preview at half intensity
// ---------------------------------------------------------------------------
class FxBrowser : public juce::Component, private juce::ListBoxModel
{
public:
    FxBrowser(KyotoAudioProcessor& p);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void setTheme(const kt::ThemePalette& t);
    int getSelectedFx() const { return selected; }
    std::function<void(int)> onSelect;
    std::function<void(int)> onHover;   // called with fx index, or -1 when leave
    std::function<void()> onLeave;

private:
    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent&) override;
    void selectedRowsChanged(int) override;

    KyotoAudioProcessor& proc;
    juce::ListBox list;
    juce::TextEditor search;
    juce::String filter;
    juce::Array<int> filtered;   // indices into kt::kFx
    int selected = 0;
    int hoverRow = -1;
    kt::ThemePalette theme = kt::kThemes[0];

    void rebuildFilter();
};

// ---------------------------------------------------------------------------
// Main editor
// ---------------------------------------------------------------------------
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
    void addSelectedEffect();
    void saveLocal();
    void publish();
    void login();
    void sendChat();
    void refreshFeed();
    void refreshCatalog();
    void loadNamed(const juce::String& name);
    void applyTheme(const juce::String& id);
    void highlightPegsForPlacement(bool on);
    juce::Rectangle<int> findFreePeg(int w, int h) const;
    bool collides(const juce::Rectangle<int>& r) const;
    juce::File sessionFile() const;
    juce::File moduleDir() const;

    KyotoAudioProcessor& proc;
    int tab = 0;                     // 0 = DreamShare, 1 = Builder
    juce::String token, account;
    kt::ThemePalette theme = kt::kThemes[0];

    // Top chrome
    juce::TextButton shareBtn { "DREAMSHARE" }, buildBtn { "BUILDER" };
    juce::Label status;

    // DreamShare tab
    juce::TextEditor userBox, passBox, msgBox, logBox;
    juce::TextButton loginBtn { "LOGIN" }, sendBtn { "SEND" }, feedBtn { "REFRESH" };
    juce::ComboBox themeBox;
    juce::TextButton catBtn { "CATALOG" };
    juce::ComboBox remoteBox;

    // Builder tab
    juce::TextEditor nameBox;
    juce::ComboBox gridStyleBox, localBox, kindBox;
    juce::TextButton addBtn { "PLACE" }, saveBtn { "SAVE" }, upBtn { "UPLOAD" };
    juce::Component panel;           // canvas
    std::unique_ptr<FxBrowser> fxBrowser;
    juce::OwnedArray<CanvasWidget> widgets;

    // Peg grid state for placement
    bool placementMode = false;
    int pendingFx = -1;
    int pendingKind = 0;             // 0=dial,1=slider,2=key,3=wave
    static constexpr int pegCols = 12;
    static constexpr int pegRows = 8;
    static constexpr int pegSize = 72;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KyotoAudioProcessorEditor)
};
