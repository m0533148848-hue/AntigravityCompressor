#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// ==============================================================================
// רכיב גרפי זמני עבור תצוגת הגלים (כאן נצייר את האודיו בהמשך)
// ==============================================================================
class WaveformDisplay : public juce::Component {
public:
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::cyan);
        g.drawRect(getLocalBounds(), 2);
        g.setFont(20.0f);
        g.drawText("Waveform & Range Display Area", getLocalBounds(), juce::Justification::centred, true);
    }
};

// ==============================================================================
// רכיב גרפי זמני עבור ציור המעטפת (כאן נצייר נקודות בסגנון סרום)
// ==============================================================================
class EnvelopeDrawer : public juce::Component {
public:
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff2a2a3e));
        g.setColour(juce::Colour(0xffff0055)); // ורוד ניאון
        g.drawRect(getLocalBounds(), 2);
        g.setFont(20.0f);
        g.drawText("Envelope Drawer Area (Serum Style)", getLocalBounds(), juce::Justification::centred, true);
    }
};

// ==============================================================================
// המחלקה של הממשק הגרפי (המסך הראשי של הפלאגין)
// ==============================================================================
class AntigravityCompressorEditor  : public juce::AudioProcessorEditor
{
public:
    AntigravityCompressorEditor (AntigravityCompressorProcessor&);
    ~AntigravityCompressorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AntigravityCompressorProcessor& audioProcessor;

    // --- המסכים החדשים שלנו ---
    WaveformDisplay waveformDisplay;
    EnvelopeDrawer envelopeDrawer;

    // --- הכפתורים ---
    juce::Slider threshUpSlider;
    juce::Slider threshDownSlider;
    juce::ComboBox modeComboBox;
    juce::Slider ratioSlider;
    juce::Slider shiftSlider;
    juce::Slider targetSlider;
    juce::ToggleButton lockButton { "Lock Target to Lower Thresh" };

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
