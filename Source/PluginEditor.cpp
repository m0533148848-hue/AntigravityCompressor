#include "PluginProcessor.h"
#include "PluginEditor.h"

// =========================================================================
// הלוגיקה והציור של תצוגת האודיו העליונה
// =========================================================================
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
// הלוגיקה של צייר המעטפות (LFO)
// =========================================================================
EnvelopeDrawer::EnvelopeDrawer() {
    // יצירת נקודות התחלה וסיום כברירת מחדל
    points.push_back({0.0f, 0.5f});
    points.push_back({1.0f, 0.5f});
}

void EnvelopeDrawer::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff2a2a3e));
    g.setColour(juce::Colour(0xffff0055));
    g.drawRect(getLocalBounds(), 2);
    
    // ציור קווי רשת עדינים ברקע
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    for (int i = 1; i < 4; ++i) g.drawHorizontalLine(getHeight() * i / 4, 0, getWidth());
    for (int i = 1; i < 8; ++i) g.drawVerticalLine(getWidth() * i / 8, 0, getHeight());

    if (points.empty()) return;

    // המרת הנקודות שלנו לקואורדינטות של המסך
    juce::Path envPath;
    for (size_t i = 0; i < points.size(); ++i) {
        float x = points[i].x * getWidth();
        float y = points[i].y * getHeight();
        if (i == 0) envPath.startNewSubPath(x, y);
        else envPath.lineTo(x, y);
    }
    
    // ציור הקווים המחברים בין הנקודות
    g.setColour(juce::Colour(0xffff0055));
    g.strokePath(envPath, juce::PathStrokeType(2.0f));

    // ציור הנקודות עצמן (עיגולים לבנים)
    g.setColour(juce::Colours::white);
    for (auto point : points) {
        g.fillEllipse(point.x * getWidth() - 4, point.y * getHeight() - 4, 8, 8);
    }
}

void EnvelopeDrawer::sortPoints() {
    // ממיין את הנקודות משמאל לימין, ונועל את הראשונה והאחרונה בקצוות
    std::sort(points.begin(), points.end(), [](const juce::Point<float>& a, const juce::Point<float>& b) { return a.x < b.x; });
    if (!points.empty()) { points.front().x = 0.0f; points.back().x = 1.0f; }
}

void EnvelopeDrawer::mouseDown(const juce::MouseEvent& e) {
    float mx = e.position.x / getWidth();
    float my = e.position.y / getHeight();
    draggingIndex = -1;
    // מחפש אם לחצת קרוב לנקודה קיימת כדי להתחיל לגרור אותה
    for (size_t i = 0; i < points.size(); ++i) {
        if (std::abs(points[i].x - mx) < 0.05f && std::abs(points[i].y - my) < 0.05f) {
            draggingIndex = (int)i;
            break;
        }
    }
}

void EnvelopeDrawer::mouseDrag(const juce::MouseEvent& e) {
    if (draggingIndex >= 0) {
        float mx = juce::jlimit(0.0f, 1.0f, e.position.x / getWidth());
        float my = juce::jlimit(0.0f, 1.0f, e.position.y / getHeight());
        points[draggingIndex].y = my; // הגובה תמיד ניתן לשינוי
        
        // לא מאפשר להזיז את נקודת ההתחלה והסיום בציר ה-X
        if (draggingIndex != 0 && draggingIndex != points.size() - 1) {
            // נועל כדי שנקודה לא תעבור את השכנה שלה
            float prevX = points[draggingIndex - 1].x;
            float nextX = points[draggingIndex + 1].x;
            points[draggingIndex].x = juce::jlimit(prevX + 0.01f, nextX - 0.01f, mx);
        }
        repaint();
    }
}

void EnvelopeDrawer::mouseDoubleClick(const juce::MouseEvent& e) {
    float mx = e.position.x / getWidth();
    float my = e.position.y / getHeight();
    
    // בודק אם עשינו דאבל קליק על נקודה קיימת כדי למחוק אותה
    for (size_t i = 1; i < points.size() - 1; ++i) {
        if (std::abs(points[i].x - mx) < 0.05f && std::abs(points[i].y - my) < 0.05f) {
            points.erase(points.begin() + i);
            repaint();
            return;
        }
    }
    // אם לא, מוסיף נקודה חדשה וממיין
    points.push_back({mx, my});
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

    // Sidechain UI and APVTS Attachments
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

    // חלון קומפקטי ונעים יותר למשתמש כברירת מחדל
    setResizable(true, true);
    setResizeLimits(800, 500, 1600, 1000);
    setSize(950, 600); 
    
    startTimerHz(30); 
}

AntigravityCompressorEditor::~AntigravityCompressorEditor() { stopTimer(); }

void AntigravityCompressorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour(0xff181822)); // רקע קצת יותר כהה ומקצועי
    g.setColour (juce::Colours::cyan);
    g.setFont (24.0f);
    g.drawFittedText ("Antigravity Compressor Engine", 0, 10, getWidth(), 30, juce::Justification::centred, 1);

    g.setColour (juce::Colours::white);
    g.setFont (13.0f);
    
    // טקסטים לכפתורים מסודרים במדויק מתחת לכפתורים
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

    // מטר ההנחתה בימין הקיצון
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

    // רקע לסיידצ'יין
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
    
    // תיקון מיקום הטקסט של נעילת הטארגט
    lockButton.setBounds(x - 5, rowY + knobH, 90, 20);
    x += knobW + space;
    
    kneeSlider.setBounds(x, rowY, knobW, knobH); x += knobW + space;
    mixSlider.setBounds(x, rowY, knobW, knobH); x += knobW + space;
    outGainSlider.setBounds(x, rowY, knobW, knobH);
    
    deltaButton.setBounds(x + 5, rowY + knobH, 60, 20);
    
    // עיצוב החלק התחתון
    int bottomY = 410;
    int bottomH = bounds.getHeight() - 425;
    
    int scPanelW = 280;
    int scPanelX = bounds.getWidth() - scPanelW - 15;
    
    // חלון ה-LFO ממלא את כל השטח עד פאנל הסיידצ'יין
    envelopeDrawer.setBounds(15, bottomY, bounds.getWidth() - scPanelW - 40, bottomH);

    // סידור אלגנטי של תפריטי ה-Sidechain
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
