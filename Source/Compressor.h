//#include "../JuceLibraryCode/JuceHeader.h"
//
//class RMSCompressor
//{
//public:
//    RMSCompressor() = default;
//    ~RMSCompressor() = default;
//
//    void prepare(const juce::dsp::ProcessSpec& spec)
//    {
//        sampleRate = spec.sampleRate;
//        rmsInput.setSize(1, spec.maximumBlockSize);  // RMS değerini depolamak için buffer
//        rmsInput.clear();
//        compressorEnabled = true;
//    }
//
//    void setThreshold(float dB)
//    {
//        threshold = juce::Decibels::decibelsToGain(dB);  // Threshold dB cinsinden alınır ve gain'e çevrilir
//    }
//
//    void setAttack(float ms)
//    {
//        attack = ms;
//    }
//
//    void setRelease(float ms)
//    {
//        release = ms;
//    }
//
//    void setRatio(float compressionRatio)
//    {
//        ratio = compressionRatio;
//    }
//    
//    void setSampleRate(float sr)
//    {
//        sampleRate = sr;
//    }
//
//    void process(juce::AudioBuffer<float>& buffer, float rmsValue)
//    {
//        if (!compressorEnabled)
//            return;
//
//        auto numSamples = buffer.getNumSamples();
//        auto numChannels = buffer.getNumChannels();
//
//        for (int sample = 0; sample < numSamples; ++sample)
//        {
//            float currentGain = 1.0f;
//            if (rmsValue > threshold)
//            {
//                float excess = rmsValue - threshold;
//                float gainReduction = excess - (excess / ratio);
//                currentGain = juce::Decibels::decibelsToGain(-gainReduction);
//            }
//
//            currentGain = gainSmoothing(currentGain, sampleRate);
//
//            for (int channel = 0; channel < numChannels; ++channel)
//            {
//                buffer.getWritePointer(channel)[sample] *= currentGain;
//            }
//        }
//    }
//
//    void enableCompressor(bool enable)
//    {
//        compressorEnabled = enable;
//    }
//
//private:
//    bool compressorEnabled = true;
//    float threshold = 1.0f;  // Default olarak 0 dB
//    float attack = 10.0f;    // ms cinsinden
//    float release = 100.0f;  // ms cinsinden
//    float ratio = 4.0f;      // Oran
//    float sampleRate = 44100.0f;  // Varsayılan örnek oranı
//
//    juce::AudioBuffer<float> rmsInput;
//
//    float gainSmoothing(float targetGain, float sampleRate)
//    {
//        // Gain değerini yavaşlatarak (attack/release süreleri dikkate alınarak) uygular
//        static float smoothedGain = 1.0f;
//        float smoothingRate = (targetGain < smoothedGain) ? (attack / sampleRate) : (release / sampleRate);
//        smoothedGain += (targetGain - smoothedGain) * smoothingRate;
//        return smoothedGain;
//    }
//};
