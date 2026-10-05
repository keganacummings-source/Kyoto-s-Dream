#pragma once
#include <juce_core/juce_core.h>
#include <vector>

namespace kyoto {

enum class BuilderElementType { Dial=0, Slider, WaveScreen, Text };
enum class WaveScreenType { Oscilloscope=0, Spectrum, Envelope, Vectorscope, XYPad };
enum class GridStyle { Columns=0, Rows, Cards, Compact, Wide, Pedal, Synth, Split, Modular, Freeform };

struct UiElement {
    BuilderElementType type=BuilderElementType::Dial;
    juce::String id;
    juce::String label;
    float x=0.1f, y=0.1f;
    WaveScreenType screen=WaveScreenType::Oscilloscope;
    juce::String text;
};

struct BuilderLayout {
    bool custom=true;
    GridStyle grid=GridStyle::Cards;
    std::vector<UiElement> elements;
};

inline const char* gridName(GridStyle g){
    static const char* n[] = {"Columns","Rows","Cards","Compact","Wide","Pedal","Synth","Split","Modular","Freeform"};
    return n[static_cast<int>(g)];
}
inline const char* screenName(WaveScreenType s){
    static const char* n[] = {"Oscilloscope","Spectrum","Envelope","Vectorscope","XY Pad"};
    return n[static_cast<int>(s)];
}

inline juce::var layoutToVar(const BuilderLayout& l){
    auto* o=new juce::DynamicObject(); o->setProperty("custom",l.custom); o->setProperty("grid",(int)l.grid);
    juce::Array<juce::var> a;
    for(const auto& e:l.elements){
        auto* x=new juce::DynamicObject(); x->setProperty("type",(int)e.type); x->setProperty("id",e.id); x->setProperty("label",e.label); x->setProperty("x",e.x); x->setProperty("y",e.y); x->setProperty("screen",(int)e.screen); x->setProperty("text",e.text); a.add(juce::var(x));
    }
    o->setProperty("elements",juce::var(a)); return juce::var(o);
}
inline BuilderLayout layoutFromVar(const juce::var& v){
    BuilderLayout l; auto* o=v.getDynamicObject(); if(!o) return l; l.custom=(bool)o->getProperty("custom"); l.grid=(GridStyle)juce::jlimit(0,9,(int)o->getProperty("grid"));
    if(auto* a=o->getProperty("elements").getArray()) for(auto& item:*a){ if(auto* x=item.getDynamicObject()){ UiElement e; e.type=(BuilderElementType)juce::jlimit(0,3,(int)x->getProperty("type")); e.id=x->getProperty("id").toString(); e.label=x->getProperty("label").toString(); e.x=(float)x->getProperty("x"); e.y=(float)x->getProperty("y"); e.screen=(WaveScreenType)juce::jlimit(0,4,(int)x->getProperty("screen")); e.text=x->getProperty("text").toString(); l.elements.push_back(e); }}
    return l;
}

inline BuilderLayout defaultLayout(){
    BuilderLayout l; l.custom=true; l.grid=GridStyle::Cards;
    l.elements={{BuilderElementType::Dial,"tone","Tone",0.08f,0.12f},{BuilderElementType::Dial,"punch","Punch",0.30f,0.12f},{BuilderElementType::Slider,"space","Space",0.52f,0.12f},{BuilderElementType::Slider,"motion","Motion",0.66f,0.12f},{BuilderElementType::WaveScreen,"scope","Wave Shape",0.08f,0.40f,WaveScreenType::Oscilloscope},{BuilderElementType::Text,"title","Kyoto's Dream",0.55f,0.78f,WaveScreenType::Oscilloscope,"Built in Kyoto's Dream"}};
    return l;
}
}
