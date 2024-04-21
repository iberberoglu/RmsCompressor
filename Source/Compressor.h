#include <JuceHeader.h>

class Compressor {
public:
    Compressor() : thresholdDB(-20.0f), ratio(4.0f), attackTimeMS(10.0f), releaseTimeMS(100.0f) {}

    void setThreshold(float dB) {
        thresholdDB = dB;
    }
    
    //seviye görmek için değişip değişmedii
//    float threshOut(){
//        return thresholdDB;
//    }
    
    void setRatio(float newRatio) {
        ratio = newRatio;
    }

    void setAttackTime(float ms) {
        attackTimeMS = ms;
    }

    void setReleaseTime(float ms) {
        releaseTimeMS = ms;
    }

    float processSample(float input, float rmsLevel) {
        float thresholdLevel = juce::Decibels::decibelsToGain(thresholdDB);
        float rmsLvl = juce::Decibels::decibelsToGain(rmsLevel);
        //DBG("Threshold Level (gain): " << thresholdLevel << ", RMS Level (gain): " << rmsLvl);
        // Basit bir threshold kontrolü
        if (rmsLvl > thresholdLevel) {
            float compressionAmount = 1.0f - (1.0f / ratio);
            float compressedSample = input * (1.0f - compressionAmount);
            //DBG("Compressing: " << input << " to " << compressedSample);
            return compressedSample;
        }

        return input;
    }

private:
    float thresholdDB;
    float ratio;
    float attackTimeMS;
    float releaseTimeMS;
};
