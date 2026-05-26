cmake_minimum_required(VERSION 3.15)
project(AntigravityCompressor VERSION 1.0.0)

# 1. Fetching JUCE audio framework directly from the cloud
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.0
)
FetchContent_MakeAvailable(juce)

# 2. Defining our Plugin parameters
juce_add_plugin(AntigravityCompressor
    COMPANY_NAME "Studio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE "Manu"
    PLUGIN_CODE "Agcp"
    FORMATS VST3
    PRODUCT_NAME "Antigravity Compressor")

# 3. Informing the compiler about the files we will create next
target_sources(AntigravityCompressor
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
)

# 4. Linking everything together
target_link_libraries(AntigravityCompressor
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
