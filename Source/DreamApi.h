#pragma once
#include <JuceHeader.h>

namespace kt
{
struct DreamResult
{
    bool ok = false;
    juce::String error, token, user, body, raw;
    juce::var parsed;
};

DreamResult login(const juce::String& user, const juce::String& pass);
DreamResult sendChat(const juce::String& token, const juce::String& text);
DreamResult getFeed(const juce::String& token);
DreamResult getThreads(const juce::String& token);
DreamResult getCatalog(const juce::String& token);
DreamResult getModule(const juce::String& token, const juce::String& id);
DreamResult publishModule(const juce::String& token, const juce::String& name, const juce::String& jsonBody);
DreamResult postAction(const juce::String& action, juce::var body, const juce::String& token);
}
