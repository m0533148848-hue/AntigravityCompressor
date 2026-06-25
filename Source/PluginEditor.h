#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// ==============================================================================
// המחלקה של הממשק הגרפי (המסך של הפלאגין)
// ==============================================================================
class AntigravityCompressorEditor  : public juce::AudioProcessorEditor
{
public:
    AntigravityCompressorEditor (AntigravityCompressorProcessor&);
    ~AntigravityCompressorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // קישור למנוע הסאונד שלנו
    AntigravityCompressorProcessor& audioProcessor;

    // --- הרכיבים הגרפיים (כפתורים ותפריטים) ---
    juce::Slider threshUpSlider;
    juce::Slider threshDownSlider;
    juce::ComboBox modeComboBox;
    juce::Slider ratioSlider;
    juce::Slider shiftSlider;
    juce::Slider targetSlider;
    juce::ToggleButton lockButton { "Lock Target to Lower Thresh" };

    // --- החיבורים (Attachments) שמקשרים בין הכפתור הגרפי ללוגיקה ---
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> threshUpAttachment;
    std::unique_ptr<SliderAttachment> threshDownAttachment;
    std::unique_ptr<ComboBoxAttachment> modeAttachment;
    std::unique_ptr<SliderAttachment> ratioAttachment;
    std::unique_ptr<SliderAttachment> shiftAttachment;
    std::unique_ptr<SliderAttachment> targetAttachment;
    std::unique_ptr<ButtonAttachment> lockAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AntigravityCompressorEditor)
};
