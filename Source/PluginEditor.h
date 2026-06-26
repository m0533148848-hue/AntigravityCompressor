#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include <array>
#include <vector>

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

// --- המבנה החדש ששומר גם את הנקודה וגם את העקומה שיוצאת ממנה ---
struct EnvPoint {
    float x;
    float y;
    float curve; // ערך בין 1.0- ל-1.0 (0 זה קו ישר)
};

class EnvelopeDrawer : public juce::Component {
public:
    EnvelopeDrawer();
    void paint(juce::Graphics& g) override;
    
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

private:
    std::vector<EnvPoint> points;
    int draggingIndex = -1;       // גרירת נקודה
    int draggingCurveIndex = -1;  // גרירת ידית העיקול
    float dragStartCurve = 0.0f;  // שמירת ערך העקומה בתחילת הגרירה
    void sortPoints();
};

class AntigravityCompressorEditor  : public juce::AudioProcessorEditor, public juce::Timer {
public:
    AntigravityCompressorEditor (AntigravityCompressorProcessor&);
    ~AntigravityCompressorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override { repaint(); } 

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
    
    juce::Slider kneeSlider;
    juce::Slider mixSlider;
    juce::ToggleButton deltaButton { "Delta" };

    juce::ComboBox scTriggerSelect;
    juce::TextButton scDeleteButton { "X" };
    juce::ComboBox scActionBox;
    juce::ComboBox scConditionBox;
    juce::ComboBox scRangeBox;

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
    
    std::unique_ptr<SliderAttachment> kneeAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<ButtonAttachment> deltaAttachment;

    std::unique_ptr<ComboBoxAttachment> scActionAttachment;
    std::unique_ptr<ComboBoxAttachment> scCondAttachment;
    std::unique_ptr<ComboBoxAttachment> scRangeAttachment;

    void updateVisibility();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AntigravityCompressorEditor)
};
