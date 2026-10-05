#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PresetCodec.h"
#include "BinaryData.h"
#include <algorithm>

static juce::StringArray splitModuleString(const juce::String& s){ return juce::StringArray::fromTokens(s,"|",""); }

KyotoSpxritProcessor::KyotoSpxritProcessor()
: AudioProcessor(BusesProperties()
    .withInput("Input", juce::AudioChannelSet::stereo(), false)
    .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
  state(*this,nullptr,"PARAMETERS",createParams()), activeTheme(themeManager.get("trippah"))
{
    if(!themeManager.loadFromJson(juce::String::fromUTF8(
        reinterpret_cast<const char*>(BinaryData::themes_json),
        (int) BinaryData::themes_jsonSize
    ))){
        auto resources=juce::File::getCurrentWorkingDirectory().getChildFile("Resources").getChildFile("themes").getChildFile("themes.json");
        if(resources.existsAsFile()) themeManager.loadFromFile(resources);
    }
    activeTheme=themeManager.get("trippah");
    for(int i=0;i<kyoto::kNumEffects;i++){auto s=juce::String(i);enabled[i]=state.getRawParameterValue("fx"+s);amount[i]=state.getRawParameterValue("amt"+s);tone[i]=state.getRawParameterValue("tone"+s);motion[i]=state.getRawParameterValue("motion"+s);mix[i]=state.getRawParameterValue("mix"+s);shape[i]=state.getRawParameterValue("shape"+s);}
    for(int i=0;i<fxSlots;i++)fxSelect[i]=state.getRawParameterValue("slot"+juce::String(i));
    prepared = false;
}
const juce::String KyotoSpxritProcessor::getName() const {
#if defined(KYOTO_IS_FX)
    return "KyotoSpxrit FX";
#else
    return "KyotoSpxrit";
#endif
}
juce::AudioProcessorValueTreeState::ParameterLayout KyotoSpxritProcessor::createParams(){
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    // FL Studio aborts the host if two parameters share a name. Every
    // host-visible name must be unique, even though the IDs already were.
    for(int i=0;i<kyoto::kNumEffects;i++){
        auto s=juce::String(i);
        auto tag=juce::String(i+1);
        p.push_back(std::make_unique<juce::AudioParameterBool>("fx"+s,"FX "+tag+" On",false));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("amt"+s,"FX "+tag+" Amount",0,1,0.5f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("tone"+s,"FX "+tag+" Tone",0,1,0.5f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("motion"+s,"FX "+tag+" Motion",0,1,0.5f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("mix"+s,"FX "+tag+" Mix",0,1,0.65f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("shape"+s,"FX "+tag+" Shape",0,1,0.5f));
    }
    for(int i=0;i<fxSlots;i++)p.push_back(std::make_unique<juce::AudioParameterInt>("slot"+juce::String(i),"FX Slot "+juce::String(i+1),0,199,i));
    p.push_back(std::make_unique<juce::AudioParameterInt>("osc1","Oscillator 1",0,5,0));p.push_back(std::make_unique<juce::AudioParameterInt>("osc2","Oscillator 2",0,5,3));p.push_back(std::make_unique<juce::AudioParameterInt>("osc3","Oscillator 3",0,5,1));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("oscMix1","Osc Mix 1",0,1,.8f));p.push_back(std::make_unique<juce::AudioParameterFloat>("oscMix2","Osc Mix 2",0,1,.35f));p.push_back(std::make_unique<juce::AudioParameterFloat>("oscMix3","Osc Mix 3",0,1,.2f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("detune2","Detune 2",-50,50,7));p.push_back(std::make_unique<juce::AudioParameterFloat>("detune3","Detune 3",-50,50,-7));p.push_back(std::make_unique<juce::AudioParameterInt>("octave","Octave",-4,4,0));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("cutoff","Cutoff",0.01f,0.99f,0.72f));p.push_back(std::make_unique<juce::AudioParameterFloat>("resonance","Resonance",0,1,0.15f));p.push_back(std::make_unique<juce::AudioParameterFloat>("attack","Attack",0.001f,2.0f,0.01f));p.push_back(std::make_unique<juce::AudioParameterFloat>("decay","Decay",0.001f,2.0f,0.18f));p.push_back(std::make_unique<juce::AudioParameterFloat>("sustain","Sustain",0,1,0.72f));p.push_back(std::make_unique<juce::AudioParameterFloat>("release","Release",0.001f,4.0f,0.25f));p.push_back(std::make_unique<juce::AudioParameterFloat>("noise","Noise",0,1,0));p.push_back(std::make_unique<juce::AudioParameterFloat>("drive","Drive",0,1,0));p.push_back(std::make_unique<juce::AudioParameterFloat>("lfoRate","LFO Rate",0.05f,20,4));p.push_back(std::make_unique<juce::AudioParameterFloat>("lfoDepth","LFO Depth",0,1,0));p.push_back(std::make_unique<juce::AudioParameterBool>("arp","Arpeggiator",false));p.push_back(std::make_unique<juce::AudioParameterFloat>("arpRate","Arp Rate",1,32,8));
    return {p.begin(),p.end()};
}
void KyotoSpxritProcessor::prepareToPlay(double sr, int bs)
{
    const double rate = sr > 0.0 ? sr : 44100.0;
    const int block = bs > 0 ? bs : 512;
    synth.prepare(rate, block);
    fx.prepare(rate);
    publishExpertSnapshot();
    prepared = true;
}

void KyotoSpxritProcessor::releaseResources()
{
    synth.reset();
    fx.reset();
    prepared = false;
}

bool KyotoSpxritProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    // FL probes disabled, mono and stereo inputs while scanning. Rejecting
    // the layout it asks for aborts the scan and takes the host down.
    const auto out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    const auto in = l.getMainInputChannelSet();
    return in.isDisabled() || in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void KyotoSpxritProcessor::processBlock(juce::AudioBuffer<float>& b, juce::MidiBuffer& m)
{
    juce::ScopedNoDenormals noDenormals;
    if (b.getNumSamples() <= 0 || b.getNumChannels() <= 0)
        return;
    if (!prepared)
    {
        prepareToPlay(getSampleRate() > 0.0 ? getSampleRate() : 44100.0, b.getNumSamples());
    }

#if !defined(KYOTO_IS_FX)
    b.clear();
    synth.render(b, m);
#endif

    auto* left = b.getWritePointer(0);
    auto* right = b.getNumChannels() > 1 ? b.getWritePointer(1) : nullptr;
    if (left == nullptr)
        return;

    if (expertMode.load(std::memory_order_acquire))
    {
        // The editor/state thread publishes immutable chain snapshots. The
        // audio thread only reads the currently published snapshot, so vector
        // mutation can never race the DSP engine.
        int snapshot = activeExpertSnapshot.load(std::memory_order_acquire);
        expertSnapshotReaders[(size_t) snapshot].fetch_add(1, std::memory_order_acquire);
        if (snapshot != activeExpertSnapshot.load(std::memory_order_acquire))
        {
            expertSnapshotReaders[(size_t) snapshot].fetch_sub(1, std::memory_order_release);
            snapshot = activeExpertSnapshot.load(std::memory_order_acquire);
            expertSnapshotReaders[(size_t) snapshot].fetch_add(1, std::memory_order_acquire);
        }

        const auto& snap = expertSnapshots[(size_t) snapshot];
        if (snap.count > 0)
            fx.process(left, right, b.getNumSamples(), snap.slots.data(), snap.count);

        expertSnapshotReaders[(size_t) snapshot].fetch_sub(1, std::memory_order_release);
        return;
    }

    // Normal mode is deliberately a fixed eight-slot chain. Each slot has its
    // own DSP state, so selecting the same effect twice intentionally stacks it.
    std::array<kyoto::FxSlotParams, fxSlots> slots{};
    for (int slot = 0; slot < fxSlots; ++slot)
    {
        const int idx = fxIndex(slot);
        auto loadOr = [](std::atomic<float>* p, float fallback) {
            return p != nullptr ? p->load(std::memory_order_relaxed) : fallback;
        };
        slots[(size_t) slot].effect = idx;
        slots[(size_t) slot].on = true;
        slots[(size_t) slot].amount = loadOr(amount[(size_t) idx], 0.5f);
        slots[(size_t) slot].tone = loadOr(tone[(size_t) idx], 0.5f);
        slots[(size_t) slot].motion = loadOr(motion[(size_t) idx], 0.5f);
        slots[(size_t) slot].mix = loadOr(mix[(size_t) idx], 0.65f);
        slots[(size_t) slot].shape = loadOr(shape[(size_t) idx], 0.5f);
    }
    fx.process(left, right, b.getNumSamples(), slots.data(), fxSlots);
}

void KyotoSpxritProcessor::publishExpertSnapshot()
{
    int active = activeExpertSnapshot.load(std::memory_order_acquire);
    int target = -1;

    for (int i = 0; i < expertSnapshotBuffers; ++i)
    {
        if (i == active)
            continue;
        if (expertSnapshotReaders[(size_t) i].load(std::memory_order_acquire) == 0)
        {
            target = i;
            break;
        }
    }

    if (target < 0)
        return;

    auto& snap = expertSnapshots[(size_t) target];
    snap.count = std::min<int>((int) expertFxChain.size(), kyoto::kMaxFxSlots);
    for (int i = 0; i < snap.count; ++i)
    {
        const auto& f = expertFxChain[(size_t) i];
        auto& d = snap.slots[(size_t) i];
        d.effect = juce::jlimit(0, kyoto::kNumEffects - 1, f.effect);
        d.on = true;
        d.amount = juce::jlimit(0.0f, 1.0f, f.amount);
        d.tone = juce::jlimit(0.0f, 1.0f, f.tone);
        d.motion = juce::jlimit(0.0f, 1.0f, f.motion);
        d.mix = juce::jlimit(0.0f, 1.0f, f.mix);
        d.shape = juce::jlimit(0.0f, 1.0f, f.shape);
    }
    activeExpertSnapshot.store(target, std::memory_order_release);
}

void KyotoSpxritProcessor::getStateInformation(juce::MemoryBlock&dest){
    auto root=state.copyState();
    root.setProperty("modules",modules.joinIntoString("|"),nullptr);
    root.setProperty(
        "builderLayout",
        juce::JSON::toString(
            kyoto::layoutToVar(builderLayout)),
        nullptr);root.setProperty("activeModule",currentModule,nullptr);root.setProperty("dreamUser",dreamUser,nullptr);root.setProperty("dreamRole",dreamRole,nullptr);root.setProperty("dreamTheme",dreamTheme,nullptr);root.setProperty("uiVariant",(int)uiMode,nullptr);root.setProperty("expertMode", isExpertMode(), nullptr);
    juce::Array<juce::var> ms; for(auto&kv:modulePresets) { auto v=kyoto::presetToVar(kv.second); if(auto*o=v.getDynamicObject()) o->setProperty("id",juce::String(kv.first)); ms.add(v); } root.setProperty("moduleStates",juce::var(juce::JSON::toString(juce::var(ms))),nullptr);
    juce::Array<juce::var> ct;for(auto&t:userThemes)ct.add(kyoto::ThemeManager::toVar(t));root.setProperty("customThemes",juce::var(juce::JSON::toString(juce::var(ct))),nullptr);
    auto chain=kyoto::InstrumentPreset{}; chain.expertFxChain=expertFxChain; root.setProperty("expertFxChain",juce::var(juce::JSON::toString(kyoto::presetToVar(chain))),nullptr);
    if(auto xml=root.createXml())copyXmlToBinary(*xml,dest);
}
void KyotoSpxritProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Hosts may call this with empty, foreign, legacy, or malformed state.
    // Never pass an arbitrary XML root into APVTS::replaceState().
    if (data == nullptr || sizeInBytes <= 0)
        return;

    constexpr int maxStateBytes = 16 * 1024 * 1024;
    if (sizeInBytes > maxStateBytes)
        return;

    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml == nullptr || !xml->hasTagName(state.state.getType()))
        return;

    auto root = juce::ValueTree::fromXml(*xml);
    if (!root.isValid() || root.getType() != state.state.getType())
        return;

    // 0.2.2 and earlier accidentally reused the FX parameter IDs mix1..mix3
    // for the three oscillator mix controls. New builds use oscMix1..oscMix3.
    // Migrate the final occurrence of each duplicated legacy ID before JUCE
    // restores APVTS state, preserving old sessions without reintroducing the
    // duplicate-ID crash condition.
    for (int n = 1; n <= 3; ++n)
    {
        const auto legacy = juce::String("mix") + juce::String(n);
        const auto replacement = juce::String("oscMix") + juce::String(n);
        bool hasReplacement = false;
        int legacyCount = 0;
        int lastLegacy = -1;
        for (int i = 0; i < root.getNumChildren(); ++i)
        {
            const auto child = root.getChild(i);
            const auto id = child.getProperty("id").toString();
            if (id == replacement) hasReplacement = true;
            if (id == legacy) { ++legacyCount; lastLegacy = i; }
        }
        if (!hasReplacement && legacyCount > 1 && lastLegacy >= 0)
            root.getChild(lastLegacy).setProperty("id", replacement, nullptr);
    }

    state.replaceState(root);

    modules = splitModuleString(root.getProperty("modules").toString());
    currentModule = root.getProperty("activeModule").toString();
    dreamUser = root.getProperty("dreamUser").toString();
    dreamRole = root.getProperty("dreamRole").toString();
    dreamTheme = root.getProperty("dreamTheme").toString();
    uiMode = (kyoto::UiVariant) juce::jlimit(
        0, 3, (int) root.getProperty("uiVariant", 0));
    expertMode.store((bool) root.getProperty("expertMode", false), std::memory_order_release);

    builderLayout = kyoto::defaultLayout();
    const auto layoutJson = root.getProperty("builderLayout").toString();
    if (layoutJson.isNotEmpty())
    {
        auto lv = juce::JSON::parse(layoutJson);
        if (!lv.isVoid() && lv.getDynamicObject() != nullptr)
            builderLayout = kyoto::layoutFromVar(lv);
    }

    expertFxChain.clear();
    const auto chainJson = root.getProperty("expertFxChain").toString();
    if (chainJson.isNotEmpty())
    {
        auto cv = juce::JSON::parse(chainJson);
        if (!cv.isVoid())
        {
            kyoto::InstrumentPreset cp;
            if (kyoto::presetFromVar(cv, cp))
                expertFxChain = cp.expertFxChain;
        }
    }

    modulePresets.clear();
    const auto moduleJson = root.getProperty("moduleStates").toString();
    if (moduleJson.isNotEmpty())
    {
        auto mv = juce::JSON::parse(moduleJson);
        if (auto* a = mv.getArray())
        {
            for (auto& v : *a)
            {
                if (auto* o = v.getDynamicObject())
                {
                    kyoto::InstrumentPreset p;
                    const auto id = o->getProperty("id").toString();
                    if (id.isNotEmpty() && kyoto::presetFromVar(v, p))
                        modulePresets[id.toStdString()] = p;
                }
            }
        }
    }

    userThemes.clear();
    const auto themesJson = root.getProperty("customThemes").toString();
    if (themesJson.isNotEmpty())
    {
        auto tv = juce::JSON::parse(themesJson);
        if (auto* a = tv.getArray())
        {
            for (auto& v : *a)
                userThemes.push_back(
                    kyoto::ThemeManager::fromVar(
                        v, themeManager.get(dreamTheme)));
        }
    }

    publishExpertSnapshot();

    activeTheme = themeManager.get(dreamTheme);
    for (auto& t : userThemes)
    {
        if (t.id == dreamTheme)
        {
            activeTheme = t;
            break;
        }
    }

    synth.setPreset(getInstrumentPreset());
}

std::vector<juce::String> KyotoSpxritProcessor::installedModules()const{std::vector<juce::String> out;for(auto&m:modules)out.push_back(m);return out;}
void KyotoSpxritProcessor::addModule(const juce::String&id){ if(id.isEmpty())return; if(!modules.contains(id))modules.add(id); currentModule=id; auto it=modulePresets.find(id.toStdString()); if(it!=modulePresets.end()){setInstrumentPreset(it->second);return;} kyoto::InstrumentPreset p=getInstrumentPreset();p.id=id;p.name=id.toUpperCase();auto n=id.toLowerCase();if(n.contains("bass")||n.contains("sub")){p.osc1=1;p.osc2=2;p.osc3=0;p.octave=-1;p.cutoff=.34f;p.resonance=.12f;p.attack=.005f;p.decay=.22f;p.sustain=.65f;p.release=.18f;p.drive=.18f;}else if(n.contains("bell")||n.contains("marimba")){p.osc1=0;p.osc2=3;p.osc3=4;p.cutoff=.78f;p.resonance=.28f;p.attack=.003f;p.decay=.7f;p.sustain=.25f;p.release=1.1f;}else if(n.contains("pad")||n.contains("choir")||n.contains("organ")){p.osc1=0;p.osc2=4;p.osc3=0;p.cutoff=.58f;p.attack=.35f;p.decay=.8f;p.sustain=.72f;p.release=1.4f;p.detune2=11;p.detune3=-11;}else if(n.contains("pluck")||n.contains("keys")||n.contains("clav")){p.osc1=3;p.osc2=2;p.osc3=0;p.cutoff=.7f;p.attack=.002f;p.decay=.24f;p.sustain=.18f;p.release=.28f;}else if(n.contains("noise")){p.osc1=5;p.osc2=5;p.osc3=5;p.noise=.7f;p.cutoff=.9f;p.attack=.01f;p.decay=.25f;p.sustain=.45f;p.release=.35f;}else if(n.contains("drone")){p.osc1=0;p.osc2=0;p.osc3=1;p.cutoff=.42f;p.attack=.7f;p.decay=.5f;p.sustain=.9f;p.release=2.0f;}else if(n.contains("saw")||n.contains("lead")||n.contains("analog")){p.osc1=1;p.osc2=1;p.osc3=0;p.detune2=6;p.detune3=-6;p.cutoff=.66f;p.resonance=.22f;p.attack=.005f;p.decay=.16f;p.sustain=.7f;p.release=.22f;p.drive=.08f;}else {p.osc1=0;p.osc2=3;p.osc3=2;p.cutoff=.72f;p.attack=.01f;p.decay=.18f;p.sustain=.72f;p.release=.25f;}setInstrumentPreset(p);}
void KyotoSpxritProcessor::saveModule(const juce::String&name){auto p=getInstrumentPreset(); p.uiLayout=kyoto::layoutToVar(builderLayout); p.uiTheme=kyoto::ThemeManager::toVar(activeTheme);p.name=name.trim().isEmpty()?"KyotoSpxrit Instrument":name.trim();p.id=p.name.toLowerCase().replaceCharacters(" ","-");if(!modules.contains(p.id))modules.add(p.id);currentModule=p.id;modulePresets[p.id.toStdString()]=p;}
bool KyotoSpxritProcessor::loadModule(const juce::String&id){auto it=modulePresets.find(id.toStdString());if(it==modulePresets.end())return false;currentModule=id;setInstrumentPreset(it->second);return true;}
bool KyotoSpxritProcessor::hasModules()const{return modules.size()>0;} juce::String KyotoSpxritProcessor::activeModule()const{return currentModule;} void KyotoSpxritProcessor::setActiveModule(const juce::String&id){currentModule=id;loadModule(id);}
kyoto::InstrumentPreset KyotoSpxritProcessor::getInstrumentPreset() const
{
    auto raw = [this](const char* id, float fallback) {
        if (auto* v = state.getRawParameterValue(id))
            return v->load();
        return fallback;
    };
    kyoto::InstrumentPreset p;
    p.id = currentModule.isEmpty() ? "custom" : currentModule;
    p.name = currentModule.isEmpty() ? "KyotoSpxrit Init" : currentModule;
    p.osc1 = (int) raw("osc1", 0);
    p.osc2 = (int) raw("osc2", 3);
    p.osc3 = (int) raw("osc3", 1);
    p.mix1 = raw("oscMix1", 0.8f);
    p.mix2 = raw("oscMix2", 0.35f);
    p.mix3 = raw("oscMix3", 0.2f);
    p.detune2 = raw("detune2", 7);
    p.detune3 = raw("detune3", -7);
    p.octave = (int) raw("octave", 0);
    p.cutoff = raw("cutoff", 0.72f);
    p.resonance = raw("resonance", 0.15f);
    p.attack = raw("attack", 0.01f);
    p.decay = raw("decay", 0.18f);
    p.sustain = raw("sustain", 0.72f);
    p.release = raw("release", 0.25f);
    p.noise = raw("noise", 0);
    p.drive = raw("drive", 0);
    p.lfoRate = raw("lfoRate", 4);
    p.lfoDepth = raw("lfoDepth", 0);
    p.arp = raw("arp", 0) > 0.5f;
    p.arpRate = raw("arpRate", 8);
    for (int i = 0; i < fxSlots; ++i)
    {
        p.fx[i] = fxIndex(i);
        auto* amt = amount[(size_t) p.fx[i]];
        p.fxAmount[i] = amt != nullptr ? amt->load() : 0.5f;
    }
    p.expertFxChain = expertFxChain;
    p.uiLayout = kyoto::layoutToVar(builderLayout);
    p.uiTheme = kyoto::ThemeManager::toVar(activeTheme);
    return p;
}

void KyotoSpxritProcessor::setInstrumentPreset(const kyoto::InstrumentPreset& p)
{
    auto setFloat = [&](const char* name, float value)
    {
        if (auto* parameter = state.getParameter(name))
            parameter->setValueNotifyingHost(
                parameter->convertTo0to1(value));
    };

    auto setInt = [&](const char* name, int value)
    {
        if (auto* parameter = state.getParameter(name))
            parameter->setValueNotifyingHost(
                parameter->convertTo0to1((float) value));
    };

    setInt("osc1", p.osc1);
    setInt("osc2", p.osc2);
    setInt("osc3", p.osc3);

    setFloat("oscMix1", p.mix1);
    setFloat("oscMix2", p.mix2);
    setFloat("oscMix3", p.mix3);

    setFloat("detune2", p.detune2);
    setFloat("detune3", p.detune3);

    setInt("octave", p.octave);

    setFloat("cutoff", p.cutoff);
    setFloat("resonance", p.resonance);
    setFloat("attack", p.attack);
    setFloat("decay", p.decay);
    setFloat("sustain", p.sustain);
    setFloat("release", p.release);
    setFloat("noise", p.noise);
    setFloat("drive", p.drive);
    setFloat("lfoRate", p.lfoRate);
    setFloat("lfoDepth", p.lfoDepth);
    setFloat("arp", p.arp ? 1.0f : 0.0f);
    setFloat("arpRate", p.arpRate);

    for (int i = 0; i < fxSlots; ++i)
    {
        setFxIndex(i, p.fx[i]);
        setFxParam(i, 0, p.fxAmount[i]);
    }

    expertFxChain = p.expertFxChain;
    publishExpertSnapshot();

    if (!p.uiLayout.isVoid())
        builderLayout = kyoto::layoutFromVar(p.uiLayout);

    if (!p.uiTheme.isVoid())
    {
        auto ct =
            kyoto::ThemeManager::fromVar(
                p.uiTheme,
                activeTheme);

        ct.custom = true;
        activeTheme = ct;
        dreamTheme = ct.id;

        bool found = false;

        for (auto& t : userThemes)
        {
            if (t.id == ct.id)
            {
                found = true;
                break;
            }
        }

        if (!found)
            userThemes.push_back(ct);
    }

    if (expertFxChain.empty())
    {
        for (int i = 0; i < fxSlots; ++i)
        {
            kyoto::FxSlot f;
            f.effect = p.fx[i];
            f.amount = p.fxAmount[i];
            expertFxChain.push_back(f);
        }
        publishExpertSnapshot();
    }

    synth.setPreset(p);
}
void KyotoSpxritProcessor::savePreset(const juce::String&name){saveModule(name);}
int KyotoSpxritProcessor::fxIndex(int slot)const{auto* p=fxSelect[(size_t)juce::jlimit(0,fxSlots-1,slot)]; if(p==nullptr) return juce::jlimit(0,199,slot); return juce::jlimit(0,199,(int)std::round(p->load()));}
void KyotoSpxritProcessor::setFxIndex(int slot, int idx)
{
    if (slot < 0 || slot >= fxSlots)
        return;

    const auto name =
        "slot" + juce::String(slot);

    if (auto* parameter = state.getParameter(name))
    {
        const auto value =
            (float) juce::jlimit(0, 199, idx);

        parameter->setValueNotifyingHost(
            parameter->convertTo0to1(value));
    }
}
float KyotoSpxritProcessor::fxParam(int slot,int which)const{int idx=fxIndex(slot);switch(which){case 0:return amount[idx]->load();case 1:return tone[idx]->load();case 2:return motion[idx]->load();case 3:return mix[idx]->load();default:return shape[idx]->load();}}
void KyotoSpxritProcessor::setFxParam(
    int slot,
    int which,
    float value)
{
    const int idx = fxIndex(slot);
    value = juce::jlimit(0.0f, 1.0f, value);

    const char* prefix = "shape";

    switch (which)
    {
        case 0: prefix = "amt";   break;
        case 1: prefix = "tone";  break;
        case 2: prefix = "motion";break;
        case 3: prefix = "mix";   break;
        default: break;
    }

    const auto name =
        juce::String(prefix) + juce::String(idx);

    if (auto* parameter = state.getParameter(name))
        parameter->setValueNotifyingHost(
            parameter->convertTo0to1(value));
}
void KyotoSpxritProcessor::setSession(const kyoto::DreamSession&s){if(s.ok){dreamToken=s.token;dreamUser=s.user;dreamRole=s.role;dreamTheme=s.theme;setTheme(dreamTheme);}}
void KyotoSpxritProcessor::logout(){dreamToken.clear();dreamUser.clear();dreamRole.clear();}
void KyotoSpxritProcessor::setTheme(const juce::String&id){dreamTheme=id;activeTheme=themeManager.get(id);for(auto&t:userThemes)if(t.id==id)activeTheme=t;}
void KyotoSpxritProcessor::setUiVariant(kyoto::UiVariant v){uiMode=v;}
void KyotoSpxritProcessor::addCustomTheme(const kyoto::ThemePalette&t){userThemes.push_back(t);dreamTheme=t.id;activeTheme=t;}

std::vector<kyoto::FxSlot> KyotoSpxritProcessor::fxChain() const { return expertFxChain; }
void KyotoSpxritProcessor::setFxChain(const std::vector<kyoto::FxSlot>& c){ expertFxChain=c; publishExpertSnapshot(); }
void KyotoSpxritProcessor::addFxSlot(int effect){ kyoto::FxSlot f; f.effect=juce::jlimit(0,199,effect); expertFxChain.push_back(f); publishExpertSnapshot(); }
void KyotoSpxritProcessor::removeFxSlot(size_t index){ if(index<expertFxChain.size()) { expertFxChain.erase(expertFxChain.begin()+static_cast<std::ptrdiff_t>(index)); publishExpertSnapshot(); } }
void KyotoSpxritProcessor::setFxChainSlot(size_t index,const kyoto::FxSlot& f){ if(index>=expertFxChain.size()) return; expertFxChain[index]=f; publishExpertSnapshot(); }


juce::AudioProcessorEditor* KyotoSpxritProcessor::createEditor()
{
    return new KyotoSpxritEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KyotoSpxritProcessor();
}
