#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include <array>

class WaveformDisplay : public juce::Component, public juce::Timer {
public:
    WaveformDisplay(AntigravityCompressorProcessor& p) : processor(p) { startTimerHz(60); }
    ~WaveformDisplay() override { stopTimer(); }
    void timerCallback() override;
    void paint(juce::Graphics& g) override;
private:
    AntigravityCompressorProcessor& processor;
    std::array<float, 600> inHistory { 0.0f };
    std::array<float, 600> outHistory { 0.0f };
};

class EnvelopeDrawer : public juce::Component {
public:
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff2a2a3e));
        g.setColour(juce::Colour(0xffff0055));
        g.drawRect(getLocalBounds(), 2);
        g.setFont(20.0f);
        g.drawText("LFO / Envelope Area", getLocalBounds(), juce::Justification::centred, true);
    }
};

class AntigravityCompressorEditor  : public juce::AudioProcessorEditor {
public:
    AntigravityCompressorEditor (AntigravityCompressorProcessor&);
    ~AntigravityCompressorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AntigravityCompressorProcessor& audioProcessor;
    WaveformDisplay waveformDisplay;
    EnvelopeDrawer envelopeDrawer;

    juce::Slider inGainSlider;
    juce::Slider threshDownSlider;
    juce::Slider threshUpSlider;
    juce::ComboBox modeComboBox;
    juce::Slider ratioSlider;
    juce::Slider shiftSlider;
    juce::Slider targetSlider;
    juce::Slider outGainSlider;
    juce::ToggleButton lockButton { "Lock Target" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> inGainAttachment;
    std::unique_ptr<SliderAttachment> threshDownAttachment;
    std::unique_ptr<SliderAttachment> threshUpAttachment;
    std::unique_ptr<ComboBoxAttachment> modeAttachment;
    std::unique_ptr<SliderAttachment> ratioAttachment;
    std::unique_ptr<SliderAttachment> shiftAttachment;
    std::unique_ptr<SliderAttachment> targetAttachment;
    std::unique_ptr<SliderAttachment> outGainAttachment;
    std::unique_ptr<ButtonAttachment> lockAttachment;

    // --- הפונקציה שאחראית להעלים ולהציג כפתורים ---
    void updateVisibility();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AntigravityCompressorEditor)
};
