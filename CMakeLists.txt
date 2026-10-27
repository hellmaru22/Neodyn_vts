cmake_minimum_required(VERSION 3.22)
project(NeoDyn VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(NEODYN_INSTALL "Copiar el VST3 a la carpeta del sistema tras compilar" OFF)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(NeoDyn
    COMPANY_NAME "NeoDynAudio"
    PLUGIN_MANUFACTURER_CODE Ndyn
    PLUGIN_CODE Ndy1
    FORMATS VST3 Standalone
    PRODUCT_NAME "NeoDyn"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD ${NEODYN_INSTALL})

juce_generate_juce_header(NeoDyn)

target_sources(NeoDyn PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(NeoDyn PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(NeoDyn
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags)
