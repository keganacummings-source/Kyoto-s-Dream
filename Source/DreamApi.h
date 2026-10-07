#pragma once
#include <JuceHeader.h>

namespace kt
{
struct DreamResult
{
    bool ok = false;
    juce::String error, token, user, role, body, raw;
    juce::var parsed;
};

DreamResult login(const juce::String& user, const juce::String& pass);
DreamResult sendChat(const juce::String& token, const juce::String& text);
DreamResult getFeed(const juce::String& token);
DreamResult getThreads(const juce::String& token);
DreamResult getCatalog(const juce::String& token);
DreamResult getModule(const juce::String& token, const juce::String& id);
DreamResult publishModule(const juce::String& token, const juce::String& name, const juce::String& jsonBody);
DreamResult deleteModule(const juce::String& token, const juce::String& id);
DreamResult approveModule(const juce::String& token, const juce::String& id);
DreamResult denyModule(const juce::String& token, const juce::String& id);
DreamResult tagModule(const juce::String& token, const juce::String& id, const juce::String& tags);
DreamResult getMyModules(const juce::String& token);
DreamResult getPendingModules(const juce::String& token);
DreamResult getCatalogTagged(const juce::String& token, const juce::String& tags);
DreamResult getSocial(const juce::String& token);
DreamResult getDM(const juce::String& token, const juce::String& peer);
DreamResult sendDM(const juce::String& token, const juce::String& peer, const juce::String& text);
DreamResult friendRequest(const juce::String& token, const juce::String& action, const juce::String& target);
DreamResult react(const juce::String& token, const juce::String& kind, const juce::String& id, const juce::String& emoji);
DreamResult postAction(const juce::String& action, juce::var body, const juce::String& token);
}
