#pragma once
#include <juce_core/juce_core.h>
#include <vector>
namespace kyoto { struct ModuleInfo { juce::String id,name,type,sourceHtml; }; class ModuleRegistry { public: static std::vector<ModuleInfo> builtins(); }; }
