cmake_minimum_required(VERSION 3.22)
project(KyotosDream VERSION 0.2.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.6 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

set(KYOTO_SOURCES
  Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/InstrumentEngine.cpp
  Source/DreamAPI.cpp Source/UpdateService.cpp Source/ModuleRegistry.cpp Source/DspEngine.cpp
  Source/ThemeManager.cpp Source/PresetCodec.cpp Source/BuilderModel.h)

juce_add_binary_data(BinaryData SOURCES Resources/themes.json)

juce_add_plugin(KyotosDream
  COMPANY_NAME "Kyoto's Dream" PLUGIN_MANUFACTURER_CODE Kyto PLUGIN_CODE KyD1
  FORMATS VST3 PRODUCT_NAME "Kyoto's Dream" VST3_CATEGORIES Instrument Synth
  IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT FALSE EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE)
juce_generate_juce_header(KyotosDream)
target_sources(KyotosDream PRIVATE ${KYOTO_SOURCES})
target_link_libraries(KyotosDream PRIVATE BinaryData juce::juce_audio_utils juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
target_compile_definitions(KyotosDream PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_STRICT_REFCOUNTEDPOINTER=1)

juce_add_plugin(KyotosDreamFX
  COMPANY_NAME "Kyoto's Dream" PLUGIN_MANUFACTURER_CODE Kyto PLUGIN_CODE KyD2
  FORMATS VST3 PRODUCT_NAME "Kyoto's Dream FX" VST3_CATEGORIES Fx
  IS_SYNTH FALSE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT FALSE EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE)
juce_generate_juce_header(KyotosDreamFX)
target_sources(KyotosDreamFX PRIVATE ${KYOTO_SOURCES})
target_link_libraries(KyotosDreamFX PRIVATE BinaryData juce::juce_audio_utils juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
target_compile_definitions(KyotosDreamFX PRIVATE KYOTO_IS_FX=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_STRICT_REFCOUNTEDPOINTER=1)

install(TARGETS KyotosDream KyotosDreamFX LIBRARY DESTINATION . RUNTIME DESTINATION . ARCHIVE DESTINATION .)
