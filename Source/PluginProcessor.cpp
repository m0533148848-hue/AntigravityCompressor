#include "PluginProcessor.h"

AntigravityCompressorAudioProcessor::AntigravityCompressorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
}

AntigravityCompressorAudioProcessor::~AntigravityCompressorAudioProcessor()
{
}

// בניית עץ הפרמטרים מתוך המפרט שהגדרנו
juce::AudioProcessorValueTreeState::ParameterLayout AntigravityCompressorAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Gain In / Out: -24dB to +24dB
    layout.add(std::make_unique<juce::AudioParameterFloat>("GAIN_IN", "Gain In", -24.0f, 24.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("GAIN_OUT", "Gain Out", -24.0f, 24.0f, 0.0f));

    // Thresholds (Lower/Upper): -80dB to 0dB
    layout.add(std::make_unique<juce::AudioParameterFloat>("LOWER_THRESH", "Lower Threshold", -80.0f, 0.0f, -20.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("UPPER_THRESH", "Upper Threshold", -80.0f, 0.0f, 0.0f));

    // Ratio: 0.1 to 100 (Covers negative, upward, and limiter modes mathematically)
    layout.add(std::make_unique<juce::AudioParameterFloat>("RATIO", "Ratio", 0.1f, 100.0f, 4.0f));

    return layout;
}

const juce::String AntigravityCompressorAudioProcessor::getName() const { return "Antigravity Compressor"; }
bool AntigravityCompressorAudioProcessor::acceptsMidi() const { return false; }
bool AntigravityCompressorAudioProcessor::producesMidi() const { return false; }
bool AntigravityCompressorAudioProcessor::isMidiEffect() const { return false; }
double AntigravityCompressorAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int AntigravityCompressorAudioProcessor::getNumPrograms() { return 1; }
int AntigravityCompressorAudioProcessor::getCurrentProgram() { return 0; }
void AntigravityCompressorAudioProcessor::setCurrentProgram (int index) {}
const juce::String AntigravityCompressorAudioProcessor::getProgramName (int index) { return {}; }
void AntigravityCompressorAudioProcessor::changeProgramName (int index, const juce::String& newName) {}

void AntigravityCompressorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // הכנות של מנוע האודיו יכנסו לכאן בהמשך
}

void AntigravityCompressorAudioProcessor::releaseResources()
{
}

void AntigravityCompressorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // אזור זה יעבד את האודיו בפועל ויחיל עליו את דחיסת הסאונד
}

bool AntigravityCompressorAudioProcessor::hasEditor() const
{
    return false; // בינתיים משתמשים בממשק הגנרי של ה-DAW
}

juce::AudioProcessorEditor* AntigravityCompressorAudioProcessor::createEditor()
{
    return nullptr;
}

void AntigravityCompressorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
}

void AntigravityCompressorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AntigravityCompressorAudioProcessor();
}
