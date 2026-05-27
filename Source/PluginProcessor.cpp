#include "PluginProcessor.h"

// ==============================================================================
// חלק 1: מנוע הסאונד והפרמטרים
// ==============================================================================

AntigravityCompressorProcessor::AntigravityCompressorProcessor()
     : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
       apvts (*this, nullptr, "Parameters", createParameters())
{
}

AntigravityCompressorProcessor::~AntigravityCompressorProcessor() {}

// כאן אנחנו יוצרים את הפרמטרים שיופיעו בתוכנת הסאונד
juce::AudioProcessorValueTreeState::ParameterLayout AntigravityCompressorProcessor::createParameters() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>("THRESHOLD", "Threshold", -60.0f, 0.0f, -20.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("RATIO", "Ratio", 1.0f, 20.0f, 4.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ATTACK", "Attack (ms)", 0.1f, 100.0f, 10.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("RELEASE", "Release (ms)", 10.0f, 1000.0f, 100.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ANTIGRAVITY", "Antigravity Amount", 0.0f, 100.0f, 0.0f));
    
    return { params.begin(), params.end() };
}

void AntigravityCompressorProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
    // כאן נוסיף בהמשך את חישובי התדרים
}

void AntigravityCompressorProcessor::releaseResources() {}

void AntigravityCompressorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    
    // כרגע הסאונד פשוט עובר דרך הפלאגין כדי לוודא שהכל עובד חלק
    // בשלב הבא אנחנו נכניס לכאן את הלוגיקה של הקימפרוס
}

juce::AudioProcessorEditor* AntigravityCompressorProcessor::createEditor() {
    return new AntigravityCompressorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new AntigravityCompressorProcessor();
}


// ==============================================================================
// חלק 2: עיצוב ממשק המשתמש (החלון של הפלאגין)
// ==============================================================================

AntigravityCompressorEditor::AntigravityCompressorEditor (AntigravityCompressorProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p) {
    
    // גודל החלון של הפלאגין
    setSize (600, 400);
}

AntigravityCompressorEditor::~AntigravityCompressorEditor() {}

void AntigravityCompressorEditor::paint (juce::Graphics& g) {
    // צבע רקע
    g.fillAll (juce::Colour (0xff1a1a2e)); // כחול-כהה חללי

    // כותרת הפלאגין
    g.setColour (juce::Colours::cyan);
    g.setFont (30.0f);
    g.drawFittedText ("Antigravity Compressor", 0, 30, getWidth(), 40, juce::Justification::centred, 1);

    // טקסט זמני עד שנצייר את הכפתורים העגולים
    g.setColour (juce::Colours::white);
    g.setFont (18.0f);
    g.drawFittedText ("Threshold | Ratio | Attack | Release", 0, 150, getWidth(), 30, juce::Justification::centred, 1);
    
    g.setColour (juce::Colour (0xffff0055)); // ורוד ניאון לכפתור המיוחד
    g.setFont (22.0f);
    g.drawFittedText ("--- ANTIGRAVITY ENGINE ---", 0, 220, getWidth(), 30, juce::Justification::centred, 1);
}

void AntigravityCompressorEditor::resized() {
    // כאן נגדיר בהמשך את המיקומים של הכפתורים
}
