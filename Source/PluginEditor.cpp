#include "PluginProcessor.h"
#include "PluginEditor.h"

// ==============================================================================
// אתחול המסך, יצירת הכפתורים וחיבורם למנוע
// ==============================================================================
AntigravityCompressorEditor::AntigravityCompressorEditor (AntigravityCompressorProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // 1. הגדרות סליידר טרשהולד עליון
    threshUpSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    threshUpSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 80, 20);
    addAndMakeVisible(threshUpSlider);
    threshUpAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "THRESH_UP", threshUpSlider);

    // 2. הגדרות סליידר טרשהולד תחתון
    threshDownSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    threshDownSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 80, 20);
    addAndMakeVisible(threshDownSlider);
    threshDownAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "THRESH_DOWN", threshDownSlider);

    // 3. תפריט בחירת מצב עבודה
    modeComboBox.addItem("Ratio (Relative)", 1);
    modeComboBox.addItem("Shift (Arbitrary)", 2);
    modeComboBox.addItem("Target (Absolute)", 3);
    addAndMakeVisible(modeComboBox);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "MODE", modeComboBox);

    // 4. הגדרות סליידר יחס (Ratio)
    ratioSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    ratioSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 80, 20);
    addAndMakeVisible(ratioSlider);
    ratioAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RATIO", ratioSlider);

    // 5. הגדרות סליידר הסטה (Shift)
    shiftSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    shiftSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 80, 20);
    addAndMakeVisible(shiftSlider);
    shiftAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "SHIFT", shiftSlider);

    // 6. הגדרות סליידר יעד מוחלט (Target)
    targetSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    targetSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 80, 20);
    addAndMakeVisible(targetSlider);
    targetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "TARGET", targetSlider);

    // 7. כפתור נעילת יעד לטרשהולד תחתון
    addAndMakeVisible(lockButton);
    lockAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "LOCK_TO_LOWER", lockButton);

    // קביעת גודל חלון הפלאגין המלא
    setSize (500, 500);
}

AntigravityCompressorEditor::~AntigravityCompressorEditor()
{
}

// ==============================================================================
// ציור הרקע והטקסטים (כותרות)
// ==============================================================================
void AntigravityCompressorEditor::paint (juce::Graphics& g)
{
    // צבע רקע כהה
    g.fillAll (juce::Colour(0xff1a1a2e));

    // כותרת הפלאגין
    g.setColour (juce::Colours::cyan);
    g.setFont (24.0f);
    g.drawFittedText ("Antigravity Compressor Engine", 0, 10, getWidth(), 30, juce::Justification::centred, 1);

    // טקסטים לכפתורים
    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
    
    g.drawText("Upper Thresh:", 20, 70, 100, 20, juce::Justification::left);
    g.drawText("Lower Thresh:", 20, 120, 100, 20, juce::Justification::left);
    g.drawText("Mode:", 20, 170, 100, 20, juce::Justification::left);
    g.drawText("Ratio:", 20, 220, 100, 20, juce::Justification::left);
    g.drawText("Shift:", 20, 270, 100, 20, juce::Justification::left);
    g.drawText("Target Level:", 20, 320, 100, 20, juce::Justification::left);
}

// ==============================================================================
// סידור הכפתורים על המסך (מיקומים וגדלים)
// ==============================================================================
void AntigravityCompressorEditor::resized()
{
    int startX = 130;
    int width = getWidth() - startX - 20;
    int height = 20;

    threshUpSlider.setBounds(startX, 70, width, height);
    threshDownSlider.setBounds(startX, 120, width, height);
    modeComboBox.setBounds(startX, 170, width, height);
    ratioSlider.setBounds(startX, 220, width, height);
    shiftSlider.setBounds(startX, 270, width, height);
    targetSlider.setBounds(startX, 320, width, height);
    
    lockButton.setBounds(startX, 360, width, 30);
}
