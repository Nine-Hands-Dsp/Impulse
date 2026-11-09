cmake_minimum_required(VERSION 3.22)

# macOS : binaire universel (Apple Silicon + Intel), macOS 10.13 minimum (avant project())
if(APPLE)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "" FORCE)
endif()

project(WigaImpulse VERSION 1.5.2)

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

# polices Inter (SIL OFL), logos IMPULSE et NINE HANDS | DSP (vectoriels) integres au plugin
juce_add_binary_data(ImpulseAssets SOURCES
  assets/Inter-SemiBold.otf
  assets/Inter-Bold.otf
  assets/Inter-Medium.otf
  assets/v5/bg.png
  assets/v5/knob_center.png
  assets/v5/knob_crown.png
  assets/v5/knob_reflect.png
  assets/v5/btn120_down.png
  assets/v5/btn100_down.png
  assets/v5/thumb.png
  assets/v5/ico_power.svg
  assets/v5/ico_list_search.svg
  assets/v5/ico_prev.svg
  assets/v5/ico_next.svg
  assets/v5/ico_save.svg
  assets/v5/ico_gear.svg
  assets/v5/ico_mute.svg
  assets/v5/ico_phase.svg)

if(APPLE)
  set(IMPULSE_FORMATS VST3 AU)
else()
  set(IMPULSE_FORMATS VST3)
endif()

juce_add_plugin(Impulse
  COMPANY_NAME "NINE HANDS | DSP"
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
  PluginEditor.cpp
  ImpulseStack.cpp)

# mode stack (NhdStack.h) : interface VST3 de REAPER (funknown.h du SDK VST3 fourni avec JUCE)
target_include_directories(Impulse PRIVATE
  ${JUCE_SOURCE_DIR}/modules/juce_audio_processors_headless/format_types/VST3_SDK)

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
