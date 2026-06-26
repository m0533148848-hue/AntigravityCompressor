#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

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

// =========================================================================
// צייר המעטפות (LFO) עם תמיכה בעיקולים
// =========================================================================
EnvelopeDrawer::EnvelopeDrawer() {
    points.push_back({0.0f, 0.5f, 0.0f});
    points.push_back({1.0f, 0.5f, 0.0f});
}

void EnvelopeDrawer::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff2a2a3e));
    g.setColour(juce::Colour(0xffff0055));
    g.drawRect(getLocalBounds(), 2);
    
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    for (int i = 1; i < 4; ++i) g.drawHorizontalLine(getHeight() * i / 4, 0, getWidth());
    for (int i = 1; i < 8; ++i) g.drawVerticalLine(getWidth() * i / 8, 0, getHeight());

    if (points.empty()) return;

    juce::Path envPath;
    float w = getWidth();
    float h = getHeight();

    // ציור העקומות בין הנקודות
    for (size_t i = 0; i < points.size() - 1; ++i) {
        float x1 = points[i].x * w;
        float y1 = points[i].y * h;
        float x2 = points[i+1].x * w;
        float y2 = points[i+1].y * h;
        float curve = points[i].curve;

        if (i == 0) envPath.startNewSubPath(x1, y1);

        // מחלקים כל קו ל-20 חלקים קטנים כדי ליצור עקומה חלקה
        int numSteps = 20;
        for(int step = 1; step <= numSteps; ++step) {
            float t = (float)step / numSteps;
            float exp = std::pow(2.0f, curve * 3.0f); // מתמטיקה של העיקול
            float curvedT = std::pow(t, exp);
            float currentX = x1 + (x2 - x1) * t;
            float currentY = y1 + (y2 - y1) * curvedT;
            envPath.lineTo(currentX, currentY);
        }
    }
    
    g.setColour(juce::Colour(0xffff0055));
    g.strokePath(envPath, juce::PathStrokeType(2.5f));

    // ציור ידיות העיקול (עיגולים תכלת) באמצע כל קו
    for (size_t i = 0; i < points.size() - 1; ++i) {
        float x1 = points[i].x * w;
        float y1 = points[i].y * h;
        float x2 = points[i+1].x * w;
        float y2 = points[i+1].y * h;
        float curve = points[i].curve;

        float t = 0.5f;
        float exp = std::pow(2.0f, curve * 3.0f);
        float curvedT = std::pow(t, exp);
        float hX = x1 + (x2 - x1) * t;
        float hY = y1 + (y2 - y1) * curvedT;

        g.setColour(juce::Colours::cyan);
        g.drawEllipse(hX - 4, hY - 4, 8, 8, 2.0f);
    }

    // ציור הנקודות הראשיות (עיגולים לבנים מלאים)
    g.setColour(juce::Colours::white);
    for (auto point : points) {
        g.fillEllipse(point.x * w - 5, point.y * h - 5, 10, 10);
    }
}

void EnvelopeDrawer::sortPoints() {
    std::sort(points.begin(), points.end(), [](const EnvPoint& a, const EnvPoint& b) { return a.x < b.x; });
    if (!points.empty()) { points.front().x = 0.0f; points.back().x = 1.0f; }
}

void EnvelopeDrawer::mouseDown(const juce::MouseEvent& e) {
    float mx = e.position.x / getWidth();
    float my = e.position.y / getHeight();
    draggingIndex = -1;
    draggingCurveIndex = -1;
    
    // 1. קודם בודקים אם לחצו על נקודה ראשית
    for (size_t i = 0; i < points.size(); ++i) {
        if (std::abs(points[i].x - mx) < 0.03f && std::abs(points[i].y - my) < 0.05f) {
            draggingIndex = (int)i;
            return;
        }
    }
    
    // 2. אם לא לחצו על נקודה, בודקים אם לחצו על ידית עיקול
    for (size_t i = 0; i < points.size() - 1; ++i) {
        float x1 = points[i].x; float y1 = points[i].y;
        float x2 = points[i+1].x; float y2 = points[i+1].y;
        float curve = points[i].curve;

        float t = 0.5f;
        float exp = std::pow(2.0f, curve * 3.0f);
        float curvedT = std::pow(t, exp);
        float hX = x1 + (x2 - x1) * t;
        float hY = y1 + (y2 - y1) * curvedT;

        if (std::abs(hX - mx) < 0.03f && std::abs(hY - my) < 0.05f) {
            draggingCurveIndex = (int)i;
            dragStartCurve = points[i].curve;
            return;
        }
    }
}

void EnvelopeDrawer::mouseDrag(const juce::MouseEvent& e) {
    // גרירת נקודה רגילה
    if (draggingIndex >= 0) {
        float mx = juce::jlimit(0.0f, 1.0f, e.position.x / getWidth());
        float my = juce::jlimit(0.0f, 1.0f, e.position.y / getHeight());
        points[draggingIndex].y = my; 
        
        if (draggingIndex != 0 && draggingIndex != points.size() - 1) {
            float prevX = points[draggingIndex - 1].x;
            float nextX = points[draggingIndex + 1].x;
            points[draggingIndex].x = juce::jlimit(prevX + 0.01f, nextX - 0.01f, mx);
        }
        repaint();
    }
    // גרירת ידית עיקול
    else if (draggingCurveIndex >= 0) {
        // מחשבים את המרחק שהעכבר זז למעלה או למטה
        float deltaY = e.getDistanceFromDragStartY() / (float)getHeight();
        // מעדכנים את העקומה ומגבילים בין 1- ל-1
        points[draggingCurveIndex].curve = juce::jlimit(-1.0f, 1.0f, dragStartCurve + deltaY * 2.5f);
        repaint();
    }
}

void EnvelopeDrawer::mouseDoubleClick(const juce::MouseEvent& e) {
    float mx = e.position.x / getWidth();
    float my = e.position.y / getHeight();
    
    for (size_t i = 1; i < points.size() - 1; ++i) {
        if (std::abs(points[i].x - mx) < 0.05f && std::abs(points[i].y - my) < 0.05f) {
            points.erase(points.begin() + i);
            repaint();
            return;
        }
    }
    // הוספת נקודה חדשה עם קו ישר (curve = 0)
    points.push_back({mx, my, 0.0f});
    sortPoints();
    repaint();
}


// =========================================================================
// המסך הראשי
// =========================================================================
AntigravityCompressorEditor::AntigravityCompressorEditor (AntigravityCompressorProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), waveformDisplay(p)
{
    addAndMakeVisible(waveformDisplay);
    addAndMakeVisible(envelopeDrawer);

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

    modeComboBox.addItem("Ratio", 1); modeComboBox.addItem("Shift", 2); modeComboBox.addItem("Target", 3);
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
    
    setupRotaryKnob(kneeSlider);
    kneeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "KNEE", kneeSlider);
    setupRotaryKnob(mixSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "MIX", mixSlider);
    
    addAndMakeVisible(deltaButton);
    deltaAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "DELTA", deltaButton);

    modeComboBox.onChange = [this] { updateVisibility(); };
    updateVisibility();

    scTriggerSelect.addItem("Trigger 1", 1); scTriggerSelect.addItem("Add New...", 2);
    scTriggerSelect.setSelectedId(1);
    addAndMakeVisible(scTriggerSelect);
    
    scDeleteButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
    addAndMakeVisible(scDeleteButton);
    
    scActionBox.addItem("Cut Volume", 1); scActionBox.addItem("Boost Volume", 2);
    addAndMakeVisible(scActionBox);
    scActionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "SC_ACTION", scActionBox);
    
    scConditionBox.addItem("When Signal Present", 1); scConditionBox.addItem("When Signal Absent", 2);
    addAndMakeVisible(scConditionBox);
    scCondAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "SC_COND", scConditionBox);
    
    scRangeBox.addItem("Inside Threshold", 1); scRangeBox.addItem("Outside Threshold", 2);
    addAndMakeVisible(scRangeBox);
    scRangeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "SC_RANGE", scRangeBox);

    setResizable(true, true);
    setResizeLimits(800, 500, 1600, 1000);
    setSize(950, 600); 
    
    startTimerHz(30); 
}

AntigravityCompressorEditor::~AntigravityCompressorEditor() { stopTimer(); }

void AntigravityCompressorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour(0xff181822)); 
    g.setColour (juce::Colours::cyan);
    g.setFont (24.0f);
    g.drawFittedText ("Antigravity Compressor Engine", 0, 10, getWidth(), 30, juce::Justification::centred, 1);

    g.setColour (juce::Colours::white);
    g.setFont (13.0f);
    
    int rowY = 365;
    int knobW = 75;
    int space = 15;
    int x = 15;

    g.drawText("In Gain", x, rowY, knobW, 20, juce::Justification::centred); x += knobW + space;
    g.drawText("Low Thresh", x, rowY, knobW, 20, juce::Justification::centred); x += knobW + space;
    g.drawText("Up Thresh", x, rowY, knobW, 20, juce::Justification::centred); x += knobW + space;
    g.drawText("Mode", x, rowY, knobW, 20, juce::Justification::centred); x += knobW + space;
    
    if (ratioSlider.isVisible())  g.drawText("Ratio", x, rowY, knobW, 20, juce::Justification::centred); 
    if (shiftSlider.isVisible())  g.drawText("Shift", x, rowY, knobW, 20, juce::Justification::centred); 
    if (targetSlider.isVisible()) g.drawText("Target", x, rowY, knobW, 20, juce::Justification::centred); 
    x += knobW + space;

    g.drawText("Knee", x, rowY, knobW, 20, juce::Justification::centred); x += knobW + space;
    g.drawText("Mix", x, rowY, knobW, 20, juce::Justification::centred); x += knobW + space;
    g.drawText("Out Gain", x, rowY, knobW, 20, juce::Justification::centred);

    int meterX = getWidth() - 25;
    int meterY = 270;
    int meterW = 10;
    int meterH = 95;
    
    g.setColour(juce::Colour(0xff222222));
    g.fillRect(meterX, meterY, meterW, meterH);
    
    float gr = audioProcessor.currentGR.load();
    if (gr > 0.0f) {
        float grHeight = juce::jmin((gr / 24.0f) * meterH, (float)meterH);
        g.setColour(juce::Colours::red);
        g.fillRect((float)meterX, (float)meterY, (float)meterW, grHeight); 
    }

    auto bounds = getLocalBounds();
    int scPanelW = 280;
    int scPanelX = bounds.getWidth() - scPanelW - 15;
    int scPanelY = 410;
    int scPanelH = bounds.getHeight() - 425;

    g.setColour(juce::Colour(0xff222233));
    g.fillRoundedRectangle(scPanelX, scPanelY, scPanelW, scPanelH, 8.0f);
    g.setColour(juce::Colours::cyan.withAlpha(0.3f));
    g.drawRoundedRectangle(scPanelX, scPanelY, scPanelW, scPanelH, 8.0f, 1.0f);

    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    g.drawText("Sidechain Rules", scPanelX, scPanelY + 5, scPanelW, 25, juce::Justification::centred);
}

void AntigravityCompressorEditor::resized()
{
    auto bounds = getLocalBounds();
    waveformDisplay.setBounds(15, 45, bounds.getWidth() - 30, 210);
    
    int rowY = 270;
    int knobW = 75;
    int knobH = 95;
    int space = 15;
    int x = 15;

    inGainSlider.setBounds(x, rowY, knobW, knobH); x += knobW + space;
    threshDownSlider.setBounds(x, rowY, knobW, knobH); x += knobW + space;
    threshUpSlider.setBounds(x, rowY, knobW, knobH); x += knobW + space;
    
    modeComboBox.setBounds(x, rowY + 30, knobW, 30); x += knobW + space;
    
    ratioSlider.setBounds(x, rowY, knobW, knobH); 
    shiftSlider.setBounds(x, rowY, knobW, knobH); 
    targetSlider.setBounds(x, rowY, knobW, knobH); 
    
    lockButton.setBounds(x - 5, rowY + knobH, 90, 20);
    x += knobW + space;
    
    kneeSlider.setBounds(x, rowY, knobW, knobH); x += knobW + space;
    mixSlider.setBounds(x, rowY, knobW, knobH); x += knobW + space;
    outGainSlider.setBounds(x, rowY, knobW, knobH);
    
    deltaButton.setBounds(x + 5, rowY + knobH, 60, 20);
    
    int bottomY = 410;
    int bottomH = bounds.getHeight() - 425;
    
    int scPanelW = 280;
    int scPanelX = bounds.getWidth() - scPanelW - 15;
    
    envelopeDrawer.setBounds(15, bottomY, bounds.getWidth() - scPanelW - 40, bottomH);

    int scX = scPanelX + 15;
    int scW = scPanelW - 30;
    
    scTriggerSelect.setBounds(scX, bottomY + 35, scW - 35, 25);
    scDeleteButton.setBounds(scX + scW - 30, bottomY + 35, 30, 25);
    scActionBox.setBounds(scX, bottomY + 70, scW, 25);
    scConditionBox.setBounds(scX, bottomY + 105, scW, 25);
    scRangeBox.setBounds(scX, bottomY + 140, scW, 25);
}

void AntigravityCompressorEditor::updateVisibility()
{
    int mode = modeComboBox.getSelectedItemIndex();
    ratioSlider.setVisible(mode == 0);
    shiftSlider.setVisible(mode == 1);
    targetSlider.setVisible(mode == 2);
    lockButton.setVisible(mode == 2);
    repaint();
}
