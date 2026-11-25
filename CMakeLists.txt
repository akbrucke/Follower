cmake_minimum_required(VERSION 3.22)
project(Follower VERSION 1.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Follower
    COMPANY_NAME "Hanging Puppet Audio"
    VST3_CATEGORIES Fx Distortion
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Brck
    PLUGIN_CODE Fllw
    FORMATS VST3
    PRODUCT_NAME "Follower"
)

# Embed the 4 Orange PPC412 Impulse Responses + Cross-Platform Typography (EBGaramond & Inter)
# Copy EBGaramond.ttf and Inter.ttf from your Meadow/Resources folder into Follower/Resources/
juce_add_binary_data(FollowerAssets
    SOURCES
        "Resources/MARST75VINTAGE.wav"
        "Resources/MEGAV30PUNCHY.wav"
        "Resources/ORNGV30DARK.wav"
        "Resources/FENDJENGRITTY.wav"
        "Resources/EBGaramond.ttf"
        "Resources/Inter.ttf"
        "Resources/FollowerLogo.svg"
        "Resources/JewelIndicator.svg"
)

target_sources(Follower
    PRIVATE
        Source/PluginProcessor.h
        Source/PluginProcessor.cpp
        Source/PluginEditor.h
        Source/PluginEditor.cpp
        Source/Dsp/DspHelpers.h
        Source/Dsp/Overdrive.h
        Source/Dsp/PreAmpEffects.h
        Source/Dsp/TrainerMarkIII.h
        Source/Dsp/Rocker100.h
        Source/Dsp/DuskTapeEcho.h
)

target_compile_definitions(Follower
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Follower
    PRIVATE
        FollowerAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
