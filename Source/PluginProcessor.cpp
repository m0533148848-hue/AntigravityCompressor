#include "PluginProcessor.h"
#include "PluginEditor.h"

// הוספנו כניסת Sidechain ל-BusesProperties
AntigravityCompressorProcessor::AntigravityCompressorProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                       .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)), // כניסה אופציונלית
       apvts (*this, nullptr, "Parameters", createParameters())
{
}

AntigravityCompressorProcessor::~AntigravityCompressorProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout AntigravityCompressorProcessor::createParameters() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>("IN_GAIN", "In Gain (dB)", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("OUT_GAIN", "Out Gain (dB)", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("THRESH_UP", "Upper Thresh", -60.0f, 0.0f, -10.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("THRESH_DOWN", "Lower Thresh", -60.0f, 0.0f, -30.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("MODE", "Mode", juce::StringArray{"Ratio", "Shift", "Target"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("RATIO", "Ratio", 1.0f, 20.0f, 2.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SHIFT", "Shift (dB)", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TARGET", "Target (dB)", -60.0f, 0.0f, -20.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("LOCK_TO_LOWER", "Lock Target", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("KNEE", "Knee (%)", 0.0f, 100.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MIX", "Mix (%)", 0.0f, 100.0f, 100.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("DELTA", "Delta", false));

    // פרמטרים חדשים לסיידצ'יין מתוך התפריטים
    params.push_back(std::make_unique<juce::AudioParameterChoice>("SC_ACTION", "SC Action", juce::StringArray{"Cut Volume", "Boost Volume"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("SC_COND", "SC Condition", juce::StringArray{"Signal Present", "Signal Absent"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("SC_RANGE", "SC Range", juce::StringArray{"Inside Threshold", "Outside Threshold"}, 0));

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

    float inGainLinear = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("IN_GAIN")->load());
    float outGainLinear = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("OUT_GAIN")->load());
    float threshUp = apvts.getRawParameterValue("THRESH_UP")->load();
    float threshDown = apvts.getRawParameterValue("THRESH_DOWN")->load();
    int mode = static_cast<int>(apvts.getRawParameterValue("MODE")->load());
    float ratio = apvts.getRawParameterValue("RATIO")->load();
    float shift = apvts.getRawParameterValue("SHIFT")->load();
    float target = apvts.getRawParameterValue("TARGET")->load();
    bool lockToLower = apvts.getRawParameterValue("LOCK_TO_LOWER")->load() > 0.5f;
    float mix = apvts.getRawParameterValue("MIX")->load() / 100.0f;
    bool delta = apvts.getRawParameterValue("DELTA")->load() > 0.5f;

    // קריאת פרמטרי ה-Sidechain
    int scAction = static_cast<int>(apvts.getRawParameterValue("SC_ACTION")->load());
    int scCondition = static_cast<int>(apvts.getRawParameterValue("SC_COND")->load());
    
    if (lockToLower) { target = threshDown; }

    // האזנה לערוץ ה-Sidechain (אם הוא מחובר)
    auto scBus = getBusBuffer(buffer, true, 1); 
    bool hasSidechain = (scBus.getNumChannels() > 0);

    float maxIn = 0.0f; float maxOut = 0.0f; float maxGR = 0.0f;

    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        auto* channelData = buffer.getWritePointer(channel);
        const float* scData = hasSidechain ? scBus.getReadPointer(juce::jmin(channel, scBus.getNumChannels() - 1)) : nullptr;

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            float inSample = channelData[sample] * inGainLinear;
            float rawIn = inSample;
            float processedSample = inSample;
            
            // בדיקת סיידצ'יין - האם יש סיגנל שחצה סף מינימלי מסוים בערוץ המאזין?
            bool scSignalPresent = false;
            if (scData != nullptr) {
                scSignalPresent = (std::abs(scData[sample]) > 0.001f); // רף סיידצ'יין מינימלי בסיסי לאבטיפוס
            }

            // לוגיקת האם להפעיל את הפלאגין או לא לפי חוקי ה-Sidechain
            bool shouldProcess = true; 
            if (hasSidechain) {
                if (scCondition == 0 && !scSignalPresent) shouldProcess = false; // "רגיל" - עובד רק כשיש סאונד
                if (scCondition == 1 && scSignalPresent) shouldProcess = false;  // "הפוך" - עובד רק כשאין סאונד
            }

            if (shouldProcess && std::abs(inSample) > 0.000001f) {
                float inDb = juce::Decibels::gainToDecibels(std::abs(inSample));

                if (inDb > threshDown && inDb < threshUp) {
                    float outDb = inDb;
                    if (mode == 0) { outDb = threshDown + ((inDb - threshDown) / ratio); } 
                    else if (mode == 1) { outDb = inDb + shift; } 
                    else if (mode == 2) { outDb = target; }

                    // התאמת חיתוך/הגברה לפי סיידצ'יין (Action)
                    if (hasSidechain) {
                        if (scAction == 0) { outDb = juce::jmin(outDb, inDb); } // Cut Volume
                        if (scAction == 1) { outDb = juce::jmax(outDb, inDb); } // Boost Volume
                    }

                    float outGain = juce::Decibels::decibelsToGain(outDb);
                    processedSample = (inSample > 0.0f) ? outGain : -outGain;
                }
            }

            if (std::abs(rawIn) > 0.000001f && std::abs(processedSample) > 0.000001f) {
                float diffDb = juce::Decibels::gainToDecibels(std::abs(rawIn)) - juce::Decibels::gainToDecibels(std::abs(processedSample));
                if (diffDb > maxGR) maxGR = diffDb; 
            }

            if (delta) { processedSample = rawIn - processedSample; } 
            else { processedSample = (rawIn * (1.0f - mix)) + (processedSample * mix); }
            
            processedSample *= outGainLinear;
            channelData[sample] = processedSample;

            if (std::abs(inSample) > maxIn) maxIn = std::abs(inSample);
            if (std::abs(processedSample) > maxOut) maxOut = std::abs(processedSample);
        }
    }

    if (maxIn > currentInPeak.load()) currentInPeak.store(maxIn);
    if (maxOut > currentOutPeak.load()) currentOutPeak.store(maxOut);
    float currentGRVal = currentGR.load();
    currentGR.store(currentGRVal * 0.8f + maxGR * 0.2f);
}

juce::AudioProcessorEditor* AntigravityCompressorProcessor::createEditor() { return new AntigravityCompressorEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AntigravityCompressorProcessor(); }
