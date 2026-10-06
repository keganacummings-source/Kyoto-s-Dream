#include "PluginEditor.h"
#include "DreamApi.h"
#include "FxCatalog.h"
#include <thread>
#include <array>
#include <algorithm>
#include <initializer_list>

namespace
{
static juce::var propertyOr(const juce::DynamicObject* object,
                            const juce::Identifier& propertyName,
                            const juce::var& fallback)
{
    return object != nullptr && object->hasProperty(propertyName)
        ? object->getProperty(propertyName)
        : fallback;
}

class KyotoLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void setTheme(const kt::ThemePalette& t) { theme = t; }

    juce::Font getTextButtonFont(juce::TextButton& button, int buttonHeight) override
    {
        return kt::font(theme, buttonHeight < 30 ? 12.5f : 13.5f, true);
    }

    juce::Font getComboBoxFont(juce::ComboBox&) override
    {
        return kt::font(theme, 13.0f);
    }

    juce::Font getLabelFont(juce::Label&) override
    {
        return kt::font(theme, 12.0f);
    }

    juce::Font getPopupMenuFont() override
    {
        return kt::font(theme, 13.0f);
    }

    juce::Font getSliderPopupFont(juce::Slider&) override
    {
        return kt::font(theme, 12.0f);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&, bool isMouseOver, bool isButtonDown) override
    {
        auto r = button.getLocalBounds().toFloat().reduced(0.8f);
        auto fill = kt::c(theme.panel).brighter(isMouseOver ? 0.13f : 0.06f);
        if (button.getToggleState()) fill = kt::c(theme.accent).withAlpha(isButtonDown ? 0.38f : 0.25f);
        if (isButtonDown) fill = fill.brighter(0.10f);
        g.setColour(fill);
        g.fillRoundedRectangle(r, 8.f);
        g.setColour(kt::c(theme.border).withAlpha(0.95f));
        g.drawRoundedRectangle(r, 8.f, 1.f);
        if (button.getToggleState())
        {
            g.setColour(kt::c(theme.accent).withAlpha(0.75f));
            g.drawRoundedRectangle(r.reduced(2.f), 6.f, 1.f);
        }
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool, bool) override
    {
        g.setColour(kt::c(theme.text));
        g.setFont(kt::font(theme, button.getHeight() < 30 ? 12.5f : 13.5f, true));
        g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(6, 2), juce::Justification::centred, 1);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float>(0.5f, 0.5f, (float)width - 1.f, (float)height - 1.f);
        g.setColour(kt::c(theme.panel).brighter(0.05f));
        g.fillRoundedRectangle(r, 7.f);
        g.setColour(kt::c(theme.border));
        g.drawRoundedRectangle(r, 7.f, 1.f);
        g.setColour(kt::c(theme.text));
        g.setFont(kt::font(theme, 13.f));
        g.drawText(box.getText(), 10, 0, width - 30, height, juce::Justification::centredLeft);
        g.setColour(kt::c(theme.accent));
        juce::Path p;
        p.startNewSubPath((float)width - 17.f, height * 0.42f);
        p.lineTo((float)width - 11.f, height * 0.42f);
        p.lineTo((float)width - 14.f, height * 0.62f);
        p.closeSubPath();
        g.fillPath(p);
    }

    void drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& editor) override
    {
        g.setColour(editor.hasKeyboardFocus(true) ? kt::c(theme.accent).withAlpha(0.9f) : kt::c(theme.border));
        g.drawRoundedRectangle(0.5f, 0.5f, (float)width - 1.f, (float)height - 1.f, 7.f, 1.f);
    }

private:
    kt::ThemePalette theme = kt::kThemes[0];
};

static KyotoLookAndFeel kLookAndFeel;

juce::String effectTokenFor(const juce::String& id)
{
    return "[KYOTRIPPAH_EFFECT:" + id + "]";
}

juce::String nativeKey(const juce::String& web)
{
    if (web == "amount") return "amt";
    if (web == "motion") return "mot";
    if (web == "shape") return "shp";
    return web;
}

void drawThemeSprite(juce::Graphics& g, const kt::ThemePalette& t, const juce::String& id, float x, float y, float scale, float phase)
{
    auto a = kt::c(t.accent).withAlpha(0.10f);
    g.setColour(a);
    auto S=[&](float v){return v*scale;};
    if (id=="trippah") { g.fillEllipse(x-S(10),y-S(22),S(20),S(30)); g.fillEllipse(x-S(30),y-S(8),S(24),S(14)); g.fillEllipse(x+S(6),y-S(8),S(24),S(14)); g.fillRect(x-S(3),y+S(8),S(6),S(22)); }
    else if (id=="goonr") { for(int i=0;i<5;i++){float yy=y+S((i-2)*12); g.drawLine(x-S(24),yy,x+S(24),yy,1.2f); g.drawLine(x+S((i-2)*9),y-S(24),x+S((i-2)*9),y+S(24),1.0f);} }
    else if (id=="abyss"||id=="cobalt") { g.drawEllipse(x-S(24),y-S(24),S(48),S(48),2.f); g.drawEllipse(x-S(9),y-S(9),S(18),S(18),2.f); g.fillEllipse(x+std::cos(phase)*S(28)-S(4),y+std::sin(phase)*S(28)-S(4),S(8),S(8)); }
    else if (id=="amber"||id=="honey") { for(int i=0;i<6;i++){float ang=i*1.047f+phase*.15f; g.drawLine(x,y,x+std::cos(ang)*S(28),y+std::sin(ang)*S(28),1.5f);} g.drawEllipse(x-S(10),y-S(10),S(20),S(20),2.f); }
    else if (id=="bloodmoon"||id=="wine") { g.drawEllipse(x-S(20),y-S(20),S(40),S(40),3.f); g.fillEllipse(x-S(7),y-S(24),S(28),S(28)); }
    else if (id=="ember"||id=="rust") { juce::Path p; p.startNewSubPath(x,y+S(24)); p.quadraticTo(x-S(28),y,x,y-S(28)); p.quadraticTo(x+S(22),y-S(4),x,y+S(24)); g.strokePath(p,juce::PathStrokeType(3.f)); }
    else if (id=="fog"||id=="lagoon") { for(int i=0;i<3;i++) g.drawEllipse(x-S(28-i*8),y+S(i*8),S(40),S(18),2.f); }
    else if (id=="graphite"||id=="steel") { for(int i=-2;i<=2;i++){g.drawLine(x-S(26),y+S(i*12),x+S(26),y+S(i*12),1.f);g.drawLine(x+S(i*12),y-S(26),x+S(i*12),y+S(26),1.f);} }
    else if (id=="ice") { juce::Path p; for(int i=0;i<6;i++){float ang=i*1.047f; auto q=juce::Point<float>(x+std::cos(ang)*S(25),y+std::sin(ang)*S(25)); if(i==0)p.startNewSubPath(q);else p.lineTo(q);} p.closeSubPath(); g.strokePath(p,juce::PathStrokeType(2.f)); }
    else if (id=="ink"||id=="void") { g.drawEllipse(x-S(22),y-S(14),S(44),S(28),2.5f); g.fillEllipse(x-S(6),y-S(6),S(12),S(12)); }
    else if (id=="lilac"||id=="plum") { for(int i=0;i<5;i++){float ang=i*1.256f+phase*.1f; g.fillEllipse(x+std::cos(ang)*S(18)-S(7),y+std::sin(ang)*S(18)-S(12),S(14),S(24));} }
    else if (id=="mint"||id=="pine") { g.drawLine(x-S(22),y+S(24),x+S(22),y-S(24),2.f); for(int i=0;i<5;i++){float yy=y+S(18-i*9);g.drawLine(x+S((i%2?-1:1)*4),yy,x+S((i%2?-1:1)*22),yy-S(10),1.5f);} }
    else if (id=="neon") { for(int i=1;i<=3;i++) g.drawEllipse(x-S(10*i),y-S(10*i),S(20*i),S(20*i),1.8f); }
    else { for(int i=0;i<8;i++){float ang=i*0.785f+phase*.08f; g.fillEllipse(x+std::cos(ang)*S(24)-S(3),y+std::sin(ang)*S(24)-S(3),S(6),S(6));} }
}
}

CanvasWidget::CanvasWidget(KyotoAudioProcessor& p, juce::ValueTree n)
    : proc(p), node(std::move(n))
{
    const auto k = node.getProperty("kind").toString();
    if (k == "slider") kind = Kind::Slider;
    else if (k == "key") kind = Kind::Key;
    else if (k == "wave") kind = Kind::Wave;
    else if (k == "stack") kind = Kind::Stack;
    else if (k == "board") kind = Kind::Board;
    else if (k == "cosmetic") kind = Kind::Cosmetic;
    else kind = Kind::Dial;

    caption.setText(node.getProperty("label").toString(), juce::dontSendNotification);
    caption.setJustificationType(juce::Justification::centred);
    caption.setFont(kt::font(theme, 12.f, true));
    caption.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(caption);

    if (kind == Kind::Wave)
    {
        waveDisplay = std::make_unique<WaveDisplay>(proc);
        waveDisplay->setTheme(theme);
        addAndMakeVisible(*waveDisplay);
    }
    if (kind == Kind::Dial || kind == Kind::Slider)
    {
        const int slot = (int) node.getProperty("slot", -1);
        const auto key = node.getProperty("param").toString();
        auto id = slot < 0 ? key : ("s" + juce::String(slot + 1).paddedLeft('0', 2) + key);
        const auto style = node.getProperty("style").toString();
        slider.setSliderStyle(kind == Kind::Slider
                                   ? (style == "hfader" ? juce::Slider::LinearHorizontal : juce::Slider::LinearVertical)
                                   : juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 14);
        addAndMakeVisible(slider);
        if (proc.apvts.getParameter(id) != nullptr)
            attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, id, slider);
        slider.onDragStart = [this] { if (onSelect) onSelect(); };
    }
}

void CanvasWidget::setTheme(const kt::ThemePalette& t)
{
    theme = t;
    caption.setFont(kt::font(theme, 12.f, true));
    caption.setColour(juce::Label::textColourId, kt::c(theme.text));
    slider.setColour(juce::Slider::rotarySliderFillColourId, kt::c(theme.accent));
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, kt::c(theme.border));
    slider.setColour(juce::Slider::thumbColourId, kt::c(theme.accent));
    slider.setColour(juce::Slider::trackColourId, kt::c(theme.border));
    slider.setColour(juce::Slider::backgroundColourId, kt::c(theme.panel));
    if (waveDisplay) waveDisplay->setTheme(theme);
    repaint();
}

void CanvasWidget::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.f);
    g.setColour(kt::c(theme.bg).withAlpha(0.55f));
    g.fillRoundedRectangle(bounds, theme.cornerRadius);
    g.setColour(selected ? kt::c(theme.accent).withAlpha(0.95f) : kt::c(theme.border).withAlpha(0.85f));
    g.drawRoundedRectangle(bounds, theme.cornerRadius, selected ? 2.f : 1.f);

    if (kind == Kind::Key)
    {
        g.setColour(isMouseButtonDown() ? kt::c(theme.accent) : kt::c(theme.panel).brighter(0.08f));
        g.fillRoundedRectangle(bounds.reduced(6.f), theme.cornerRadius - 2.f);
        g.setColour(kt::c(theme.text));
        g.setFont(kt::font(theme, 13.f, true));
        g.drawText(node.getProperty("label").toString(), bounds, juce::Justification::centred);
    }
    else if (kind == Kind::Board)
    {
        g.setColour(kt::c(theme.accent).withAlpha(0.18f));
        g.fillRoundedRectangle(bounds.reduced(5.f), 8.f);
        g.setColour(kt::c(theme.accent));
        g.setFont(kt::font(theme, 13.f, true));
        g.drawText("MOTHERBOARD", bounds.reduced(8.f).removeFromTop(20.f), juce::Justification::left);
        g.setColour(kt::c(theme.muted));
        g.setFont(kt::font(theme, 11.f));
        g.drawFittedText(node.getProperty("label").toString(), bounds.reduced(8.f).withTrimmedTop(20.f).toNearestInt(), juce::Justification::topLeft, 2);
    }
    else if (kind == Kind::Cosmetic)
    {
        const auto style = node.getProperty("style").toString();
        g.setColour(kt::c(theme.accent).withAlpha(0.35f));
        if (style == "vent")
            for (int i = 0; i < 4; ++i) g.drawLine(bounds.getX()+8, bounds.getY()+10+i*8.f, bounds.getRight()-8, bounds.getY()+10+i*8.f, 1.4f);
        else if (style == "rail")
            g.fillRoundedRectangle(bounds.reduced(bounds.getWidth()*0.35f, 6.f), 3.f);
        else
            g.fillEllipse(bounds.reduced(10.f));
        g.setColour(kt::c(theme.text));
        g.setFont(kt::font(theme, 11.f, true));
        g.drawText(node.getProperty("label").toString(), bounds.removeFromBottom(16.f), juce::Justification::centred);
    }
    else if (kind == Kind::Wave || kind == Kind::Stack)
    {
        g.setColour(kt::c(theme.bg).brighter(0.02f));
        g.fillRoundedRectangle(bounds.reduced(4.f), theme.cornerRadius - 2.f);
        g.setColour(kind == Kind::Stack ? kt::c(theme.accent).brighter(0.10f) : kt::c(theme.accent));
        juce::Path wave;
        const float mid = bounds.getCentreY() - 4.f;
        const float w = bounds.getWidth() - 14.f;
        const auto peaks = node.getProperty("peaks").toString();
        float live[128] {};
        if (peaks.isEmpty())
            proc.copyScope(live, 128);
        wave.startNewSubPath(bounds.getX() + 7.f, mid);
        for (int i = 0; i < 64; ++i)
        {
            float amp = 0.15f;
            if (peaks.isNotEmpty())
            {
                auto parts = juce::StringArray::fromTokens(peaks, ",", {});
                if (parts.size() > 0)
                    amp = parts[i % parts.size()].getFloatValue();
            }
            else
                amp = std::abs(live[(i * 2) % 128]);
            float x = bounds.getX() + 7.f + (w * i / 63.f);
            wave.lineTo(x, mid - amp * (bounds.getHeight() * 0.32f));
        }
        g.strokePath(wave, juce::PathStrokeType(1.6f));
        g.setColour(kt::c(theme.muted));
        g.setFont(kt::font(theme, 11.f));
        g.drawText(kind == Kind::Stack ? "STACKED FX" : "WAV", bounds.removeFromBottom(16).toNearestInt(), juce::Justification::centred);
    }
}

void CanvasWidget::resized()
{
    caption.setBounds(0, 0, getWidth(), 14);
    if (kind == Kind::Dial || kind == Kind::Slider)
        slider.setBounds(4, 14, getWidth() - 8, getHeight() - 18);
    if (waveDisplay)
        waveDisplay->setBounds(4, 14, getWidth() - 8, getHeight() - 30);
}

void CanvasWidget::mouseDown(const juce::MouseEvent&)
{
    if (onSelect) onSelect();
    if (kind == Kind::Key)
    {
        const int note = (int) node.getProperty("note", 60);
        proc.noteOn(note, 0.9f);
        proc.triggerSample();
        repaint();
    }
}

void CanvasWidget::mouseUp(const juce::MouseEvent&)
{
    if (kind == Kind::Key)
    {
        proc.noteOff((int) node.getProperty("note", 60));
        repaint();
    }
}

FxBrowser::FxBrowser(KyotoAudioProcessor& p) : proc(p)
{
    list.setModel(this);
    list.setRowHeight(42);
    list.setOutlineThickness(0);
    addAndMakeVisible(list);
    addAndMakeVisible(search);
    addAndMakeVisible(builtInTab);
    addAndMakeVisible(customTab);
    addAndMakeVisible(familyBox);
    familyBox.addItem("All categories", 1);
    for (int i = 0; i < kt::kFxFamilyCount; ++i)
        familyBox.addItem(kt::kFxFamilyNames[i], i + 2);
    familyBox.setSelectedId(1);
    familyBox.onChange = [this] { rebuildFilter(); };
    search.setTextToShowWhenEmpty("Search effects...", juce::Colours::grey);
    search.onTextChange = [this] { rebuildFilter(); };
    builtInTab.onClick = [this] { showCustom(false); };
    customTab.onClick = [this] { showCustom(true); };
    showCustom(false);
}

void FxBrowser::setTheme(const kt::ThemePalette& t)
{
    theme = t;
    search.setColour(juce::TextEditor::backgroundColourId, kt::c(theme.bg).brighter(0.03f));
    search.setColour(juce::TextEditor::textColourId, kt::c(theme.text));
    search.setColour(juce::TextEditor::outlineColourId, kt::c(theme.border));
    search.setColour(juce::TextEditor::focusedOutlineColourId, kt::c(theme.accent).withAlpha(0.8f));
    search.setFont(kt::font(theme, 13.f));
    repaint();
    list.repaint();
}
void FxBrowser::setSelectedFx(int index) { if (index >= 0 && index < kt::kFxCount) { selected = index; showCustom(false); } }

void FxBrowser::setCustomItems(const juce::Array<CustomItem>& items)
{
    customItems = items;
    if (selectedCustom >= customItems.size()) selectedCustom = -1;
    rebuildFilter();
}

void FxBrowser::showCustom(bool custom)
{
    customMode = custom;
    builtInTab.setToggleState(!custom, juce::dontSendNotification);
    customTab.setToggleState(custom, juce::dontSendNotification);
    rebuildFilter();
    resized();
    repaint();
}

void FxBrowser::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(kt::c(theme.panel).withAlpha(0.97f));
    g.fillRoundedRectangle(r, 12.f);
    g.setColour(kt::c(theme.border));
    g.drawRoundedRectangle(r, 12.f, 1.f);
    g.setColour(kt::c(theme.accent));
    g.setFont(kt::font(theme, 13.f, true));
    g.drawText(customMode ? "CUSTOM EFFECTS" : "EFFECTS BY CATEGORY", 12, 8, getWidth()-24, 22, juce::Justification::left);
}

void FxBrowser::resized()
{
    auto a = getLocalBounds().reduced(8);
    a.removeFromTop(28);
    auto tabs = a.removeFromTop(30);
    builtInTab.setBounds(tabs.removeFromLeft((tabs.getWidth()-6)/2));
    tabs.removeFromLeft(6);
    customTab.setBounds(tabs);
    a.removeFromTop(6);
    if (! customMode)
    {
        familyBox.setBounds(a.removeFromTop(30));
        a.removeFromTop(6);
        familyBox.setVisible(true);
    }
    else
        familyBox.setVisible(false);
    search.setBounds(a.removeFromTop(30));
    a.removeFromTop(8);
    list.setBounds(a);
}

int FxBrowser::getNumRows() { return customMode ? customFiltered.size() : filtered.size(); }

void FxBrowser::rebuildFilter()
{
    filter = search.getText().toLowerCase().trim();
    filtered.clear();
    if (!customMode)
    {
        const int famSel = familyBox.getSelectedId() - 2; // -1 = all
        for (int i = 0; i < kt::kFxCount; ++i)
        {
            if (famSel >= 0 && kt::kFx[i].family != famSel) continue;
            const auto name = juce::String(kt::kFx[i].name);
            const auto cat = juce::String(kt::fxFamilyName(kt::kFx[i].family));
            if (filter.isEmpty() || name.toLowerCase().contains(filter) || cat.toLowerCase().contains(filter))
                filtered.add(i);
        }
    }
    else
    {
        customFiltered.clear();
        for (int i = 0; i < customItems.size(); ++i)
        {
            const auto& it = customItems[i];
            if (filter.isEmpty()
                || it.name.toLowerCase().contains(filter)
                || it.author.toLowerCase().contains(filter)
                || it.categories.toLowerCase().contains(filter))
                customFiltered.add(i);
        }
        // Sort custom effects by their derived categories string for stable grouping
        std::sort(customFiltered.begin(), customFiltered.end(), [this](int a, int b)
        {
            const int cmp = customItems[a].categories.compareIgnoreCase(customItems[b].categories);
            if (cmp != 0) return cmp < 0;
            return customItems[a].name.compareIgnoreCase(customItems[b].name) < 0;
        });
    }
    list.updateContent();
    list.repaint();
}

void FxBrowser::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool isSelected)
{
    if (customMode)
    {
        if (row < 0 || row >= customFiltered.size()) return;
        const auto& item = customItems[customFiltered[row]];
        g.setColour(isSelected ? kt::c(theme.accent).withAlpha(0.30f) : juce::Colours::transparentBlack);
        g.fillRoundedRectangle(2.f, 2.f, (float)w-4.f, (float)h-4.f, 6.f);
        g.setColour(kt::c(theme.text));
        g.setFont(kt::font(theme, 13.f, true));
        g.drawText(item.name, 10, 3, w-20, 18, juce::Justification::centredLeft);
        g.setColour(kt::c(theme.muted));
        g.setFont(kt::font(theme, 11.f));
        const auto meta = item.categories.isNotEmpty()
            ? item.categories
            : (item.remote ? "DreamShare  -  " + item.author : "My custom effect");
        g.drawText(meta, 10, 22, w-20, 15, juce::Justification::centredLeft);
        return;
    }
    if (row < 0 || row >= filtered.size()) return;
    const int fx = filtered[row];
    g.setColour(isSelected ? kt::c(theme.accent).withAlpha(0.30f) : juce::Colours::transparentBlack);
    g.fillRoundedRectangle(2.f, 2.f, (float)w-4.f, (float)h-4.f, 6.f);
    g.setColour(kt::c(theme.text));
    g.setFont(kt::font(theme, 13.f, true));
    g.drawText(kt::kFx[fx].name, 10, 3, w - 20, 18, juce::Justification::centredLeft);
    g.setColour(kt::c(theme.muted));
    g.setFont(kt::font(theme, 11.f));
    g.drawText(kt::fxFamilyName(kt::kFx[fx].family), 10, 22, w - 20, 15, juce::Justification::centredLeft);
}

void FxBrowser::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    if (customMode)
    {
        if (row >= 0 && row < customFiltered.size())
        {
            selectedCustom = customFiltered[row];
            if (onCustomSelect) onCustomSelect(customItems[selectedCustom]);
        }
        return;
    }
    if (row >= 0 && row < filtered.size())
    {
        selected = filtered[row];
        if (onSelect) onSelect(selected);
    }
}

void FxBrowser::selectedRowsChanged(int) {}

KyotoAudioProcessorEditor::KyotoAudioProcessorEditor(KyotoAudioProcessor& p)
    : AudioProcessorEditor(p), proc(p)
{
    kLookAndFeel.setTheme(theme);
    socialRail.setHostTheme(theme);
    setLookAndFeel(&kLookAndFeel);
    setSize(1180, 760);
    setResizable(true, true);
    setResizeLimits(980, 620, 1900, 1150);

    for (auto* b : { &shareBtn, &chainBtn, &fxBtn, &logoutBtn, &loginBtn, &sendBtn, &feedBtn,
                     &addBtn, &saveBtn, &upBtn, &wavBtn, &chainBreakBtn, &chainMixBtn, &chainRemoveBtn, &chainUndoBtn,
                     &fxAddBtn, &fxSaveBtn, &fxUpBtn, &fxShareChatBtn, &fxShareThreadBtn, &fxRemoveBtn, &fxUndoBtn,
                     &fxBreakBtn, &fxMixBtn, &fxRandomBtn, &fxClearBtn, &pluginViewBtn, &pluginBackBtn, &newMachineBtn, &randomMachineBtn, &chatRefreshBtn, &threadsBtn, &socialBtn, &dmBtn, &adminDeleteBtn, &utilityGoBtn, &catalogModeBtn, &threadsModeBtn, &railChatBtn, &railOnlineBtn,
                     &proToggleBtn, &wizardNextBtn, &wizardSkipBtn })
    {
        addAndMakeVisible(b);
        b->setClickingTogglesState(false);
    }
    proToggleBtn.setClickingTogglesState(true);

    shareBtn.onClick = [this] { showTab(0); };
    chainBtn.onClick = [this]
    {
        if (! loggedIn) return;
        if (! proMode && builderWizardStep == 0 && ! proc.uiState.getProperty("builderWizardDone", false))
            enterBuilderWizard();
        else
            showTab(1);
    };
    fxBtn.onClick = [this] { if (loggedIn) showTab(2); };
    proToggleBtn.onClick = [this]
    {
        proMode = ! proMode;
        proToggleBtn.setButtonText(proMode ? "PRO  -  ON" : "PRO  -  OFF");
        proToggleBtn.setToggleState(proMode, juce::dontSendNotification);
        proc.uiState.setProperty("proMode", proMode, nullptr);
        status.setText(proMode ? "Pro enabled - Plugin Builder opens straight into the workshop."
                               : "Pro off - Plugin Builder will guide template, then playground theme.", juce::dontSendNotification);
        resized();
    };
    wizardNextBtn.onClick = [this] { advanceBuilderWizard(); };
    wizardSkipBtn.onClick = [this]
    {
        builderWizardStep = 0;
        proc.uiState.setProperty("builderWizardDone", true, nullptr);
        showTab(1);
        status.setText("Skipped guide - full builder ready.", juce::dontSendNotification);
    };
    pluginViewBtn.onClick = [this] { setPluginView(true); };
    pluginBackBtn.onClick = [this] { setPluginView(false); };
    newMachineBtn.onClick = [this] {
        juce::PopupMenu menu;
        menu.addItem(1, "4 : 5  -  PORTRAIT"); menu.addItem(2, "1 : 1  -  SQUARE"); menu.addItem(3, "5 : 4  -  LANDSCAPE"); menu.addItem(4, "FREEFORM");
        menu.showMenuAsync(
            juce::PopupMenu::Options().withTargetComponent(&newMachineBtn),
            [this](int choice)
            {
                if (choice >= 1 && choice <= 4)
                    startNewMachine(choice - 1);
            });
    };
    randomMachineBtn.onClick = [this] { randomizeMachine(); };
    logoutBtn.onClick = [this] { logout(); };
    loginBtn.onClick = [this] { login(); };
    sendBtn.onClick = [this] { sendChat(); };
    feedBtn.onClick = [this] { refreshFeed(); refreshCatalog(); };
    chatRefreshBtn.onClick = [this] { chatUtility("chat_list"); };
    threadsBtn.onClick = [this] { chatUtility("list_threads"); };
    socialBtn.onClick = [this] { chatUtility("social_list"); };
    dmBtn.onClick = [this] { chatUtility("dm_list"); };
    adminDeleteBtn.onClick = [this] { if (selectedCatalogId.isNotEmpty()) deleteCatalogId(selectedCatalogId); };
    utilityGoBtn.onClick = [this] {
        static const char* actions[] = {
            "chat_list", "list_threads", "social_list", "dm_list", "dm_send",
            "friend_request", "friend_accept", "friend_decline", "friend_remove",
            "wav_request", "react_heart", "chat_delete", "chat_clear", "presence",
            "create_thread", "comment"
        };
        if (centerMode == 1 && railMode == 0)
        {
            if (selectedThreadId.isNotEmpty()) utilityBox.setText(selectedThreadId, juce::dontSendNotification);
            chatUtility("comment");
            return;
        }
        const int index = utilityActionBox.getSelectedId() - 1;
        if (index >= 0 && index < 16) chatUtility(actions[index]);
    };

    addBtn.onClick = [this] { armPlacement(); };
    chainRemoveBtn.onClick = [this] { removeSelectedChainStep(); };
    chainUndoBtn.onClick = [this] { undoLast(); };
    chainBreakBtn.onClick = [this] { pendingSpecial = true; pendingSpecialType = KyotoAudioProcessor::kBreakType; pendingLabel = "CHAIN BREAK"; armedStyle = "dial"; placing = true; status.setText("Break is a knob part - click a glowing knob bay.", juce::dontSendNotification); panel.placing = true; panel.armedStyle = armedStyle; panel.repaint(); };
    chainMixBtn.onClick = [this] { pendingSpecial = true; pendingSpecialType = KyotoAudioProcessor::kMixType; pendingLabel = "MASTER MIX"; armedStyle = "fader"; placing = true; status.setText("Mix is a fader part - click a glowing fader bay.", juce::dontSendNotification); panel.placing = true; panel.armedStyle = armedStyle; panel.repaint(); };
    saveBtn.onClick = [this] { saveLocal(); };
    upBtn.onClick = [this] { publish(); };
    wavBtn.onClick = [this] { loadWav(); };

    fxAddBtn.onClick = [this] {
        if (fxStack.getNumChildren() >= 16) { status.setText("FX Builder is full - save it as a custom effect.", juce::dontSendNotification); return; }
        captureSnapshot();
        const int type = fxBrowser ? fxBrowser->getSelectedFx() : 0;
        auto step = juce::ValueTree("step");
        step.setProperty("fx", type, nullptr);
        step.setProperty("name", kt::kFx[juce::jlimit(0, kt::kFxCount - 1, type)].name, nullptr);
        step.setProperty("amount", (double) fxAmount.getValue(), nullptr);
        step.setProperty("tone", (double)fxTone.getValue(), nullptr);
        step.setProperty("motion", (double)fxMotion.getValue(), nullptr);
        step.setProperty("mix", (double)fxMix.getValue(), nullptr);
        step.setProperty("shape", (double)fxShape.getValue(), nullptr);
        fxStack.appendChild(step, nullptr);
        selectFxStep(fxStack.getNumChildren() - 1);
        stackLabel.setText("Stack " + juce::String(fxStack.getNumChildren()) + " / 16  -  custom effect lab", juce::dontSendNotification);
    };
    fxSaveBtn.onClick = [this] { saveEffect(); };
    fxUpBtn.onClick = [this] { publishEffect(); };
    fxShareChatBtn.onClick = [this] { shareEffectToChat(); };
    fxShareThreadBtn.onClick = [this] { shareEffectToThread(); };
    fxRemoveBtn.onClick = [this] { removeSelectedFxStep(); };
    fxUndoBtn.onClick = [this] { undoLast(); };
    fxBreakBtn.onClick = [this] {
        if (fxStack.getNumChildren() >= 16) { status.setText("FX Builder is full - save it as a custom effect.", juce::dontSendNotification); return; }
        captureSnapshot();
        auto step = juce::ValueTree("step");
        step.setProperty("fx", KyotoAudioProcessor::kBreakType, nullptr);
        step.setProperty("name", "CHAIN BREAK", nullptr);
        step.setProperty("amount", 1.0, nullptr);
        step.setProperty("tone", 0.5, nullptr); step.setProperty("motion", 0.5, nullptr);
        step.setProperty("mix", 1.0, nullptr); step.setProperty("shape", 0.5, nullptr);
        fxStack.appendChild(step, nullptr); selectFxStep(fxStack.getNumChildren() - 1);
    };
    fxMixBtn.onClick = [this] {
        if (fxStack.getNumChildren() >= 16) { status.setText("FX Builder is full - save it as a custom effect.", juce::dontSendNotification); return; }
        captureSnapshot();
        auto step = juce::ValueTree("step");
        step.setProperty("fx", KyotoAudioProcessor::kMixType, nullptr);
        step.setProperty("name", "MASTER MIX", nullptr);
        step.setProperty("amount", 1.0, nullptr);
        step.setProperty("tone", 0.5, nullptr); step.setProperty("motion", 0.5, nullptr);
        step.setProperty("mix", 1.0, nullptr); step.setProperty("shape", 0.5, nullptr);
        fxStack.appendChild(step, nullptr); selectFxStep(fxStack.getNumChildren() - 1);
    };
    fxRandomBtn.onClick = [this] { captureSnapshot(); randomizeFxControls(); };
    fxClearBtn.onClick = [this] { if (fxStack.getNumChildren() > 0) { captureSnapshot(); fxStack.removeAllChildren(nullptr); selectedFxStep = -1; stackLabel.setText("Empty effect  -  ready for a new build", juce::dontSendNotification); } };

    addAndMakeVisible(status);
    status.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(pluginViewBtn); addAndMakeVisible(pluginBackBtn); addAndMakeVisible(newMachineBtn); addAndMakeVisible(randomMachineBtn);
    pluginBackBtn.setVisible(false);
    addAndMakeVisible(whoLabel);
    whoLabel.setJustificationType(juce::Justification::centredRight);
    pluginViewBtn.toFront(false);
    addAndMakeVisible(userBox);
    addAndMakeVisible(passBox);
    addAndMakeVisible(msgBox);
    addAndMakeVisible(logBox);
    addAndMakeVisible(utilityBox);
    addAndMakeVisible(utilityActionBox); addAndMakeVisible(utilityGoBtn);
    addAndMakeVisible(chatRefreshBtn); addAndMakeVisible(threadsBtn); addAndMakeVisible(socialBtn); addAndMakeVisible(dmBtn); addAndMakeVisible(adminDeleteBtn);
    addAndMakeVisible(catalogModeBtn); addAndMakeVisible(threadsModeBtn); addAndMakeVisible(railChatBtn); addAndMakeVisible(railOnlineBtn);
    addAndMakeVisible(themeBox);
    addAndMakeVisible(catalogView);
    addAndMakeVisible(chatView);
    addAndMakeVisible(nameBox);
    addAndMakeVisible(effectNameBox);
    addAndMakeVisible(presetBox);
    addAndMakeVisible(kindBox);
    addAndMakeVisible(effectBox);
    addAndMakeVisible(fxAmount); addAndMakeVisible(fxTone); addAndMakeVisible(fxMotion); addAndMakeVisible(fxMix); addAndMakeVisible(fxShape);
    addAndMakeVisible(fxAmountLabel); addAndMakeVisible(fxToneLabel); addAndMakeVisible(fxMotionLabel); addAndMakeVisible(fxMixLabel); addAndMakeVisible(fxShapeLabel);
    addAndMakeVisible(fxBreakBtn); addAndMakeVisible(fxMixBtn); addAndMakeVisible(fxRandomBtn); addAndMakeVisible(fxClearBtn);
    addAndMakeVisible(stackLabel); addAndMakeVisible(panel);
    panel.setInterceptsMouseClicks(false, true);

    userBox.setTextToShowWhenEmpty("Username", juce::Colours::grey);
    passBox.setTextToShowWhenEmpty("Password", juce::Colours::grey);
    passBox.setPasswordCharacter((juce::juce_wchar) 0x2022);
    nameBox.setTextToShowWhenEmpty("Chain name", juce::Colours::grey);
    effectNameBox.setTextToShowWhenEmpty("Custom effect name", juce::Colours::grey);
    msgBox.setTextToShowWhenEmpty("Message  -  shared FX links appear as clickable cards", juce::Colours::grey);
    utilityBox.setTextToShowWhenEmpty("Target / ID / thread ID", juce::Colours::grey);
    const char* utilityItems[] = {
        "Chat list", "Threads", "Friends", "Direct messages", "Send DM",
        "Friend request", "Accept friend", "Decline friend", "Remove friend",
        "WAV request", "React: heart", "Delete chat", "Clear chat", "Presence",
        "Create thread", "Comment"
    };
    for (int i=0;i<16;++i) utilityActionBox.addItem(utilityItems[i], i+1);
    utilityActionBox.setSelectedId(1);
    logBox.setMultiLine(true); logBox.setReadOnly(true); logBox.setScrollbarsShown(true); logBox.setOpaque(true); logBox.setLineSpacing(2.0f);
    fxAmount.setRange(0.0, 1.0, 0.01); fxAmount.setValue(0.55); fxAmount.setSliderStyle(juce::Slider::LinearHorizontal); fxAmount.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 18);
    auto setupFx = [](juce::Slider& s) { s.setRange(0.0, 1.0, 0.001); s.setValue(0.5); s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag); s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 48, 16); };
    setupFx(fxTone); setupFx(fxMotion); setupFx(fxMix); setupFx(fxShape);
    auto setupLabel = [](juce::Label& l, const juce::String& text) { l.setText(text, juce::dontSendNotification); l.setJustificationType(juce::Justification::centred); };
    setupLabel(fxAmountLabel, "AMOUNT"); setupLabel(fxToneLabel, "TONE"); setupLabel(fxMotionLabel, "MOTION"); setupLabel(fxMixLabel, "MIX"); setupLabel(fxShapeLabel, "SHAPE");
    for (auto* s : { &fxTone, &fxMotion, &fxMix, &fxShape }) s->onValueChange = [this] { writeFxStepFromControls(); };
    fxAmount.onValueChange = [this] { writeFxStepFromControls(); };

    kindBox.addItem("Knob / Arc", 1); kindBox.addItem("Knob / Pointer", 2); kindBox.addItem("Fader", 3); kindBox.addItem("Slide", 4); kindBox.addItem("Button", 5); kindBox.addItem("Screen", 6); kindBox.addItem("Vent", 7); kindBox.addItem("Badge", 8); kindBox.addItem("Rail", 9); kindBox.setSelectedId(1);
    addAndMakeVisible(shellBox);
    for (int i = 0; i < pb::kShellCount; ++i) shellBox.addItem(pb::kShells[i].name, i + 1);
    shellBox.setSelectedId(1);
    shellBox.onChange = [this] { applyShell(shellBox.getSelectedId() - 1); };
    addAndMakeVisible(playgroundThemeBox);
    for (auto& t : kt::kThemes) playgroundThemeBox.addItem(t.name, playgroundThemeBox.getNumItems() + 1);
    playgroundThemeBox.setSelectedId(1);
    playgroundThemeBox.onChange = [this]
    {
        const int i = playgroundThemeBox.getSelectedId() - 1;
        if (i >= 0 && i < kt::kThemeCount)
            applyPlaygroundTheme(kt::kThemes[i].id);
    };
    panel.onSlot = [this](int slot) { placeInSlot(slot); };
    panel.occupied = [this](int slot) { return slotOccupied(slot); };
    panel.anchor = [this](int slot) { return slotAnchor(slot); };
    panel.theme = theme;
    effectBox.setVisible(false);
    presetBox.onChange = [this] {
        auto display = presetBox.getText().trim();
        if (display.isEmpty()) return;
        auto name = display.fromFirstOccurrenceOf("-", false, false).trim();
        if (name.isEmpty()) name = display;
        loadCatalogId({}, name);
    };

    for (auto& t : kt::kThemes) themeBox.addItem(t.name, themeBox.getNumItems() + 1);
    themeBox.setSelectedId(1);
    themeBox.onChange = [this] { const int i = themeBox.getSelectedId()-1; if (i >= 0 && i < kt::kThemeCount) applyTheme(kt::kThemes[i].id); };

    fxBrowser = std::make_unique<FxBrowser>(proc);
    fxBrowser->onSelect = [this](int) { updateFxControls(); };
    fxBrowser->onCustomSelect = [this](const FxBrowser::CustomItem& item) { loadCatalogId(item.remote ? item.id : juce::String(), item.name); };
    addAndMakeVisible(*fxBrowser);
    updateFxControls();
    catalogView.setViewedComponent(&catalogHolder, false);
    chatView.setViewedComponent(&socialRail, false);
    chatView.setScrollBarsShown(true, false);
    catalogModeBtn.onClick = [this] { setCenterMode(0); };
    threadsModeBtn.onClick = [this] { setCenterMode(1); refreshFeed(); };
    railChatBtn.onClick = [this] { setRailMode(0); refreshFeed(); };
    railOnlineBtn.onClick = [this] { setRailMode(1); refreshFeed(); chatUtility("social_list"); };
    catalogModeBtn.setClickingTogglesState(true);
    threadsModeBtn.setClickingTogglesState(true);
    railChatBtn.setClickingTogglesState(true);
    railOnlineBtn.setClickingTogglesState(true);

    auto session = juce::JSON::parse(sessionFile().loadFileAsString());
    if (auto* o = session.getDynamicObject())
    {
        token = o->getProperty("token").toString();
        account = o->getProperty("user").toString();
        isAdmin = (bool) o->getProperty("admin");
        if (token.isNotEmpty() && account.isNotEmpty()) setLoggedIn(true);
    }
    if (! loggedIn) setLoggedIn(false);
    if (proc.uiState.hasProperty("machineDesign")) machineDesign = MachineDesign::fromVar(juce::JSON::parse(proc.uiState.getProperty("machineDesign").toString()));
    else { machineDesign.choosePlayground((MachineDesign::PlaygroundMode) juce::jlimit(0, 3, (int)proc.uiState.getProperty("playgroundMode"))); machineDesign.theme = proc.uiState.getProperty("theme").toString(); machineDesign.bodyDesign = proc.uiState.getProperty("bodyDesign").toString(); }
    applyTheme(proc.uiState.getProperty("theme", juce::var("trippah")).toString());
    proMode = (bool) proc.uiState.getProperty("proMode", false);
    proToggleBtn.setButtonText(proMode ? "PRO  -  ON" : "PRO  -  OFF");
    proToggleBtn.setToggleState(proMode, juce::dontSendNotification);
    playgroundTheme = theme;
    applyPlaygroundTheme(proc.uiState.getProperty("playgroundTheme", juce::var(theme.id)).toString());
    {
        const auto savedShell = proc.uiState.getProperty("shell").toString();
        for (int i = 0; i < pb::kShellCount; ++i)
            if (savedShell == pb::kShells[i].id) { shellIndex = i; shellBox.setSelectedId(i + 1, juce::dontSendNotification); }
    }
    refreshEffectBox();
    startTimerHz(8);
}

KyotoAudioProcessorEditor::~KyotoAudioProcessorEditor()
{
    catalogView.setViewedComponent(nullptr, false);
    setLookAndFeel(nullptr);
}

void KyotoAudioProcessorEditor::setLoggedIn(bool on)
{
    loggedIn = on;
    whoLabel.setText(on ? account : "Sign in", juce::dontSendNotification);
    logoutBtn.setVisible(on);
    userBox.setVisible(! on);
    passBox.setVisible(! on);
    loginBtn.setVisible(! on);
    if (on)
    {
        userBox.clear();
        passBox.clear();
        showTab(0);
        refreshFeed();
        refreshCatalog();
    }
    else
        showTab(0);
}

void KyotoAudioProcessorEditor::syncMachineDesignToUi()
{
    proc.uiState.setProperty("playgroundMode", (int) machineDesign.playgroundMode, nullptr);
    proc.uiState.setProperty("playgroundWidth", machineDesign.playgroundWidth, nullptr);
    proc.uiState.setProperty("playgroundHeight", machineDesign.playgroundHeight, nullptr);
    proc.uiState.setProperty("aspectRatio", machineDesign.aspectRatio, nullptr);
    proc.uiState.setProperty("bodyDesign", machineDesign.bodyDesign, nullptr);
    proc.uiState.setProperty("theme", machineDesign.theme, nullptr);
    proc.uiState.setProperty("machineDesign", juce::JSON::toString(machineDesign.toVar()), nullptr);
}

void KyotoAudioProcessorEditor::startNewMachine(int mode)
{
    machineDesign = MachineDesign{};
    machineDesign.choosePlayground((MachineDesign::PlaygroundMode) juce::jlimit(0, 3, mode));
    machineDesign.theme = theme.id;
    machineDesign.bodyDesign = "Bare Frame";
    syncMachineDesignToUi();
    proc.uiState.removeAllChildren(nullptr);
    fxStack.removeAllChildren(nullptr);
    widgets.clear();
    selectedFxStep = -1;
    status.setText("New machine: " + machineDesign.aspectRatio + " playground", juce::dontSendNotification);
    showTab(1);
}

void KyotoAudioProcessorEditor::randomizeMachine()
{
    machineDesign.theme = theme.id;
    juce::Random rng((juce::int64) juce::Time::getMillisecondCounterHiRes());
    machineDesign.randomize(rng);
    syncMachineDesignToUi();
    status.setText("Randomized " + machineDesign.bodyDesign + " - " + machineDesign.aspectRatio + " - collision checked", juce::dontSendNotification);
    repaint();
}

void KyotoAudioProcessorEditor::setPluginView(bool on)
{
    pluginView = on;
    pluginViewBtn.setVisible(!on && loggedIn);
    pluginBackBtn.setVisible(on);
    shareBtn.setVisible(!on); chainBtn.setVisible(!on); fxBtn.setVisible(!on); logoutBtn.setVisible(!on); whoLabel.setVisible(!on);
    if (on)
    {
        showTab(1);
        panel.setVisible(false);
        if (fxBrowser) fxBrowser->setVisible(false);
        for (auto* w : widgets) w->setVisible(false);
    }
    else
    {
        showTab(tab);
        status.setVisible(true);
        for (auto* w : widgets) w->setVisible(true);
    }
    resized(); repaint();
}

void KyotoAudioProcessorEditor::timerCallback()
{
    animPhase += 0.035f;
    if (animPhase > juce::MathConstants<float>::twoPi) animPhase -= juce::MathConstants<float>::twoPi;
    socialRail.setPhase(animPhase);
    socialRail.setHostTheme(theme);
    repaint();
    for (auto* w : widgets)
        if (w->kind == CanvasWidget::Kind::Wave || w->kind == CanvasWidget::Kind::Stack)
            w->repaint();
}


namespace {
void drawThemeField(juce::Graphics& g, const kt::ThemePalette& t, juce::Rectangle<float> b, float phase)
{
    const juce::String id = t.id;
    auto accent = kt::c(t.accent);
    auto alt = kt::c(t.pegHot); // second accent; ThemePalette has no accent2 field
    auto ink = kt::c(t.text);
    const int seed = id.hashCode() & 0x7fffffff;
    const int kind = seed % 9;
    const int count = 6 + (seed % 5);
    g.setColour(accent.withAlpha(0.14f));
    if (id == "terminal" || id == "mono" || id == "carbon" || id == "ink")
    {
        for (int i = 0; i < 8; ++i)
        {
            const float y = b.getY() + std::fmod(i * 46.f + phase * 22.f, b.getHeight());
            g.drawLine(b.getX(), y, b.getRight(), y, 1.f);
        }
        g.setFont(kt::font(t, 9.f));
        g.drawText(id.toUpperCase() + "  " + juce::String((int)(phase * 40) % 99), b.getRight() - 120, b.getY() + 8, 108, 14, juce::Justification::right);
        return;
    }
    if (id == "arcade" || id == "neon" || id == "candy")
    {
        for (int i = 0; i < 10; ++i)
        {
            const float x = b.getX() + std::fmod(i * 90.f + phase * 36.f, b.getWidth());
            g.fillRect(x, b.getY() + 10 + (i % 3) * 18.f, 28.f, 3.f);
            g.setColour(alt.withAlpha(0.2f));
            g.drawEllipse(x, b.getBottom() - 40 - std::sin(phase + i) * 8.f, 16, 16, 1.4f);
            g.setColour(accent.withAlpha(0.16f));
        }
        return;
    }
    if (id == "sakura" || id == "rose" || id == "orchid" || id == "lilac" || id == "plum" || id == "wine")
    {
        for (int i = 0; i < count; ++i)
        {
            const float x = b.getX() + std::fmod(i * 78.f + phase * 14.f, b.getWidth());
            const float y = b.getY() + 20 + std::fmod(i * 33.f + phase * 10.f, b.getHeight() * 0.45f);
            g.fillEllipse(x, y, 7, 7);
            g.drawEllipse(x - 4, y - 4, 15, 15, 1.f);
        }
        return;
    }
    if (id == "ocean" || id == "lagoon" || id == "arctic" || id == "ice" || id == "fog" || id == "cobalt")
    {
        for (int i = 0; i < 4; ++i)
        {
            juce::Path wave;
            const float y = b.getY() + 24 + i * 16.f;
            wave.startNewSubPath(b.getX(), y);
            for (float x = 0; x < b.getWidth(); x += 12.f)
                wave.lineTo(b.getX() + x, y + std::sin(x * 0.03f + phase + i) * (6.f + i));
            g.strokePath(wave, juce::PathStrokeType(1.3f));
        }
        return;
    }
    if (id == "forest" || id == "pine" || id == "moss" || id == "mint")
    {
        for (int i = 0; i < count; ++i)
        {
            const float x = b.getX() + 20 + (i * 67 + seed % 20) % (int) juce::jmax(40.f, b.getWidth() - 30);
            const float h = 16.f + std::sin(phase + i) * 6.f;
            g.drawLine(x, b.getBottom() - 8, x, b.getBottom() - 8 - h, 1.4f);
            g.drawEllipse(x - 5, b.getBottom() - 12 - h, 10, 10, 1.f);
        }
        return;
    }
    if (id == "solar" || id == "amber" || id == "honey" || id == "sunset" || id == "ember" || id == "copper" || id == "rust")
    {
        const float cx = b.getRight() - 70, cy = b.getY() + 36;
        g.drawEllipse(cx - 12, cy - 12, 24, 24, 1.6f);
        for (int i = 0; i < 8; ++i)
        {
            const float a = phase * 0.6f + i * 0.785f;
            g.drawLine(cx, cy, cx + std::cos(a) * 28.f, cy + std::sin(a) * 28.f, 1.3f);
        }
        return;
    }
    if (id == "bloodmoon" || id == "void" || id == "abyss" || id == "midnight" || id == "ghost" || id == "ultraviolet" || id == "vapor")
    {
        const float cx = b.getX() + 48 + std::sin(phase) * 10.f, cy = b.getY() + 30;
        g.drawEllipse(cx, cy, 22, 22, 1.5f);
        g.fillEllipse(cx + 8, cy + 2, 16, 16);
        for (int i = 0; i < 5; ++i)
            g.fillEllipse(b.getRight() - 40 - i * 18.f, b.getY() + 12 + std::sin(phase + i) * 4.f, 2.5f, 2.5f);
        return;
    }
    if (id == "graphite" || id == "steel" || id == "default")
    {
        for (int i = 0; i < 5; ++i)
            g.drawRoundedRectangle(b.getX() + 16 + i * 34.f, b.getY() + 14, 22, 22, 4.f, 1.f);
        return;
    }
    // trippah / goonr and any remaining preset still get a signature orbit
    const float cx = b.getX() + 36, cy = b.getY() + 28;
    for (int i = 0; i < 3; ++i)
        g.drawEllipse(cx - 8 - i * 7, cy - 8 - i * 7 + std::sin(phase + i) * 2.f, 18 + i * 14.f, 18 + i * 14.f, 1.2f);
    juce::ignoreUnused(kind, ink);
}
}

void SocialRail::paint(juce::Graphics& g)
{
    g.fillAll(kt::c(host.bg).withAlpha(0.2f));
    if (mode == 0)
    {
        if (bubbles.isEmpty())
        {
            g.setColour(kt::c(host.muted));
            g.setFont(kt::font(host, 11.f));
            g.drawText("Live chat stays on this rail.", 12, 16, getWidth() - 24, 20, juce::Justification::left);
            return;
        }
        int y = 8;
        for (const auto& m : bubbles)
        {
            const auto pal = kt::themeById(m.themeId.isEmpty() ? "trippah" : m.themeId);
            auto card = juce::Rectangle<float>(8.f, (float) y, (float) getWidth() - 16.f, 66.f);
            g.setColour(kt::c(pal.panel));
            g.fillRoundedRectangle(card, 9.f);
            g.setColour(kt::c(pal.accent).withAlpha(0.9f));
            g.fillRoundedRectangle(card.getX(), card.getY(), 3.5f, card.getHeight(), 2.f);
            g.setColour(kt::c(pal.accent));
            g.drawRoundedRectangle(card, 9.f, 1.f);
            g.setFont(kt::font(pal, 11.f, true));
            g.drawText(m.user, card.getX() + 12, card.getY() + 6, card.getWidth() - 20, 16, juce::Justification::left);
            g.setColour(kt::c(pal.text));
            g.setFont(kt::font(pal, 10.f));
            g.drawFittedText(m.text, juce::Rectangle<int>((int) card.getX() + 12, (int) card.getY() + 24, (int) card.getWidth() - 20, 36), juce::Justification::topLeft, 2);
            y += 74;
        }
    }
    else
    {
        if (people.isEmpty())
        {
            g.setColour(kt::c(host.muted));
            g.setFont(kt::font(host, 11.f));
            g.drawText("No friends online yet.", 12, 16, getWidth() - 24, 20, juce::Justification::left);
            return;
        }
        int y = 8;
        for (const auto& person : people)
        {
            const auto pal = kt::themeById(person.themeId.isEmpty() ? host.id : person.themeId);
            auto card = juce::Rectangle<float>(8.f, (float) y, (float) getWidth() - 16.f, 40.f);
            g.setColour(kt::c(pal.panel));
            g.fillRoundedRectangle(card, 8.f);
            g.setColour(person.online ? kt::c(pal.accent) : kt::c(pal.muted));
            g.fillEllipse(card.getX() + 10, card.getY() + 14, 10, 10);
            g.setColour(kt::c(pal.accent));
            g.setFont(kt::font(pal, 11.f, true));
            g.drawText(person.name, card.getX() + 28, card.getY() + 4, card.getWidth() - 36, 16, juce::Justification::left);
            g.setColour(kt::c(pal.muted));
            g.setFont(kt::font(pal, 9.f));
            g.drawText(person.detail, card.getX() + 28, card.getY() + 20, card.getWidth() - 36, 14, juce::Justification::left);
            y += 48;
        }
    }
}

struct BoardCard : public juce::Component
{
    juce::String title, meta, body, themeId;
    std::function<void()> onOpen;
    void paint(juce::Graphics& g) override
    {
        const auto pal = kt::themeById(themeId.isEmpty() ? "trippah" : themeId);
        auto r = getLocalBounds().toFloat().reduced(1.f);
        g.setColour(kt::c(pal.panel));
        g.fillRoundedRectangle(r, 10.f);
        g.setColour(kt::c(pal.accent).withAlpha(0.9f));
        g.fillRoundedRectangle(r.getX(), r.getY(), 4.f, r.getHeight(), 2.f);
        g.setColour(kt::c(pal.accent).withAlpha(0.55f));
        g.drawRoundedRectangle(r, 10.f, 1.f);
        g.setColour(kt::c(pal.accent));
        g.setFont(kt::font(pal, 12.f, true));
        g.drawText(title, 14, 8, getWidth() - 22, 18, juce::Justification::left);
        g.setColour(kt::c(pal.muted));
        g.setFont(kt::font(pal, 9.f));
        g.drawText(meta, 14, 28, getWidth() - 22, 14, juce::Justification::left);
        g.setColour(kt::c(pal.text));
        g.setFont(kt::font(pal, 10.f));
        g.drawFittedText(body, juce::Rectangle<int>(14, 46, getWidth() - 24, getHeight() - 54), juce::Justification::topLeft, 2);
    }
    void mouseUp(const juce::MouseEvent& e) override
    {
        if (onOpen && ! e.mouseWasDraggedSinceMouseDown()) onOpen();
    }
};

void KyotoAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(kt::c(theme.bg));
    if (pluginView)
    {
        auto r = getLocalBounds().reduced(18).toFloat();
        g.setColour(kt::c(theme.panel)); g.fillRoundedRectangle(r, 18.f);
        g.setColour(kt::c(theme.border)); g.drawRoundedRectangle(r, 18.f, 1.f);
        g.setColour(kt::c(theme.accent)); g.setFont(kt::font(theme, 18.f, true));
        g.drawText(machineDesign.bodyDesign.toUpperCase(), r.getX()+26, r.getY()+52, r.getWidth()-52, 28, juce::Justification::centred);
        g.setColour(kt::c(theme.muted)); g.setFont(kt::font(theme, 10.f));
        g.drawText(machineDesign.aspectRatio + "  -  " + machineDesign.bodyDesign + "  -  " + theme.name, r.getX()+26, r.getY()+82, r.getWidth()-52, 18, juce::Justification::centred);
        for (const auto& part : machineDesign.modules)
        {
            auto q = juce::Rectangle<float>(r.getX()+part.bounds.getX()*0.72f, r.getY()+part.bounds.getY()*0.72f+112.f, part.bounds.getWidth()*0.72f, part.bounds.getHeight()*0.72f);
            q.setPosition(juce::jlimit(r.getX()+16.f, r.getRight()-q.getWidth()-16.f, q.getX()), juce::jlimit(r.getY()+112.f, r.getBottom()-q.getHeight()-24.f, q.getY()));
            g.setColour(kt::c(theme.panel).brighter(0.08f)); g.fillRoundedRectangle(q, 10.f);
            g.setColour(kt::c(theme.accent).withAlpha(0.72f)); g.drawRoundedRectangle(q, 10.f, 1.5f);
            g.setColour(kt::c(theme.text)); g.setFont(kt::font(theme, 9.f, true)); g.drawFittedText(part.type, q.reduced(7.f).toNearestInt(), juce::Justification::centred, 1);
        }
        return;
    }
    drawThemeField(g, theme, getLocalBounds().toFloat().reduced(10.f), animPhase);
    for (int i=0;i<2;i++)
    {
        const float px = 90.f + std::fmod((float)i*280.f + animPhase*10.f, (float)juce::jmax(120,getWidth()-220));
        const float py = 168.f + std::fmod((float)i*140.f + std::sin(animPhase+i)*8.f, (float)juce::jmax(160,getHeight()-240));
        drawThemeSprite(g, theme, theme.id, px, py, 0.22f, animPhase + i);
    }

    g.setColour(kt::c(theme.panel).withAlpha(0.98f));
    g.fillRoundedRectangle(8.f, 8.f, (float)getWidth()-16.f, 42.f, 11.f);
    g.setColour(kt::c(theme.border)); g.drawRoundedRectangle(8.5f, 8.5f, (float)getWidth()-17.f, 41.f, 11.f, 1.f);
    g.setColour(kt::c(theme.accent)); g.setFont(kt::font(theme, 17.f, true));
    g.drawText("DREAMSHARE", 20, 14, 180, 24, juce::Justification::left);

    if (!loggedIn)
    {
        auto card = getLocalBounds().withSizeKeepingCentre(420, 280).toFloat();
        g.setColour(kt::c(theme.panel).withAlpha(0.97f)); g.fillRoundedRectangle(card, 16.f);
        g.setColour(kt::c(theme.accent).withAlpha(0.85f)); g.fillRoundedRectangle(card.getX(), card.getY(), 4.f, card.getHeight(), 2.f);
        g.setColour(kt::c(theme.border)); g.drawRoundedRectangle(card, 16.f, 1.f);
        g.setColour(kt::c(theme.accent)); g.setFont(kt::font(theme, 20.f, true)); g.drawText("ENTER THE ROOM", card.getX()+28, card.getY()+22, card.getWidth()-56, 28, juce::Justification::left);
        g.setColour(kt::c(theme.muted)); g.setFont(kt::font(theme, 12.f));
        g.drawFittedText("Saved presets and shared effects stay with your DreamShare account.", juce::Rectangle<int>((int)card.getX()+28, (int)card.getY()+54, (int)card.getWidth()-56, 36), juce::Justification::topLeft, 2);
        return;
    }

    if (tab == 0)
    {
        auto shell = getLocalBounds().withTrimmedTop(62).reduced(12).toFloat();
        g.setColour(kt::c(theme.panel).withAlpha(0.97f));
        g.fillRoundedRectangle(shell, 15.f);
        g.setColour(kt::c(theme.border));
        g.drawRoundedRectangle(shell, 15.f, 1.f);

        auto hero = shell.reduced(12.f);
        hero.setHeight(82.f);
        g.setColour(kt::c(theme.accent).withAlpha(0.10f));
        g.fillRoundedRectangle(hero, 12.f);
        g.setColour(kt::c(theme.accent));
        g.setFont(kt::font(theme, 20.f, true));
        g.drawText("DREAMSHARE HOME", hero.getX()+18, hero.getY()+12, 300, 26, juce::Justification::left);
        g.setColour(kt::c(theme.text));
        g.setFont(kt::font(theme, 13.f, false));
        g.drawFittedText("Catalog in the center. Threads on their own tab. Chat stays on the side rail.", juce::Rectangle<float>(hero.getX()+18, hero.getY()+42, juce::jmax(120.f, hero.getWidth()-250.f), 28.f).toNearestInt(), juce::Justification::topLeft, 2);
        g.setColour(kt::c(theme.accent));
        g.setFont(kt::font(theme, 12.f, true));
        g.drawText("LIVE  -  " + (account.isEmpty() ? juce::String("SIGNED IN") : account.toUpperCase()), hero.getRight()-220, hero.getY()+18, 200, 18, juce::Justification::right);
        g.setColour(kt::c(theme.muted));
        g.setFont(kt::font(theme, 11.f));
        g.drawText("Theme: " + juce::String(theme.name) + "  -  " + juce::String(kt::kThemeCount) + " presets", hero.getRight()-220, hero.getY()+40, 200, 16, juce::Justification::right);

        g.setColour(kt::c(theme.border).withAlpha(0.55f));
        g.drawLine(shell.getX()+12.f, hero.getBottom()+10.f, shell.getRight()-12.f, hero.getBottom()+10.f, 1.f);

        auto chatCard = chatView.getBounds().toFloat().expanded(1.f);
        auto catalogCard = catalogView.getBounds().toFloat().expanded(1.f);
        g.setColour(kt::c(theme.accent).withAlpha(0.055f));
        g.fillRoundedRectangle(chatCard, 10.f);
        g.fillRoundedRectangle(catalogCard, 10.f);
        g.setColour(kt::c(theme.accent));
        g.setFont(kt::font(theme, 12.f, true));
        g.drawText(railMode == 0 ? "SIDE CHAT" : "FRIENDS / ONLINE", chatCard.getX()+12, chatCard.getY()+6, 180, 18, juce::Justification::left);
        g.drawText(centerMode == 0 ? "CATALOG BROWSER" : "THREADS", catalogCard.getX()+12, catalogCard.getY()+6, 200, 18, juce::Justification::left);
    }

    if (tab == 1)
    {
        // Always drive the live template preview from playground theme + selected shell.
        panel.theme = playgroundTheme;
        panel.shellIndex = shellIndex;
        panel.placing = placing && builderWizardStep == 0;
        panel.armedStyle = armedStyle;

        if (builderWizardStep > 0)
        {
            // Guide card on the left; live shell preview is the panel on the right (laid out in resized).
            auto area = getLocalBounds().withTrimmedTop(56).reduced(14);
            auto card = area.removeFromLeft(juce::jmin(360, area.getWidth() / 2)).toFloat().reduced(4.f);
            g.setColour(kt::c(theme.panel).withAlpha(0.98f));
            g.fillRoundedRectangle(card, 16.f);
            g.setColour(kt::c(theme.border));
            g.drawRoundedRectangle(card, 16.f, 1.f);

            g.setColour(kt::c(theme.accent));
            g.setFont(kt::font(theme, 18.f, true));
            const juce::String title = builderWizardStep == 1 ? "HARDWARE TEMPLATE" : "PLAYGROUND THEME";
            g.drawText(title, card.getX() + 20, card.getY() + 16, card.getWidth() - 40, 26, juce::Justification::left);

            g.setColour(kt::c(theme.muted));
            g.setFont(kt::font(theme, 12.f));
            const juce::String guide = builderWizardStep == 1
                ? "Choose a shell first. The live preview on the right shows every bay on that template. Motherboard is always the start of the chain."
                : "Theme colours only the plugin playground preview and builder. DreamShare keeps your home theme. Change the theme anytime from the builder bar.";
            g.drawFittedText(guide,
                             juce::Rectangle<int>((int) card.getX() + 20, (int) card.getY() + 48, (int) card.getWidth() - 40, 70),
                             juce::Justification::topLeft, 4);

            g.setColour(kt::c(theme.accent).withAlpha(0.75f));
            g.setFont(kt::font(theme, 12.f, true));
            g.drawText("STEP " + juce::String(builderWizardStep) + " / 2",
                       card.getX() + 20, card.getBottom() - 32, 140, 18, juce::Justification::left);
        }
    }

    if (tab == 2)
    {
        auto pb = panel.getBounds().toFloat();
        g.setColour(kt::c(theme.panel).withAlpha(0.96f)); g.fillRoundedRectangle(pb, 12.f); g.setColour(kt::c(theme.border)); g.drawRoundedRectangle(pb, 12.f, 1.f);
        g.setColour(kt::c(theme.accent)); g.setFont(kt::font(theme, 15.f, true)); g.drawText("CUSTOM EFFECT", pb.getX()+16, pb.getY()+12, 220, 20, juce::Justification::left);
        g.setColour(kt::c(theme.muted)); g.setFont(kt::font(theme, 12.f)); g.drawText("Build up to 16 stages. Categories auto-tag from the FX you stack.", pb.getX()+16, pb.getY()+34, pb.getWidth()-32, 18, juce::Justification::left);

        const int cols = juce::jmax(1, (int)(pb.getWidth() / 220.f));
        const float gap = 10.f, cw = (pb.getWidth()-gap*(cols+1))/cols, ch = 80.f;
        for (int i=0;i<fxStack.getNumChildren();++i)
        {
            auto st = fxStack.getChild(i); const int col=i%cols, row=i/cols;
            auto r = juce::Rectangle<float>(pb.getX()+gap+col*(cw+gap), pb.getY()+62.f+row*(ch+gap), cw, ch);
            const bool selected=i==selectedFxStep;
            g.setColour(selected ? kt::c(theme.accent).withAlpha(0.20f) : kt::c(theme.panel).brighter(0.06f)); g.fillRoundedRectangle(r, 9.f);
            g.setColour(kt::c(theme.border).withAlpha(selected?0.95f:0.75f)); g.drawRoundedRectangle(r, 9.f, selected?2.f:1.f);
            g.setColour(kt::c(theme.accent)); g.setFont(kt::font(theme, 12.f, true)); g.drawText(juce::String(i+1).paddedLeft('0',2), r.getX()+9, r.getY()+8, 28, 16, juce::Justification::left);
            g.setColour(kt::c(theme.text)); g.setFont(kt::font(theme, 13.f, true)); g.drawFittedText(st.getProperty("name").toString(), r.getX()+36, r.getY()+7, r.getWidth()-46, 18, juce::Justification::left, 1);
            g.setColour(kt::c(theme.muted)); g.setFont(kt::font(theme, 11.f));
            const int fxType = (int) st.getProperty("fx", 0);
            const juce::String fam = (fxType >= 0 && fxType < kt::kFxCount) ? kt::fxFamilyName(kt::kFx[fxType].family) : "FX";
            g.drawText(fam + "   A " + juce::String((double)st.getProperty("amount",0.5),2) + "   T " + juce::String((double)st.getProperty("tone",0.5),2), r.getX()+10, r.getBottom()-24, r.getWidth()-20, 16, juce::Justification::left);
        }
    }
}

void KyotoAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    if (tab != 2) return;
    const auto pb = panel.getBounds().toFloat();
    const int cols = juce::jmax(1, (int)(pb.getWidth() / 220.f));
    const float gap = 10.f, cw = (pb.getWidth()-gap*(cols+1))/cols, ch = 80.f;
    for (int i=0;i<fxStack.getNumChildren();++i)
    {
        const int col=i%cols, row=i/cols;
        auto r=juce::Rectangle<float>(pb.getX()+gap+col*(cw+gap),pb.getY()+62.f+row*(ch+gap),cw,ch);
        if (r.contains(e.position)) { selectFxStep(i); return; }
    }
}

void KyotoAudioProcessorEditor::showTab(int next)
{
    tab = loggedIn ? next : 0;
    const bool share = tab == 0;
    const bool chain = tab == 1;
    const bool fx = tab == 2;

    logBox.setVisible(false);
    utilityBox.setVisible(share && loggedIn && (railMode == 1 || centerMode == 1));
    utilityActionBox.setVisible(share && loggedIn && railMode == 1);
    utilityGoBtn.setVisible(share && loggedIn && (railMode == 1 || centerMode == 1));
    chatRefreshBtn.setVisible(false); threadsBtn.setVisible(false); socialBtn.setVisible(false); dmBtn.setVisible(false);
    catalogModeBtn.setVisible(share && loggedIn); threadsModeBtn.setVisible(share && loggedIn);
    railChatBtn.setVisible(share && loggedIn); railOnlineBtn.setVisible(share && loggedIn);
    adminDeleteBtn.setVisible(share && loggedIn && isAdmin); msgBox.setVisible(share && loggedIn && railMode == 0); sendBtn.setVisible(share && loggedIn && railMode == 0); feedBtn.setVisible(share && loggedIn);
    themeBox.setVisible(share && loggedIn); // global UI theme only on DreamShare home
    const bool wizard = chain && builderWizardStep > 0;
    // Step 1 = shell template, Step 2 = playground theme; both stay available after the guide.
    shellBox.setVisible(chain && loggedIn && (builderWizardStep == 0 || builderWizardStep == 1));
    playgroundThemeBox.setVisible(chain && loggedIn && (builderWizardStep == 0 || builderWizardStep == 2));
    wizardNextBtn.setVisible(wizard);
    wizardSkipBtn.setVisible(wizard);
    proToggleBtn.setVisible(share && loggedIn);
    catalogView.setVisible(share && loggedIn);
    chatView.setVisible(share && loggedIn);
    for (auto* b : feedEffectButtons) b->setVisible(share && loggedIn);

    const bool builderReady = chain && builderWizardStep == 0;
    // Keep the live shell preview visible during the guide as well as in the full builder.
    panel.setVisible(chain);
    if (fxBrowser) fxBrowser->setVisible(builderReady || fx);
    addBtn.setVisible(builderReady); chainBreakBtn.setVisible(builderReady); chainMixBtn.setVisible(builderReady); chainRemoveBtn.setVisible(builderReady); chainUndoBtn.setVisible(builderReady);
    nameBox.setVisible(builderReady); presetBox.setVisible(builderReady); saveBtn.setVisible(builderReady); upBtn.setVisible(builderReady); kindBox.setVisible(builderReady); wavBtn.setVisible(builderReady);
    gridStyleBox.setVisible(false); effectBox.setVisible(false);
    newMachineBtn.setVisible(builderReady && !pluginView); randomMachineBtn.setVisible(builderReady && !pluginView);

    effectNameBox.setVisible(fx); fxAddBtn.setVisible(fx); fxSaveBtn.setVisible(fx); fxUpBtn.setVisible(fx); fxShareChatBtn.setVisible(fx); fxShareThreadBtn.setVisible(fx); fxRemoveBtn.setVisible(fx); fxUndoBtn.setVisible(fx);
    fxAmount.setVisible(fx); fxTone.setVisible(fx); fxMotion.setVisible(fx); fxMix.setVisible(fx); fxShape.setVisible(fx);
    fxAmountLabel.setVisible(fx); fxToneLabel.setVisible(fx); fxMotionLabel.setVisible(fx); fxMixLabel.setVisible(fx); fxShapeLabel.setVisible(fx);
    fxBreakBtn.setVisible(fx); fxMixBtn.setVisible(fx); fxRandomBtn.setVisible(fx); fxClearBtn.setVisible(fx); stackLabel.setVisible(fx);

    shareBtn.setToggleState(share, juce::dontSendNotification); chainBtn.setToggleState(chain, juce::dontSendNotification); fxBtn.setToggleState(fx, juce::dontSendNotification);
    catalogModeBtn.setToggleState(centerMode == 0, juce::dontSendNotification); threadsModeBtn.setToggleState(centerMode == 1, juce::dontSendNotification);
    railChatBtn.setToggleState(railMode == 0, juce::dontSendNotification); railOnlineBtn.setToggleState(railMode == 1, juce::dontSendNotification);
    pluginViewBtn.setVisible(!pluginView && loggedIn); pluginBackBtn.setVisible(pluginView);
    if (! chain) { newMachineBtn.setVisible(false); randomMachineBtn.setVisible(false); }
    resized();
    if (chain) { ensureMotherboard(); rebuildCanvas(); }
    repaint();
}

void KyotoAudioProcessorEditor::resized()
{
    const int W = getWidth(), H = getHeight();
    if (pluginView)
    {
        pluginBackBtn.setBounds(24, 20, 104, 34);
        pluginViewBtn.setVisible(false); newMachineBtn.setVisible(false); randomMachineBtn.setVisible(false);
        shareBtn.setVisible(false); chainBtn.setVisible(false); fxBtn.setVisible(false); logoutBtn.setVisible(false); whoLabel.setVisible(false); status.setVisible(false);
        return;
    }
    const int navY = 8;
    const int rightPad = 18;
    const int logoutW = 100;
    const int whoW = 132;
    const int pluginW = 132;
    const int rightX = W - rightPad;
    logoutBtn.setBounds(rightX - logoutW, navY, logoutW, 32);
    whoLabel.setBounds(rightX - logoutW - whoW - 8, navY, whoW, 32);
    pluginViewBtn.setBounds(rightX - logoutW - whoW - pluginW - 16, navY, pluginW, 32);
    shareBtn.setBounds(190, navY, 106, 32);
    chainBtn.setBounds(292, navY, 150, 32);
    fxBtn.setBounds(448, navY, 112, 32);
    const int statusX = 572;
    const int statusRight = pluginViewBtn.getX() - 12;
    status.setBounds(statusX, navY, juce::jmax(120, statusRight - statusX), 32);
    pluginViewBtn.toFront(false);
    whoLabel.toFront(false);
    logoutBtn.toFront(false);

    auto area = getLocalBounds().withTrimmedTop(52).reduced(12);
    if (! loggedIn)
    {
        auto box = getLocalBounds().withSizeKeepingCentre(360, 118).translated(0, 28);
        userBox.setBounds(box.removeFromTop(34)); box.removeFromTop(8); passBox.setBounds(box.removeFromTop(34)); box.removeFromTop(10); loginBtn.setBounds(box.removeFromTop(34));
        return;
    }

    if (tab == 0)
    {
        area.removeFromTop(88); // dashboard hero
        auto top = area.removeFromTop(36);
        themeBox.setBounds(top.removeFromLeft(150)); top.removeFromLeft(8);
        proToggleBtn.setBounds(top.removeFromLeft(110)); top.removeFromLeft(8);
        catalogModeBtn.setBounds(top.removeFromLeft(92)); top.removeFromLeft(6);
        threadsModeBtn.setBounds(top.removeFromLeft(92)); top.removeFromLeft(6);
        feedBtn.setBounds(top.removeFromLeft(84));
        if (isAdmin && top.getWidth() > 84) { top.removeFromLeft(6); adminDeleteBtn.setBounds(top.removeFromLeft(78)); }
        const int railW = juce::jlimit(236, 286, getWidth() / 5);
        auto rail = area.removeFromRight(railW);
        area.removeFromRight(10);
        if (centerMode == 1)
        {
            auto reply = area.removeFromBottom(34);
            utilityBox.setBounds(reply.removeFromLeft(juce::jmax(160, reply.getWidth() - 92)));
            reply.removeFromLeft(8);
            utilityGoBtn.setBounds(reply);
            utilityGoBtn.setButtonText("REPLY");
        }
        catalogView.setBounds(area.reduced(0, 6));
        auto railHead = rail.removeFromTop(28);
        railChatBtn.setBounds(railHead.removeFromLeft((railHead.getWidth() - 6) / 2));
        railHead.removeFromLeft(6);
        railOnlineBtn.setBounds(railHead);
        rail.removeFromTop(6);
        if (railMode == 0)
        {
            auto composer = rail.removeFromBottom(34);
            msgBox.setBounds(composer.removeFromLeft(juce::jmax(120, composer.getWidth() - 72)));
            composer.removeFromLeft(6);
            sendBtn.setBounds(composer);
        }
        else
        {
            auto util = rail.removeFromBottom(34);
            utilityBox.setBounds(util.removeFromLeft(juce::jmax(80, util.getWidth() - 148)));
            util.removeFromLeft(4);
            utilityActionBox.setBounds(util.removeFromLeft(92));
            util.removeFromLeft(4);
            utilityGoBtn.setBounds(util);
            utilityGoBtn.setButtonText("GO");
        }
        chatView.setBounds(rail.reduced(0, 4));
        socialRail.setBounds(0, 0, juce::jmax(200, chatView.getWidth() - 8), socialRail.contentHeight());
        chatView.setViewedComponent(&socialRail, false);
        const int catalogW = juce::jmax(220, catalogView.getWidth()-18);
        const int cols = catalogW >= 500 ? 2 : 1;
        const int gap = 8;
        const int cardW = juce::jmax(160, (catalogW - gap * (cols + 1)) / cols);
        const int cardH = 84;
        catalogHolder.setSize(catalogW, juce::jmax(catalogView.getHeight(), ((catalog.size()+cols-1)/cols) * (cardH+gap) + gap));
        for (int i = 0; i < catalogHolder.getNumChildComponents(); ++i)
        {
            auto* card = catalogHolder.getChildComponent(i);
            const int col = i % cols, row = i / cols;
            card->setBounds(gap + col * (cardW + gap), gap + row * (cardH + gap), cardW, cardH);
        }
        for (int i=0;i<feedEffectButtons.size();++i)
        {
            const int bw = juce::jmin(240, chatView.getWidth()-16);
            feedEffectButtons[i]->setBounds(chatView.getX()+8, chatView.getY()+8+36*i, bw, 30);
        }
    }
    else if (tab == 1)
    {
        if (builderWizardStep > 0)
        {
            // Left: guide controls. Right: live template preview (panel).
            auto left = area.removeFromLeft(juce::jmin(360, area.getWidth() / 2)).reduced(8, 4);
            area.removeFromLeft(10);
            panel.setBounds(area.reduced(2));

            // Reserve space matching the painted title + guide text.
            left.removeFromTop(120);
            if (builderWizardStep == 1)
                shellBox.setBounds(left.removeFromTop(36));
            else
                playgroundThemeBox.setBounds(left.removeFromTop(36));
            left.removeFromTop(14);
            auto row = left.removeFromTop(36);
            wizardNextBtn.setBounds(row.removeFromLeft(120));
            row.removeFromLeft(10);
            wizardSkipBtn.setBounds(row.removeFromLeft(150));
            panel.theme = playgroundTheme;
            panel.shellIndex = shellIndex;
            panel.repaint();
            return;
        }

        auto top = area.removeFromTop(48);
        playgroundThemeBox.setBounds(top.removeFromLeft(138)); top.removeFromLeft(6);
        shellBox.setBounds(top.removeFromLeft(138)); top.removeFromLeft(6);
        nameBox.setBounds(top.removeFromLeft(120)); top.removeFromLeft(6);
        kindBox.setBounds(top.removeFromLeft(128)); top.removeFromLeft(6);
        addBtn.setBounds(top.removeFromLeft(68)); top.removeFromLeft(5);
        chainBreakBtn.setBounds(top.removeFromLeft(68)); top.removeFromLeft(5);
        chainMixBtn.setBounds(top.removeFromLeft(68)); top.removeFromLeft(5);
        chainRemoveBtn.setBounds(top.removeFromLeft(74)); top.removeFromLeft(5);
        chainUndoBtn.setBounds(top.removeFromLeft(64));

        auto actions = area.removeFromTop(40);
        presetBox.setBounds(actions.removeFromLeft(238)); actions.removeFromLeft(8);
        newMachineBtn.setBounds(actions.removeFromLeft(96)); actions.removeFromLeft(6);
        randomMachineBtn.setBounds(actions.removeFromLeft(128)); actions.removeFromLeft(6);
        wavBtn.setBounds(actions.removeFromLeft(58)); actions.removeFromLeft(6);
        saveBtn.setBounds(actions.removeFromLeft(68)); actions.removeFromLeft(6);
        upBtn.setBounds(actions.removeFromLeft(78));

        auto left = area.removeFromLeft(300);
        if (fxBrowser) fxBrowser->setBounds(left);
        area.removeFromLeft(10);
        panel.setBounds(area);
        reflowSeries();
    }
    else
    {
        auto top = area.removeFromTop(58);
        effectNameBox.setBounds(top.removeFromLeft(190)); top.removeFromLeft(7); fxAddBtn.setBounds(top.removeFromLeft(70)); top.removeFromLeft(5); fxBreakBtn.setBounds(top.removeFromLeft(60)); top.removeFromLeft(5); fxMixBtn.setBounds(top.removeFromLeft(92)); top.removeFromLeft(5); fxRemoveBtn.setBounds(top.removeFromLeft(70)); top.removeFromLeft(5); fxUndoBtn.setBounds(top.removeFromLeft(60)); top.removeFromLeft(5); fxSaveBtn.setBounds(top.removeFromLeft(62)); top.removeFromLeft(5); fxUpBtn.setBounds(top.removeFromLeft(74)); top.removeFromLeft(5); fxShareChatBtn.setBounds(top.removeFromLeft(54)); top.removeFromLeft(5); fxShareThreadBtn.setBounds(top.removeFromLeft(62));
        auto left = area.removeFromLeft(282); if (fxBrowser) fxBrowser->setBounds(left); area.removeFromLeft(10);
        auto inspector = area.removeFromRight(286);
        stackLabel.setBounds(area.removeFromTop(34));
        auto tools = inspector.removeFromTop(42); fxRandomBtn.setBounds(tools.removeFromLeft(92)); tools.removeFromLeft(6); fxClearBtn.setBounds(tools.removeFromLeft(72));
        auto labels = inspector.removeFromTop(20);
        fxAmountLabel.setBounds(labels.removeFromLeft(54)); fxToneLabel.setBounds(labels.removeFromLeft(54)); fxMotionLabel.setBounds(labels.removeFromLeft(54)); fxMixLabel.setBounds(labels.removeFromLeft(54)); fxShapeLabel.setBounds(labels.removeFromLeft(54));
        auto knobs = inspector.removeFromTop(104);
        fxAmount.setBounds(knobs.removeFromLeft(54)); fxTone.setBounds(knobs.removeFromLeft(54)); fxMotion.setBounds(knobs.removeFromLeft(54)); fxMix.setBounds(knobs.removeFromLeft(54)); fxShape.setBounds(knobs.removeFromLeft(54));
        panel.setBounds(area);
    }
}

juce::Rectangle<int> KyotoAudioProcessorEditor::cellFor(int index, const juce::String& kind) const
{
    juce::Rectangle<int> r;
    if (! findAutoCell(index, kind, r)) return {};
    return r;
}

bool KyotoAudioProcessorEditor::findAutoCell(int index, const juce::String& kind, juce::Rectangle<int>& result) const
{
    int ww = 84, hh = 90;
    if (kind == "slider") { ww = 64; hh = 128; }
    else if (kind == "key") { ww = 66; hh = 66; }
    else if (kind == "wave") { ww = 190; hh = 94; }
    else if (kind == "stack") { ww = 156; hh = 86; }

    const int W = panel.getWidth(), H = panel.getHeight();
    const int margin = 14, gap = 14;
    if (W < ww + margin * 2 || H < hh + margin * 2) return false;
    const int cols = juce::jmax(1, (W - margin*2 + gap) / (ww + gap));
    const int rows = juce::jmax(1, (H - margin*2 + gap) / (hh + gap));
    const int cells = cols * rows;
    if (index >= cells && cells <= 1) return false;

    auto overlaps = [&](const juce::Rectangle<int>& candidate)
    {
        for (auto* w : widgets)
            if (candidate.expanded(gap/2, gap/2).intersects(w->getBounds())) return true;
        return false;
    };

    const int start = (index * 17 + 5) % juce::jmax(1, cells);
    for (int attempt = 0; attempt < cells; ++attempt)
    {
        const int cell = (start + attempt * 7) % cells;
        const int col = cell % cols, row = cell / cols;
        const int x = margin + col * (ww + gap) + ((cell * 13) % 7) - 3;
        const int y = margin + row * (hh + gap) + ((cell * 19) % 7) - 3;
        juce::Rectangle<int> candidate(x, y, ww, hh);
        if (candidate.getRight() <= W - margin && candidate.getBottom() <= H - margin && !overlaps(candidate))
        { result = candidate; return true; }
    }
    return false;
}

void KyotoAudioProcessorEditor::reflowSeries()
{
    const auto& shell = pb::kShells[juce::jlimit(0, pb::kShellCount - 1, shellIndex)];
    auto face = pb::faceRect(panel.getLocalBounds().toFloat());
    for (auto* w : widgets) w->setBounds({});
    for (auto* w : widgets)
    {
        const int slot = (int) w->node.getProperty("shellSlot", -1);
        if (slot < 0 || slot >= shell.slotCount) continue;
        auto r = pb::slotRect(face, shell.slots[slot]).reduced(4.f).toNearestInt();
        if (r.getWidth() < 8 || r.getHeight() < 8) continue;
        w->node.setProperty("x", r.getX(), nullptr);
        w->node.setProperty("y", r.getY(), nullptr);
        w->setBounds(r);
        w->setTheme(theme);
    }
    panel.theme = theme;
    panel.shellIndex = shellIndex;
    panel.placing = placing;
    panel.armedStyle = armedStyle;
    panel.repaint();
}

bool KyotoAudioProcessorEditor::slotOccupied(int slot) const
{
    for (auto* w : widgets)
        if ((int) w->node.getProperty("shellSlot", -1) == slot) return true;
    return false;
}

juce::Point<float> KyotoAudioProcessorEditor::slotAnchor(int slot) const
{
    for (auto* w : widgets)
        if ((int) w->node.getProperty("shellSlot", -1) == slot)
            return w->getBounds().getCentre().toFloat();
    const auto& shell = pb::kShells[juce::jlimit(0, pb::kShellCount - 1, shellIndex)];
    if (slot < 0 || slot >= shell.slotCount) return {};
    return pb::slotRect(pb::faceRect(panel.getLocalBounds().toFloat()), shell.slots[slot]).getCentre();
}

void KyotoAudioProcessorEditor::applyShell(int index)
{
    shellIndex = juce::jlimit(0, pb::kShellCount - 1, index);
    proc.uiState.setProperty("shell", pb::kShells[shellIndex].id, nullptr);
    panel.shellIndex = shellIndex;
    ensureMotherboard();
    reflowSeries();
    repaint();
}

void KyotoAudioProcessorEditor::ensureMotherboard()
{
    const auto& shell = pb::kShells[shellIndex];
    juce::ValueTree board;
    for (int i = 0; i < proc.uiState.getNumChildren(); ++i)
    {
        auto child = proc.uiState.getChild(i);
        if (child.hasType("w") && child.getProperty("kind").toString() == "board") board = child;
    }
    const juce::String prefix = "s01";
    if (auto* type = proc.apvts.getParameter(prefix + "type")) type->setValueNotifyingHost(type->convertTo0to1((float) shell.hiddenFx));
    if (auto* on = proc.apvts.getParameter(prefix + "on")) on->setValueNotifyingHost(1.f);
    if (auto* mix = proc.apvts.getParameter(prefix + "mix")) mix->setValueNotifyingHost(mix->convertTo0to1(shell.hiddenMix));
    if (! board.isValid())
    {
        auto node = juce::ValueTree("w");
        node.setProperty("slot", 0, nullptr);
        node.setProperty("shellSlot", 0, nullptr);
        node.setProperty("param", "mix", nullptr);
        node.setProperty("label", juce::String(shell.name) + " bus", nullptr);
        node.setProperty("kind", "board", nullptr);
        node.setProperty("style", "board", nullptr);
        node.setProperty("series", 0, nullptr);
        proc.uiState.addChild(node, 0, nullptr);
        rebuildCanvas();
    }
    else
    {
        board.setProperty("label", juce::String(shell.name) + " bus", nullptr);
        board.setProperty("shellSlot", 0, nullptr);
    }
    float colour = shell.hiddenMix;
    for (int i = 0; i < proc.uiState.getNumChildren(); ++i)
    {
        auto child = proc.uiState.getChild(i);
        if (pb::cosmeticHiddenFx(child.getProperty("style").toString()) >= 0) colour += 0.035f;
    }
    proc.setHardwareColour(shell.hiddenFx, colour);
}

void KyotoAudioProcessorEditor::armPlacement()
{
    armedStyle = pb::styleToken(kindBox.getSelectedId());
    pendingSpecial = false;
    pendingFx = fxBrowser ? fxBrowser->getSelectedFx() : 0;
    pendingLabel = (pendingFx >= 0 && pendingFx < kt::kFxCount) ? kt::kFx[pendingFx].name : "Part";
    if (pb::styleSlot(armedStyle) == pb::SlotKind::Cosmetic)
        pendingLabel = kindBox.getText();
    placing = true;
    panel.placing = true;
    panel.armedStyle = armedStyle;
    status.setText("Theme is " + juce::String(theme.name) + ". Click a glowing " + kindBox.getText() + " bay.", juce::dontSendNotification);
    panel.repaint();
}

void KyotoAudioProcessorEditor::placeInSlot(int slot)
{
    if (!placing) return;
    const auto& shell = pb::kShells[juce::jlimit(0, pb::kShellCount - 1, shellIndex)];
    if (slot < 0 || slot >= shell.slotCount) return;
    if (!pb::styleFits(armedStyle, shell.slots[slot].kind) || slotOccupied(slot))
    {
        status.setText("That bay does not take this part.", juce::dontSendNotification);
        return;
    }
    captureSnapshot();
    const bool cosmetic = shell.slots[slot].kind == pb::SlotKind::Cosmetic;
    int dsp = -1;
    if (!cosmetic)
    {
        for (int i = 1; i < proc.slotCount(); ++i)
            if (auto* on = proc.apvts.getParameter("s" + juce::String(i + 1).paddedLeft('0', 2) + "on"); on && on->getValue() < 0.5f) { dsp = i; break; }
        if (dsp < 0) { status.setText("DSP bays are full.", juce::dontSendNotification); return; }
        const auto prefix = "s" + juce::String(dsp + 1).paddedLeft('0', 2);
        const int type = pendingSpecial ? pendingSpecialType : pendingFx;
        if (auto* param = proc.apvts.getParameter(prefix + "type")) param->setValueNotifyingHost(param->convertTo0to1((float) type));
        if (auto* on = proc.apvts.getParameter(prefix + "on")) on->setValueNotifyingHost(1.f);
        auto setValue = [this, &prefix](const juce::String& suffix, float value) { if (auto* param = proc.apvts.getParameter(prefix + suffix)) param->setValueNotifyingHost(param->convertTo0to1(value)); };
        setValue("amt", 0.45f); setValue("tone", 0.5f); setValue("mot", 0.3f); setValue("mix", 0.35f); setValue("shp", 0.5f);
    }
    auto node = juce::ValueTree("w");
    node.setProperty("slot", dsp, nullptr);
    node.setProperty("shellSlot", slot, nullptr);
    node.setProperty("param", "amt", nullptr);
    node.setProperty("label", pendingLabel, nullptr);
    const auto slotKind = shell.slots[slot].kind;
    node.setProperty("kind", slotKind == pb::SlotKind::Fader ? "slider" : slotKind == pb::SlotKind::Key ? "key" : slotKind == pb::SlotKind::Screen ? "wave" : slotKind == pb::SlotKind::Cosmetic ? "cosmetic" : "dial", nullptr);
    node.setProperty("style", armedStyle, nullptr);
    node.setProperty("series", proc.uiState.getNumChildren(), nullptr);
    proc.uiState.appendChild(node, nullptr);
    placing = false;
    pendingSpecial = false;
    panel.placing = false;
    rebuildCanvas();
    ensureMotherboard();
    status.setText("Snapped " + pendingLabel + " into " + juce::String(shell.slots[slot].name) + ". Wired from the motherboard.", juce::dontSendNotification);
}


void KyotoAudioProcessorEditor::rebuildCanvas()
{
    widgets.clear();
    for (int i = 0; i < proc.uiState.getNumChildren(); ++i)
    {
        auto child = proc.uiState.getChild(i);
        if (! child.hasType("w")) continue;
        const int widgetIndex = widgets.size();
        auto* w = widgets.add(new CanvasWidget(proc, child));
        w->setTheme(theme);
        w->selected = (widgetIndex == selectedChainWidget);
        w->onSelect = [this, widgetIndex] { selectedChainWidget = widgetIndex; for (auto* item : widgets) { item->selected = false; item->repaint(); } if (widgetIndex >= 0 && widgetIndex < widgets.size()) { widgets[widgetIndex]->selected = true; widgets[widgetIndex]->repaint(); } repaint(); };
        panel.addAndMakeVisible(w);
    }
    reflowSeries();
}

juce::String KyotoAudioProcessorEditor::serializeFxStack() const
{
    juce::Array<juce::var> arr;
    for (int i = 0; i < fxStack.getNumChildren(); ++i)
    {
        auto s = fxStack.getChild(i);
        auto* o = new juce::DynamicObject();
        o->setProperty("fx", (int)s.getProperty("fx", 0)); o->setProperty("name", s.getProperty("name").toString());
        o->setProperty("amount", (double)s.getProperty("amount", 0.5)); o->setProperty("tone", (double)s.getProperty("tone", 0.5));
        o->setProperty("motion", (double)s.getProperty("motion", 0.35)); o->setProperty("mix", (double)s.getProperty("mix", 0.4)); o->setProperty("shape", (double)s.getProperty("shape", 0.5));
        arr.add(juce::var(o));
    }
    return juce::JSON::toString(juce::var(arr));
}

void KyotoAudioProcessorEditor::restoreFxStack(const juce::String& json)
{
    fxStack.removeAllChildren(nullptr);
    auto parsed = juce::JSON::parse(json);
    if (auto* arr = parsed.getArray())
        for (auto& value : *arr)
            if (auto* o = value.getDynamicObject())
            {
                auto s = juce::ValueTree("step");
                s.setProperty("fx", (int)propertyOr(o, "fx", 0), nullptr); s.setProperty("name", o->getProperty("name").toString(), nullptr);
                s.setProperty("amount", (double)propertyOr(o, "amount", 0.5), nullptr); s.setProperty("tone", (double)propertyOr(o, "tone", 0.5), nullptr);
                s.setProperty("motion", (double)propertyOr(o, "motion", 0.35), nullptr); s.setProperty("mix", (double)propertyOr(o, "mix", 0.4), nullptr); s.setProperty("shape", (double)propertyOr(o, "shape", 0.5), nullptr);
                fxStack.appendChild(s, nullptr);
            }
    selectedFxStep = fxStack.getNumChildren() > 0 ? juce::jlimit(0, fxStack.getNumChildren()-1, selectedFxStep) : -1;
    if (selectedFxStep >= 0) selectFxStep(selectedFxStep);
    stackLabel.setText(fxStack.getNumChildren() > 0 ? "Stack " + juce::String(fxStack.getNumChildren()) + " / 16  -  custom effect lab" : "Empty effect  -  ready for a new build", juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::captureSnapshot()
{
    EditorSnapshot snapshot;
    proc.getStateInformation(snapshot.processorState);
    snapshot.fxStackJson = serializeFxStack();
    snapshot.selectedFx = selectedFxStep;
    undoStack.push_back(std::move(snapshot));
    if (undoStack.size() > 24) undoStack.erase(undoStack.begin());
    lastPublishedEffectId.clear();
}

void KyotoAudioProcessorEditor::restoreSnapshot(const EditorSnapshot& snapshot)
{
    proc.setStateInformation(snapshot.processorState.getData(), (int)snapshot.processorState.getSize());
    restoreFxStack(snapshot.fxStackJson);
    selectedFxStep = snapshot.selectedFx;
    selectedChainWidget = -1;
    rebuildCanvas();
    repaint();
}

void KyotoAudioProcessorEditor::undoLast()
{
    if (undoStack.empty()) { status.setText("Nothing to undo.", juce::dontSendNotification); return; }
    auto snapshot = std::move(undoStack.back());
    undoStack.pop_back();
    restoreSnapshot(snapshot);
    status.setText("Undid last builder change.", juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::removeSelectedChainStep()
{
    if (selectedChainWidget < 0 || selectedChainWidget >= widgets.size()) { status.setText("Select a control first.", juce::dontSendNotification); return; }
    auto node = widgets[selectedChainWidget]->node;
    if (node.getProperty("kind").toString() == "board") { status.setText("The motherboard stays. It is the start of the chain.", juce::dontSendNotification); return; }
    captureSnapshot();
    const int firstSlot = (int)node.getProperty("slot", -1);
    const int count = juce::jmax(1, (int)node.getProperty("slotCount", 1));
    if (firstSlot >= 0)
        for (int i=firstSlot; i<juce::jmin(proc.slotCount(), firstSlot+count); ++i)
            if (auto* on=proc.apvts.getParameter("s"+juce::String(i+1).paddedLeft('0',2)+"on")) on->setValueNotifyingHost(0.f);
    proc.uiState.removeChild(node, nullptr);
    selectedChainWidget = -1;
    rebuildCanvas();
    status.setText("Removed chain control. UNDO is available.", juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::removeSelectedFxStep()
{
    if (selectedFxStep < 0 || selectedFxStep >= fxStack.getNumChildren()) { status.setText("Select an FX step first.", juce::dontSendNotification); return; }
    captureSnapshot();
    fxStack.removeChild(selectedFxStep, nullptr);
    selectedFxStep = fxStack.getNumChildren() > 0 ? juce::jmin(selectedFxStep, fxStack.getNumChildren()-1) : -1;
    if (selectedFxStep >= 0) selectFxStep(selectedFxStep);
    stackLabel.setText(fxStack.getNumChildren() > 0 ? "Stack " + juce::String(fxStack.getNumChildren()) + " / 16  -  custom effect lab" : "Empty effect  -  ready for a new build", juce::dontSendNotification);
    repaint();
}

void KyotoAudioProcessorEditor::addSeriesStep()
{
    const int kindId = kindBox.getSelectedId();
    juce::String kind = kindId == 2 ? "slider" : kindId == 3 ? "key" : kindId == 4 ? "wave" : "dial";
    if (kindId == 5)
    {
        // A custom catalog selection is loaded into FX Builder when clicked; the preset shelf
        // also exposes saved custom effects for placing them as one chain stage.
        auto name = presetBox.getText().trim().fromFirstOccurrenceOf("-", false, false).trim();
        auto file = effectDir().getChildFile(name + ".json");
        if (!file.existsAsFile())
        {
            status.setText("Choose a saved custom effect from the preset shelf first.", juce::dontSendNotification);
            return;
        }
        auto parsed = juce::JSON::parse(file.loadFileAsString());
        auto* obj = parsed.getDynamicObject();
        auto* steps = obj != nullptr ? obj->getProperty("steps").getArray() : nullptr;
        if (steps == nullptr || steps->isEmpty()) { status.setText("Custom effect has no steps.", juce::dontSendNotification); return; }
        juce::Rectangle<int> room;
        if (! findAutoCell(widgets.size(), "stack", room)) { status.setText("No room left. Build a custom FX to keep the chain compact.", juce::dontSendNotification); return; }
        captureSnapshot();
        int first = -1;
        for (auto& value : *steps)
        {
            auto* src = value.getDynamicObject();
            if (!src) continue;
            int slot = -1;
            for (int i = 0; i < proc.slotCount(); ++i)
                if (auto* on = proc.apvts.getParameter("s" + juce::String(i + 1).paddedLeft('0', 2) + "on"); on && on->getValue() < 0.5f) { slot = i; break; }
            if (slot < 0) { status.setText("Not enough DSP slots for this custom effect.", juce::dontSendNotification); undoLast(); return; }
            if (first < 0) first = slot;
            const auto prefix = "s" + juce::String(slot + 1).paddedLeft('0', 2);
            auto setP = [&](const juce::String& key, float v) { if (auto* p = proc.apvts.getParameter(prefix + key)) p->setValueNotifyingHost(p->convertTo0to1(v)); };
            setP("type", (float)(int)propertyOr(src, "fx", 0)); setP("amt", (float)propertyOr(src, "amount", 0.5)); setP("tone", (float)propertyOr(src, "tone", 0.5)); setP("mot", (float)propertyOr(src, "motion", 0.35)); setP("mix", (float)propertyOr(src, "mix", 0.4)); setP("shp", (float)propertyOr(src, "shape", 0.5));
            if (auto* p = proc.apvts.getParameter(prefix + "on")) p->setValueNotifyingHost(1.f);
        }
        auto node = juce::ValueTree("w"); node.setProperty("slot", first, nullptr); node.setProperty("param", "amt", nullptr); node.setProperty("label", name, nullptr); node.setProperty("kind", "stack", nullptr); node.setProperty("series", proc.uiState.getNumChildren(), nullptr); node.setProperty("slotCount", (int)steps->size(), nullptr); proc.uiState.appendChild(node, nullptr);
        rebuildCanvas(); status.setText("Added custom effect: " + name, juce::dontSendNotification); return;
    }

    juce::Rectangle<int> room;
    if (! findAutoCell(widgets.size(), kind, room))
    {
        status.setText("No safe room left. Use FX Builder to build a custom effect.", juce::dontSendNotification);
        return;
    }
    if (kind == "key")
    {
        captureSnapshot();
        auto node = juce::ValueTree("w"); node.setProperty("slot", -1, nullptr); node.setProperty("param", "amt", nullptr); node.setProperty("label", "C" + juce::String(widgets.size() % 8 + 3), nullptr); node.setProperty("kind", "key", nullptr); node.setProperty("note", 60 + widgets.size(), nullptr); node.setProperty("series", proc.uiState.getNumChildren(), nullptr); node.setProperty("slotCount", 1, nullptr); proc.uiState.appendChild(node, nullptr);
    }
    else if (kind == "wave")
    {
        captureSnapshot();
        auto node = juce::ValueTree("w"); node.setProperty("slot", -1, nullptr); node.setProperty("param", "amt", nullptr); node.setProperty("label", "WAV", nullptr); node.setProperty("kind", "wave", nullptr); node.setProperty("series", proc.uiState.getNumChildren(), nullptr); node.setProperty("slotCount", 1, nullptr); proc.uiState.appendChild(node, nullptr);
    }
    else
    {
        const int type = fxBrowser ? fxBrowser->getSelectedFx() : 0;
        int slot = -1;
        for (int i = 0; i < proc.slotCount(); ++i)
            if (auto* on = proc.apvts.getParameter("s" + juce::String(i + 1).paddedLeft('0', 2) + "on"); on && on->getValue() < 0.5f) { slot = i; break; }
        if (slot < 0) { status.setText("DSP slots are full - build a custom FX instead.", juce::dontSendNotification); return; }
        captureSnapshot();
        const auto prefix = "s" + juce::String(slot + 1).paddedLeft('0', 2);
        if (auto* t = proc.apvts.getParameter(prefix + "type")) t->setValueNotifyingHost(t->convertTo0to1((float)type));
        if (auto* on = proc.apvts.getParameter(prefix + "on")) on->setValueNotifyingHost(1.f);
        auto setValue = [this, &prefix](const juce::String& suffix, float value) { if (auto* p = proc.apvts.getParameter(prefix + suffix)) p->setValueNotifyingHost(p->convertTo0to1(value)); };
        setValue("amt", (float)fxAmount.getValue()); setValue("tone", (float)fxTone.getValue()); setValue("mot", (float)fxMotion.getValue()); setValue("mix", (float)fxMix.getValue()); setValue("shp", (float)fxShape.getValue());
        auto node = juce::ValueTree("w"); node.setProperty("slot", slot, nullptr); node.setProperty("param", "amt", nullptr); node.setProperty("label", kt::kFx[type].name, nullptr); node.setProperty("kind", kind, nullptr); node.setProperty("series", proc.uiState.getNumChildren(), nullptr); node.setProperty("slotCount", 1, nullptr); proc.uiState.appendChild(node, nullptr);
    }
    rebuildCanvas();
    status.setText("Added safely - " + juce::String(widgets.size()) + " control" + (widgets.size() == 1 ? "" : "s"), juce::dontSendNotification);
}

juce::File KyotoAudioProcessorEditor::sessionFile() const
{
    auto f = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("KYOTRIPPAH");
    f.createDirectory();
    return f.getChildFile("session.json");
}
juce::File KyotoAudioProcessorEditor::moduleDir() const
{
    auto dir = sessionFile().getParentDirectory().getChildFile(proc.isFx() ? "fx" : "kyoto");
    dir.createDirectory();
    return dir;
}
juce::File KyotoAudioProcessorEditor::effectDir() const
{
    auto dir = sessionFile().getParentDirectory().getChildFile("effects");
    dir.createDirectory();
    return dir;
}

void KyotoAudioProcessorEditor::login()
{
    const auto user = userBox.getText().trim();
    const auto pass = passBox.getText();
    juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe, user, pass] {
        auto r = kt::login(user, pass);
        juce::MessageManager::callAsync([safe, r, user] {
            if (safe == nullptr) return;
            if (!r.ok || r.token.isEmpty()) { safe->status.setText(r.error.isEmpty() ? "Login failed" : r.error, juce::dontSendNotification); return; }
            safe->token = r.token; safe->account = r.user.isNotEmpty() ? r.user : user;
            safe->isAdmin = (r.role == "super" || safe->account.equalsIgnoreCase("Trippah") || safe->account.equalsIgnoreCase("Goonr"));
            auto* o = new juce::DynamicObject(); o->setProperty("user", safe->account); o->setProperty("token", safe->token); o->setProperty("admin", safe->isAdmin);
            safe->sessionFile().replaceWithText(juce::JSON::toString(juce::var(o)));
            safe->setLoggedIn(true); safe->status.setText("Signed in as " + safe->account, juce::dontSendNotification);
        });
    }).detach();
}

void KyotoAudioProcessorEditor::logout()
{
    sessionFile().deleteFile();
    token.clear();
    account.clear();
    setLoggedIn(false);
    status.setText("Logged out", juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::sendChat()
{
    const auto text = msgBox.getText().trim(); const auto tokenCopy = token;
    if (text.isEmpty() || tokenCopy.isEmpty()) return;
    juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe, text, tokenCopy] {
        auto r = kt::sendChat(tokenCopy, text);
        juce::MessageManager::callAsync([safe, r] { if (safe == nullptr) return; if (!r.ok) safe->status.setText(r.error, juce::dontSendNotification); else { safe->msgBox.clear(); safe->refreshFeed(); } });
    }).detach();
}

void KyotoAudioProcessorEditor::chatUtility(const juce::String& selectedAction)
{
    if (token.isEmpty()) return;
    const auto action = selectedAction.trim(), target = utilityBox.getText().trim(), message = msgBox.getText().trim(), tokenCopy = token;
    juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe, action, target, message, tokenCopy] {
        kt::DreamResult r;
        if (action == "dm_list") r = kt::getDM(tokenCopy, target);
        else if (action == "dm_send") r = kt::sendDM(tokenCopy, target, message);
        else if (action == "social_list") r = kt::getSocial(tokenCopy);
        else if (action == "friend_request" || action == "friend_accept" || action == "friend_decline" || action == "friend_remove" || action == "wav_request") r = kt::friendRequest(tokenCopy, action, target);
        else if (action == "react_heart") r = kt::react(tokenCopy, "chat", target, "heart");
        else if (action == "chat_delete") { auto* o=new juce::DynamicObject(); o->setProperty("id",target); r=kt::postAction("chat_delete",juce::var(o),tokenCopy); }
        else if (action == "chat_clear") r=kt::postAction("chat_clear",juce::var(new juce::DynamicObject()),tokenCopy);
        else if (action == "create_thread") { auto* o=new juce::DynamicObject(); o->setProperty("text",message); r=kt::postAction("create_thread",juce::var(o),tokenCopy); }
        else if (action == "comment") { auto* o=new juce::DynamicObject(); o->setProperty("threadId", target.isEmpty()? message : target); o->setProperty("text", message.isEmpty()? target : message); r=kt::postAction("comment",juce::var(o),tokenCopy); }
        else r=kt::postAction(action,juce::var(new juce::DynamicObject()),tokenCopy);
        juce::MessageManager::callAsync([safe, r, action] {
            if (safe == nullptr) return;
            if (!r.ok) { safe->logBox.setText(r.error.isEmpty() ? r.raw : r.error); safe->status.setText("Utility failed: " + r.error, juce::dontSendNotification); return; }
            juce::String out;
            if (action == "list_threads")
            {
                if (auto* arr = r.parsed.getDynamicObject() ? r.parsed.getDynamicObject()->getProperty("threads").getArray() : nullptr)
                    for (int i=0;i<arr->size();++i) if (auto* t=arr->getReference(i).getDynamicObject()) out << "THREAD " << juce::String(i+1) << "  " << t->getProperty("title").toString() << "\n" << t->getProperty("text").toString() << "\nby " << t->getProperty("user").toString() << "\n\n";
            }
            else out = r.raw.isEmpty() ? r.body : r.raw;
            safe->logBox.setText(out.isEmpty() ? r.body : out); safe->renderEffectLinks(out.isEmpty() ? r.body : out);
            if (action == "social_list" || action == "dm_list")
            {
                juce::Array<SocialRail::Person> people;
                auto* obj = r.parsed.getDynamicObject();
                if (obj != nullptr)
                {
                    for (auto key : { "friendsDetailed", "friends", "online", "users", "messages" })
                        if (auto* arr = obj->getProperty(key).getArray())
                            for (auto& item : *arr)
                            {
                                SocialRail::Person person;
                                if (auto* o = item.getDynamicObject())
                                {
                                    person.name = o->getProperty("name").toString();
                                    if (person.name.isEmpty()) person.name = o->getProperty("user").toString();
                                    if (person.name.isEmpty()) person.name = o->getProperty("from").toString();
                                    person.themeId = o->getProperty("theme").toString();
                                    person.detail = o->getProperty("text").toString();
                                }
                                else person.name = item.toString();
                                person.online = juce::String(key) == "online";
                                if (person.detail.isEmpty()) person.detail = key;
                                if (person.name.isNotEmpty()) people.add(person);
                            }
                }
                if (! people.isEmpty()) safe->socialRail.setPeople(people);
            }
            if (action == "create_thread" || action == "comment") { safe->setCenterMode(1); safe->refreshFeed(); }
            safe->status.setText("DreamShare utility complete", juce::dontSendNotification);
            if (action == "chat_list") safe->refreshFeed();
        });
    }).detach();
}

void KyotoAudioProcessorEditor::deleteCatalogId(const juce::String& id)
{
    if (!isAdmin || id.isEmpty()) return;
    const auto tokenCopy = token; juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe, id, tokenCopy] { auto r=kt::deleteModule(tokenCopy,id); juce::MessageManager::callAsync([safe,r]{ if(safe==nullptr)return; safe->status.setText(r.ok?"CATALOG ITEM REMOVED":"REMOVE FAILED: "+r.error,juce::dontSendNotification); if(r.ok){safe->selectedCatalogId.clear();safe->refreshCatalog();} }); }).detach();
}

void KyotoAudioProcessorEditor::clearEffectLinks()
{
    feedEffectButtons.clear();
}

void KyotoAudioProcessorEditor::renderEffectLinks(const juce::String& text)
{
    clearEffectLinks();
    const juce::String marker = "[KYOTRIPPAH_EFFECT:";
    int pos = 0;
    int count = 0;
    while (count < 8)
    {
        const int start = text.indexOf(pos, marker);
        if (start < 0) break;
        const int idStart = start + marker.length();
        const int end = text.indexOfChar(idStart, ']');
        if (end <= idStart) { pos = idStart; continue; }
        const auto id = text.substring(idStart, end).trim();
        if (id.isEmpty()) { pos = end + 1; continue; }
        auto tail = text.substring(end + 1).upToFirstOccurrenceOf("\n", false, false).trim();
        if (tail.isEmpty()) tail = "Shared custom effect";
        auto* button = feedEffectButtons.add(new juce::TextButton("LOAD FX  -  " + tail));
        button->onClick = [this, id] { loadCatalogId(id, {}); };
        addAndMakeVisible(button);
        ++count;
        pos = end + 1;
    }
    resized();
}


void KyotoAudioProcessorEditor::setCenterMode(int mode)
{
    centerMode = mode == 1 ? 1 : 0;
    catalogModeBtn.setToggleState(centerMode == 0, juce::dontSendNotification);
    threadsModeBtn.setToggleState(centerMode == 1, juce::dontSendNotification);
    catalogView.setViewedComponent(centerMode == 0 ? static_cast<juce::Component*>(&catalogHolder) : static_cast<juce::Component*>(&threadHolder), false);
    if (centerMode == 1) rebuildThreadBoard();
    showTab(tab);
}

void KyotoAudioProcessorEditor::setRailMode(int mode)
{
    railMode = mode == 1 ? 1 : 0;
    railChatBtn.setToggleState(railMode == 0, juce::dontSendNotification);
    railOnlineBtn.setToggleState(railMode == 1, juce::dontSendNotification);
    socialRail.setMode(railMode);
    showTab(tab);
}

void KyotoAudioProcessorEditor::rebuildCenter()
{
    const int catalogW = juce::jmax(220, catalogView.getWidth() - 18);
    const int cols = catalogW >= 640 ? 3 : (catalogW >= 420 ? 2 : 1);
    const int gap = 8;
    const int cardW = juce::jmax(160, (catalogW - gap * (cols + 1)) / cols);
    const int cardH = 96;
    auto* holder = centerMode == 0 ? &catalogHolder : &threadHolder;
    holder->setSize(catalogW, juce::jmax(catalogView.getHeight(), ((holder->getNumChildComponents() + cols - 1) / juce::jmax(1, cols)) * (cardH + gap) + gap));
    for (int i = 0; i < holder->getNumChildComponents(); ++i)
    {
        const int col = i % cols, row = i / cols;
        holder->getChildComponent(i)->setBounds(gap + col * (cardW + gap), gap + row * (cardH + gap), cardW, cardH);
    }
}

void KyotoAudioProcessorEditor::rebuildThreadBoard()
{
    threadHolder.removeAllChildren();
    int i = 0;
    for (const auto& t : threads)
    {
        auto* card = new BoardCard();
        card->title = t.title.isEmpty() ? "Thread" : t.title;
        card->meta = t.user + "  -  " + juce::String(t.comments) + " replies  -  " + t.themeId;
        card->body = t.text;
        card->themeId = t.themeId;
        const auto id = t.id;
        card->onOpen = [this, id] { selectedThreadId = id; utilityBox.setText(id, juce::dontSendNotification); status.setText("Reply target " + id, juce::dontSendNotification); };
        threadHolder.addAndMakeVisible(card);
        ++i;
    }
    juce::ignoreUnused(i);
    rebuildCenter();
}

void KyotoAudioProcessorEditor::refreshFeed()
{
    if (token.isEmpty()) return;
    const auto tokenCopy = token; juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe, tokenCopy] {
        auto r = kt::getFeed(tokenCopy);
        juce::MessageManager::callAsync([safe, r] {
            if (safe == nullptr) return;
            if (!r.ok) { safe->clearEffectLinks(); safe->status.setText(r.error.isEmpty()?"DreamShare feed failed":r.error, juce::dontSendNotification); safe->logBox.setText(r.raw); return; }
            juce::String log;
            juce::Array<SocialRail::Bubble> bubbles;
            if (auto* arr = r.parsed.getDynamicObject() ? r.parsed.getDynamicObject()->getProperty("chat").getArray() : nullptr)
                for (auto& item : *arr) if (auto* m=item.getDynamicObject())
                {
                    SocialRail::Bubble b;
                    b.id = m->getProperty("id").toString();
                    b.user = m->getProperty("user").toString();
                    b.text = m->getProperty("text").toString();
                    b.themeId = m->getProperty("theme").toString();
                    if (b.themeId.isEmpty()) b.themeId = "trippah";
                    bubbles.add(b);
                    log << b.user << ": " << b.text << "\n";
                }
            safe->socialRail.setBubbles(bubbles);
            safe->threads.clear();
            if (auto* arr = r.parsed.getDynamicObject() ? r.parsed.getDynamicObject()->getProperty("threads").getArray() : nullptr)
                for (auto& item : *arr) if (auto* t=item.getDynamicObject())
                {
                    KyotoAudioProcessorEditor::ThreadItem th;
                    th.id = t->getProperty("id").toString();
                    th.user = t->getProperty("user").toString();
                    th.title = t->getProperty("title").toString();
                    th.text = t->getProperty("text").toString();
                    th.themeId = t->getProperty("theme").toString();
                    if (auto* comments = t->getProperty("comments").getArray()) th.comments = comments->size();
                    safe->threads.add(th);
                }
            if (safe->railMode == 1)
            {
                juce::Array<SocialRail::Person> people;
                auto* root = r.parsed.getDynamicObject();
                auto* arr = root ? root->getProperty("onlineUsers").getArray() : nullptr;
                if (arr == nullptr && root) arr = root->getProperty("online").getArray();
                if (arr != nullptr)
                    for (auto& item : *arr)
                    {
                        SocialRail::Person person;
                        if (auto* o = item.getDynamicObject()) { person.name = o->getProperty("name").toString(); person.themeId = o->getProperty("theme").toString(); }
                        else person.name = item.toString();
                        person.detail = "online";
                        person.online = true;
                        if (person.name.isNotEmpty()) people.add(person);
                    }
                safe->socialRail.setPeople(people);
            }
            safe->rebuildThreadBoard();
            safe->logBox.setText(log.isEmpty()?r.body:log); safe->renderEffectLinks(log); safe->status.setText(safe->centerMode==0?"Catalog browser":"Threads", juce::dontSendNotification);
        });
    }).detach();
}

void KyotoAudioProcessorEditor::refreshCatalog()
{
    if (token.isEmpty()) return;
    const auto tokenCopy = token; juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe, tokenCopy] {
        auto r = kt::getCatalog(tokenCopy);
        juce::MessageManager::callAsync([safe, r] {
            if (safe == nullptr) return;
            safe->catalog.clear(); safe->catalogHolder.removeAllChildren();
            auto* mods = r.parsed.getDynamicObject() ? r.parsed.getDynamicObject()->getProperty("modules").getArray() : nullptr;
            if (mods == nullptr) { safe->status.setText(r.ok?"Catalog empty":"Catalog needs module_list on the worker", juce::dontSendNotification); safe->refreshEffectBox(); return; }
            int i=0;
            for (auto& item:*mods)
            {
                auto* m=item.getDynamicObject(); if(!m) continue;
                CatalogItem c{m->getProperty("id").toString(),m->getProperty("name").toString(),m->getProperty("face").toString(),m->getProperty("author").toString()}; safe->catalog.add(c);
                auto* card=new BoardCard(); card->title=c.name; card->meta=c.face+"  -  "+c.author; card->body="Open in the builder"; card->themeId=safe->theme.id;
                card->onOpen=[safe,id=c.id,name=c.name]{ if(safe!=nullptr){safe->selectedCatalogId=id;safe->loadCatalogId(id,name);} }; safe->catalogHolder.addAndMakeVisible(card); ++i;
            }
            safe->catalogHolder.setSize(juce::jmax(220, safe->catalogView.getWidth()-18), juce::jmax(220, ((i+1)/2)*92)); safe->catalogView.setViewedComponent(&safe->catalogHolder,false); safe->resized(); safe->refreshEffectBox();
        });
    }).detach();
}

void KyotoAudioProcessorEditor::loadCatalogId(const juce::String& id, const juce::String& name)
{
    if (id.isEmpty())
    {
        auto file = moduleDir().getChildFile(name + ".json");
        if (! file.existsAsFile()) file = effectDir().getChildFile(name + ".json");
        if (! file.existsAsFile()) return;
        auto parsed = juce::JSON::parse(file.loadFileAsString());
        if (auto* obj = parsed.getDynamicObject())
        {
            if (obj->getProperty("face").toString() == "effect" || obj->getProperty("format").toString() == "kyoteppah-effect-1")
            {
                fxStack.removeAllChildren(nullptr);
                if (auto* steps=obj->getProperty("steps").getArray()) for(auto& v:*steps) if(auto* so=v.getDynamicObject()){auto st=juce::ValueTree("step");st.setProperty("fx",(int)so->getProperty("fx"),nullptr);st.setProperty("name",so->getProperty("name").toString(),nullptr);st.setProperty("amount",(double)(so->hasProperty("amount") ? so->getProperty("amount") : juce::var(0.5)),nullptr);st.setProperty("tone",(double)(so->hasProperty("tone") ? so->getProperty("tone") : juce::var(0.5)),nullptr);st.setProperty("motion",(double)(so->hasProperty("motion") ? so->getProperty("motion") : juce::var(0.35)),nullptr);st.setProperty("mix",(double)(so->hasProperty("mix") ? so->getProperty("mix") : juce::var(0.4)),nullptr);st.setProperty("shape",(double)(so->hasProperty("shape") ? so->getProperty("shape") : juce::var(0.5)),nullptr);fxStack.appendChild(st,nullptr);}
                effectNameBox.setText(obj->getProperty("name").toString(), juce::dontSendNotification); if (fxStack.getNumChildren() > 0) selectFxStep(0); showTab(2); return;
            }
            nameBox.setText(obj->getProperty("name").toString(), juce::dontSendNotification);
            if (auto* slots = obj->getProperty("slots").getArray())
            {
                for (int i = 0; i < proc.slotCount(); ++i)
                {
                    const auto prefix = "s" + juce::String(i + 1).paddedLeft('0', 2);
                    if (auto* on = proc.apvts.getParameter(prefix + "on")) on->setValueNotifyingHost(0.f);
                }
                for (int i = 0; i < juce::jmin(proc.slotCount(), slots->size()); ++i)
                {
                    auto* src = slots->getReference(i).getDynamicObject(); if (!src) continue;
                    const auto prefix = "s" + juce::String(i + 1).paddedLeft('0', 2);
                    if (auto* p = proc.apvts.getParameter(prefix + "on")) p->setValueNotifyingHost((src->hasProperty("on") ? src->getProperty("on") : juce::var(true)) ? 1.f : 0.f);
                    if (auto* p = proc.apvts.getParameter(prefix + "type")) p->setValueNotifyingHost(p->convertTo0to1((float)(src->hasProperty("fx") ? src->getProperty("fx") : juce::var(0))));
                    if (auto* p = proc.apvts.getParameter(prefix + "amt")) p->setValueNotifyingHost(p->convertTo0to1((float)(src->hasProperty("amount") ? src->getProperty("amount") : juce::var(0.5))));
                    if (auto* p = proc.apvts.getParameter(prefix + "tone")) p->setValueNotifyingHost(p->convertTo0to1((float)(src->hasProperty("tone") ? src->getProperty("tone") : juce::var(0.5))));
                    if (auto* p = proc.apvts.getParameter(prefix + "mot")) p->setValueNotifyingHost(p->convertTo0to1((float)(src->hasProperty("motion") ? src->getProperty("motion") : juce::var(0.35))));
                    if (auto* p = proc.apvts.getParameter(prefix + "mix")) p->setValueNotifyingHost(p->convertTo0to1((float)(src->hasProperty("mix") ? src->getProperty("mix") : juce::var(0.4))));
                    if (auto* p = proc.apvts.getParameter(prefix + "shp")) p->setValueNotifyingHost(p->convertTo0to1((float)(src->hasProperty("shape") ? src->getProperty("shape") : juce::var(0.5))));
                }
            }
            if (auto* md = obj->getProperty("machineDesign").getDynamicObject())
            {
                machineDesign = MachineDesign::fromVar(juce::var(md));
                syncMachineDesignToUi();
            }
            if (auto* warr = obj->getProperty("widgets").getArray())
            {
                proc.uiState.removeAllChildren(nullptr);
                syncMachineDesignToUi();
                for (auto& item : *warr)
                {
                    auto* wsrc = item.getDynamicObject();
                    if (wsrc == nullptr) continue;
                    auto w = juce::ValueTree("w");
                    w.setProperty("slot", (int) wsrc->getProperty("slot"), nullptr);
                    w.setProperty("param", nativeKey(wsrc->getProperty("param").toString()), nullptr);
                    w.setProperty("label", wsrc->getProperty("label").toString(), nullptr);
                    w.setProperty("kind", wsrc->getProperty("kind").toString(), nullptr);
                    w.setProperty("slotCount", (int)propertyOr(wsrc, "slotCount", 1), nullptr);
                    w.setProperty("peaks", wsrc->getProperty("peaks").toString(), nullptr);
                    proc.uiState.appendChild(w, nullptr);
                }
            }
            showTab(1);
        }
        return;
    }
    const auto tokenCopy = token; juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe, id, tokenCopy] {
        auto r = kt::getModule(tokenCopy, id);
        juce::MessageManager::callAsync([safe, r] {
            if (safe == nullptr) return;
            if (!r.ok) { safe->status.setText(r.error, juce::dontSendNotification); return; }
            auto* root = r.parsed.getDynamicObject(); if (!root) return;
            auto mod = root->getProperty("module"); auto* mo = mod.getDynamicObject(); if (!mo) return;
            const auto modName = mo->getProperty("name").toString();
            if (mo->getProperty("face").toString() == "effect" || mo->getProperty("format").toString() == "kyoteppah-effect-1")
            {
                safe->fxStack.removeAllChildren(nullptr);
                if (auto* steps=mo->getProperty("steps").getArray()) for(auto& v:*steps) if(auto* so=v.getDynamicObject())
                {
                    auto st=juce::ValueTree("step"); st.setProperty("fx",(int)so->getProperty("fx"),nullptr); st.setProperty("name",so->getProperty("name").toString(),nullptr);
                    st.setProperty("amount",(double)propertyOr(so, "amount", 0.5),nullptr); st.setProperty("tone",(double)propertyOr(so, "tone", 0.5),nullptr); st.setProperty("motion",(double)propertyOr(so, "motion", 0.35),nullptr); st.setProperty("mix",(double)propertyOr(so, "mix", 0.4),nullptr); st.setProperty("shape",(double)propertyOr(so, "shape", 0.5),nullptr); safe->fxStack.appendChild(st,nullptr);
                }
                auto downloaded = safe->effectDir().getChildFile(modName + ".json"); downloaded.replaceWithText(juce::JSON::toString(mod)); safe->refreshEffectBox();
                safe->lastPublishedEffectId = mo->getProperty("id").toString(); safe->effectNameBox.setText(modName, juce::dontSendNotification); safe->selectedFxStep = safe->fxStack.getNumChildren()>0?0:-1; if(safe->selectedFxStep>=0)safe->selectFxStep(0); safe->showTab(2); safe->status.setText("Loaded effect: "+modName,juce::dontSendNotification);
            }
            else
            {
                auto file=safe->moduleDir().getChildFile(modName+".json"); file.replaceWithText(juce::JSON::toString(mod));
                if (mo->hasProperty("machineDesign")) { safe->machineDesign = MachineDesign::fromVar(mo->getProperty("machineDesign")); safe->syncMachineDesignToUi(); }
                safe->loadCatalogId({},modName); safe->status.setText("Loaded catalog module",juce::dontSendNotification);
            }
        });
    }).detach();
}

void KyotoAudioProcessorEditor::applyTheme(const juce::String& id)
{
    theme = kt::themeById(id);
    proc.uiState.setProperty("theme", theme.id, nullptr);
    for (int i = 0; i < kt::kThemeCount; ++i)
        if (juce::String(kt::kThemes[i].id).equalsIgnoreCase(theme.id))
        {
            themeBox.setSelectedId(i + 1, juce::dontSendNotification);
            break;
        }
    machineDesign.theme = theme.id;
    machineDesign.normalizeThemeIds();
    panel.theme = theme;
    for (auto* w : widgets) w->setTheme(theme);
    panel.repaint();
    if (fxBrowser) fxBrowser->setTheme(theme);
    for (auto* e : { &logBox, &msgBox, &utilityBox, &userBox, &passBox, &nameBox, &effectNameBox })
    {
        e->setColour(juce::TextEditor::backgroundColourId, kt::c(theme.bg).brighter(0.03f));
        e->setColour(juce::TextEditor::textColourId, kt::c(theme.text));
        e->setColour(juce::TextEditor::outlineColourId, kt::c(theme.border));
        e->setColour(juce::TextEditor::focusedOutlineColourId, kt::c(theme.accent).withAlpha(0.85f));
        e->setColour(juce::TextEditor::highlightColourId, kt::c(theme.accent).withAlpha(0.30f));
        e->setColour(juce::TextEditor::highlightedTextColourId, kt::c(theme.text));
        e->setColour(juce::TextEditor::shadowColourId, juce::Colours::transparentBlack);
        e->setFont(kt::font(theme, 13.f));
    }
    for (auto* l : { &status, &whoLabel, &fxAmountLabel, &fxToneLabel, &fxMotionLabel, &fxMixLabel, &fxShapeLabel, &stackLabel })
    {
        l->setColour(juce::Label::textColourId, kt::c(theme.text));
        l->setFont(kt::font(theme, 12.f, true));
    }
    for (auto* w : widgets) w->setTheme(theme);
    kLookAndFeel.setTheme(theme);
    socialRail.setHostTheme(theme);
    kLookAndFeel.setColour(juce::PopupMenu::backgroundColourId, kt::c(theme.panel));
    kLookAndFeel.setColour(juce::PopupMenu::textColourId, kt::c(theme.text));
    kLookAndFeel.setColour(juce::PopupMenu::highlightedBackgroundColourId, kt::c(theme.accent).withAlpha(0.32f));
    kLookAndFeel.setColour(juce::PopupMenu::highlightedTextColourId, kt::c(theme.text));
    for (auto* b : { &shareBtn, &chainBtn, &fxBtn, &logoutBtn, &feedBtn, &chatRefreshBtn, &threadsBtn, &socialBtn, &dmBtn, &adminDeleteBtn, &sendBtn, &utilityGoBtn, &addBtn, &chainBreakBtn, &chainMixBtn, &chainRemoveBtn, &chainUndoBtn, &saveBtn, &upBtn, &wavBtn, &fxAddBtn, &fxBreakBtn, &fxMixBtn, &fxRandomBtn, &fxClearBtn, &fxSaveBtn, &fxUpBtn, &fxShareChatBtn, &fxShareThreadBtn, &fxRemoveBtn, &fxUndoBtn, &pluginViewBtn, &pluginBackBtn, &newMachineBtn, &randomMachineBtn, &catalogModeBtn, &threadsModeBtn, &railChatBtn, &railOnlineBtn, &proToggleBtn, &wizardNextBtn, &wizardSkipBtn })
    {
        b->setColour(juce::TextButton::buttonColourId, kt::c(theme.panel).brighter(0.08f));
        b->setColour(juce::TextButton::buttonOnColourId, kt::c(theme.accent).withAlpha(0.30f));
        b->setColour(juce::TextButton::textColourOffId, kt::c(theme.text));
        b->setColour(juce::TextButton::textColourOnId, kt::c(theme.text));
    }
    syncMachineDesignToUi();
    repaint();
}

void KyotoAudioProcessorEditor::applyPlaygroundTheme(const juce::String& id)
{
    playgroundTheme = kt::themeById(id);
    proc.uiState.setProperty("playgroundTheme", playgroundTheme.id, nullptr);
    machineDesign.theme = playgroundTheme.id;
    machineDesign.normalizeThemeIds();
    // Isolate: only the builder canvas + machine widgets use playground theme.
    panel.theme = playgroundTheme;
    for (auto* w : widgets) w->setTheme(playgroundTheme);
    panel.repaint();
    for (int i = 0; i < kt::kThemeCount; ++i)
        if (juce::String(kt::kThemes[i].id).equalsIgnoreCase(playgroundTheme.id))
        {
            playgroundThemeBox.setSelectedId(i + 1, juce::dontSendNotification);
            break;
        }
    syncMachineDesignToUi();
    repaint();
}

void KyotoAudioProcessorEditor::enterBuilderWizard()
{
    // Step 1 = hardware template, Step 2 = playground theme (previewed on that template).
    builderWizardStep = 1;
    applyShell(shellBox.getSelectedId() - 1);
    showTab(1);
    status.setText("Step 1 of 2 - pick a hardware template. Preview updates as you change the shell.", juce::dontSendNotification);
    resized();
    repaint();
}

void KyotoAudioProcessorEditor::advanceBuilderWizard()
{
    if (builderWizardStep == 1)
    {
        applyShell(shellBox.getSelectedId() - 1);
        builderWizardStep = 2;
        status.setText("Step 2 of 2 - pick a playground theme. It colours only the template preview and builder.", juce::dontSendNotification);
        resized();
        repaint();
        return;
    }
    if (builderWizardStep == 2)
    {
        const int i = playgroundThemeBox.getSelectedId() - 1;
        if (i >= 0 && i < kt::kThemeCount)
            applyPlaygroundTheme(kt::kThemes[i].id);
        applyShell(shellBox.getSelectedId() - 1);
        builderWizardStep = 0;
        proc.uiState.setProperty("builderWizardDone", true, nullptr);
        status.setText("Builder ready - effects are listed by category on the left.", juce::dontSendNotification);
        resized();
        repaint();
    }
}

juce::String KyotoAudioProcessorEditor::deriveCategoriesFromStack() const
{
    bool used[8] = {};
    for (int i = 0; i < fxStack.getNumChildren(); ++i)
    {
        const int fx = (int) fxStack.getChild(i).getProperty("fx", 0);
        if (fx >= 0 && fx < kt::kFxCount)
        {
            const int fam = kt::kFx[fx].family;
            if (fam >= 0 && fam < 8) used[fam] = true;
        }
    }
    juce::StringArray cats;
    for (int f = 0; f < 8; ++f)
        if (used[f]) cats.add(kt::kFxFamilyNames[f]);
    return cats.joinIntoString(" - ");
}

void KyotoAudioProcessorEditor::loadWav()
{
    auto chooser = std::make_shared<juce::FileChooser>("Load WAV", juce::File(), "*.wav");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc) {
            auto file = fc.getResult();
            if (! file.existsAsFile()) return;
            juce::AudioFormatManager fm;
            fm.registerBasicFormats();
            std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(file));
            if (reader == nullptr) { status.setText("Could not read WAV", juce::dontSendNotification); return; }
            juce::AudioBuffer<float> buf((int) reader->numChannels, (int) reader->lengthInSamples);
            reader->read(&buf, 0, (int) reader->lengthInSamples, 0, true, true);
            proc.loadSample(buf, reader->sampleRate);
            juce::String peaks;
            const int bins = 48;
            const int hop = juce::jmax(1, buf.getNumSamples() / bins);
            for (int b = 0; b < bins; ++b)
            {
                float m = 0.f;
                for (int i = 0; i < hop && b * hop + i < buf.getNumSamples(); ++i)
                    m = juce::jmax(m, std::abs(buf.getSample(0, b * hop + i)));
                peaks << juce::String(m, 3) << (b == bins - 1 ? "" : ",");
            }
            auto node = juce::ValueTree("w");
            node.setProperty("slot", -1, nullptr);
            node.setProperty("param", "amt", nullptr);
            node.setProperty("label", file.getFileNameWithoutExtension(), nullptr);
            node.setProperty("kind", "wave", nullptr);
            node.setProperty("peaks", peaks, nullptr);
            proc.uiState.appendChild(node, nullptr);
            showTab(1);
            rebuildCanvas();
            status.setText("WAV viewer loaded " + file.getFileName(), juce::dontSendNotification);
        });
}

void KyotoAudioProcessorEditor::saveEffect()
{
    if (fxStack.getNumChildren() == 0) { status.setText("Add at least one FX stage before saving.", juce::dontSendNotification); return; }
    if (fxStack.getNumChildren() > 16) { status.setText("FX Builder limit is 16 stages.", juce::dontSendNotification); return; }
    auto* obj = new juce::DynamicObject();
    obj->setProperty("format", "kyoteppah-effect-1");
    obj->setProperty("face", "effect");
    const auto name = effectNameBox.getText().trim().isEmpty() ? "untitled-fx" : effectNameBox.getText().trim();
    obj->setProperty("name", name);
    juce::Array<juce::var> steps;
    for (int i = 0; i < fxStack.getNumChildren(); ++i)
    {
        auto s = fxStack.getChild(i);
        auto* o = new juce::DynamicObject();
        o->setProperty("fx", (int) s.getProperty("fx"));
        o->setProperty("name", s.getProperty("name").toString());
        o->setProperty("amount", (double) s.getProperty("amount", 0.5));
        o->setProperty("tone", (double) s.getProperty("tone", 0.5));
        o->setProperty("motion", (double) s.getProperty("motion", 0.35));
        o->setProperty("mix", (double) s.getProperty("mix", 0.4));
        o->setProperty("shape", (double) s.getProperty("shape", 0.5));
        steps.add(juce::var(o));
    }
    obj->setProperty("steps", steps);
    const auto cats = deriveCategoriesFromStack();
    obj->setProperty("categories", cats);
    effectDir().getChildFile(name + ".json").replaceWithText(juce::JSON::toString(juce::var(obj)));
    refreshEffectBox();
    status.setText(cats.isNotEmpty() ? ("Saved effect " + name + "  -  " + cats) : ("Saved effect " + name), juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::publishEffect()
{
    if (fxStack.getNumChildren() == 0) { status.setText("Add at least one FX stage before publishing.", juce::dontSendNotification); return; }
    saveEffect();
    const auto name = effectNameBox.getText().trim().isEmpty() ? "untitled-fx" : effectNameBox.getText().trim();
    const auto body = effectDir().getChildFile(name + ".json").loadFileAsString(); const auto tokenCopy = token; juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe,name,body,tokenCopy]{ auto r=kt::publishModule(tokenCopy,name,body); juce::MessageManager::callAsync([safe,r]{ if(safe==nullptr)return; if(r.ok){ if(auto* o=r.parsed.getDynamicObject()) safe->lastPublishedEffectId=o->getProperty("id").toString(); safe->status.setText(safe->lastPublishedEffectId.isNotEmpty()?"Published FX - "+safe->lastPublishedEffectId:"Effect catalogued",juce::dontSendNotification); safe->refreshCatalog(); } else safe->status.setText(r.error,juce::dontSendNotification); }); }).detach();
}

juce::String KyotoAudioProcessorEditor::effectShareText() const
{
    if (lastPublishedEffectId.isEmpty()) return {};
    const auto name = effectNameBox.getText().trim().isEmpty() ? "Custom FX" : effectNameBox.getText().trim();
    return effectTokenFor(lastPublishedEffectId) + " " + name + "  -  click to load into FX Builder";
}

void KyotoAudioProcessorEditor::shareEffectToChat()
{
    const auto text=effectShareText(), tokenCopy=token; if(tokenCopy.isEmpty()||text.isEmpty()){status.setText("Publish this effect first, then share it.",juce::dontSendNotification);return;} juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe,text,tokenCopy]{auto r=kt::sendChat(tokenCopy,text);juce::MessageManager::callAsync([safe,r]{if(safe==nullptr)return;safe->status.setText(r.ok?"FX shared to chat":r.error,juce::dontSendNotification);if(r.ok)safe->refreshFeed();});}).detach();
}

void KyotoAudioProcessorEditor::shareEffectToThread()
{
    const auto text=effectShareText(), tokenCopy=token; if(tokenCopy.isEmpty()||text.isEmpty()){status.setText("Publish this effect first, then share it.",juce::dontSendNotification);return;} const auto title=effectNameBox.getText().trim().isEmpty()?"Custom KYOTRIPPAH FX":effectNameBox.getText().trim(); juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe,text,title,tokenCopy]{auto* o=new juce::DynamicObject();o->setProperty("title",title);o->setProperty("text",text);auto r=kt::postAction("create_thread",juce::var(o),tokenCopy);juce::MessageManager::callAsync([safe,r]{if(safe==nullptr)return;safe->status.setText(r.ok?"FX shared to Threads":r.error,juce::dontSendNotification);});}).detach();
}

void KyotoAudioProcessorEditor::updateFxControls()
{
    const int type = fxBrowser ? fxBrowser->getSelectedFx() : 0;
    const int fam = (type >= 0 && type < kt::kFxCount) ? kt::kFx[type].family : 0;
    if (fam == 0) { fxAmountLabel.setText("TIME", juce::dontSendNotification); fxToneLabel.setText("TONE", juce::dontSendNotification); fxMotionLabel.setText("FEEDBACK", juce::dontSendNotification); fxMixLabel.setText("MIX", juce::dontSendNotification); fxShapeLabel.setText("SPREAD", juce::dontSendNotification); }
    else if (fam == 1) { fxAmountLabel.setText("SIZE", juce::dontSendNotification); fxToneLabel.setText("TONE", juce::dontSendNotification); fxMotionLabel.setText("DECAY", juce::dontSendNotification); fxMixLabel.setText("MIX", juce::dontSendNotification); fxShapeLabel.setText("DIFFUSION", juce::dontSendNotification); }
    else if (fam == 2 || fam == 3) { fxAmountLabel.setText("DEPTH", juce::dontSendNotification); fxToneLabel.setText("COLOR", juce::dontSendNotification); fxMotionLabel.setText("RATE", juce::dontSendNotification); fxMixLabel.setText("MIX", juce::dontSendNotification); fxShapeLabel.setText("WIDTH", juce::dontSendNotification); }
    else if (fam == 4) { fxAmountLabel.setText("CUTOFF", juce::dontSendNotification); fxToneLabel.setText("RESONANCE", juce::dontSendNotification); fxMotionLabel.setText("SWEEP", juce::dontSendNotification); fxMixLabel.setText("MIX", juce::dontSendNotification); fxShapeLabel.setText("SLOPE", juce::dontSendNotification); }
    else if (fam == 5) { fxAmountLabel.setText("DRIVE", juce::dontSendNotification); fxToneLabel.setText("TONE", juce::dontSendNotification); fxMotionLabel.setText("BIAS", juce::dontSendNotification); fxMixLabel.setText("MIX", juce::dontSendNotification); fxShapeLabel.setText("SHAPE", juce::dontSendNotification); }
    else if (fam == 6) { fxAmountLabel.setText("THRESH", juce::dontSendNotification); fxToneLabel.setText("RATIO", juce::dontSendNotification); fxMotionLabel.setText("ATTACK", juce::dontSendNotification); fxMixLabel.setText("MIX", juce::dontSendNotification); fxShapeLabel.setText("RELEASE", juce::dontSendNotification); }
    else { fxAmountLabel.setText("AMOUNT", juce::dontSendNotification); fxToneLabel.setText("TONE", juce::dontSendNotification); fxMotionLabel.setText("MOTION", juce::dontSendNotification); fxMixLabel.setText("MIX", juce::dontSendNotification); fxShapeLabel.setText("SHAPE", juce::dontSendNotification); }
}

void KyotoAudioProcessorEditor::selectFxStep(int index)
{
    if (index < 0 || index >= fxStack.getNumChildren()) return;
    selectedFxStep = index;
    auto s = fxStack.getChild(index);
    const int type = (int)s.getProperty("fx", 0);
    if (fxBrowser && type >= 0 && type < kt::kFxCount) fxBrowser->setSelectedFx(type);
    fxAmount.setValue((double)s.getProperty("amount", 0.5), juce::dontSendNotification);
    fxTone.setValue((double)s.getProperty("tone", 0.5), juce::dontSendNotification);
    fxMotion.setValue((double)s.getProperty("motion", 0.35), juce::dontSendNotification);
    fxMix.setValue((double)s.getProperty("mix", 0.4), juce::dontSendNotification);
    fxShape.setValue((double)s.getProperty("shape", 0.5), juce::dontSendNotification);
    updateFxControls();
    repaint();
}

void KyotoAudioProcessorEditor::writeFxStepFromControls()
{
    if (selectedFxStep < 0 || selectedFxStep >= fxStack.getNumChildren()) return;
    auto s = fxStack.getChild(selectedFxStep);
    s.setProperty("amount", fxAmount.getValue(), nullptr);
    s.setProperty("tone", fxTone.getValue(), nullptr);
    s.setProperty("motion", fxMotion.getValue(), nullptr);
    s.setProperty("mix", fxMix.getValue(), nullptr);
    s.setProperty("shape", fxShape.getValue(), nullptr);
    lastPublishedEffectId.clear();
    repaint();
}

void KyotoAudioProcessorEditor::randomizeFxControls()
{
    juce::Random rng;
    fxAmount.setValue(rng.nextFloat(), juce::sendNotificationSync);
    fxTone.setValue(rng.nextFloat(), juce::sendNotificationSync);
    fxMotion.setValue(rng.nextFloat(), juce::sendNotificationSync);
    fxMix.setValue(0.25f + rng.nextFloat() * 0.7f, juce::sendNotificationSync);
    fxShape.setValue(rng.nextFloat(), juce::sendNotificationSync);
}

void KyotoAudioProcessorEditor::addSpecialChainStep(int type, const juce::String& name)
{
    juce::Rectangle<int> room;
    if (!findAutoCell(widgets.size(), "stack", room)) { status.setText("No safe room left. Use FX Builder to build a custom effect.", juce::dontSendNotification); return; }
    int slot = -1;
    for (int i = 0; i < proc.slotCount(); ++i)
        if (auto* on = proc.apvts.getParameter("s" + juce::String(i + 1).paddedLeft('0', 2) + "on"); on != nullptr && on->getValue() < 0.5f) { slot = i; break; }
    if (slot < 0) { status.setText("DSP slots are full - build a custom FX instead.", juce::dontSendNotification); return; }
    captureSnapshot();
    const auto prefix = "s" + juce::String(slot + 1).paddedLeft('0', 2);
    auto setFloat = [this, &prefix](const juce::String& suffix, float value) { if (auto* p = proc.apvts.getParameter(prefix + suffix)) p->setValueNotifyingHost(p->convertTo0to1(value)); };
    if (auto* p = proc.apvts.getParameter(prefix + "type")) p->setValueNotifyingHost(p->convertTo0to1((float)type));
    if (auto* p = proc.apvts.getParameter(prefix + "on")) p->setValueNotifyingHost(1.f);
    setFloat("amt", type == KyotoAudioProcessor::kMixType ? 1.f : 0.5f);
    setFloat("mix", 1.f);
    auto node = juce::ValueTree("w");
    node.setProperty("slot", slot, nullptr);
    node.setProperty("param", "amt", nullptr);
    node.setProperty("label", name, nullptr);
    node.setProperty("kind", "stack", nullptr);
    node.setProperty("slotCount", 1, nullptr);
    node.setProperty("series", proc.uiState.getNumChildren(), nullptr);
    proc.uiState.appendChild(node, nullptr);
    rebuildCanvas();
}

void KyotoAudioProcessorEditor::refreshEffectBox()
{
    auto writeEffectPreset = [this](const juce::String& name, std::initializer_list<std::array<double, 6>> rows)
    {
        const auto file = effectDir().getChildFile(name + ".json");
        if (file.existsAsFile()) return;
        auto* obj = new juce::DynamicObject(); obj->setProperty("format", "kyoteppah-effect-1"); obj->setProperty("face", "effect"); obj->setProperty("name", name);
        juce::Array<juce::var> steps;
        for (const auto& row : rows)
        {
            auto* step = new juce::DynamicObject(); step->setProperty("fx", (int)row[0]); step->setProperty("name", row[0] >= 0 && row[0] < kt::kFxCount ? kt::kFx[(int)row[0]].name : "Utility");
            step->setProperty("amount", row[1]); step->setProperty("tone", row[2]); step->setProperty("motion", row[3]); step->setProperty("mix", row[4]); step->setProperty("shape", row[5]); steps.add(juce::var(step));
        }
        obj->setProperty("steps", steps); file.replaceWithText(juce::JSON::toString(juce::var(obj)));
    };
    writeEffectPreset("Velvet Glue", {{20,0.48,0.62,0.25,0.55,0.40},{21,0.34,0.70,0.18,0.42,0.35},{30,0.28,0.78,0.12,0.30,0.55}});
    writeEffectPreset("Ghost Corridor", {{2,0.42,0.52,0.58,0.48,0.40},{3,0.55,0.64,0.72,0.42,0.62},{59,0.38,0.70,0.35,0.34,0.72}});
    writeEffectPreset("Neon Damage", {{0,0.64,0.42,0.22,0.58,0.72},{1,0.46,0.70,0.38,0.46,0.68},{11,0.55,0.38,0.66,0.52,0.82}});

    auto writeChainPreset = [this](const juce::String& name, std::initializer_list<std::array<double, 6>> rows)
    {
        const auto dir = sessionFile().getParentDirectory().getChildFile("kyoto"); dir.createDirectory();
        const auto file = dir.getChildFile(name + ".json"); if (file.existsAsFile()) return;
        auto* obj = new juce::DynamicObject(); obj->setProperty("format", "kyoteppah-module-1"); obj->setProperty("face", "chain"); obj->setProperty("name", name); obj->setProperty("grid", 0); obj->setProperty("theme", proc.uiState.getProperty("theme", juce::var("trippah")));
        juce::Array<juce::var> slots;
        for (const auto& row : rows) { auto* step = new juce::DynamicObject(); step->setProperty("on", true); step->setProperty("fx", (int)row[0]); step->setProperty("amount", row[1]); step->setProperty("tone", row[2]); step->setProperty("motion", row[3]); step->setProperty("mix", row[4]); step->setProperty("shape", row[5]); slots.add(juce::var(step)); }
        obj->setProperty("slots", slots); obj->setProperty("widgets", juce::var(juce::Array<juce::var>())); file.replaceWithText(juce::JSON::toString(juce::var(obj)));
    };
    writeChainPreset("Midnight Bloom", {{3,0.52,0.60,0.62,0.42,0.58},{1,0.42,0.72,0.38,0.40,0.62},{30,0.30,0.78,0.18,0.28,0.55},{20,0.38,0.60,0.22,0.44,0.40}});
    writeChainPreset("Tape Prayer", {{15,0.40,0.62,0.52,0.46,0.50},{2,0.36,0.54,0.45,0.38,0.46},{21,0.32,0.72,0.20,0.40,0.38},{200,1.0,0.5,0.5,1.0,0.5}});
    writeChainPreset("Wide Dream", {{4,0.55,0.50,0.32,0.50,0.78},{1,0.35,0.70,0.28,0.40,0.72},{59,0.32,0.68,0.40,0.34,0.70},{200,0.82,0.5,0.5,1.0,0.5}});

    const auto isStarter = [](const juce::String& n)
    {
        return n == "Midnight Bloom" || n == "Tape Prayer" || n == "Wide Dream" || n == "Velvet Glue" || n == "Ghost Corridor" || n == "Neon Damage";
    };
    presetBox.clear();
    int id = 1;
    for (auto f : moduleDir().findChildFiles(juce::File::findFiles, false, "*.json"))
        if (!isStarter(f.getFileNameWithoutExtension())) presetBox.addItem("CHAIN  -  " + f.getFileNameWithoutExtension(), id++);
    for (auto f : effectDir().findChildFiles(juce::File::findFiles, false, "*.json"))
        if (!isStarter(f.getFileNameWithoutExtension())) presetBox.addItem("FX  -  " + f.getFileNameWithoutExtension(), id++);

    juce::Array<FxBrowser::CustomItem> custom;
    for (auto f : effectDir().findChildFiles(juce::File::findFiles, false, "*.json"))
    {
        if (isStarter(f.getFileNameWithoutExtension())) continue;
        FxBrowser::CustomItem item;
        item.name = f.getFileNameWithoutExtension();
        item.author = account;
        item.remote = false;
        auto parsed = juce::JSON::parse(f.loadFileAsString());
        if (auto* o = parsed.getDynamicObject())
        {
            item.categories = o->getProperty("categories").toString();
            if (item.categories.isEmpty() && o->getProperty("steps").isArray())
            {
                bool used[8] = {};
                for (auto& v : *o->getProperty("steps").getArray())
                    if (auto* so = v.getDynamicObject())
                    {
                        const int fx = (int) so->getProperty("fx");
                        if (fx >= 0 && fx < kt::kFxCount)
                        {
                            const int fam = kt::kFx[fx].family;
                            if (fam >= 0 && fam < 8) used[fam] = true;
                        }
                    }
                juce::StringArray cats;
                for (int fam = 0; fam < 8; ++fam)
                    if (used[fam]) cats.add(kt::kFxFamilyNames[fam]);
                item.categories = cats.joinIntoString(" - ");
            }
        }
        custom.add(item);
    }
    for (const auto& c : catalog)
        if (c.face.equalsIgnoreCase("effect")) custom.add({c.id, c.name, c.author, {}, true});
    if (fxBrowser) fxBrowser->setCustomItems(custom);
}

void KyotoAudioProcessorEditor::saveLocal()
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("format", "kyoteppah-module-1");
    obj->setProperty("face", proc.isFx() ? "fx" : "chain");
    const auto name = nameBox.getText().trim().isEmpty() ? "untitled" : nameBox.getText().trim();
    obj->setProperty("name", name);
    obj->setProperty("grid", 0);
    obj->setProperty("theme", proc.uiState.getProperty("theme", juce::var("trippah")));
    obj->setProperty("machineDesign", machineDesign.toVar());
    juce::Array<juce::var> widgetsArr, slots, steps;
    for (int i = 0; i < proc.slotCount(); ++i)
    {
        auto* s = new juce::DynamicObject();
        const auto prefix = "s" + juce::String(i + 1).paddedLeft('0', 2);
        s->setProperty("on", proc.apvts.getRawParameterValue(prefix + "on")->load() >= 0.5f);
        s->setProperty("fx", (int) proc.apvts.getRawParameterValue(prefix + "type")->load());
        s->setProperty("amount", proc.apvts.getRawParameterValue(prefix + "amt")->load());
        s->setProperty("tone", proc.apvts.getRawParameterValue(prefix + "tone")->load());
        s->setProperty("motion", proc.apvts.getRawParameterValue(prefix + "mot")->load());
        s->setProperty("mix", proc.apvts.getRawParameterValue(prefix + "mix")->load());
        s->setProperty("shape", proc.apvts.getRawParameterValue(prefix + "shp")->load());
        slots.add(juce::var(s));
        if (proc.apvts.getRawParameterValue(prefix + "on")->load() >= 0.5f)
        {
            auto* step = new juce::DynamicObject();
            const int fx = (int) proc.apvts.getRawParameterValue(prefix + "type")->load();
            step->setProperty("fx", fx);
            step->setProperty("name", fx == KyotoAudioProcessor::kMixType ? "MASTER MIX" : (fx == KyotoAudioProcessor::kBreakType ? "CHAIN BREAK" : (fx >= 0 && fx < kt::kFxCount ? kt::kFx[fx].name : "FX")));
            step->setProperty("amount", proc.apvts.getRawParameterValue(prefix + "amt")->load());
            step->setProperty("tone", proc.apvts.getRawParameterValue(prefix + "tone")->load());
            step->setProperty("motion", proc.apvts.getRawParameterValue(prefix + "mot")->load());
            step->setProperty("mix", proc.apvts.getRawParameterValue(prefix + "mix")->load());
            step->setProperty("shape", proc.apvts.getRawParameterValue(prefix + "shp")->load());
            steps.add(juce::var(step));
        }
    }
    for (int i = 0; i < proc.uiState.getNumChildren(); ++i)
    {
        auto w = proc.uiState.getChild(i);
        if (! w.hasType("w")) continue;
        auto* o = new juce::DynamicObject();
        o->setProperty("slot", (int) w.getProperty("slot"));
        o->setProperty("param", w.getProperty("param").toString());
        o->setProperty("label", w.getProperty("label").toString());
        o->setProperty("kind", w.getProperty("kind").toString());
        o->setProperty("slotCount", (int)w.getProperty("slotCount", 1));
        o->setProperty("peaks", w.getProperty("peaks").toString());
        o->setProperty("x", (int) w.getProperty("x"));
        o->setProperty("y", (int) w.getProperty("y"));
        widgetsArr.add(juce::var(o));
    }
    obj->setProperty("slots", slots);
    obj->setProperty("steps", steps);
    obj->setProperty("widgets", widgetsArr);
    moduleDir().getChildFile(name + ".json").replaceWithText(juce::JSON::toString(juce::var(obj)));
    refreshEffectBox();
    status.setText("Saved chain " + name, juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::publish()
{
    saveLocal();
    const auto name = nameBox.getText().trim().isEmpty() ? "untitled" : nameBox.getText().trim();
    const auto body = moduleDir().getChildFile(name + ".json").loadFileAsString(); const auto tokenCopy = token; juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe, name, body, tokenCopy] { auto r=kt::publishModule(tokenCopy,name,body); juce::MessageManager::callAsync([safe,r]{ if(safe==nullptr)return; safe->status.setText(r.ok?"Chain published to DreamShare":r.error,juce::dontSendNotification); if(r.ok)safe->refreshCatalog(); }); }).detach();
}
