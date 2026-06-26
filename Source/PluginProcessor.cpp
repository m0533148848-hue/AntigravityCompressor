#include "PluginProcessor.h"
#include "PluginEditor.h"

AntigravityCompressorProcessor::AntigravityCompressorProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       apvts (*this, nullptr, "Parameters", createParameters())
{
}

AntigravityCompressorProcessor::~AntigravityCompressorProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout AntigravityCompressorProcessor::createParameters() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>("THRESH_UP", "Upper Threshold (dB)", -60.0f, 0.0f, -10.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("THRESH_DOWN", "Lower Threshold (dB)", -60.0f, 0.0f, -30.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("MODE", "Processing Mode", 
        juce::StringArray{"Ratio (Relative)", "Shift (Arbitrary)", "Target (Absolute)"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("RATIO", "Ratio", 1.0f, 20.0f, 2.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SHIFT", "Shift Volume (dB)", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TARGET", "Target Level (dB)", -60.0f, 0.0f, -20.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("LOCK_TO_LOWER", "Lock Target to Lower Thresh", false));
    
    return { params.begin(), params.end() };
}

void AntigravityCompressorProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {}
void AntigravityCompressorProcessor::releaseResources() {}

void AntigravityCompressorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    float threshUp = apvts.getRawParameterValue("THRESH_UP")->load();
    float threshDown = apvts.getRawParameterValue("THRESH_DOWN")->load();
    int mode = static_cast<int>(apvts.getRawParameterValue("MODE")->load());
    float ratio = apvts.getRawParameterValue("RATIO")->load();
    float shift = apvts.getRawParameterValue("SHIFT")->load();
    float target = apvts.getRawParameterValue("TARGET")->load();
    bool lockToLower = apvts.getRawParameterValue("LOCK_TO_LOWER")->load() > 0.5f;

    if (lockToLower) { target = threshDown; }

    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        auto* channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            float inSample = channelData[sample];
            float finalSample = inSample; // מצב ברירת מחדל: לא משנים כלום

            if (std::abs(inSample) > 0.000001f) {
                float inDb = juce::Decibels::gainToDecibels(std::abs(inSample));

                if (inDb > threshDown && inDb < threshUp) {
                    float outDb = inDb;
                    if (mode == 0) { outDb = threshDown + ((inDb - threshDown) / ratio); } 
                    else if (mode == 1) { outDb = inDb + shift; } 
                    else if (mode == 2) { outDb = target; }

                    float outGain = juce::Decibels::decibelsToGain(outDb);
                    finalSample = (inSample > 0.0f) ? outGain : -outGain;
                }
            }

            channelData[sample] = finalSample;

            // שמירת הדגימה בזיכרון של המסך (רק לערוץ השמאלי/ראשי כדי לא להכביד)
            if (channel == 0) {
                int index = visualBufferIndex.load();
                inVisualBuffer[index] = inSample;
                outVisualBuffer[index] = finalSample;
                visualBufferIndex.store((index + 1) % visualBufferSize);
            }
        }
    }
}

juce::AudioProcessorEditor* AntigravityCompressorProcessor::createEditor() {
    return new AntigravityCompressorEditor (*this);
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new AntigravityCompressorProcessor();
}
