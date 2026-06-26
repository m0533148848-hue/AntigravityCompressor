#include "PluginProcessor.h"
#include "PluginEditor.h"

// הלוגיקה של ציור האודיו
void WaveformDisplay::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::black);
    auto bounds = getLocalBounds().toFloat();
    float centerY = bounds.getHeight() / 2.0f;
    
    // משיכת הערכים של הטרשהולד מהכפתורים
    float threshUpDb = processor.apvts.getRawParameterValue("THRESH_UP")->load();
    float threshDownDb = processor.apvts.getRawParameterValue("THRESH_DOWN")->load();
    
    // המרה למתמטיקה של גובה במסך
    float ampUp = juce::Decibels::decibelsToGain(threshUpDb);
    float ampDown = juce::Decibels::decibelsToGain(threshDownDb);
    
    float yUpTop = centerY - (ampUp * centerY);
    float yUpBottom = centerY + (ampUp * centerY);
    float yDownTop = centerY - (ampDown * centerY);
    float yDownBottom = centerY + (ampDown * centerY);
    
    // צביעת הרקע של חלון העבודה שלנו (השטח הפעיל) בכחול שקוף
    g.setColour(juce::Colour(0x3000ccff));
    g.fillRect(0.0f, yUpTop, bounds.getWidth(), yDownTop - yUpTop); // החצי העליון של הגל
    g.fillRect(0.0f, yDownBottom, bounds.getWidth(), yUpBottom - yDownBottom); // החצי התחתון של הגל
    
    // ציור קווי הגל
    juce::Path inPath;
    juce::Path outPath;
    
    int numSamples = AntigravityCompressorProcessor::visualBufferSize;
    int currentIndex = processor.visualBufferIndex.load();
    float xStep = bounds.getWidth() / (float)numSamples;
    
    for (int i = 0; i < numSamples; ++i) {
        int readIndex = (currentIndex + i) % numSamples;
        float inSample = processor.inVisualBuffer[readIndex];
        float outSample = processor.outVisualBuffer[readIndex];
        
        float x = i * xStep;
        float yIn = centerY - (inSample * centerY);
        float yOut = centerY - (outSample * centerY);
        
        if (i == 0) {
            inPath.startNewSubPath(x, yIn);
            outPath.startNewSubPath(x, yOut);
        } else {
            inPath.lineTo(x, yIn);
            outPath.lineTo(x, yOut);
        }
    }
    
    // הגל המקורי (אפור)
    g.setColour(juce::Colours::grey.withAlpha(0.6f));
    g.strokePath(inPath, juce::PathStrokeType(1.5f));
    
    // הגל המעובד (ירוק בוהק)
    g.setColour(juce::Colours::limegreen);
    g.strokePath(outPath, juce::PathStrokeType(1.5f));
    
    // שרטוט קווי הטרשהולד הקשיחים
    g.setColour(juce::Colours::cyan);
    g.drawLine(0.0f, yUpTop, bounds.getWidth(), yUpTop, 1.0f);
    g.drawLine(0.0f, yUpBottom, bounds.getWidth(), yUpBottom, 1.0f);
    
    g.setColour(juce::Colours::blue);
    g.drawLine(0.0f, yDownTop, bounds.getWidth(), yDownTop, 1.0f);
    g.drawLine(0.0f, yDownBottom, bounds.getWidth(), yDownBottom, 1.0f);
    
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.drawRect(bounds, 1.0f);
}

// בניית המסך הראשי
AntigravityCompressorEditor::AntigravityCompressorEditor (AntigravityCompressorProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), waveformDisplay(p) // חיבור התצוגה למנוע
{
    addAndMakeVisible(waveformDisplay);
    addAndMakeVisible(envelopeDrawer);

    threshUpSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    threshUpSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 80, 20);
    addAndMakeVisible(threshUpSlider);
    threshUpAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "THRESH_UP", threshUpSlider);

    threshDownSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    threshDownSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 80, 20);
    addAndMakeVisible(threshDownSlider);
    threshDownAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "THRESH_DOWN", threshDownSlider);

    modeComboBox.addItem("Ratio (Relative)", 1);
    modeComboBox.addItem("Shift (Arbitrary)", 2);
    modeComboBox.addItem("Target (Absolute)", 3);
    addAndMakeVisible(modeComboBox);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "MODE", modeComboBox);

    ratioSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    ratioSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 80, 20);
    addAndMakeVisible(ratioSlider);
    ratioAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RATIO", ratioSlider);

    shiftSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    shiftSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 80, 20);
    addAndMakeVisible(shiftSlider);
    shiftAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "SHIFT", shiftSlider);

    targetSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    targetSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 80, 20);
    addAndMakeVisible(targetSlider);
    targetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "TARGET", targetSlider);

    addAndMakeVisible(lockButton);
    lockAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "LOCK_TO_LOWER", lockButton);

    setSize (600, 750);
}

AntigravityCompressorEditor::~AntigravityCompressorEditor() {}

void AntigravityCompressorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour(0xff1a1a2e));

    g.setColour (juce::Colours::cyan);
    g.setFont (24.0f);
    g.drawFittedText ("Antigravity Compressor Engine", 0, 10, getWidth(), 30, juce::Justification::centred, 1);

    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
    
    int labelX = 20;
    int startY = 460;
    
    g.drawText("Upper Thresh:", labelX, startY, 100, 20, juce::Justification::left);
    g.drawText("Lower Thresh:", labelX, startY + 40, 100, 20, juce::Justification::left);
    g.drawText("Mode:", labelX, startY + 80, 100, 20, juce::Justification::left);
    g.drawText("Ratio:", labelX, startY + 120, 100, 20, juce::Justification::left);
    g.drawText("Shift:", labelX, startY + 160, 100, 20, juce::Justification::left);
    g.drawText("Target Level:", labelX, startY + 200, 100, 20, juce::Justification::left);
}

void AntigravityCompressorEditor::resized()
{
    waveformDisplay.setBounds(20, 50, getWidth() - 40, 180);
    envelopeDrawer.setBounds(20, 240, getWidth() - 40, 180);

    int startX = 130;
    int width = getWidth() - startX - 20;
    int height = 20;
    int startY = 460;

    threshUpSlider.setBounds(startX, startY, width, height);
    threshDownSlider.setBounds(startX, startY + 40, width, height);
    modeComboBox.setBounds(startX, startY + 80, width, height);
    ratioSlider.setBounds(startX, startY + 120, width, height);
    shiftSlider.setBounds(startX, startY + 160, width, height);
    targetSlider.setBounds(startX, startY + 200, width, height);
    
    lockButton.setBounds(startX, startY + 240, width, 30);
}
