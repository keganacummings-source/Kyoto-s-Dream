#pragma once
#include "InstrumentEngine.h"
namespace kyoto { juce::var presetToVar(const InstrumentPreset& p); bool presetFromVar(const juce::var& v, InstrumentPreset& p); }
