#pragma once
#include <juce_core/juce_core.h>
#include <functional>
#include <vector>

namespace kyoto {
struct DreamThread { juce::String id,user,title,text,audioUrl,imageUrl; int64_t at=0; bool hasAudio=false,hasImage=false; };
struct DreamSession { bool ok=false; juce::String token,user,role,theme,error; int onlineCount=0; };
struct CommunityInstrument { juce::String id,name,author,description,created; int downloads=0; juce::var state; };
class DreamAPI {
public:
    explicit DreamAPI(juce::String endpoint = "https://dreamshare-api.keganacummings.workers.dev");
    void setEndpoint(const juce::String& e){endpoint=e.trimCharactersAtEnd("/");}
    DreamSession login(const juce::String& user,const juce::String& pass);
    DreamSession registerAccount(const juce::String& user,const juce::String& pass);
    bool feed(std::vector<DreamThread>& out, int& onlineCount);
    bool postText(const juce::String& token,const juce::String& text,const juce::String& title);
    bool postAudio(const juce::String& token,const juce::File& file,const juce::String& text);
    bool postImage(const juce::String& token,const juce::File& file,const juce::String& text);
    bool communityList(std::vector<CommunityInstrument>& out);
    bool communityGet(const juce::String& id, CommunityInstrument& out);
    bool communityPublish(const juce::String& token,const CommunityInstrument& item);
    bool setTheme(const juce::String& token,const juce::String& theme);
private:
    juce::var postJson(const juce::var& body,const juce::String& token,juce::String& error);
    juce::String endpoint;
};
}
