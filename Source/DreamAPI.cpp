#include "DreamAPI.h"
namespace kyoto {
DreamAPI::DreamAPI(juce::String e):endpoint(e.trimCharactersAtEnd("/")){}
juce::var DreamAPI::postJson(const juce::var& body,const juce::String& token,juce::String& error){
    juce::DynamicObject::Ptr obj(body.getDynamicObject()); if(!obj){error="invalid request";return {};}
    juce::URL u(endpoint); auto json=juce::JSON::toString(body);
    juce::URL::InputStreamOptions opt(juce::URL::ParameterHandling::inPostData);
    opt.withConnectionTimeoutMs(12000).withExtraHeaders("Content-Type: application/json\r\nAccept: application/json\r\n"+(token.isNotEmpty()?"X-SW-Token: "+token+"\r\n":""));
    auto in=u.withPOSTData(json).createInputStream(opt); if(!in){error="API connection failed";return {};}
    auto text=in->readEntireStreamAsString(); auto v=juce::JSON::parse(text); if(v.isVoid()){error="API returned invalid JSON";return {};}
    if(auto*d=v.getDynamicObject();d&&d->hasProperty("error")&&d->getProperty("ok")==false) error=d->getProperty("error").toString();
    return v;
}
DreamSession DreamAPI::login(const juce::String&u,const juce::String&p){return registerAccount(u,p);}
DreamSession DreamAPI::registerAccount(const juce::String&u,const juce::String&p){
    DreamSession s; juce::DynamicObject::Ptr o=new juce::DynamicObject();o->setProperty("action","login");o->setProperty("user",u);o->setProperty("pass",p);juce::String e;auto v=postJson(juce::var(o.get()),{},e);if(auto*d=v.getDynamicObject()){s.ok=(bool)d->getProperty("ok");s.token=d->getProperty("token").toString();s.user=d->getProperty("user").toString();s.role=d->getProperty("role").toString();s.theme=d->getProperty("theme").toString();s.onlineCount=(int)d->getProperty("onlineCount");}s.error=e;return s;}
bool DreamAPI::feed(std::vector<DreamThread>&out,int&onlineCount){
    juce::URL u(endpoint);auto in=u.createInputStream(juce::URL::InputStreamOptions().withConnectionTimeoutMs(10000));if(!in)return false;auto v=juce::JSON::parse(in->readEntireStreamAsString());auto*d=v.getDynamicObject();if(!d)return false;onlineCount=(int)d->getProperty("onlineCount");out.clear();auto a=d->getProperty("threads");if(auto*arr=a.getArray())for(auto&x:*arr)if(auto*t=x.getDynamicObject()){DreamThread q;q.id=t->getProperty("id").toString();q.user=t->getProperty("user").toString();q.title=t->getProperty("title").toString();q.text=t->getProperty("text").toString();q.audioUrl=t->getProperty("audioUrl").toString();q.imageUrl=t->getProperty("imageUrl").toString();q.hasAudio=(bool)t->getProperty("hasAudio");q.hasImage=(bool)t->getProperty("hasImage")||q.imageUrl.isNotEmpty();q.at=(int64_t)t->getProperty("at");out.push_back(q);}return true;}
bool DreamAPI::postText(const juce::String&t,const juce::String&text,const juce::String&title){juce::DynamicObject::Ptr o=new juce::DynamicObject();o->setProperty("action","create_thread");o->setProperty("token",t);o->setProperty("text",text);o->setProperty("title",title);juce::String e;auto v=postJson(juce::var(o.get()),t,e);return v.getDynamicObject()&& (bool)v.getDynamicObject()->getProperty("ok");}
bool DreamAPI::postAudio(const juce::String&t,const juce::File&f,const juce::String&text){auto b=f.loadFileAsData();auto data="data:audio/wav;base64,"+b.toBase64Encoding();juce::DynamicObject::Ptr o=new juce::DynamicObject();o->setProperty("action","create_thread");o->setProperty("token",t);o->setProperty("text",text);o->setProperty("title",f.getFileNameWithoutExtension());o->setProperty("audioData",data);o->setProperty("hasAudio",true);juce::String e;auto v=postJson(juce::var(o.get()),t,e);return v.getDynamicObject()&&(bool)v.getDynamicObject()->getProperty("ok");}
bool DreamAPI::postImage(const juce::String&t,const juce::File&f,const juce::String&text){
    auto bytes=f.loadFileAsData(); if(bytes.isEmpty()) return false;
    auto b64=bytes.toBase64Encoding(); const int chunk=600000; int parts=(b64.length()+chunk-1)/chunk; if(parts>20) return false;
    juce::String upload="img"+juce::String(juce::Time::getCurrentTime().toMilliseconds());
    auto mime=f.hasFileExtension("png")?"image/png":f.hasFileExtension("jpg;jpeg")?"image/jpeg":f.hasFileExtension("gif")?"image/gif":"application/octet-stream";
    juce::String sink="bin"; for(int i=0;i<parts;i++){
        juce::DynamicObject::Ptr o=new juce::DynamicObject();o->setProperty("action","image_part");o->setProperty("token",t);o->setProperty("upload",upload);o->setProperty("index",i);o->setProperty("parts",parts);o->setProperty("mime",mime);o->setProperty("b64",b64.substring(i*chunk,juce::jmin(b64.length(),(i+1)*chunk)));juce::String e;auto v=postJson(juce::var(o.get()),t,e);if(!v.getDynamicObject()||(bool)v.getDynamicObject()->getProperty("ok")!=true)return false; sink=v.getDynamicObject()->getProperty("sink").toString();
    }
    juce::DynamicObject::Ptr o=new juce::DynamicObject();o->setProperty("action","create_thread");o->setProperty("token",t);o->setProperty("text",text);o->setProperty("title",f.getFileNameWithoutExtension());o->setProperty("hasImage",true);o->setProperty("imageStore",sink);o->setProperty("imageUpload",upload);o->setProperty("imageParts",parts);o->setProperty("imageBytes",(int64_t)bytes.getSize());o->setProperty("imageMime",mime);juce::String e;auto v=postJson(juce::var(o.get()),t,e);return v.getDynamicObject()&&(bool)v.getDynamicObject()->getProperty("ok");
}
bool DreamAPI::communityList(std::vector<CommunityInstrument>& out){
    out.clear();
    auto u=juce::URL(endpoint).withParameter("community","instruments");
    auto in=u.createInputStream(juce::URL::InputStreamOptions().withConnectionTimeoutMs(10000)); if(!in) return false;
    auto v=juce::JSON::parse(in->readEntireStreamAsString()); auto* d=v.getDynamicObject(); if(!d) return false;
    if(auto* a=d->getProperty("instruments").getArray()) for(auto& x:*a) if(auto* o=x.getDynamicObject()) { CommunityInstrument q; q.id=o->getProperty("id").toString(); q.name=o->getProperty("name").toString(); q.author=o->getProperty("author").toString(); q.description=o->getProperty("description").toString(); q.created=o->getProperty("created").toString(); q.downloads=(int)o->getProperty("downloads"); out.push_back(q); }
    return true;
}
bool DreamAPI::communityGet(const juce::String& id, CommunityInstrument& out){
    auto u=juce::URL(endpoint).withParameter("community",id);
    auto in=u.createInputStream(juce::URL::InputStreamOptions().withConnectionTimeoutMs(10000)); if(!in) return false;
    auto v=juce::JSON::parse(in->readEntireStreamAsString()); auto* o=v.getDynamicObject(); if(!o || !(bool)o->getProperty("ok")) return false;
    out.id=o->getProperty("id").toString(); out.name=o->getProperty("name").toString(); out.author=o->getProperty("author").toString(); out.description=o->getProperty("description").toString(); out.created=o->getProperty("created").toString(); out.downloads=(int)o->getProperty("downloads"); out.state=o->getProperty("state"); return out.state.isObject();
}
bool DreamAPI::setTheme(const juce::String& t,const juce::String& theme){auto*o=new juce::DynamicObject();o->setProperty("action","set_theme");o->setProperty("token",t);o->setProperty("theme",theme);juce::String e;auto v=postJson(juce::var(o),t,e);auto*d=v.getDynamicObject();return d&&(bool)d->getProperty("ok");}
bool DreamAPI::communityPublish(const juce::String& t,const CommunityInstrument& item){
    auto* o=new juce::DynamicObject(); o->setProperty("action","community_publish"); o->setProperty("token",t); o->setProperty("name",item.name); o->setProperty("description",item.description); o->setProperty("state",item.state);
    juce::String err; auto v=postJson(juce::var(o),t,err); auto* d=v.getDynamicObject(); return d && (bool)d->getProperty("ok");
}

}
