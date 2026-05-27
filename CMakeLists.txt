cmake_minimum_required(VERSION 3.15)
project(AntigravityCompressor VERSION 1.0.0)

# הבאת ספריית הסאונד ישירות מהרשת
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
)
FetchContent_MakeAvailable(juce)

# הגדרות הפלאגין שלנו
juce_add_plugin(AntigravityCompressor
    COMPANY_NAME "MyAudioCompany"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Agcp
    FORMATS VST3
    PRODUCT_NAME "Antigravity Compressor"
)

# ==========================================================
# התיקון שלנו: ביטול מוחלט של תאימות לפורמט הישן VST2
# ==========================================================
target_compile_definitions(AntigravityCompressor
    PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

# חיבור קבצי הקוד שלנו
target_sources(AntigravityCompressor
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
)

# חיבור למנוע של JUCE
target_link_libraries(AntigravityCompressor
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
