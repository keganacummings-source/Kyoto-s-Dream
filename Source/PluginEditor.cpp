#include "PluginEditor.h"
#include "DreamApi.h"
#include "FxCatalog.h"
#include <thread>

namespace
{
const char* kGrids[] = {
    "Series Row", "Series Column", "Performance Deck", "Keys Wall", "Diagonal Cascade",
    "Twin Columns", "Console Faders", "Hero Wave", "Arc Satellites", "Split Bay"
};

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
    else kind = Kind::Dial;

    caption.setText(node.getProperty("label").toString(), juce::dontSendNotification);
    caption.setJustificationType(juce::Justification::centred);
    caption.setFont(juce::FontOptions(10.f));
    caption.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(caption);

    if (kind == Kind::Dial || kind == Kind::Slider)
    {
        const int slot = (int) node.getProperty("slot", -1);
        const auto key = node.getProperty("param").toString();
        auto id = slot < 0 ? key : ("s" + juce::String(slot + 1).paddedLeft('0', 2) + key);
        slider.setSliderStyle(kind == Kind::Dial ? juce::Slider::RotaryHorizontalVerticalDrag
                                                 : juce::Slider::LinearVertical);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 14);
        addAndMakeVisible(slider);
        if (proc.apvts.getParameter(id) != nullptr)
            attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, id, slider);
    }
}

void CanvasWidget::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.f);
    g.setColour(juce::Colour(0x33000000));
    g.fillRoundedRectangle(bounds, 6.f);
    g.setColour(juce::Colour(0x55ffffff));
    g.drawRoundedRectangle(bounds, 6.f, 1.f);

    if (kind == Kind::Key)
    {
        g.setColour(isMouseButtonDown() ? juce::Colour(0xffc77dff) : juce::Colour(0xff2a2a3a));
        g.fillRoundedRectangle(bounds.reduced(6.f), 4.f);
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(11.f).withStyle("Bold"));
        g.drawText(node.getProperty("label").toString(), bounds, juce::Justification::centred);
    }
    else if (kind == Kind::Wave || kind == Kind::Stack)
    {
        g.setColour(juce::Colour(0xff10141c));
        g.fillRoundedRectangle(bounds.reduced(4.f), 4.f);
        g.setColour(kind == Kind::Stack ? juce::Colour(0xffffb060) : juce::Colour(0xff60c0ff));
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
        g.setColour(juce::Colour(0xffb0b8c8));
        g.setFont(juce::FontOptions(9.f));
        g.drawText(kind == Kind::Stack ? "STACKED FX" : "WAV", bounds.removeFromBottom(14).toNearestInt(), juce::Justification::centred);
    }
}

void CanvasWidget::resized()
{
    caption.setBounds(0, 0, getWidth(), 14);
    if (kind == Kind::Dial || kind == Kind::Slider)
        slider.setBounds(4, 14, getWidth() - 8, getHeight() - 18);
}

void CanvasWidget::mouseDown(const juce::MouseEvent&)
{
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
    list.setRowHeight(26);
    list.setOutlineThickness(0);
    addAndMakeVisible(list);
    addAndMakeVisible(search);
    search.setTextToShowWhenEmpty("Search FX", juce::Colours::grey);
    search.onTextChange = [this] { rebuildFilter(); };
    rebuildFilter();
}

void FxBrowser::setTheme(const kt::ThemePalette& t) { theme = t; repaint(); }
void FxBrowser::paint(juce::Graphics& g)
{
    g.setColour(kt::c(theme.panel));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.f);
}
void FxBrowser::resized()
{
    auto a = getLocalBounds().reduced(6);
    search.setBounds(a.removeFromTop(24));
    a.removeFromTop(4);
    list.setBounds(a);
}
int FxBrowser::getNumRows() { return filtered.size(); }
void FxBrowser::rebuildFilter()
{
    filter = search.getText().toLowerCase();
    filtered.clear();
    for (int i = 0; i < kt::kFxCount; ++i)
        if (filter.isEmpty() || juce::String(kt::kFx[i].name).toLowerCase().contains(filter))
            filtered.add(i);
    list.updateContent();
    list.repaint();
}
void FxBrowser::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool isSelected)
{
    if (row < 0 || row >= filtered.size()) return;
    g.setColour(isSelected ? kt::c(theme.accent).withAlpha(0.35f) : juce::Colours::transparentBlack);
    g.fillRect(0, 0, w, h);
    g.setColour(kt::c(theme.text));
    g.setFont(juce::FontOptions(12.f));
    g.drawText(kt::kFx[filtered[row]].name, 8, 0, w - 12, h, juce::Justification::centredLeft);
}
void FxBrowser::listBoxItemClicked(int row, const juce::MouseEvent&)
{
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
    setSize(1100, 700);
    setResizable(true, true);
    setResizeLimits(900, 560, 1600, 1000);

    for (auto* b : { &shareBtn, &chainBtn, &fxBtn, &logoutBtn, &loginBtn, &sendBtn, &feedBtn,
                     &addBtn, &saveBtn, &upBtn, &wavBtn, &fxAddBtn, &fxSaveBtn, &fxUpBtn, &chatRefreshBtn, &threadsBtn, &socialBtn, &dmBtn, &adminDeleteBtn })
    {
        addAndMakeVisible(b);
        b->setClickingTogglesState(false);
    }
    shareBtn.onClick = [this] { showTab(0); };
    chainBtn.onClick = [this] { if (loggedIn) showTab(1); };
    fxBtn.onClick = [this] { if (loggedIn) showTab(2); };
    logoutBtn.onClick = [this] { logout(); };
    loginBtn.onClick = [this] { login(); };
    sendBtn.onClick = [this] { sendChat(); };
    feedBtn.onClick = [this] { refreshFeed(); refreshCatalog(); };
    chatRefreshBtn.onClick = [this] { chatUtility("chat_list"); };
    threadsBtn.onClick = [this] { chatUtility("list_threads"); };
    socialBtn.onClick = [this] { chatUtility("social_list"); };
    dmBtn.onClick = [this] { chatUtility("dm_list"); };
    adminDeleteBtn.onClick = [this] { if (selectedCatalogId.isNotEmpty()) deleteCatalogId(selectedCatalogId); };
    utilityGoBtn.onClick = [this] { chatUtility(utilityActionBox.getText()); };
    addBtn.onClick = [this] { addSeriesStep(); };
    saveBtn.onClick = [this] { saveLocal(); };
    upBtn.onClick = [this] { publish(); };
    wavBtn.onClick = [this] { loadWav(); };
    fxAddBtn.onClick = [this] { 
        const int type = fxBrowser ? fxBrowser->getSelectedFx() : 0;
        auto step = juce::ValueTree("step");
        step.setProperty("fx", type, nullptr);
        step.setProperty("name", kt::kFx[juce::jlimit(0, kt::kFxCount - 1, type)].name, nullptr);
        step.setProperty("amount", (double) fxAmount.getValue(), nullptr);
        fxStack.appendChild(step, nullptr);
        stackLabel.setText("Stack " + juce::String(fxStack.getNumChildren()) + " effects — themed signal chain", juce::dontSendNotification);
    };
    fxSaveBtn.onClick = [this] { saveEffect(); };
    fxUpBtn.onClick = [this] { publishEffect(); };

    addAndMakeVisible(status);
    addAndMakeVisible(whoLabel);
    addAndMakeVisible(userBox);
    addAndMakeVisible(passBox);
    addAndMakeVisible(msgBox);
    addAndMakeVisible(logBox);
    addAndMakeVisible(utilityBox);
    addAndMakeVisible(utilityActionBox); addAndMakeVisible(utilityGoBtn);
    addAndMakeVisible(chatRefreshBtn); addAndMakeVisible(threadsBtn); addAndMakeVisible(socialBtn); addAndMakeVisible(dmBtn); addAndMakeVisible(adminDeleteBtn);
    addAndMakeVisible(themeBox);
    addAndMakeVisible(catalogView);
    addAndMakeVisible(nameBox);
    addAndMakeVisible(effectNameBox);
    addAndMakeVisible(gridStyleBox);
    addAndMakeVisible(localBox);
    addAndMakeVisible(kindBox);
    addAndMakeVisible(effectBox);
    addAndMakeVisible(fxAmount);
    addAndMakeVisible(stackLabel);
    addAndMakeVisible(panel);

    userBox.setTextToShowWhenEmpty("Username", juce::Colours::grey);
    passBox.setTextToShowWhenEmpty("Password", juce::Colours::grey);
    passBox.setPasswordCharacter((juce::juce_wchar) 0x2022);
    nameBox.setTextToShowWhenEmpty("Chain name", juce::Colours::grey);
    effectNameBox.setTextToShowWhenEmpty("Effect name", juce::Colours::grey);
    msgBox.setTextToShowWhenEmpty("Message", juce::Colours::grey);
    utilityBox.setTextToShowWhenEmpty("Target / ID / thread ID", juce::Colours::grey);
    const char* utilityItems[] = {"chat_list","list_threads","social_list","dm_list","dm_send","friend_request","friend_accept","friend_decline","friend_remove","wav_request","react_heart","chat_delete","chat_clear","presence","create_thread","comment"};
    for (int i=0;i<16;++i) utilityActionBox.addItem(utilityItems[i], i+1);
    utilityActionBox.setSelectedId(1);
    logBox.setMultiLine(true);
    logBox.setReadOnly(true);
    fxAmount.setRange(0.0, 1.0, 0.01);
    fxAmount.setValue(0.55);
    fxAmount.setSliderStyle(juce::Slider::LinearHorizontal);
    fxAmount.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 18);

    for (int i = 0; i < 10; ++i)
        gridStyleBox.addItem(kGrids[i], i + 1);
    gridStyleBox.setSelectedId(1);
    gridStyleBox.onChange = [this] {
        proc.uiState.setProperty("grid", gridStyleBox.getSelectedId() - 1, nullptr);
        reflowSeries();
    };
    kindBox.addItem("Dial", 1);
    kindBox.addItem("Fader", 2);
    kindBox.addItem("Key", 3);
    kindBox.addItem("WAV view", 4);
    kindBox.addItem("Saved effect", 5);
    kindBox.setSelectedId(1);
    localBox.onChange = [this] {
        const auto name = localBox.getText();
        if (name.isNotEmpty())
            loadCatalogId({}, name);
    };

    for (auto& t : kt::kThemes)
        themeBox.addItem(t.name, themeBox.getNumItems() + 1);
    themeBox.setSelectedId(1);
    themeBox.onChange = [this] {
        const int i = themeBox.getSelectedId() - 1;
        if (i >= 0 && i < (int) (sizeof(kt::kThemes) / sizeof(kt::kThemes[0])))
            applyTheme(kt::kThemes[i].id);
    };

    fxBrowser = std::make_unique<FxBrowser>(proc);
    addAndMakeVisible(*fxBrowser);
    catalogView.setViewedComponent(&catalogHolder, false);

    auto session = juce::JSON::parse(sessionFile().loadFileAsString());
    if (auto* o = session.getDynamicObject())
    {
        token = o->getProperty("token").toString();
        account = o->getProperty("user").toString();
        isAdmin = o->getProperty("admin", false);
        if (token.isNotEmpty() && account.isNotEmpty())
            setLoggedIn(true);
    }
    if (! loggedIn)
        setLoggedIn(false);
    applyTheme(proc.uiState.getProperty("theme", "trippah").toString());
    refreshEffectBox();
    startTimerHz(12);
}

KyotoAudioProcessorEditor::~KyotoAudioProcessorEditor() { catalogView.setViewedComponent(nullptr, false); }

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

void KyotoAudioProcessorEditor::timerCallback()
{
    animPhase += 0.035f;
    if (animPhase > juce::MathConstants<float>::twoPi) animPhase -= juce::MathConstants<float>::twoPi;
    repaint();
    for (auto* w : widgets)
        if (w->kind == CanvasWidget::Kind::Wave || w->kind == CanvasWidget::Kind::Stack)
            w->repaint();
}

void KyotoAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(kt::c(theme.bg));
    // Animated, theme-specific faint sprites. They remain behind controls and links.
    for (int i=0;i<7;i++) {
        const float px = 70.f + std::fmod((float)i*183.f + animPhase*28.f, (float)juce::jmax(100,getWidth()-120));
        const float py = 105.f + std::fmod((float)i*97.f + std::sin(animPhase+i)*18.f, (float)juce::jmax(120,getHeight()-170));
        drawThemeSprite(g, theme, theme.id, px, py, 0.55f + 0.12f*(i%3), animPhase + i);
    }
    g.setColour(kt::c(theme.panel).withAlpha(0.96f));
    g.fillRoundedRectangle(0, 0, (float)getWidth(), 44.f, 10.f);
    g.setColour(kt::c(theme.accent));
    g.setFont(juce::FontOptions(16.f).withStyle("Bold"));
    g.drawText(proc.getName(), 12, 8, 180, 26, juce::Justification::centredLeft);

    if (tab == 2)
    {
        auto a = panel.getBounds().toFloat();
        if (!a.isEmpty())
        {
            a = juce::Rectangle<float>((float)(getWidth()/3 + 20), 118.f, (float)(getWidth() - getWidth()/3 - 40), (float)(getHeight()-150));
            g.setColour(kt::c(theme.panel).withAlpha(0.90f)); g.fillRoundedRectangle(a, 14.f);
            g.setColour(kt::c(theme.border)); g.drawRoundedRectangle(a,14.f,1.f);
            g.setColour(kt::c(theme.accent)); g.setFont(juce::FontOptions(13.f).withStyle("Bold"));
            g.drawText("EFFECT STACK", a.getX()+18.f, a.getY()+12.f, 180.f, 22.f, juce::Justification::left);
            const int cols=4; const float cw=(a.getWidth()-36.f)/cols, ch=82.f;
            for(int i=0;i<fxStack.getNumChildren();++i)
            {
                auto st=fxStack.getChild(i); int col=i%cols,row=i/cols;
                auto r=juce::Rectangle<float>(a.getX()+12.f+col*cw,a.getY()+42.f+row*(ch+10.f),cw-8.f,ch);
                g.setColour(kt::c(theme.panel).brighter(0.10f)); g.fillRoundedRectangle(r,9.f);
                g.setColour(kt::c(theme.accent).withAlpha(0.55f)); g.drawRoundedRectangle(r,9.f,1.2f);
                g.setColour(kt::c(theme.text)); g.setFont(juce::FontOptions(11.f).withStyle("Bold"));
                g.drawText(juce::String(i+1)+"  "+st.getProperty("name").toString(),r.getX()+8,r.getY()+8,r.getWidth()-16,24,juce::Justification::left);
                g.setColour(kt::c(theme.muted)); g.setFont(juce::FontOptions(10.f));
                g.drawText("AMOUNT  "+juce::String((double)st.getProperty("amount"),2),r.getX()+8,r.getBottom()-24,r.getWidth()-16,16,juce::Justification::left);
                if(i+1<fxStack.getNumChildren()) { g.setColour(kt::c(theme.accent).withAlpha(0.55f)); g.drawLine(r.getRight()-2,r.getCentreY(),r.getRight()+8,r.getCentreY(),2.f); }
            }
        }
    }

    if (tab == 2)
    {
        auto a = panel.getBounds().toFloat();
        a = juce::Rectangle<float>((float)(getWidth()/3 + 20), 118.f, (float)(getWidth() - getWidth()/3 - 40), (float)(getHeight()-150));
        g.setColour(kt::c(theme.panel).withAlpha(0.90f)); g.fillRoundedRectangle(a, 14.f);
        g.setColour(kt::c(theme.border)); g.drawRoundedRectangle(a,14.f,1.f);
        g.setColour(kt::c(theme.accent)); g.setFont(juce::FontOptions(13.f).withStyle("Bold"));
        g.drawText("EFFECT STACK", a.getX()+18.f, a.getY()+12.f, 180.f, 22.f, juce::Justification::left);
        const int cols=4; const float cw=(a.getWidth()-36.f)/cols, ch=82.f;
        for(int i=0;i<fxStack.getNumChildren();++i)
        {
            auto st=fxStack.getChild(i); int col=i%cols,row=i/cols;
            auto r=juce::Rectangle<float>(a.getX()+12.f+col*cw,a.getY()+42.f+row*(ch+10.f),cw-8.f,ch);
            g.setColour(kt::c(theme.panel).brighter(0.10f)); g.fillRoundedRectangle(r,9.f);
            g.setColour(kt::c(theme.accent).withAlpha(0.55f)); g.drawRoundedRectangle(r,9.f,1.2f);
            g.setColour(kt::c(theme.text)); g.setFont(juce::FontOptions(11.f).withStyle("Bold"));
            g.drawText(juce::String(i+1)+"  "+st.getProperty("name").toString(),r.getX()+8,r.getY()+8,r.getWidth()-16,24,juce::Justification::left);
            g.setColour(kt::c(theme.muted)); g.setFont(juce::FontOptions(10.f));
            g.drawText("AMOUNT  "+juce::String((double)st.getProperty("amount"),2),r.getX()+8,r.getBottom()-24,r.getWidth()-16,16,juce::Justification::left);
            if(i+1<fxStack.getNumChildren()) { g.setColour(kt::c(theme.accent).withAlpha(0.55f)); g.drawLine(r.getRight()-2,r.getCentreY(),r.getRight()+8,r.getCentreY(),2.f); }
        }
    }

    if (tab == 1)
    {
        auto pb = panel.getBounds().toFloat();
        g.setColour(kt::c(theme.panel));
        g.fillRoundedRectangle(pb, 8.f);
        g.setColour(kt::c(theme.border));
        g.drawRoundedRectangle(pb, 8.f, 1.f);
        g.setColour(kt::c(theme.accent).withAlpha(0.30f));
        for (int i = 1; i < widgets.size(); ++i)
        {
            auto a = widgets[i - 1]->getBounds().getCentre().translated(panel.getX(), panel.getY()).toFloat();
            auto b = widgets[i]->getBounds().getCentre().translated(panel.getX(), panel.getY()).toFloat();
            g.drawLine(a.x, a.y, b.x, b.y, 5.f);
            g.setColour(kt::c(theme.accent).withAlpha(0.78f));
            g.drawLine(a.x, a.y, b.x, b.y, 1.6f);
            const float t = std::fmod(animPhase*0.6f + i*0.23f, 1.f);
            auto p = a + (b-a)*t;
            g.fillEllipse(p.x-3.f,p.y-3.f,6.f,6.f);
            g.setColour(kt::c(theme.accent).withAlpha(0.30f));
        }
    }
    if (! loggedIn)
    {
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillRect(getLocalBounds().withTrimmedTop(42));
    }
}

void KyotoAudioProcessorEditor::showTab(int next)
{
    tab = loggedIn ? next : 0;
    const bool share = tab == 0;
    const bool chain = tab == 1;
    const bool fx = tab == 2;
    logBox.setVisible(share && loggedIn);
    utilityBox.setVisible(share && loggedIn);
    utilityActionBox.setVisible(share && loggedIn); utilityGoBtn.setVisible(share && loggedIn);
    chatRefreshBtn.setVisible(share && loggedIn); threadsBtn.setVisible(share && loggedIn); socialBtn.setVisible(share && loggedIn); dmBtn.setVisible(share && loggedIn);
    adminDeleteBtn.setVisible(share && loggedIn && isAdmin);
    msgBox.setVisible(share && loggedIn);
    sendBtn.setVisible(share && loggedIn);
    feedBtn.setVisible(share && loggedIn);
    themeBox.setVisible(share && loggedIn);
    catalogView.setVisible(share && loggedIn);
    panel.setVisible(chain);
    if (fxBrowser) fxBrowser->setVisible(chain || fx);
    addBtn.setVisible(chain);
    gridStyleBox.setVisible(chain);
    nameBox.setVisible(chain);
    localBox.setVisible(chain);
    saveBtn.setVisible(chain);
    upBtn.setVisible(chain);
    kindBox.setVisible(chain);
    wavBtn.setVisible(chain);
    effectBox.setVisible(chain);
    effectNameBox.setVisible(fx);
    fxAddBtn.setVisible(fx);
    fxSaveBtn.setVisible(fx);
    fxUpBtn.setVisible(fx);
    fxAmount.setVisible(fx);
    stackLabel.setVisible(fx);
    shareBtn.setToggleState(share, juce::dontSendNotification);
    chainBtn.setToggleState(chain, juce::dontSendNotification);
    fxBtn.setToggleState(fx, juce::dontSendNotification);
    resized();
    if (chain) rebuildCanvas();
    repaint();
}

void KyotoAudioProcessorEditor::resized()
{
    shareBtn.setBounds(190, 7, 120, 28);
    chainBtn.setBounds(316, 7, 90, 28);
    fxBtn.setBounds(412, 7, 120, 28);
    whoLabel.setBounds(getWidth() - 280, 7, 150, 28);
    logoutBtn.setBounds(getWidth() - 120, 7, 100, 28);
    status.setBounds(540, 7, getWidth() - 840, 28);

    auto area = getLocalBounds().withTrimmedTop(48).reduced(10);
    if (! loggedIn)
    {
        auto box = area.withSizeKeepingCentre(320, 140);
        userBox.setBounds(box.removeFromTop(32));
        box.removeFromTop(8);
        passBox.setBounds(box.removeFromTop(32));
        box.removeFromTop(10);
        loginBtn.setBounds(box.removeFromTop(32).removeFromLeft(140));
        return;
    }
    if (tab == 0)
    {
        auto top = area.removeFromTop(34);
        themeBox.setBounds(top.removeFromLeft(170)); top.removeFromLeft(6);
        feedBtn.setBounds(top.removeFromLeft(72)); top.removeFromLeft(4);
        chatRefreshBtn.setBounds(top.removeFromLeft(58)); top.removeFromLeft(4);
        threadsBtn.setBounds(top.removeFromLeft(72)); top.removeFromLeft(4);
        socialBtn.setBounds(top.removeFromLeft(72)); top.removeFromLeft(4);
        dmBtn.setBounds(top.removeFromLeft(52)); top.removeFromLeft(4);
        if (isAdmin) adminDeleteBtn.setBounds(top.removeFromLeft(72));
        auto bottom = area.removeFromBottom(38);
        msgBox.setBounds(bottom.removeFromLeft(bottom.getWidth() - 104)); sendBtn.setBounds(bottom);
        auto util = area.removeFromBottom(30); utilityBox.setBounds(util.removeFromLeft(util.getWidth()-170)); util.removeFromLeft(6); utilityActionBox.setBounds(util.removeFromLeft(120)); util.removeFromLeft(6); utilityGoBtn.setBounds(util);
        auto left = area.removeFromLeft(juce::jmax(340, area.getWidth() / 2));
        logBox.setBounds(left.reduced(0, 6));
        catalogView.setBounds(area.reduced(6, 6));
        catalogHolder.setSize(area.getWidth() - 20, juce::jmax(area.getHeight(), (catalog.size() + 1) / 2 * 100));
    }
    else if (tab == 1)
    {
        auto top = area.removeFromTop(34);
        nameBox.setBounds(top.removeFromLeft(160));
        top.removeFromLeft(6);
        gridStyleBox.setBounds(top.removeFromLeft(150));
        top.removeFromLeft(6);
        kindBox.setBounds(top.removeFromLeft(120));
        top.removeFromLeft(6);
        effectBox.setBounds(top.removeFromLeft(140));
        top.removeFromLeft(6);
        addBtn.setBounds(top.removeFromLeft(90));
        top.removeFromLeft(6);
        wavBtn.setBounds(top.removeFromLeft(90));
        top.removeFromLeft(6);
        saveBtn.setBounds(top.removeFromLeft(70));
        upBtn.setBounds(top.removeFromLeft(80));
        auto bottom = area.removeFromBottom(30);
        localBox.setBounds(bottom);
        auto left = area.removeFromLeft(230);
        if (fxBrowser) fxBrowser->setBounds(left);
        area.removeFromLeft(8);
        panel.setBounds(area);
        reflowSeries();
    }
    else
    {
        auto top = area.removeFromTop(40);
        effectNameBox.setBounds(top.removeFromLeft(230)); top.removeFromLeft(8);
        fxAmount.setBounds(top.removeFromLeft(240)); top.removeFromLeft(8);
        fxAddBtn.setBounds(top.removeFromLeft(100)); top.removeFromLeft(6);
        fxSaveBtn.setBounds(top.removeFromLeft(110)); top.removeFromLeft(6);
        fxUpBtn.setBounds(top.removeFromLeft(120));
        stackLabel.setBounds(area.removeFromTop(30));
        if (fxBrowser) fxBrowser->setBounds(area.removeFromLeft(300));
    }
}

juce::Rectangle<int> KyotoAudioProcessorEditor::cellFor(int index, const juce::String& kind) const
{
    int ww = 78, hh = 86;
    if (kind == "slider") { ww = 52; hh = 120; }
    else if (kind == "key") { ww = 58; hh = 58; }
    else if (kind == "wave") { ww = 180; hh = 90; }
    else if (kind == "stack") { ww = 150; hh = 84; }
    const int grid = juce::jlimit(0, 9, gridStyleBox.getSelectedId() - 1);
    const int W = juce::jmax(1, panel.getWidth());
    const int H = juce::jmax(1, panel.getHeight());
    const int n = juce::jmax(1, index);
    int x = 12, y = 12;
    switch (grid)
    {
        case 0: x = 16 + (index % 6) * (W / 6); y = 20 + (index / 6) * (H / 3); break;
        case 1: x = 24 + (index / 5) * (W / 3); y = 12 + (index % 5) * (H / 5); break;
        case 2:
            if (kind == "wave") { x = W / 2 - ww / 2; y = H / 2 - hh / 2; }
            else { x = (int) (W * 0.5 + std::cos(index * 0.9f) * W * 0.34) - ww / 2; y = (int) (H * 0.5 + std::sin(index * 0.9f) * H * 0.32) - hh / 2; }
            break;
        case 3: x = 10 + (index % 8) * (W / 8); y = kind == "key" ? H - hh - 8 : 16 + (index / 8) * 90; break;
        case 4: x = 16 + index * (W / 10); y = 16 + index * (H / 12); break;
        case 5: x = (index % 2 == 0 ? 20 : W / 2); y = 16 + (index / 2) * (H / 5); break;
        case 6: x = 12 + index * (W / 10); y = H - hh - 10; break;
        case 7: x = kind == "wave" ? 12 : W / 2 + (index % 4) * 90; y = kind == "wave" ? 20 : 20 + (index / 4) * 100; break;
        case 8: x = (int) (W * 0.5 + std::cos(-1.2f + index * 0.35f) * (W * 0.42)) - ww / 2; y = (int) (H * 0.72 + std::sin(-1.2f + index * 0.35f) * (H * 0.38)) - hh / 2; break;
        default: x = index < 4 ? 16 : W / 2; y = 16 + (index % 4) * (H / 4); break;
    }
    return { juce::jlimit(0, juce::jmax(0, W - ww), x), juce::jlimit(0, juce::jmax(0, H - hh), y), ww, hh };
}

void KyotoAudioProcessorEditor::reflowSeries()
{
    int i = 0;
    for (auto* w : widgets)
    {
        auto kind = w->node.getProperty("kind").toString();
        auto r = cellFor(i++, kind);
        w->node.setProperty("x", r.getX(), nullptr);
        w->node.setProperty("y", r.getY(), nullptr);
        w->setBounds(r);
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
    reflowSeries();
}

void KyotoAudioProcessorEditor::addSeriesStep()
{
    const int kindId = kindBox.getSelectedId();
    juce::String kind = "dial";
    if (kindId == 2) kind = "slider";
    else if (kindId == 3) kind = "key";
    else if (kindId == 4) kind = "wave";
    else if (kindId == 5) kind = "stack";

    auto place = [&](int slot, const juce::String& param, const juce::String& label, const juce::String& k, int note)
    {
        auto node = juce::ValueTree("w");
        node.setProperty("slot", slot, nullptr);
        node.setProperty("param", param, nullptr);
        node.setProperty("label", label, nullptr);
        node.setProperty("kind", k, nullptr);
        node.setProperty("series", proc.uiState.getNumChildren(), nullptr);
        if (note >= 0) node.setProperty("note", note, nullptr);
        proc.uiState.appendChild(node, nullptr);
    };

    if (kind == "key")
    {
        place(-1, "amt", "C" + juce::String(widgets.size() % 8 + 3), "key", 60 + widgets.size());
    }
    else if (kind == "wave")
    {
        place(-1, "amt", "WAV", "wave", -1);
    }
    else if (kind == "stack")
    {
        const auto file = effectDir().getChildFile(effectBox.getText() + ".json");
        auto parsed = juce::JSON::parse(file.loadFileAsString());
        auto* obj = parsed.getDynamicObject();
        if (obj == nullptr || ! obj->getProperty("steps").isArray())
        {
            status.setText("Save an effect in FX Builder first", juce::dontSendNotification);
            return;
        }
        int first = -1;
        auto* steps = obj->getProperty("steps").getArray();
        for (int n = 0; n < steps->size(); ++n)
        {
            int slot = -1;
            for (int i = 0; i < proc.slotCount(); ++i)
                if (proc.apvts.getParameter("s" + juce::String(i + 1).paddedLeft('0', 2) + "on")->getValue() < 0.5f)
                { slot = i; break; }
            if (slot < 0) break;
            if (first < 0) first = slot;
            auto* s = steps->getReference(n).getDynamicObject();
            const auto prefix = "s" + juce::String(slot + 1).paddedLeft('0', 2);
            if (auto* t = proc.apvts.getParameter(prefix + "type"))
                t->setValueNotifyingHost(t->convertTo0to1((float) (int) s->getProperty("fx")));
            if (auto* on = proc.apvts.getParameter(prefix + "on"))
                on->setValueNotifyingHost(1.f);
            if (auto* a = proc.apvts.getParameter(prefix + "amt"))
                a->setValueNotifyingHost(a->convertTo0to1((float) s->getProperty("amount")));
        }
        place(first, "amt", effectBox.getText(), "stack", -1);
    }
    else
    {
        int type = fxBrowser ? fxBrowser->getSelectedFx() : 0;
        int slot = -1;
        for (int i = 0; i < proc.slotCount(); ++i)
            if (proc.apvts.getParameter("s" + juce::String(i + 1).paddedLeft('0', 2) + "on")->getValue() < 0.5f)
            { slot = i; break; }
        if (slot < 0)
        {
            status.setText("Series full", juce::dontSendNotification);
            return;
        }
        const auto prefix = "s" + juce::String(slot + 1).paddedLeft('0', 2);
        if (auto* t = proc.apvts.getParameter(prefix + "type"))
            t->setValueNotifyingHost(t->convertTo0to1((float) type));
        if (auto* on = proc.apvts.getParameter(prefix + "on"))
            on->setValueNotifyingHost(1.f);
        place(slot, "amt", kt::kFx[type].name, kind, -1);
    }
    rebuildCanvas();
    status.setText("Linked step " + juce::String(widgets.size()) + " on " + gridStyleBox.getText(), juce::dontSendNotification);
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
    std::thread([this, user, pass] {
        auto r = kt::login(user, pass);
        juce::MessageManager::callAsync([this, r] {
            if (! r.ok || r.token.isEmpty())
            {
                status.setText(r.error.isEmpty() ? "Login failed" : r.error, juce::dontSendNotification);
                return;
            }
            token = r.token;
            account = r.user.isNotEmpty() ? r.user : userBox.getText().trim();
            isAdmin = (r.role == "super" || account.equalsIgnoreCase("Trippah") || account.equalsIgnoreCase("Goonr"));
            auto* o = new juce::DynamicObject();
            o->setProperty("user", account);
            o->setProperty("token", token);
            o->setProperty("admin", isAdmin);
            sessionFile().replaceWithText(juce::JSON::toString(juce::var(o)));
            setLoggedIn(true);
            status.setText("Signed in as " + account, juce::dontSendNotification);
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
    const auto text = msgBox.getText().trim();
    if (text.isEmpty() || token.isEmpty()) return;
    std::thread([this, text] {
        auto r = kt::sendChat(token, text);
        juce::MessageManager::callAsync([this, r] {
            if (! r.ok) status.setText(r.error, juce::dontSendNotification);
            else { msgBox.clear(); refreshFeed(); }
        });
    }).detach();
}

void KyotoAudioProcessorEditor::chatUtility(const juce::String& selectedAction)
{
    if (token.isEmpty()) return;
    const auto action = selectedAction.trim();
    const auto target = utilityBox.getText().trim();
    const auto message = msgBox.getText().trim();
    std::thread([this, action, target, message] {
        kt::DreamResult r;
        if (action == "dm_list") r = kt::getDM(token, target);
        else if (action == "dm_send") r = kt::sendDM(token, target, message);
        else if (action == "social_list") r = kt::getSocial(token);
        else if (action == "friend_request" || action == "friend_accept" || action == "friend_decline" || action == "friend_remove" || action == "wav_request") r = kt::friendRequest(token, action, target);
        else if (action == "react_heart") r = kt::react(token, "chat", target, "heart");
        else if (action == "chat_delete") { auto* o=new juce::DynamicObject(); o->setProperty("id",target); r=kt::postAction("chat_delete",juce::var(o),token); }
        else if (action == "chat_clear") r=kt::postAction("chat_clear",juce::var(new juce::DynamicObject()),token);
        else if (action == "create_thread") { auto* o=new juce::DynamicObject(); o->setProperty("text",message); r=kt::postAction("create_thread",juce::var(o),token); }
        else if (action == "comment") { auto* o=new juce::DynamicObject(); o->setProperty("threadId",target); o->setProperty("text",message); r=kt::postAction("comment",juce::var(o),token); }
        else r=kt::postAction(action,juce::var(new juce::DynamicObject()),token);
        juce::MessageManager::callAsync([this, r] {
            logBox.setText(r.ok ? (r.raw.isEmpty() ? r.body : r.raw) : (r.error.isEmpty() ? r.raw : r.error));
            status.setText(r.ok ? "DreamShare utility complete" : "Utility failed: " + r.error, juce::dontSendNotification);
            if (r.ok && (r.body.contains("chat") || r.raw.contains("chat"))) refreshFeed();
        });
    }).detach();
}

void KyotoAudioProcessorEditor::deleteCatalogId(const juce::String& id)
{
    if (!isAdmin || id.isEmpty()) return;
    std::thread([this,id]{
        auto r=kt::deleteModule(token,id);
        juce::MessageManager::callAsync([this,r]{ status.setText(r.ok?"CATALOG ITEM REMOVED":"REMOVE FAILED: "+r.error,juce::dontSendNotification); if(r.ok){selectedCatalogId.clear();refreshCatalog();} });
    }).detach();
}

void KyotoAudioProcessorEditor::refreshFeed()
{
    if (token.isEmpty()) return;
    std::thread([this] {
        auto r = kt::getFeed(token);
        juce::MessageManager::callAsync([this, r] {
            if (! r.ok)
            {
                status.setText(r.error.isEmpty() ? "DreamShare feed failed" : r.error, juce::dontSendNotification);
                logBox.setText(r.raw);
                return;
            }
            juce::String log;
            if (auto* arr = r.parsed.getDynamicObject() != nullptr
                    ? r.parsed.getDynamicObject()->getProperty("chat").getArray() : nullptr)
            {
                for (auto& item : *arr)
                    if (auto* m = item.getDynamicObject())
                        log << m->getProperty("user").toString() << ": " << m->getProperty("text").toString() << "\n";
            }
            logBox.setText(log.isEmpty() ? r.body : log);
            status.setText("DreamShare live", juce::dontSendNotification);
        });
    }).detach();
}

void KyotoAudioProcessorEditor::refreshCatalog()
{
    if (token.isEmpty()) return;
    std::thread([this] {
        auto r = kt::getCatalog(token);
        juce::MessageManager::callAsync([this, r] {
            catalog.clear();
            catalogHolder.removeAllChildren();
            auto* mods = r.parsed.getDynamicObject() != nullptr
                             ? r.parsed.getDynamicObject()->getProperty("modules").getArray() : nullptr;
            if (mods == nullptr)
            {
                status.setText(r.ok ? "Catalog empty" : "Catalog needs module_list on the worker", juce::dontSendNotification);
                return;
            }
            int i = 0;
            for (auto& item : *mods)
            {
                auto* m = item.getDynamicObject();
                if (m == nullptr) continue;
                CatalogItem c;
                c.id = m->getProperty("id").toString();
                c.name = m->getProperty("name").toString();
                c.face = m->getProperty("face").toString();
                c.author = m->getProperty("author").toString();
                catalog.add(c);
                auto* card = new juce::TextButton(c.name + "\n" + c.face + "  ·  " + c.author);
                card->setBounds((i % 2) * 250, (i / 2) * 104, 238, 94);
                card->onClick = [this, id = c.id, name = c.name] { selectedCatalogId=id; loadCatalogId(id, name); };
                catalogHolder.addAndMakeVisible(card);
                ++i;
            }
            catalogHolder.setSize(500, juce::jmax(220, ((i + 1) / 2) * 104));
            catalogView.setViewedComponent(&catalogHolder, false);
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
                if (auto* steps=obj->getProperty("steps").getArray()) for(auto& v:*steps) if(auto* so=v.getDynamicObject()){auto st=juce::ValueTree("step");st.setProperty("fx",(int)so->getProperty("fx"),nullptr);st.setProperty("name",so->getProperty("name").toString(),nullptr);st.setProperty("amount",(double)so->getProperty("amount"),nullptr);fxStack.appendChild(st,nullptr);}
                effectNameBox.setText(obj->getProperty("name").toString(), juce::dontSendNotification); showTab(2); return;
            }
            nameBox.setText(obj->getProperty("name").toString(), juce::dontSendNotification);
            if (auto* warr = obj->getProperty("widgets").getArray())
            {
                proc.uiState.removeAllChildren(nullptr);
                for (auto& item : *warr)
                {
                    auto* wsrc = item.getDynamicObject();
                    if (wsrc == nullptr) continue;
                    auto w = juce::ValueTree("w");
                    w.setProperty("slot", (int) wsrc->getProperty("slot"), nullptr);
                    w.setProperty("param", nativeKey(wsrc->getProperty("param").toString()), nullptr);
                    w.setProperty("label", wsrc->getProperty("label").toString(), nullptr);
                    w.setProperty("kind", wsrc->getProperty("kind").toString(), nullptr);
                    w.setProperty("peaks", wsrc->getProperty("peaks").toString(), nullptr);
                    proc.uiState.appendChild(w, nullptr);
                }
            }
            gridStyleBox.setSelectedId((int) obj->getProperty("grid") + 1, juce::dontSendNotification);
            showTab(1);
        }
        return;
    }
    std::thread([this, id] {
        auto r = kt::getModule(token, id);
        juce::MessageManager::callAsync([this, r] {
            if (! r.ok) { status.setText(r.error, juce::dontSendNotification); return; }
            auto mod = r.parsed.getDynamicObject()->getProperty("module");
            auto* mo = mod.getDynamicObject();
            if (!mo) return;
            const auto modName = mo->getProperty("name").toString();
            if (mo->getProperty("face").toString() == "effect" || mo->getProperty("format").toString() == "kyoteppah-effect-1")
            {
                fxStack.removeAllChildren(nullptr);
                if (auto* steps = mo->getProperty("steps").getArray()) for (auto& v : *steps)
                { if (auto* so=v.getDynamicObject()) { auto st=juce::ValueTree("step"); st.setProperty("fx",(int)so->getProperty("fx"),nullptr); st.setProperty("name",so->getProperty("name").toString(),nullptr); st.setProperty("amount",(double)so->getProperty("amount"),nullptr); fxStack.appendChild(st,nullptr); } }
                effectNameBox.setText(modName, juce::dontSendNotification); showTab(2);
                status.setText("Loaded effect: " + modName, juce::dontSendNotification);
            }
            else
            {
                auto file = moduleDir().getChildFile(modName + ".json"); file.replaceWithText(juce::JSON::toString(mod));
                loadCatalogId({}, modName);
                status.setText("Loaded catalog module", juce::dontSendNotification);
            }
        });
    }).detach();
}

void KyotoAudioProcessorEditor::applyTheme(const juce::String& id)
{
    for (auto& t : kt::kThemes)
        if (id == t.id) { theme = t; break; }
    proc.uiState.setProperty("theme", id, nullptr);
    if (fxBrowser) fxBrowser->setTheme(theme);
    repaint();
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
        o->setProperty("amount", (double) s.getProperty("amount"));
        steps.add(juce::var(o));
    }
    obj->setProperty("steps", steps);
    effectDir().getChildFile(name + ".json").replaceWithText(juce::JSON::toString(juce::var(obj)));
    refreshEffectBox();
    status.setText("Saved effect " + name, juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::publishEffect()
{
    saveEffect();
    const auto name = effectNameBox.getText().trim().isEmpty() ? "untitled-fx" : effectNameBox.getText().trim();
    const auto body = effectDir().getChildFile(name + ".json").loadFileAsString();
    std::thread([this, name, body] {
        auto r = kt::publishModule(token, name, body);
        juce::MessageManager::callAsync([this, r] {
            status.setText(r.ok ? "Effect catalogued" : r.error, juce::dontSendNotification);
        });
    }).detach();
}

void KyotoAudioProcessorEditor::refreshEffectBox()
{
    effectBox.clear();
    int id = 1;
    for (auto f : effectDir().findChildFiles(juce::File::findFiles, false, "*.json"))
        effectBox.addItem(f.getFileNameWithoutExtension(), id++);
    if (effectBox.getNumItems() > 0) effectBox.setSelectedId(1);
    localBox.clear();
    id = 1;
    for (auto f : moduleDir().findChildFiles(juce::File::findFiles, false, "*.json"))
        localBox.addItem(f.getFileNameWithoutExtension(), id++);
}

void KyotoAudioProcessorEditor::saveLocal()
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("format", "kyoteppah-module-1");
    obj->setProperty("face", proc.isFx() ? "fx" : "chain");
    const auto name = nameBox.getText().trim().isEmpty() ? "untitled" : nameBox.getText().trim();
    obj->setProperty("name", name);
    obj->setProperty("grid", gridStyleBox.getSelectedId() - 1);
    obj->setProperty("theme", proc.uiState.getProperty("theme", "trippah"));
    juce::Array<juce::var> widgetsArr, slots;
    for (int i = 0; i < proc.slotCount(); ++i)
    {
        auto* s = new juce::DynamicObject();
        const auto prefix = "s" + juce::String(i + 1).paddedLeft('0', 2);
        s->setProperty("on", proc.apvts.getRawParameterValue(prefix + "on")->load() >= 0.5f);
        s->setProperty("fx", (int) proc.apvts.getRawParameterValue(prefix + "type")->load());
        s->setProperty("amount", proc.apvts.getRawParameterValue(prefix + "amt")->load());
        slots.add(juce::var(s));
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
        o->setProperty("peaks", w.getProperty("peaks").toString());
        o->setProperty("x", (int) w.getProperty("x"));
        o->setProperty("y", (int) w.getProperty("y"));
        widgetsArr.add(juce::var(o));
    }
    obj->setProperty("slots", slots);
    obj->setProperty("widgets", widgetsArr);
    moduleDir().getChildFile(name + ".json").replaceWithText(juce::JSON::toString(juce::var(obj)));
    refreshEffectBox();
    status.setText("Saved chain " + name, juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::publish()
{
    saveLocal();
    const auto name = nameBox.getText().trim().isEmpty() ? "untitled" : nameBox.getText().trim();
    const auto body = moduleDir().getChildFile(name + ".json").loadFileAsString();
    std::thread([this, name, body] {
        auto r = kt::publishModule(token, name, body);
        juce::MessageManager::callAsync([this, r] {
            status.setText(r.ok ? "Chain catalogued" : r.error, juce::dontSendNotification);
            if (r.ok) refreshCatalog();
        });
    }).detach();
}
