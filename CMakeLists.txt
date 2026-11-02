cmake_minimum_required(VERSION 3.22)
project(StudioBolVocalManager VERSION 0.9.0)

# JUCE is fetched at configure time. Requires internet on the build computer,
# or replace FetchContent with add_subdirectory(path/to/JUCE).
include(FetchContent)
set(STUDIO_BOL_JUCE_SOURCE_DIR "" CACHE PATH "Optional path to a local JUCE 8.0.6 checkout")
if (STUDIO_BOL_JUCE_SOURCE_DIR AND EXISTS "${STUDIO_BOL_JUCE_SOURCE_DIR}/CMakeLists.txt")
    add_subdirectory("${STUDIO_BOL_JUCE_SOURCE_DIR}" JUCE)
else()
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.6
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(StudioBolVocalManager
    COMPANY_NAME "Herman Audio Tools"
    BUNDLE_ID nl.studiobol.vocalmanager
    PLUGIN_MANUFACTURER_CODE HmAt
    PLUGIN_CODE RvM1
    FORMATS VST3 Standalone
    PRODUCT_NAME "Studio Bol Vocal Manager"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(StudioBolVocalManager PRIVATE
    PluginProcessor.cpp
    PluginProcessor.h
    PluginEditor.cpp
    PluginEditor.h
)
juce_generate_juce_header(StudioBolVocalManager)
target_compile_features(StudioBolVocalManager PRIVATE cxx_std_17)
target_compile_definitions(StudioBolVocalManager PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
)
target_link_libraries(StudioBolVocalManager PRIVATE
    juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags
)
