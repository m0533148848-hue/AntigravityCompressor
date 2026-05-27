#include "PluginProcessor.h"
#include "PluginEditor.h"

AntigravityCompressorProcessor::AntigravityCompressorProcessor()
     : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                        .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}

AntigravityCompressorProcessor::~AntigravityCompressorProcessor() {}

const juce::String AntigravityCompressorProcessor::getName() const { return "AntigravityCompressor"; }
bool AntigravityCompressorProcessor::acceptsMidi() const { return false; }
bool AntigravityCompressorProcessor::producesMidi() const { return false; }
bool AntigravityCompressorProcessor::isMidiEffect() const { return false; }
double AntigravityCompressorProcessor::getTailLengthSeconds() const { return 0.0; }
int AntigravityCompressorProcessor::getNumPrograms() { return 1; }
int AntigravityCompressorProcessor::getCurrentProgram() { return 0; }
void AntigravityCompressorProcessor::setCurrentProgram (int index) {}
const juce::String AntigravityCompressorProcessor::getProgramName (int index) { return {}; }
void AntigravityCompressorProcessor::changeProgramName (int index, const juce::String& newName) {}
void AntigravityCompressorProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {}
void AntigravityCompressorProcessor::releaseResources() {}

void AntigravityCompressorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
}

bool AntigravityCompressorProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* AntigravityCompressorProcessor::createEditor() { return new AntigravityCompressorEditor (*this); }
void AntigravityCompressorProcessor::getStateInformation (juce::MemoryBlock& destData) {}
void AntigravityCompressorProcessor::setStateInformation (const void* data, int sizeInBytes) {}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AntigravityCompressorProcessor(); }
