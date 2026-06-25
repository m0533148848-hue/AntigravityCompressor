#include "PluginProcessor.h"
#include "PluginEditor.h"

// ==============================================================================
// אתחול הפלאגין וחיבור מערכת הפרמטרים
// ==============================================================================
AntigravityCompressorProcessor::AntigravityCompressorProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       apvts (*this, nullptr, "Parameters", createParameters())
{
}

AntigravityCompressorProcessor::~AntigravityCompressorProcessor() {}

// ==============================================================================
// הגדרת הכפתורים והטווחים של הקומפרסור המיוחד שלנו
// ==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AntigravityCompressorProcessor::createParameters() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    // 1. הגדרת טווח הפעולה (חלון הטרשהולד)
    params.push_back(std::make_unique<juce::AudioParameterFloat>("THRESH_UP", "Upper Threshold (dB)", -60.0f, 0.0f, -10.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("THRESH_DOWN", "Lower Threshold (dB)", -60.0f, 0.0f, -30.0f));
    
    // 2. בחירת מצב הפעולה
    params.push_back(std::make_unique<juce::AudioParameterChoice>("MODE", "Processing Mode", 
        juce::StringArray{"Ratio (Relative)", "Shift (Arbitrary)", "Target (Absolute)"}, 0));
    
    // 3. פרמטרים שמשתנים בהתאם למצב הנבחר
    params.push_back(std::make_unique<juce::AudioParameterFloat>("RATIO", "Ratio", 1.0f, 20.0f, 2.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SHIFT", "Shift Volume (dB)", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TARGET", "Target Level (dB)", -60.0f, 0.0f, -20.0f));
    
    // 4. כפתור הנעילה החכם (נועל את היעד לטרשהולד התחתון)
    params.push_back(std::make_unique<juce::AudioParameterBool>("LOCK_TO_LOWER", "Lock Target to Lower Thresh", false));
    
    return { params.begin(), params.end() };
}

void AntigravityCompressorProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {}
void AntigravityCompressorProcessor::releaseResources() {}

// ==============================================================================
// מנוע הסאונד (כאן קורית המתמטיקה של שינוי הווליום)
// ==============================================================================
void AntigravityCompressorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // ניקוי ערוצים מיותרים
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // משיכת הערכים מכל הכפתורים שלנו
    float threshUp = apvts.getRawParameterValue("THRESH_UP")->load();
    float threshDown = apvts.getRawParameterValue("THRESH_DOWN")->load();
    int mode = static_cast<int>(apvts.getRawParameterValue("MODE")->load());
    float ratio = apvts.getRawParameterValue("RATIO")->load();
    float shift = apvts.getRawParameterValue("SHIFT")->load();
    float target = apvts.getRawParameterValue("TARGET")->load();
    bool lockToLower = apvts.getRawParameterValue("LOCK_TO_LOWER")->load() > 0.5f;

    // לוגיקת הנעילה: אם הכפתור לחוץ, היעד הוא הטרשהולד התחתון
    if (lockToLower) {
        target = threshDown;
    }

    // מעבר על כל הסאונד שנכנס, דגימה אחר דגימה
    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        auto* channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            float inSample = channelData[sample];
            
            // הגנה מתמטית משקט מוחלט (מניעת קריסה בחישוב דציבלים)
            if (std::abs(inSample) < 0.000001f) continue;

            // המרה לדציבלים כדי לבדוק אם אנחנו בתוך הטווח
            float inDb = juce::Decibels::gainToDecibels(std::abs(inSample));

            // האם הווליום הנוכחי נמצא בתוך החלון שהגדרנו?
            if (inDb > threshDown && inDb < threshUp) {
                float outDb = inDb;

                // הפעלת הפעולה לפי מה שהמשתמש בחר
                if (mode == 0) { 
                    // מצב יחס דחיסה (Ratio)
                    outDb = threshDown + ((inDb - threshDown) / ratio);
                } 
                else if (mode == 1) { 
                    // מצב הסטה שרירותית (Shift)
                    outDb = inDb + shift;
                } 
                else if (mode == 2) { 
                    // מצב יעד מוחלט (Target)
                    outDb = target; 
                }

                // המרה חזרה מדציבלים לווליום רגיל (Gain)
                float outGain = juce::Decibels::decibelsToGain(outDb);
                
                // שמירה על הקוטביות של הגל המקורי (פלוס או מינוס)
                channelData[sample] = (inSample > 0.0f) ? outGain : -outGain;
            }
        }
    }
}

// ==============================================================================
// חיבור הממשק הגרפי (Editor)
// ==============================================================================
juce::AudioProcessorEditor* AntigravityCompressorProcessor::createEditor() {
    return new AntigravityCompressorEditor (*this);
}

// פונקציית חובה של JUCE שיוצרת את הפלאגין בתוכנת העריכה (DAW)
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new AntigravityCompressorProcessor();
}
