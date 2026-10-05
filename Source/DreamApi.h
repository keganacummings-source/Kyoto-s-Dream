#pragma once
#include <JuceHeader.h>

namespace kt
{
struct DreamResult
{
    bool ok = false;
    juce::String error, token, user, body, raw;
};

DreamResult login(const juce::String& user, const juce::String& pass);
DreamResult sendChat(const juce::String& token, const juce::String& text);
DreamResult getFeed(const juce::String& token);
DreamResult getCatalog(const juce::String& token);
DreamResult publishModule(const juce::String& token, const juce::String& name, const juce::String& jsonBody);

// Low-level
DreamResult postAction(const juce::String& action, juce::var body, const juce::String& token);
}
