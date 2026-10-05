#include "PluginEditor.h"
#include "DreamApi.h"
#include "FxCatalog.h"
#include <thread>

namespace
{
const char* webKey(const juce::String& native)
{
    if (native == "amt") return "amount";
    if (native == "mot") return "motion";
    if (native == "shp") return "shape";
    return native.toRawUTF8();
}
juce::String nativeKey(const juce::String& web)
{
    if (web == "amount") return "amt";
    if (web == "motion") return "mot";
    if (web == "shape") return "shp";
    return web;
}
}

// ===========================================================================
// CanvasWidget
// ===========================================================================
CanvasWidget::CanvasWidget(KyotoAudioProcessor& p, juce::ValueTree n)
    : proc(p), node(std::move(n))
{
    const auto k = node.getProperty("kind").toString();
    if (k == "slider") kind = Kind::Slider;
    else if (k == "key") kind = Kind::Key;
    else if (k == "wave") kind = Kind::Wave;
    else kind = Kind::Dial;

    caption.setText(node.getProperty("label").toString(), juce::dontSendNotification);
    caption.setJustificationType(juce::Justification::centred);
    caption.setFont(juce::FontOptions(10.f));
    caption.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(caption);

    if (kind == Kind::Dial || kind == Kind::Slider)
    {
        const int slot = (int) node.getProperty("slot", 0);
        const auto key = node.getProperty("param").toString();
        auto id = slot < 0 ? key : ("s" + juce::String(slot + 1).paddedLeft('0', 2) + key);

        if (kind == Kind::Dial)
            slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        else
            slider.setSliderStyle(juce::Slider::LinearVertical);

        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 14);
        slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(slider);
        if (proc.apvts.getParameter(id) != nullptr)
            attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, id, slider);
    }
}

void CanvasWidget::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.f);
    g.setColour(juce::Colour(0x22000000));
    g.fillRoundedRectangle(bounds, 6.f);

    if (kind == Kind::Key)
    {
        g.setColour(juce::Colour(0xff2a2a3a));
        g.fillRoundedRectangle(bounds.reduced(4.f), 4.f);
        g.setColour(juce::Colour(0xffc0c0d0));
        g.setFont(juce::FontOptions(11.f).withStyle("Bold"));
        g.drawText(node.getProperty("label").toString(), bounds, juce::Justification::centred);
    }
    else if (kind == Kind::Wave)
    {
        g.setColour(juce::Colour(0xff12141c));
        g.fillRoundedRectangle(bounds.reduced(3.f), 4.f);
        g.setColour(juce::Colour(0xff60c0ff));
        juce::Path wave;
        const float mid = bounds.getCentreY();
        const float w = bounds.getWidth() - 12.f;
        wave.startNewSubPath(bounds.getX() + 6.f, mid);
        for (int i = 0; i < 32; ++i)
        {
            float x = bounds.getX() + 6.f + (w * i / 31.f);
            float y = mid + std::sin(i * 0.55f + (float) juce::Time::getMillisecondCounter() * 0.004f) * (bounds.getHeight() * 0.28f);
            wave.lineTo(x, y);
        }
        g.strokePath(wave, juce::PathStrokeType(1.6f));
        g.setColour(juce::Colour(0xff8090a0));
        g.setFont(juce::FontOptions(9.f));
        g.drawText("WAVE", bounds.removeFromBottom(14).toNearestInt(), juce::Justification::centred);
    }
}

void CanvasWidget::resized()
{
    caption.setBounds(0, 0, getWidth(), 14);
    if (kind == Kind::Dial || kind == Kind::Slider)
        slider.setBounds(4, 14, getWidth() - 8, getHeight() - 18);
}

void CanvasWidget::mouseDown(const juce::MouseEvent& e)
{
    dragging = e.mods.isLeftButtonDown() && e.originalComponent == this;
}

void CanvasWidget::mouseDrag(const juce::MouseEvent& e)
{
    if (! dragging || e.getDistanceFromDragStart() < 4) return;
    auto* parent = getParentComponent();
    if (parent == nullptr) return;
    const int nx = juce::jlimit(0, parent->getWidth() - getWidth(), getX() + e.getDistanceFromDragStartX());
    const int ny = juce::jlimit(0, parent->getHeight() - getHeight(), getY() + e.getDistanceFromDragStartY());
    setTopLeftPosition(nx, ny);
    node.setProperty("x", nx * 100 / juce::jmax(1, parent->getWidth()), nullptr);
    node.setProperty("y", ny * 100 / juce::jmax(1, parent->getHeight()), nullptr);
}

void CanvasWidget::mouseUp(const juce::MouseEvent&)
{
    dragging = false;
}

// ===========================================================================
// FxBrowser
// ===========================================================================
FxBrowser::FxBrowser(KyotoAudioProcessor& p) : proc(p)
{
    list.setModel(this);
    list.setRowHeight(26);
    list.setOutlineThickness(0);
    list.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(list);

    search.setTextToShowWhenEmpty("search effects...", juce::Colours::grey);
    search.onTextChange = [this] {
        filter = search.getText().trim().toLowerCase();
        rebuildFilter();
        list.updateContent();
    };
    addAndMakeVisible(search);
    rebuildFilter();
}

void FxBrowser::setTheme(const kt::ThemePalette& t)
{
    theme = t;
    list.setColour(juce::ListBox::backgroundColourId, kt::c(t.panel));
    list.setColour(juce::ListBox::textColourId, kt::c(t.text));
    search.setColour(juce::TextEditor::backgroundColourId, kt::c(t.bg));
    search.setColour(juce::TextEditor::textColourId, kt::c(t.text));
    search.setColour(juce::TextEditor::outlineColourId, kt::c(t.border));
    repaint();
}

void FxBrowser::rebuildFilter()
{
    filtered.clear();
    for (int i = 0; i < kt::kFxCount; ++i)
    {
        juce::String name(kt::kFx[i].name);
        if (filter.isEmpty() || name.toLowerCase().contains(filter))
            filtered.add(i);
    }
}

int FxBrowser::getNumRows() { return filtered.size(); }

void FxBrowser::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool isSelected)
{
    if (row < 0 || row >= filtered.size()) return;
    const int fx = filtered[row];
    auto bounds = juce::Rectangle<int>(0, 0, w, h);

    if (isSelected || row == hoverRow)
        g.setColour(kt::c(theme.accent).withAlpha(isSelected ? 0.35f : 0.18f));
    else
        g.setColour(kt::c(theme.panel));
    g.fillRect(bounds);

    g.setColour(kt::c(theme.text));
    g.setFont(juce::FontOptions(12.f));
    g.drawText(kt::kFx[fx].name, bounds.reduced(8, 0), juce::Justification::centredLeft);
}

void FxBrowser::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    if (row < 0 || row >= filtered.size()) return;
    selected = filtered[row];
    if (onSelect) onSelect(selected);
}

void FxBrowser::selectedRowsChanged(int last)
{
    if (last >= 0 && last < filtered.size())
    {
        selected = filtered[last];
        if (onSelect) onSelect(selected);
    }
}

void FxBrowser::paint(juce::Graphics& g)
{
    g.fillAll(kt::c(theme.panel));
    g.setColour(kt::c(theme.border));
    g.drawRect(getLocalBounds(), 1);
}

void FxBrowser::resized()
{
    auto r = getLocalBounds().reduced(4);
    search.setBounds(r.removeFromTop(26));
    r.removeFromTop(4);
    list.setBounds(r);
}

// ===========================================================================
// KyotoAudioProcessorEditor
// ===========================================================================
KyotoAudioProcessorEditor::KyotoAudioProcessorEditor(KyotoAudioProcessor& p)
    : AudioProcessorEditor(p), proc(p)
{
    setSize(1080, 700);
    setResizable(true, true);
    setResizeLimits(800, 520, 1800, 1100);

    for (auto* b : { &shareBtn, &buildBtn, &loginBtn, &sendBtn, &feedBtn, &catBtn,
                     &addBtn, &saveBtn, &upBtn })
        addAndMakeVisible(b);

    for (auto* e : { &userBox, &passBox, &msgBox, &nameBox })
    {
        e->setMultiLine(false);
        addAndMakeVisible(e);
    }
    passBox.setPasswordCharacter('*');
    logBox.setMultiLine(true);
    logBox.setReadOnly(true);
    logBox.setFont(juce::FontOptions(13.f));
    addAndMakeVisible(logBox);

    addAndMakeVisible(themeBox);
    addAndMakeVisible(remoteBox);
    addAndMakeVisible(gridStyleBox);
    addAndMakeVisible(localBox);
    addAndMakeVisible(kindBox);
    addAndMakeVisible(status);
    addAndMakeVisible(panel);

    // Theme selector (single file, all themes)
    for (int i = 0; i < kt::kThemeCount; ++i)
        themeBox.addItem(kt::kThemes[i].name, i + 1);
    themeBox.setSelectedId(1, juce::dontSendNotification);
    themeBox.onChange = [this] {
        const int idx = themeBox.getSelectedId() - 1;
        if (idx >= 0 && idx < kt::kThemeCount)
            applyTheme(kt::kThemes[idx].id);
    };

    // Grid styles
    for (int i = 0; i < 10; ++i)
        gridStyleBox.addItem("Grid " + juce::String(i + 1), i + 1);
    gridStyleBox.setSelectedId((int) proc.uiState.getProperty("grid", 0) + 1, juce::dontSendNotification);
    gridStyleBox.onChange = [this] {
        proc.uiState.setProperty("grid", gridStyleBox.getSelectedId() - 1, nullptr);
        panel.repaint();
    };

    // Widget kind for placement
    kindBox.addItem("Dial", 1);
    kindBox.addItem("Slider", 2);
    kindBox.addItem("Key / Note", 3);
    kindBox.addItem("Waveform", 4);
    kindBox.setSelectedId(1, juce::dontSendNotification);

    nameBox.setText(proc.uiState.getProperty("name").toString(), juce::dontSendNotification);

    // FX Browser
    fxBrowser = std::make_unique<FxBrowser>(proc);
    addAndMakeVisible(*fxBrowser);
    fxBrowser->onSelect = [this](int fx) {
        pendingFx = fx;
        placementMode = true;
        highlightPegsForPlacement(true);
        status.setText("Click a free peg to place " + juce::String(kt::kFx[fx].name), juce::dontSendNotification);
    };
    fxBrowser->onHover = [this](int fx) {
        // Half-intensity preview: temporarily set first free (or last) slot amount to 0.5 * current
        if (fx < 0) return;
        // Find first active or first slot and nudge amount for live preview
        for (int i = 0; i < proc.slotCount(); ++i)
        {
            auto prefix = "s" + juce::String(i + 1).paddedLeft('0', 2);
            if (proc.apvts.getRawParameterValue(prefix + "on")->load() >= 0.5f)
            {
                // already has something; use a silent preview path by setting a temp property
                proc.uiState.setProperty("previewFx", fx, nullptr);
                proc.uiState.setProperty("previewAmt", 0.5f, nullptr);
                return;
            }
        }
        // No active slot: temporarily enable slot 0 at half
        auto prefix = "s01";
        if (auto* t = proc.apvts.getParameter(prefix + "type"))
            t->setValueNotifyingHost(t->convertTo0to1((float) fx));
        if (auto* a = proc.apvts.getParameter(prefix + "amt"))
            a->setValueNotifyingHost(0.5f);
        if (auto* on = proc.apvts.getParameter(prefix + "on"))
            on->setValueNotifyingHost(1.f);
        proc.uiState.setProperty("previewActive", 1, nullptr);
    };
    fxBrowser->onLeave = [this] {
        if ((int) proc.uiState.getProperty("previewActive", 0) != 0)
        {
            auto prefix = "s01";
            if (auto* on = proc.apvts.getParameter(prefix + "on"))
                on->setValueNotifyingHost(0.f);
            proc.uiState.setProperty("previewActive", 0, nullptr);
        }
        proc.uiState.removeProperty("previewFx", nullptr);
        proc.uiState.removeProperty("previewAmt", nullptr);
    };

    // Session restore
    if (auto json = juce::JSON::parse(sessionFile().loadFileAsString()); json.getDynamicObject() != nullptr)
    {
        auto* o = json.getDynamicObject();
        token = o->getProperty("token").toString();
        account = o->getProperty("user").toString();
        userBox.setText(account, juce::dontSendNotification);
        auto th = o->getProperty("theme").toString();
        if (th.isNotEmpty()) applyTheme(th);
    }
    else
    {
        applyTheme(proc.uiState.getProperty("theme", "trippah").toString());
    }

    shareBtn.onClick = [this] { showTab(0); };
    buildBtn.onClick = [this] { showTab(1); };
    saveBtn.onClick = [this] { saveLocal(); };
    upBtn.onClick = [this] { publish(); };
    loginBtn.onClick = [this] { login(); };
    sendBtn.onClick = [this] { sendChat(); };
    addBtn.onClick = [this] { addSelectedEffect(); };
    feedBtn.onClick = [this] { refreshFeed(); };
    catBtn.onClick = [this] { refreshCatalog(); };
    localBox.onChange = [this] { loadNamed(localBox.getText()); };

    // Canvas click for peg placement
    panel.setMouseCursor(juce::MouseCursor::CrosshairCursor);
    panel.addMouseListener(this, false);

    startTimer(6000);
    showTab(0);
    refreshFeed();
}

KyotoAudioProcessorEditor::~KyotoAudioProcessorEditor()
{
    stopTimer();
}

void KyotoAudioProcessorEditor::applyTheme(const juce::String& id)
{
    theme = kt::themeById(id);
    proc.uiState.setProperty("theme", id, nullptr);

    auto setBtn = [this](juce::TextButton& b) {
        b.setColour(juce::TextButton::buttonColourId, kt::c(theme.panel));
        b.setColour(juce::TextButton::buttonOnColourId, kt::c(theme.accent).withAlpha(0.4f));
        b.setColour(juce::TextButton::textColourOffId, kt::c(theme.text));
        b.setColour(juce::TextButton::textColourOnId, kt::c(theme.text));
    };
    for (auto* b : { &shareBtn, &buildBtn, &loginBtn, &sendBtn, &feedBtn, &catBtn, &addBtn, &saveBtn, &upBtn })
        setBtn(*b);

    auto setEdit = [this](juce::TextEditor& e) {
        e.setColour(juce::TextEditor::backgroundColourId, kt::c(theme.panel));
        e.setColour(juce::TextEditor::textColourId, kt::c(theme.text));
        e.setColour(juce::TextEditor::outlineColourId, kt::c(theme.border));
        e.setColour(juce::TextEditor::focusedOutlineColourId, kt::c(theme.accent));
    };
    for (auto* e : { &userBox, &passBox, &msgBox, &nameBox, &logBox })
        setEdit(*e);

    status.setColour(juce::Label::textColourId, kt::c(theme.muted));
    if (fxBrowser) fxBrowser->setTheme(theme);

    // Persist theme choice in session
    auto* o = new juce::DynamicObject();
    o->setProperty("token", token);
    o->setProperty("user", account);
    o->setProperty("theme", id);
    sessionFile().replaceWithText(juce::JSON::toString(juce::var(o)));

    repaint();
}

void KyotoAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(kt::c(theme.bg));

    // Header bar
    g.setColour(kt::c(theme.panel));
    g.fillRect(0, 0, getWidth(), 42);
    g.setColour(kt::c(theme.border));
    g.drawLine(0.f, 42.f, (float) getWidth(), 42.f, 1.f);

    g.setColour(kt::c(theme.accent));
    g.setFont(juce::FontOptions(17.f).withStyle("Bold"));
    g.drawText(proc.getName(), 14, 8, 200, 26, juce::Justification::centredLeft);

    // Builder canvas background + peg grid
    if (tab == 1)
    {
        auto pb = panel.getBounds().toFloat();
        g.setColour(kt::c(theme.panel));
        g.fillRoundedRectangle(pb, 6.f);
        g.setColour(kt::c(theme.border));
        g.drawRoundedRectangle(pb, 6.f, 1.f);

        // Draw peg grid (non-overlapping slots)
        const int cols = pegCols;
        const int rows = pegRows;
        const float cellW = pb.getWidth() / cols;
        const float cellH = pb.getHeight() / rows;

        for (int r = 0; r < rows; ++r)
        {
            for (int c = 0; c < cols; ++c)
            {
                float cx = pb.getX() + (c + 0.5f) * cellW;
                float cy = pb.getY() + (r + 0.5f) * cellH;
                bool occupied = false;
                for (auto* w : widgets)
                {
                    auto wb = w->getBounds().toFloat();
                    if (wb.contains(cx, cy)) { occupied = true; break; }
                }

                if (placementMode && ! occupied)
                    g.setColour(kt::c(theme.pegHot).withAlpha(0.85f));
                else if (occupied)
                    g.setColour(kt::c(theme.peg).withAlpha(0.25f));
                else
                    g.setColour(kt::c(theme.peg).withAlpha(0.45f));

                g.fillEllipse(cx - 5.f, cy - 5.f, 10.f, 10.f);
            }
        }
    }
}

void KyotoAudioProcessorEditor::showTab(int next)
{
    tab = next;
    const bool share = (tab == 0);

    logBox.setVisible(share);
    userBox.setVisible(share);
    passBox.setVisible(share);
    msgBox.setVisible(share);
    loginBtn.setVisible(share);
    sendBtn.setVisible(share);
    feedBtn.setVisible(share);
    remoteBox.setVisible(share);
    catBtn.setVisible(share);
    themeBox.setVisible(share);

    panel.setVisible(! share);
    if (fxBrowser) fxBrowser->setVisible(! share);
    addBtn.setVisible(! share);
    gridStyleBox.setVisible(! share);
    nameBox.setVisible(! share);
    localBox.setVisible(! share);
    saveBtn.setVisible(! share);
    upBtn.setVisible(! share);
    kindBox.setVisible(! share);

    shareBtn.setToggleState(share, juce::dontSendNotification);
    buildBtn.setToggleState(! share, juce::dontSendNotification);

    resized();
    if (! share) rebuildCanvas();
    repaint();
}

void KyotoAudioProcessorEditor::resized()
{
    shareBtn.setBounds(220, 7, 130, 28);
    buildBtn.setBounds(356, 7, 110, 28);
    status.setBounds(480, 7, getWidth() - 500, 28);

    auto area = getLocalBounds().withTrimmedTop(48).reduced(10);

    if (tab == 0)
    {
        // DreamShare layout – cleaner two-column feel
        auto left = area.removeFromLeft(280);
        userBox.setBounds(left.removeFromTop(30));
        left.removeFromTop(6);
        passBox.setBounds(left.removeFromTop(30));
        left.removeFromTop(6);
        loginBtn.setBounds(left.removeFromTop(30).removeFromLeft(120));
        left.removeFromTop(12);
        themeBox.setBounds(left.removeFromTop(28));
        left.removeFromTop(8);
        catBtn.setBounds(left.removeFromTop(28));
        left.removeFromTop(8);
        remoteBox.setBounds(left.removeFromTop(28));

        auto bottom = area.removeFromBottom(40);
        msgBox.setBounds(bottom.removeFromLeft(area.getWidth() - 200));
        bottom.removeFromLeft(8);
        sendBtn.setBounds(bottom.removeFromLeft(90));
        bottom.removeFromLeft(8);
        feedBtn.setBounds(bottom.removeFromLeft(90));

        logBox.setBounds(area);
    }
    else
    {
        // Builder: left browser, right canvas
        auto top = area.removeFromTop(34);
        nameBox.setBounds(top.removeFromLeft(180));
        top.removeFromLeft(8);
        gridStyleBox.setBounds(top.removeFromLeft(110));
        top.removeFromLeft(8);
        kindBox.setBounds(top.removeFromLeft(110));
        top.removeFromLeft(8);
        addBtn.setBounds(top.removeFromLeft(80));
        top.removeFromLeft(8);
        saveBtn.setBounds(top.removeFromLeft(70));
        top.removeFromLeft(6);
        upBtn.setBounds(top.removeFromLeft(80));

        auto bottom = area.removeFromBottom(34);
        localBox.setBounds(bottom);

        auto left = area.removeFromLeft(240);
        if (fxBrowser) fxBrowser->setBounds(left.reduced(0, 4));

        area.removeFromLeft(8);
        panel.setBounds(area);

        for (auto* w : widgets)
        {
            const int x = (int) w->node.getProperty("x", 5) * panel.getWidth() / 100;
            const int y = (int) w->node.getProperty("y", 8) * panel.getHeight() / 100;
            int ww = 72, hh = 80;
            auto k = w->node.getProperty("kind").toString();
            if (k == "slider") { ww = 48; hh = 110; }
            else if (k == "key") { ww = 56; hh = 56; }
            else if (k == "wave") { ww = 120; hh = 64; }
            w->setBounds(x, y, ww, hh);
        }
    }
}

void KyotoAudioProcessorEditor::rebuildCanvas()
{
    widgets.clear();
    for (int i = 0; i < proc.uiState.getNumChildren(); ++i)
    {
        auto child = proc.uiState.getChild(i);
        if (! child.hasType("w")) continue;
        auto* w = widgets.add(new CanvasWidget(proc, child));
        panel.addAndMakeVisible(w);
    }
    resized();
}

bool KyotoAudioProcessorEditor::collides(const juce::Rectangle<int>& r) const
{
    for (auto* w : widgets)
        if (w->getBounds().intersects(r.expanded(4)))
            return true;
    return false;
}

juce::Rectangle<int> KyotoAudioProcessorEditor::findFreePeg(int w, int h) const
{
    const int cols = pegCols;
    const int rows = pegRows;
    const int cellW = panel.getWidth() / cols;
    const int cellH = panel.getHeight() / rows;

    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
        {
            int x = c * cellW + (cellW - w) / 2;
            int y = r * cellH + (cellH - h) / 2;
            juce::Rectangle<int> candidate(x, y, w, h);
            if (! collides(candidate))
                return candidate;
        }
    return {};
}

void KyotoAudioProcessorEditor::highlightPegsForPlacement(bool)
{
    panel.repaint();
    repaint();
}

void KyotoAudioProcessorEditor::addSelectedEffect()
{
    // Only one effect at a time – use the currently selected browser item
    int type = fxBrowser ? fxBrowser->getSelectedFx() : 0;
    if (type < 0 || type >= kt::kFxCount) type = 0;

    int slot = -1;
    for (int i = 0; i < proc.slotCount(); ++i)
        if (proc.apvts.getParameter("s" + juce::String(i + 1).paddedLeft('0', 2) + "on")->getValue() < 0.5f)
        { slot = i; break; }

    if (slot < 0)
    {
        status.setText(proc.isFx() ? "FX chain full (12)" : "Instrument chain full (8)", juce::dontSendNotification);
        return;
    }

    const auto prefix = "s" + juce::String(slot + 1).paddedLeft('0', 2);
    if (auto* t = proc.apvts.getParameter(prefix + "type"))
        t->setValueNotifyingHost(t->convertTo0to1((float) type));
    if (auto* on = proc.apvts.getParameter(prefix + "on"))
        on->setValueNotifyingHost(1.f);

    // Clear any hover preview
    proc.uiState.removeProperty("previewActive", nullptr);

    const int kindId = kindBox.getSelectedId();
    juce::String kindStr = "dial";
    int ww = 72, hh = 80;
    if (kindId == 2) { kindStr = "slider"; ww = 48; hh = 110; }
    else if (kindId == 3) { kindStr = "key"; ww = 56; hh = 56; }
    else if (kindId == 4) { kindStr = "wave"; ww = 120; hh = 64; }

    // For instrument sound producers place a Key; otherwise place the requested kind
    // and also the core amount dial if it is an effect.
    auto placeOne = [&](const char* param, const juce::String& label, const juce::String& kind, int w, int h)
    {
        auto free = findFreePeg(w, h);
        if (free.isEmpty())
        {
            status.setText("No free peg for " + label, juce::dontSendNotification);
            return;
        }
        auto node = juce::ValueTree("w");
        node.setProperty("slot", slot, nullptr);
        node.setProperty("param", param, nullptr);
        node.setProperty("label", label, nullptr);
        node.setProperty("kind", kind, nullptr);
        node.setProperty("x", free.getX() * 100 / juce::jmax(1, panel.getWidth()), nullptr);
        node.setProperty("y", free.getY() * 100 / juce::jmax(1, panel.getHeight()), nullptr);
        proc.uiState.appendChild(node, nullptr);
    };

    if (kindId == 3) // Key / Note
    {
        placeOne("amt", juce::String(kt::kFx[type].name) + " Key", "key", 56, 56);
    }
    else if (kindId == 4)
    {
        placeOne("amt", juce::String(kt::kFx[type].name) + " Wave", "wave", 120, 64);
    }
    else
    {
        // Place the five standard controls but only on free pegs, stopping early if space runs out
        const char* keys[] = { "amt", "tone", "mot", "mix", "shp" };
        const char* labels[] = { "AMT", "TONE", "MOT", "MIX", "SHP" };
        for (int n = 0; n < 5; ++n)
        {
            placeOne(keys[n],
                     juce::String(kt::kFx[type].name) + " " + labels[n],
                     kindStr, ww, hh);
        }
    }

    placementMode = false;
    pendingFx = -1;
    rebuildCanvas();
    status.setText("Placed " + juce::String(kt::kFx[type].name), juce::dontSendNotification);
}

juce::File KyotoAudioProcessorEditor::sessionFile() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("KYOTRIPPAH").getChildFile("session.json");
}

juce::File KyotoAudioProcessorEditor::moduleDir() const
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("KYOTRIPPAH").getChildFile(proc.isFx() ? "fx" : "kyoto");
    dir.createDirectory();
    return dir;
}

void KyotoAudioProcessorEditor::loadNamed(const juce::String& name)
{
    const auto file = moduleDir().getChildFile(name + ".json");
    if (! file.existsAsFile()) return;
    auto parsed = juce::JSON::parse(file.loadFileAsString());
    auto* obj = parsed.getDynamicObject();
    if (obj == nullptr) return;

    nameBox.setText(obj->getProperty("name").toString(), juce::dontSendNotification);
    proc.uiState.setProperty("grid", (int) obj->getProperty("grid"), nullptr);
    auto th = obj->getProperty("theme").toString();
    if (th.isNotEmpty()) applyTheme(th);

    proc.uiState.removeAllChildren(nullptr);
    if (auto* slots = obj->getProperty("slots").getArray())
    {
        for (int i = 0; i < juce::jmin(proc.slotCount(), slots->size()); ++i)
        {
            auto* s = slots->getReference(i).getDynamicObject();
            if (s == nullptr) continue;
            const auto prefix = "s" + juce::String(i + 1).paddedLeft('0', 2);
            auto setP = [this, prefix](const juce::String& tail, float value)
            {
                if (auto* p = proc.apvts.getParameter(prefix + tail))
                    p->setValueNotifyingHost(p->convertTo0to1(value));
            };
            setP("on", (bool) s->getProperty("on") ? 1.f : 0.f);
            setP("type", (float) (int) s->getProperty("fx"));
            setP("amt", (float) s->getProperty("amount"));
            setP("tone", (float) s->getProperty("tone"));
            setP("mot", (float) s->getProperty("motion"));
            setP("mix", (float) s->getProperty("mix"));
            setP("shp", (float) s->getProperty("shape"));
        }
    }
    if (auto* warr = obj->getProperty("widgets").getArray())
    {
        for (auto& item : *warr)
        {
            auto* wsrc = item.getDynamicObject();
            if (wsrc == nullptr) continue;
            auto w = juce::ValueTree("w");
            w.setProperty("slot", (int) wsrc->getProperty("slot"), nullptr);
            w.setProperty("param", nativeKey(wsrc->getProperty("param").toString()), nullptr);
            w.setProperty("x", (int) wsrc->getProperty("x"), nullptr);
            w.setProperty("y", (int) wsrc->getProperty("y"), nullptr);
            w.setProperty("label", wsrc->getProperty("label").toString().isNotEmpty()
                                       ? wsrc->getProperty("label").toString()
                                       : wsrc->getProperty("param").toString(), nullptr);
            w.setProperty("kind", wsrc->getProperty("kind").toString().isNotEmpty()
                                      ? wsrc->getProperty("kind").toString()
                                      : "dial", nullptr);
            proc.uiState.appendChild(w, nullptr);
        }
    }
    gridStyleBox.setSelectedId((int) proc.uiState.getProperty("grid", 0) + 1, juce::dontSendNotification);
    showTab(1);
    status.setText("Loaded " + name, juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::saveLocal()
{
    proc.uiState.setProperty("name", nameBox.getText().trim(), nullptr);
    auto* obj = new juce::DynamicObject();
    obj->setProperty("format", "kyoteppah-module-1");
    obj->setProperty("face", proc.isFx() ? "fx" : "kyoto");
    obj->setProperty("name", nameBox.getText().trim().isEmpty() ? "untitled" : nameBox.getText().trim());
    obj->setProperty("grid", (int) proc.uiState.getProperty("grid", 0));
    obj->setProperty("theme", proc.uiState.getProperty("theme", "trippah"));

    juce::Array<juce::var> widgetsArr, slots;
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
    }
    for (int i = 0; i < proc.uiState.getNumChildren(); ++i)
    {
        auto child = proc.uiState.getChild(i);
        auto* w = new juce::DynamicObject();
        w->setProperty("slot", (int) child.getProperty("slot"));
        w->setProperty("param", webKey(child.getProperty("param").toString()));
        w->setProperty("x", (int) child.getProperty("x"));
        w->setProperty("y", (int) child.getProperty("y"));
        w->setProperty("label", child.getProperty("label").toString());
        w->setProperty("kind", child.getProperty("kind").toString().isNotEmpty()
                                   ? child.getProperty("kind").toString()
                                   : "dial");
        widgetsArr.add(juce::var(w));
    }
    obj->setProperty("slots", slots);
    obj->setProperty("widgets", widgetsArr);

    const auto name = obj->getProperty("name").toString();
    moduleDir().getChildFile(name + ".json").replaceWithText(juce::JSON::toString(juce::var(obj)));
    localBox.clear(juce::dontSendNotification);
    for (auto f : moduleDir().findChildFiles(juce::File::findFiles, false, "*.json"))
        localBox.addItem(f.getFileNameWithoutExtension(), localBox.getNumItems() + 1);
    status.setText("Saved " + name + " (theme + layout)", juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::publish()
{
    saveLocal();
    if (token.isEmpty())
    {
        status.setText("Login first to upload", juce::dontSendNotification);
        return;
    }
    const auto name = nameBox.getText().trim().isEmpty() ? "untitled" : nameBox.getText().trim();
    const auto file = moduleDir().getChildFile(name + ".json");
    if (! file.existsAsFile()) return;

    status.setText("Uploading " + name + "...", juce::dontSendNotification);
    std::thread([this, name, body = file.loadFileAsString()] {
        auto result = kt::DreamApi::publishModule(token, name, body);
        juce::MessageManager::callAsync([this, result, name] {
            status.setText(result.ok ? ("Published " + name) : ("Upload failed: " + result.error),
                           juce::dontSendNotification);
        });
    }).detach();
}

void KyotoAudioProcessorEditor::login()
{
    const auto user = userBox.getText().trim();
    const auto pass = passBox.getText();
    if (user.isEmpty()) return;
    status.setText("Logging in...", juce::dontSendNotification);
    std::thread([this, user, pass] {
        auto result = kt::DreamApi::login(user, pass);
        juce::MessageManager::callAsync([this, result, user] {
            if (result.ok)
            {
                token = result.token;
                account = user;
                auto* o = new juce::DynamicObject();
                o->setProperty("token", token);
                o->setProperty("user", account);
                o->setProperty("theme", proc.uiState.getProperty("theme", "trippah"));
                sessionFile().replaceWithText(juce::JSON::toString(juce::var(o)));
                status.setText("Logged in as " + account, juce::dontSendNotification);
                refreshFeed();
            }
            else
                status.setText("Login failed: " + result.error, juce::dontSendNotification);
        });
    }).detach();
}

void KyotoAudioProcessorEditor::sendChat()
{
    const auto text = msgBox.getText().trim();
    if (text.isEmpty() || token.isEmpty()) return;
    msgBox.clear();
    std::thread([this, text] {
        auto result = kt::DreamApi::sendChat(token, text);
        juce::MessageManager::callAsync([this, result] {
            if (result.ok) refreshFeed();
            else status.setText("Send failed: " + result.error, juce::dontSendNotification);
        });
    }).detach();
}

void KyotoAudioProcessorEditor::refreshFeed()
{
    std::thread([this] {
        auto result = kt::DreamApi::getFeed(token);
        juce::MessageManager::callAsync([this, result] {
            if (result.ok)
            {
                logBox.setText(result.body, juce::dontSendNotification);
                logBox.moveCaretToEnd();
            }
        });
    }).detach();
}

void KyotoAudioProcessorEditor::refreshCatalog()
{
    std::thread([this] {
        auto result = kt::DreamApi::getCatalog(token);
        juce::MessageManager::callAsync([this, result] {
            remoteBox.clear(juce::dontSendNotification);
            if (result.ok)
            {
                auto parsed = juce::JSON::parse(result.body);
                if (auto* arr = parsed.getArray())
                    for (auto& item : *arr)
                        if (auto* o = item.getDynamicObject())
                            remoteBox.addItem(o->getProperty("name").toString(), remoteBox.getNumItems() + 1);
                status.setText("Catalog loaded (" + juce::String(remoteBox.getNumItems()) + ")", juce::dontSendNotification);
            }
            else
                status.setText("Catalog error: " + result.error, juce::dontSendNotification);
        });
    }).detach();
}

void KyotoAudioProcessorEditor::timerCallback()
{
    if (tab == 0) refreshFeed();
}
