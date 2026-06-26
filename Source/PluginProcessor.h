#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

class AntigravityCompressorEditor;

class AntigravityCompressorProcessor : public juce::AudioProcessor {
public:
    AntigravityCompressorProcessor();
    ~AntigravityCompressorProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "AntigravityCompressor"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return {}; }
    void changeProgramName (int index, const juce::String& newName) override {}

    void getStateInformation (juce::MemoryBlock& destData) override {}
    void setStateInformation (const void* data, int sizeInBytes) override {}

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float> currentInPeak { 0.0f };
    std::atomic<float> currentOutPeak { 0.0f };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AntigravityCompressorProcessor)
};
