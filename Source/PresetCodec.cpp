#include "PresetCodec.h"
namespace kyoto {
juce::var presetToVar(const InstrumentPreset&p){
    auto*o=new juce::DynamicObject();
    o->setProperty("id",p.id);o->setProperty("name",p.name);o->setProperty("osc1",p.osc1);o->setProperty("osc2",p.osc2);o->setProperty("osc3",p.osc3);
    o->setProperty("mix1",p.mix1);o->setProperty("mix2",p.mix2);o->setProperty("mix3",p.mix3);o->setProperty("detune2",p.detune2);o->setProperty("detune3",p.detune3);
    o->setProperty("attack",p.attack);o->setProperty("decay",p.decay);o->setProperty("sustain",p.sustain);o->setProperty("release",p.release);o->setProperty("cutoff",p.cutoff);o->setProperty("resonance",p.resonance);
    o->setProperty("drive",p.drive);o->setProperty("noise",p.noise);o->setProperty("lfoRate",p.lfoRate);o->setProperty("lfoDepth",p.lfoDepth);o->setProperty("octave",p.octave);o->setProperty("arp",p.arp);o->setProperty("arpRate",p.arpRate);
    juce::Array<juce::var> fx,amt;for(int i=0;i<8;i++){fx.add(p.fx[i]);amt.add(p.fxAmount[i]);}o->setProperty("fx",juce::var(fx));o->setProperty("fxAmount",juce::var(amt));
    if(!p.uiLayout.isVoid())o->setProperty("uiLayout",p.uiLayout); if(!p.uiTheme.isVoid())o->setProperty("uiTheme",p.uiTheme);
    juce::Array<juce::var> chain; for(const auto& f:p.expertFxChain){ auto* x=new juce::DynamicObject(); x->setProperty("effect",f.effect); x->setProperty("amount",f.amount); x->setProperty("tone",f.tone); x->setProperty("motion",f.motion); x->setProperty("mix",f.mix); x->setProperty("shape",f.shape); chain.add(juce::var(x)); } o->setProperty("expertFxChain",juce::var(chain));
    return juce::var(o);
}
bool presetFromVar(const juce::var&v,InstrumentPreset&p){
    auto*o=v.getDynamicObject();if(!o)return false;auto g=[&](const char*k,float d){return (float)o->getProperty(k).orFallback(d);};
    p.id=o->getProperty("id").toString();p.name=o->getProperty("name").toString();p.osc1=(int)o->getProperty("osc1").orFallback(p.osc1);p.osc2=(int)o->getProperty("osc2").orFallback(p.osc2);p.osc3=(int)o->getProperty("osc3").orFallback(p.osc3);
    p.mix1=g("mix1",p.mix1);p.mix2=g("mix2",p.mix2);p.mix3=g("mix3",p.mix3);p.detune2=g("detune2",p.detune2);p.detune3=g("detune3",p.detune3);p.attack=g("attack",p.attack);p.decay=g("decay",p.decay);p.sustain=g("sustain",p.sustain);p.release=g("release",p.release);p.cutoff=g("cutoff",p.cutoff);p.resonance=g("resonance",p.resonance);p.drive=g("drive",p.drive);p.noise=g("noise",p.noise);p.lfoRate=g("lfoRate",p.lfoRate);p.lfoDepth=g("lfoDepth",p.lfoDepth);p.octave=(int)o->getProperty("octave").orFallback(p.octave);p.arp=(bool)o->getProperty("arp").orFallback(p.arp);p.arpRate=g("arpRate",p.arpRate);
    if(auto*a=o->getProperty("fx").getArray())for(int i=0;i<8&&i<a->size();i++)p.fx[i]=(int)a->getUnchecked(i);if(auto*a=o->getProperty("fxAmount").getArray())for(int i=0;i<8&&i<a->size();i++)p.fxAmount[i]=(float)a->getUnchecked(i);
    if(o->hasProperty("uiLayout"))p.uiLayout=o->getProperty("uiLayout"); if(o->hasProperty("uiTheme"))p.uiTheme=o->getProperty("uiTheme");
    p.expertFxChain.clear(); if(auto* a=o->getProperty("expertFxChain").getArray()) for(auto& item:*a) if(auto* x=item.getDynamicObject()){ FxSlot f; f.effect=(int)x->getProperty("effect").orFallback(0); f.amount=g("amount",f.amount); f.tone=(float)x->getProperty("tone").orFallback(f.tone); f.motion=(float)x->getProperty("motion").orFallback(f.motion); f.mix=(float)x->getProperty("mix").orFallback(f.mix); f.shape=(float)x->getProperty("shape").orFallback(f.shape); p.expertFxChain.push_back(f); } return true;
}
}
