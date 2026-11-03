cmake_minimum_required(VERSION 3.22)

# macOS : binaire universel (Apple Silicon + Intel), macOS 10.13 minimum (avant project())
if(APPLE)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "" FORCE)
endif()

project(WigaImpulse VERSION 1.2.3)

option(IMPULSE_TOOLS "Build the Linux render / test tools" OFF)

include(FetchContent)
if(DEFINED JUCE_LOCAL_DIR)
  add_subdirectory(${JUCE_LOCAL_DIR} juce)
else()
  FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW ON)
  FetchContent_MakeAvailable(JUCE)
endif()

# polices Inter (SIL OFL) et logo Studio Wiga integres au plugin
juce_add_binary_data(ImpulseAssets SOURCES
  assets/Inter-SemiBold.otf
  assets/Inter-Bold.otf
  assets/Inter-Medium.otf
  assets/logo_studio_wiga_blanc.png
  assets/logo_impulse.png)

if(APPLE)
  set(IMPULSE_FORMATS VST3 AU)
else()
  set(IMPULSE_FORMATS VST3)
endif()

juce_add_plugin(Impulse
  COMPANY_NAME "Studio Wiga"
  PLUGIN_MANUFACTURER_CODE Wiga
  PLUGIN_CODE WgIr
  FORMATS ${IMPULSE_FORMATS}
  AU_MAIN_TYPE kAudioUnitType_MusicEffect
  VST3_CATEGORIES Fx
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  PRODUCT_NAME "Impulse"
  COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(Impulse PRIVATE
  PluginProcessor.cpp
  PluginEditor.cpp)

target_compile_features(Impulse PRIVATE cxx_std_17)

target_compile_definitions(Impulse PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Impulse
  PRIVATE
    ImpulseAssets
    juce::juce_audio_utils
    juce::juce_dsp
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

if(IMPULSE_TOOLS)
  add_subdirectory(tools)
endif()
