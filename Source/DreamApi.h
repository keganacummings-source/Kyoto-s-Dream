#pragma once
#include <JuceHeader.h>

namespace kt
{
struct DreamResult
{
    bool ok = false;
    juce::String error, token, user, raw;
};

DreamResult postAction(const juce::String& action, juce::var body, const juce::String& token);
juce::var getFeed();
}
