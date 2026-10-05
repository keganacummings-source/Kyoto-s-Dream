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

KyotoAudioProcessorEditor::Knob::Knob(KyotoAudioProcessor& p, juce::ValueTree n)
    : proc(p), node(std::move(n))
{
    const int slot = (int) node.getProperty("slot", 0);
    const auto key = node.getProperty("param").toString();
    auto id = slot < 0 ? key : ("s" + juce::String(slot + 1).paddedLeft('0', 2) + key);
    caption.setText(node.getProperty("label").toString(), juce::dontSendNotification);
    caption.setJustificationType(juce::Justification::centred);
    caption.setFont(juce::FontOptions(11.f));
    caption.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(caption);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 16);
    addAndMakeVisible(slider);
    if (proc.apvts.getParameter(id) != nullptr)
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, id, slider);
}

void KyotoAudioProcessorEditor::Knob::resized()
{
    caption.setBounds(0, 0, getWidth(), 16);
    slider.setBounds(0, 16, getWidth(), getHeight() - 16);
}

void KyotoAudioProcessorEditor::Knob::mouseDrag(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown() || e.getDistanceFromDragStart() < 6)
        return;
    auto* parent = getParentComponent();
    if (parent == nullptr) return;
    const int nx = juce::jlimit(0, parent->getWidth() - getWidth(), getX() + e.getDistanceFromDragStartX());
    const int ny = juce::jlimit(0, parent->getHeight() - getHeight(), getY() + e.getDistanceFromDragStartY());
    setTopLeftPosition(nx, ny);
    node.setProperty("x", nx * 100 / juce::jmax(1, parent->getWidth()), nullptr);
    node.setProperty("y", ny * 100 / juce::jmax(1, parent->getHeight()), nullptr);
}

KyotoAudioProcessorEditor::KyotoAudioProcessorEditor(KyotoAudioProcessor& p)
    : AudioProcessorEditor(p), proc(p)
{
    setSize(980, 640);
    setResizable(true, true);
    setResizeLimits(720, 480, 1600, 1000);
    for (auto* b : { &shareBtn, &buildBtn, &saveBtn, &upBtn, &loginBtn, &sendBtn, &addBtn, &feedBtn, &catBtn })
        addAndMakeVisible(b);
    for (auto* e : { &userBox, &passBox, &msgBox, &nameBox })
    {
        e->setMultiLine(false);
        addAndMakeVisible(e);
    }
    passBox.setPasswordCharacter('*');
    logBox.setMultiLine(true);
    logBox.setReadOnly(true);
    addAndMakeVisible(logBox);
    addAndMakeVisible(gridBox);
    addAndMakeVisible(fxBox);
    addAndMakeVisible(localBox);
    addAndMakeVisible(remoteBox);
    addAndMakeVisible(status);
    addAndMakeVisible(panel);
    nameBox.setText(proc.uiState.getProperty("name").toString(), juce::dontSendNotification);
    for (int i = 0; i < 25; ++i)
        gridBox.addItem("Grid " + juce::String(i + 1), i + 1);
    gridBox.setSelectedId((int) proc.uiState.getProperty("grid", 0) + 1, juce::dontSendNotification);
    for (int i = 0; i < kt::kFxCount; ++i)
        fxBox.addItem(kt::kFx[i].name, i + 1);
    fxBox.setSelectedId(3, juce::dontSendNotification);
    if (auto json = juce::JSON::parse(sessionFile().loadFileAsString()); json.getDynamicObject() != nullptr)
    {
        auto* o = json.getDynamicObject();
        token = o->getProperty("token").toString();
        account = o->getProperty("user").toString();
        userBox.setText(account, juce::dontSendNotification);
    }
    shareBtn.onClick = [this] { showTab(0); };
    buildBtn.onClick = [this] { showTab(1); };
    saveBtn.onClick = [this] { saveLocal(); };
    upBtn.onClick = [this] { publish(); };
    loginBtn.onClick = [this] { login(); };
    sendBtn.onClick = [this] { sendChat(); };
    addBtn.onClick = [this] { addEffect(); };
    feedBtn.onClick = [this] { refreshFeed(); };
    catBtn.onClick = [this] { refreshCatalog(); };
    gridBox.onChange = [this] { proc.uiState.setProperty("grid", gridBox.getSelectedId() - 1, nullptr); };
    localBox.onChange = [this] { loadNamed(localBox.getText()); };
    startTimer(8000);
    showTab(0);
    refreshFeed();
}

KyotoAudioProcessorEditor::~KyotoAudioProcessorEditor() = default;

void KyotoAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff12100e));
    g.setColour(juce::Colour(0xffefe6d6));
    g.setFont(juce::FontOptions(16.f));
    g.drawText(proc.getName(), 12, 8, 220, 24, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff3a3128));
    g.drawRect(panel.getBounds());
}

void KyotoAudioProcessorEditor::showTab(int next)
{
    tab = next;
    const bool share = tab == 0;
    logBox.setVisible(share);
    userBox.setVisible(share);
    passBox.setVisible(share);
    msgBox.setVisible(share);
    loginBtn.setVisible(share);
    sendBtn.setVisible(share);
    feedBtn.setVisible(share);
    remoteBox.setVisible(share);
    catBtn.setVisible(share);
    panel.setVisible(! share);
    fxBox.setVisible(! share);
    addBtn.setVisible(! share);
    gridBox.setVisible(! share);
    nameBox.setVisible(! share);
    localBox.setVisible(! share);
    saveBtn.setVisible(! share);
    upBtn.setVisible(! share);
    resized();
    if (! share) rebuildKnobs();
}

void KyotoAudioProcessorEditor::resized()
{
    shareBtn.setBounds(240, 8, 120, 28);
    buildBtn.setBounds(366, 8, 90, 28);
    status.setBounds(470, 8, getWidth() - 482, 28);
    auto area = getLocalBounds().withTrimmedTop(44).reduced(12);
    if (tab == 0)
    {
        userBox.setBounds(area.removeFromTop(28));
        area.removeFromTop(6);
        passBox.setBounds(area.removeFromTop(28));
        area.removeFromTop(6);
        loginBtn.setBounds(area.removeFromTop(28).removeFromLeft(120));
        area.removeFromTop(8);
        logBox.setBounds(area.removeFromTop(juce::jmax(120, area.getHeight() - 120)));
        area.removeFromTop(6);
        msgBox.setBounds(area.removeFromTop(28).withTrimmedRight(220));
        sendBtn.setBounds(msgBox.getRight() + 8, msgBox.getY(), 90, 28);
        feedBtn.setBounds(sendBtn.getRight() + 8, msgBox.getY(), 80, 28);
        remoteBox.setBounds(12, getHeight() - 44, getWidth() - 160, 28);
        catBtn.setBounds(getWidth() - 140, getHeight() - 44, 120, 28);
    }
    else
    {
        nameBox.setBounds(area.removeFromTop(28).removeFromLeft(220));
        gridBox.setBounds(240, 44, 140, 28);
        fxBox.setBounds(390, 44, 220, 28);
        addBtn.setBounds(620, 44, 90, 28);
        saveBtn.setBounds(720, 44, 80, 28);
        upBtn.setBounds(808, 44, 90, 28);
        localBox.setBounds(12, getHeight() - 44, getWidth() - 24, 28);
        panel.setBounds(12, 80, getWidth() - 24, getHeight() - 136);
        for (auto* k : knobs)
        {
            const int x = (int) k->node.getProperty("x", 8) * panel.getWidth() / 100;
            const int y = (int) k->node.getProperty("y", 12) * panel.getHeight() / 100;
            k->setBounds(x, y, 84, 96);
        }
    }
}

void KyotoAudioProcessorEditor::rebuildKnobs()
{
    knobs.clear();
    for (int i = 0; i < proc.uiState.getNumChildren(); ++i)
    {
        auto child = proc.uiState.getChild(i);
        if (! child.hasType("w")) continue;
        auto* knob = knobs.add(new Knob(proc, child));
        panel.addAndMakeVisible(knob);
    }
    resized();
}

void KyotoAudioProcessorEditor::addEffect()
{
    const int type = fxBox.getSelectedId() - 1;
    int slot = -1;
    for (int i = 0; i < proc.slotCount(); ++i)
        if (proc.apvts.getParameter(juce::String("s") + juce::String(i + 1).paddedLeft('0', 2) + "on")->getValue() < 0.5f)
        { slot = i; break; }
    if (slot < 0)
    {
        status.setText(proc.isFx() ? "FX chain is full at 12" : "KYOTO chain is full at 8", juce::dontSendNotification);
        return;
    }
    const auto prefix = "s" + juce::String(slot + 1).paddedLeft('0', 2);
    if (auto* t = proc.apvts.getParameter(prefix + "type"))
        t->setValueNotifyingHost(t->convertTo0to1((float) type));
    if (auto* on = proc.apvts.getParameter(prefix + "on"))
        on->setValueNotifyingHost(1.f);
    const char* keys[] = { "amt", "tone", "mot", "mix", "shp" };
    const char* labels[] = { "AMT", "TONE", "MOT", "MIX", "SHP" };
    for (int n = 0; n < 5; ++n)
    {
        auto w = juce::ValueTree("w");
        w.setProperty("slot", slot, nullptr);
        w.setProperty("param", keys[n], nullptr);
        w.setProperty("label", juce::String(kt::kFx[type].name) + " " + labels[n], nullptr);
        w.setProperty("x", 4 + ((slot * 5 + n) % 8) * 12, nullptr);
        w.setProperty("y", 8 + ((slot * 5 + n) / 8) * 28, nullptr);
        proc.uiState.appendChild(w, nullptr);
    }
    rebuildKnobs();
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
    if (auto* widgets = obj->getProperty("widgets").getArray())
    {
        for (auto& item : *widgets)
        {
            auto* wsrc = item.getDynamicObject();
            if (wsrc == nullptr) continue;
            auto w = juce::ValueTree("w");
            w.setProperty("slot", (int) wsrc->getProperty("slot"), nullptr);
            w.setProperty("param", nativeKey(wsrc->getProperty("param").toString()), nullptr);
            w.setProperty("x", (int) wsrc->getProperty("x"), nullptr);
            w.setProperty("y", (int) wsrc->getProperty("y"), nullptr);
            w.setProperty("label", wsrc->getProperty("param").toString(), nullptr);
            proc.uiState.appendChild(w, nullptr);
        }
    }
    gridBox.setSelectedId((int) proc.uiState.getProperty("grid", 0) + 1, juce::dontSendNotification);
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
    juce::Array<juce::var> widgets, slots;
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
        w->setProperty("kind", "dial");
        widgets.add(juce::var(w));
    }
    obj->setProperty("slots", slots);
    obj->setProperty("widgets", widgets);
    const auto name = obj->getProperty("name").toString();
    moduleDir().getChildFile(name + ".json").replaceWithText(juce::JSON::toString(juce::var(obj)));
    localBox.clear(juce::dontSendNotification);
    for (auto f : moduleDir().findChildFiles(juce::File::findFiles, false, "*.json"))
        localBox.addItem(f.getFileNameWithoutExtension(), localBox.getNumItems() + 1);
    status.setText("Saved " + name + " on this machine", juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::publish()
{
    saveLocal();
    if (token.isEmpty())
    {
        status.setText("Saved here. Log in to upload the catalog entry.", juce::dontSendNotification);
        return;
    }
    const auto name = nameBox.getText().trim();
    const auto text = moduleDir().getChildFile((name.isEmpty() ? "untitled" : name) + ".json").loadFileAsString();
    auto body = juce::JSON::parse(text);
    juce::var payload(new juce::DynamicObject());
    payload.getDynamicObject()->setProperty("module", body);
    payload.getDynamicObject()->setProperty("name", name);
    payload.getDynamicObject()->setProperty("face", proc.isFx() ? "fx" : "kyoto");
    auto safe = juce::Component::SafePointer<KyotoAudioProcessorEditor>(this);
    std::thread([safe, payload, tok = token]
    {
        auto result = kt::postAction("module_publish", payload, tok);
        juce::MessageManager::callAsync([safe, result]
        {
            if (safe != nullptr)
                safe->status.setText(result.ok ? "Uploaded to the shared catalog" : ("Saved here. " + (result.error.isEmpty() ? "Catalog upload is not on the worker yet" : result.error)),
                                      juce::dontSendNotification);
        });
    }).detach();
}

void KyotoAudioProcessorEditor::login()
{
    auto body = juce::var(new juce::DynamicObject());
    body.getDynamicObject()->setProperty("user", userBox.getText().trim());
    body.getDynamicObject()->setProperty("pass", passBox.getText());
    auto safe = juce::Component::SafePointer<KyotoAudioProcessorEditor>(this);
    std::thread([safe, body]
    {
        auto result = kt::postAction("login", body, {});
        juce::MessageManager::callAsync([safe, result]
        {
            if (safe == nullptr) return;
            if (! result.ok)
            {
                safe->status.setText(result.error, juce::dontSendNotification);
                return;
            }
            safe->token = result.token;
            safe->account = result.user;
            safe->passBox.clear();
            auto* o = new juce::DynamicObject();
            o->setProperty("token", result.token);
            o->setProperty("user", result.user);
            safe->sessionFile().getParentDirectory().createDirectory();
            safe->sessionFile().replaceWithText(juce::JSON::toString(juce::var(o)));
            safe->status.setText("Signed in as " + result.user, juce::dontSendNotification);
            safe->refreshFeed();
        });
    }).detach();
}

void KyotoAudioProcessorEditor::sendChat()
{
    if (token.isEmpty() || msgBox.getText().trim().isEmpty()) return;
    auto body = juce::var(new juce::DynamicObject());
    body.getDynamicObject()->setProperty("text", msgBox.getText().trim());
    auto safe = juce::Component::SafePointer<KyotoAudioProcessorEditor>(this);
    const auto tok = token;
    std::thread([safe, body, tok]
    {
        auto result = kt::postAction("chat_send", body, tok);
        juce::MessageManager::callAsync([safe, result]
        {
            if (safe == nullptr) return;
            if (result.ok) { safe->msgBox.clear(); safe->refreshFeed(); }
            else safe->status.setText(result.error, juce::dontSendNotification);
        });
    }).detach();
}

void KyotoAudioProcessorEditor::refreshFeed()
{
    auto safe = juce::Component::SafePointer<KyotoAudioProcessorEditor>(this);
    std::thread([safe]
    {
        auto feed = kt::getFeed();
        juce::MessageManager::callAsync([safe, feed]
        {
            if (safe == nullptr) return;
            juce::String text;
            if (auto* o = feed.getDynamicObject())
            {
                auto chat = o->getProperty("chat");
                if (auto* arr = chat.getArray())
                    for (int i = juce::jmax(0, arr->size() - 20); i < arr->size(); ++i)
                        if (auto* line = arr->getReference(i).getDynamicObject())
                            text << line->getProperty("user").toString() << ": " << line->getProperty("text").toString() << "\n";
                auto threads = o->getProperty("threads");
                text << "\nTHREADS\n";
                if (auto* arr = threads.getArray())
                    for (int i = 0; i < juce::jmin(8, arr->size()); ++i)
                        if (auto* th = arr->getReference(i).getDynamicObject())
                            text << th->getProperty("user").toString() << " — " << th->getProperty("title").toString() << "\n";
            }
            safe->logBox.setText(text.isEmpty() ? "DreamShare did not answer." : text);
        });
    }).detach();
}

void KyotoAudioProcessorEditor::refreshCatalog()
{
    auto safe = juce::Component::SafePointer<KyotoAudioProcessorEditor>(this);
    auto body = juce::var(new juce::DynamicObject());
    body.getDynamicObject()->setProperty("face", proc.isFx() ? "fx" : "kyoto");
    const auto tok = token;
    std::thread([safe, body, tok]
    {
        auto result = kt::postAction("module_list", body, tok);
        juce::MessageManager::callAsync([safe, result]
        {
            if (safe == nullptr) return;
            safe->remoteBox.clear(juce::dontSendNotification);
            auto parsed = juce::JSON::parse(result.raw);
            if (auto* o = parsed.getDynamicObject())
                if (auto* arr = o->getProperty("modules").getArray())
                    for (auto& item : *arr)
                        if (auto* m = item.getDynamicObject())
                            safe->remoteBox.addItem(m->getProperty("name").toString() + " · " + m->getProperty("author").toString(), safe->remoteBox.getNumItems() + 1);
            if (safe->remoteBox.getNumItems() == 0)
                safe->status.setText(result.error.isEmpty() ? "No shared modules for this face yet" : result.error, juce::dontSendNotification);
        });
    }).detach();
}

void KyotoAudioProcessorEditor::timerCallback()
{
    if (tab == 0 && isShowing())
        refreshFeed();
}
