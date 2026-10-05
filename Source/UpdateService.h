#pragma once
#include <juce_core/juce_core.h>
namespace kyoto {
struct UpdateInfo { bool available=false; juce::String version,url,notes; };
class UpdateService {
public:
    static constexpr const char* manifestUrl="https://raw.githubusercontent.com/keganacummings-source/KyotosDream/main/updates/manifest.json";
    UpdateInfo check(const juce::String& currentVersion);
    juce::File download(const juce::String& url);
};
}
