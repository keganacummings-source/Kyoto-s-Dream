#include "InstrumentEngine.h"
#include <cmath>
#include <algorithm>
namespace kyoto {
static constexpr float pi=3.14159265358979323846f;
InstrumentEngine::InstrumentEngine()=default;
void InstrumentEngine::prepare(double rate,int block){ sr=std::clamp(rate,8000.0,192000.0); maxBlock=block; reset(); }
void InstrumentEngine::reset(){ for(auto&v:voices)v=Voice{}; }
void InstrumentEngine::setPreset(const InstrumentPreset&p){current=p;}
float InstrumentEngine::midiHz(int n){return 440.0f*std::pow(2.0f,(n-69)/12.0f);}
float InstrumentEngine::osc(float p,int type){
    p-=std::floor(p);
    switch(type%6){
        case 0:return std::sin(2*pi*p);
        case 1:return 2.0f*p-1.0f;
        case 2:return p<0.5f?1.0f:-1.0f;
        case 3:return 1.0f-4.0f*std::abs(p-0.5f);
        case 4:return std::sin(2*pi*p)+0.25f*std::sin(6*pi*p);
        default:return std::sin(2*pi*p*37.0f)*0.35f;
    }
}
float InstrumentEngine::nextEnvelope(Voice&v){
    const float a=std::max(0.0005f,current.attack),d=std::max(0.001f,current.decay),r=std::max(0.002f,current.release);
    const float dt=1.0f/(float)sr;
    if(!v.releasing){
        if(v.stage==0){v.env+=dt/a;if(v.env>=1){v.env=1;v.stage=1;}}
        else if(v.stage==1){v.env-=dt*(1.0f-current.sustain)/d;if(v.env<=current.sustain){v.env=current.sustain;v.stage=2;}}
    } else { v.env-=dt*std::max(0.001f,v.releaseStart)/r; if(v.env<=0){v.env=0;v.active=false;} }
    return v.env;
}
void InstrumentEngine::noteOn(int midi,float velocity){
    Voice*slot=nullptr;
    for(auto&v:voices)if(!v.active){slot=&v;break;}
    if(!slot){slot=&voices[0];for(auto&v:voices)if(v.age>slot->age)slot=&v;}
    *slot=Voice{};slot->active=true;slot->note=midi;slot->velocity=std::clamp(velocity,0.0f,1.0f);
}
void InstrumentEngine::noteOff(int midi){for(auto&v:voices)if(v.active&&v.note==midi&&!v.releasing){v.releasing=true;v.releaseStart=std::max(0.001f,v.env);}}
void InstrumentEngine::allNotesOff(){for(auto&v:voices)if(v.active&&!v.releasing){v.releasing=true;v.releaseStart=std::max(0.001f,v.env);}}
void InstrumentEngine::renderSample(float&l,float&r){
    l=r=0.0f;
    for(auto&v:voices){if(!v.active)continue;float e=nextEnvelope(v);if(!v.active)continue;
        const float base=midiHz(v.note+current.octave*12);float s=0;
        const int types[3]={current.osc1,current.osc2,current.osc3};const float mixes[3]={current.mix1,current.mix2,current.mix3};const float cents[3]={0,current.detune2,current.detune3};
        for(int k=0;k<3;k++){float f=base*std::pow(2.0f,cents[k]/1200.0f);v.phase[k]+=f/sr; if(v.phase[k]>=1)v.phase[k]-=std::floor(v.phase[k]);s+=osc((float)v.phase[k],types[k])*mixes[k];}
        s/=1.8f;s+=current.noise*(random.nextFloat()*2.0f-1.0f);s*=e*v.velocity;
        float cutoff=std::clamp(current.cutoff,0.01f,0.99f); static thread_local float fl=0,fr=0; float a=std::clamp(0.01f+cutoff*0.3f,0.005f,0.35f); fl+=(s-fl)*a; fr+=(s-fr)*a; float y=fl + (s-fl)*current.resonance*0.35f; y=std::tanh(y*(1.0f+current.drive*5.0f)); l+=y;r+=y;
        v.age+=1.0/sr;
    }
    l=std::tanh(l*0.8f);r=std::tanh(r*0.8f);
}
void InstrumentEngine::render(juce::AudioBuffer<float>&buffer,juce::MidiBuffer&midi){
    for(const auto metadata:midi){auto m=metadata.getMessage();if(m.isNoteOn())noteOn(m.getNoteNumber(),m.getFloatVelocity());else if(m.isNoteOff())noteOff(m.getNoteNumber());else if(m.isAllNotesOff()||m.isAllSoundOff())allNotesOff();}
    auto*L=buffer.getWritePointer(0);auto*R=buffer.getNumChannels()>1?buffer.getWritePointer(1):nullptr;
    for(int i=0;i<buffer.getNumSamples();++i){float l,r;renderSample(l,r);L[i]+=l;if(R)R[i]+=r;}
}
}
