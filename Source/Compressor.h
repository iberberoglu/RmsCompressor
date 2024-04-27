#include <JuceHeader.h>

class Compressor {
public:
    Compressor()
        : thresholdDB(-20.0f), ratio(1.0f), attackTimeMS(300.0f), releaseTimeMS(500.0f),
          sampleRate(44100.0f), gainReduction(1.0f) {}

    void setSampleRate(float newSampleRate) {
        sampleRate = newSampleRate;
    }

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
//        float thresholdLevel = juce::Decibels::decibelsToGain(thresholdDB);
//        float rmsLvl = juce::Decibels::decibelsToGain(rmsLevel);
//
////        if (rmsLvl > thresholdLevel) {
//////            float compressionAmount = 1.0f - (1.0f / ratio);
//////            float compressedSample = input * (1.0f - compressionAmount);
////            
////            float excessLevel = rmsLvl - thresholdLevel;
////            float gainReduction = 1.0f - (1.0f / ratio);
////            float compressedSample = input * (thresholdLevel + (excessLevel * gainReduction));
//
////            
////            return compressedSample;
////        }
//        
//        if (rmsLvl > thresholdLevel) {
//            float excessDB = juce::Decibels::gainToDecibels(rmsLvl) - thresholdDB; // Eşiği ne kadar aştığını hesapla (dB cinsinden)
//            float allowedIncreaseDB = excessDB / ratio; // İzin verilen artış miktarı (dB cinsinden)
//            float targetDB = thresholdDB + allowedIncreaseDB; // Hedef dB seviyesi
//
//            float targetGain = juce::Decibels::decibelsToGain(targetDB);
//            float compressedSample = input * targetGain / rmsLvl; // Kazanç oranına göre sıkıştır
//
//            return compressedSample;
//        }
//
//        return input;
        
        float thresholdLevel = juce::Decibels::decibelsToGain(thresholdDB);
        float rmsLvl = juce::Decibels::decibelsToGain(rmsLevel);

        float targetGain;
        if (rmsLvl > thresholdLevel) {
            float excessDB = juce::Decibels::gainToDecibels(rmsLvl) - thresholdDB;
            float allowedIncreaseDB = excessDB / ratio;
            float targetDB = thresholdDB + allowedIncreaseDB;
            targetGain = juce::Decibels::decibelsToGain(targetDB);

            float attackCoefficient = std::exp(-1.0 / (0.001 * attackTimeMS * sampleRate));
            gainReduction = attackCoefficient * gainReduction + (1 - attackCoefficient) * targetGain;
        } else {
            float releaseCoefficient = std::exp(-1.0 / (0.001 * releaseTimeMS * sampleRate));
            gainReduction = releaseCoefficient * gainReduction + (1 - releaseCoefficient) * 1.0f; // Release to no compression
        }

        return input * gainReduction;
    }

private:
    float thresholdDB;
    float ratio;
    float attackTimeMS;
    float releaseTimeMS;
    float sampleRate;
    float gainReduction;
};
