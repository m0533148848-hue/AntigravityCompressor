#include "PluginProcessor.h"
#include "PluginEditor.h"

void WaveformDisplay::timerCallback() {
    for (size_t i = 0; i < inHistory.size() - 1; ++i) {
        inHistory[i] = inHistory[i + 1];
        outHistory[i] = outHistory[i + 1];
    }
    inHistory.back() = processor.currentInPeak.exchange(0.0f);
    outHistory.back() = processor.currentOutPeak.exchange(0.0f);
    repaint();
}

void WaveformDisplay::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::black);
    auto bounds = getLocalBounds().toFloat();
    float centerY = bounds.getHeight() / 2.0f;
    float width = bounds.getWidth();
    float threshUpDb = processor.apvts.getRawParameterValue("THRESH_UP")->load();
    float threshDownDb = processor.apvts.getRawParameterValue("THRESH_DOWN")->load();
    float ampUp = juce::Decibels::decibelsToGain(threshUpDb);
    float ampDown = juce::Decibels::decibelsToGain(threshDownDb);
    
    float yUpTop = centerY - (ampUp * centerY);
    float yUpBottom = centerY + (ampUp * centerY);
    float yDownTop = centerY - (ampDown * centerY);
    float yDownBottom = centerY + (ampDown * centerY);
    
    g.setColour(juce::Colour(0x3000ccff));
    g.fillRect(0.0f, yUpTop, width, yDownTop - yUpTop);
    g.fillRect(0.0f, yDownBottom, width, yUpBottom - yDownBottom);
    
    juce::Path inPath; juce::Path outPath;
    int numPoints = inHistory.size();
    float xStep = width / (float)numPoints;
    
    for (int i = 0; i < numPoints; ++i) {
        float x = i * xStep;
        float inVal = inHistory[i] * centerY;
        float outVal = outHistory[i] * centerY;
        if (i == 0) {
            inPath.startNewSubPath(x, centerY - inVal);
            outPath.startNewSubPath(x, centerY - outVal);
        } else {
            inPath.lineTo(x, centerY - inVal);
            outPath.lineTo(x, centerY - outVal);
        }
    }
    for (int i = numPoints - 1; i >= 0; --i) {
        float x = i * xStep;
        inPath.lineTo(x, centerY + (inHistory[i] * centerY));
        outPath.lineTo(x, centerY + (outHistory[i] * centerY));
    }
    inPath.closeSubPath(); outPath.closeSubPath();
    
    g.setColour(juce::Colours::grey.withAlpha(0.6f)); g.fillPath(inPath);
    g.setColour(juce::Colours::limegreen.withAlpha(0.8f)); g.fillPath(outPath);
    
    g.setColour(juce::Colours::cyan);
    g.drawLine(0.0f, yUpTop, width, yUpTop, 1.0f); g.drawLine(0.0f, yUpBottom, width, yUpBottom, 1.0f);
    g.setColour(juce::Colours::blue);
    g.drawLine(0.0f, yDownTop, width, yDownTop, 1.0f); g.drawLine(0.0f, yDownBottom, width, yDownBottom, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.3f)); g.drawRect(bounds, 1.0f);
}

AntigravityCompressorEditor::AntigravityCompressorEditor (AntigravityCompressorProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), waveformDisplay(p)
{
    addAndMakeVisible(waveformDisplay);
    addAndMakeVisible(envelopeDrawer);

    // פונקציית עזר להגדרת כל כפתור להיות עגול עם מסך טקסט מעליו
    auto setupRotaryKnob = [this](juce::Slider& slider) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxAbove, false, 60, 20);
        addAndMakeVisible(slider);
    };

    setupRotaryKnob(inGainSlider);
    inGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "IN_GAIN", inGainSlider);

    setupRotaryKnob(threshDownSlider);
    threshDownAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "THRESH_DOWN", threshDownSlider);

    setupRotaryKnob(threshUpSlider);
    threshUpAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "THRESH_UP", threshUpSlider);

    modeComboBox.addItem("Ratio", 1);
    modeComboBox.addItem("Shift", 2);
    modeComboBox.addItem("Target", 3);
    addAndMakeVisible(modeComboBox);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "MODE", modeComboBox);

    setupRotaryKnob(ratioSlider);
    ratioAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RATIO", ratioSlider);

    setupRotaryKnob(shiftSlider);
    shiftAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "SHIFT", shiftSlider);

    setupRotaryKnob(targetSlider);
    targetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "TARGET", targetSlider);

    setupRotaryKnob(outGainSlider);
    outGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "OUT_GAIN", outGainSlider);

    addAndMakeVisible(lockButton);
    lockAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "LOCK_TO_LOWER", lockButton);

    setResizable(true, true);
    setResizeLimits(800, 500, 1600, 1000);
    setSize(1100, 650); // גודל פנורמי רחב כמו בציור
}

AntigravityCompressorEditor::~AntigravityCompressorEditor() {}

void AntigravityCompressorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour(0xff1a1a2e));
    g.setColour (juce::Colours::cyan);
    g.setFont (24.0f);
    g.drawFittedText ("Antigravity Compressor Engine", 0, 10, getWidth(), 30, juce::Justification::centred, 1);

    // הוספת כיתוב מתחת לכל כפתור כדי שנדע מה הוא
    g.setColour (juce::Colours::white);
    g.setFont (14.0f);
    int rowY = 370; // גובה הטקסטים
    int knobW = 90;
    int space = 30;
    int x = 40;

    g.drawText("In Gain", x, rowY, knobW, 20, juce::Justification::centred); x += knobW + space;
    g.drawText("Low Thresh", x, rowY, knobW, 20, juce::Justification::centred); x += knobW + space;
    g.drawText("Up Thresh", x, rowY, knobW, 20, juce::Justification::centred); x += knobW + space;
    
    g.drawText("Mode", x, rowY, knobW, 20, juce::Justification::centred); x += knobW + space;
    
    g.drawText("Ratio", x, rowY, knobW, 20, juce::Justification::centred); x += knobW + space;
    g.drawText("Shift", x, rowY, knobW, 20, juce::Justification::centred); x += knobW + space;
    g.drawText("Target", x, rowY, knobW, 20, juce::Justification::centred); x += knobW + space;
    g.drawText("Out Gain", x, rowY, knobW, 20, juce::Justification::centred);
}

void AntigravityCompressorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // מסך הגל רחב למעלה
    waveformDisplay.setBounds(20, 50, bounds.getWidth() - 40, 200);
    
    // שורת הכפתורים המרכזית מאוזנת!
    int rowY = 270;
    int knobW = 90;
    int knobH = 100;
    int space = 30;
    int x = 40;

    inGainSlider.setBounds(x, rowY, knobW, knobH); x += knobW + space;
    threshDownSlider.setBounds(x, rowY, knobW, knobH); x += knobW + space;
    threshUpSlider.setBounds(x, rowY, knobW, knobH); x += knobW + space;
    
    // תפריט הבחירה יושב בין הטרשהולדים לכפתורי הפעולה
    modeComboBox.setBounds(x, rowY + 30, knobW, 30); x += knobW + space;
    
    ratioSlider.setBounds(x, rowY, knobW, knobH); x += knobW + space;
    shiftSlider.setBounds(x, rowY, knobW, knobH); x += knobW + space;
    targetSlider.setBounds(x, rowY, knobW, knobH); x += knobW + space;
    outGainSlider.setBounds(x, rowY, knobW, knobH);
    
    lockButton.setBounds(x - (knobW + space), rowY + knobH, 120, 20); // מתחת לטארגט

    // אזור ה-LFO נכנס למטה
    envelopeDrawer.setBounds(20, 420, bounds.getWidth() - 40, bounds.getHeight() - 440);
}
