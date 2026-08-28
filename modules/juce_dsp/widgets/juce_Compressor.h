/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 7 End-User License
   Agreement and JUCE Privacy Policy.

   End User License Agreement: www.juce.com/juce-7-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce::dsp
{

/**
    A simple compressor with standard threshold, ratio, attack time and release time
    controls.

    @tags{DSP}
*/
template <typename SampleType>
class Compressor
{
public:
    //==============================================================================
    /** Constructor. */
    Compressor();

    //==============================================================================
    /** Sets the threshold in dB of the compressor.*/
    void setThreshold (SampleType newThreshold);

    /** Sets the ratio of the compressor (must be higher or equal to 1).*/
    void setRatio (SampleType newRatio);

    /** Sets the attack time in milliseconds of the compressor.*/
    void setAttack (SampleType newAttack);

    /** Sets the release time in milliseconds of the compressor.*/
    void setRelease (SampleType newRelease);

    //==============================================================================
    /** Initialises the processor. */
    void prepare (const ProcessSpec& spec);

    /** Resets the internal state variables of the processor. */
    void reset();

    //==============================================================================
    /** Processes the input and output samples supplied in the processing context. */
    template <typename ProcessContext>
    void process (const ProcessContext& context, const std::vector<float>& rmsLevels, bool peak, bool rms, double attackCoef, double releaseCoef) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numChannels = outputBlock.getNumChannels();
        const auto numSamples  = outputBlock.getNumSamples();

        jassert (inputBlock.getNumChannels() == numChannels);
        jassert (inputBlock.getNumSamples()  == numSamples);

        if (context.isBypassed || bypassed)
        {
            outputBlock.copyFrom (inputBlock);
            return;
        }

        for (size_t channel = 0; channel < numChannels; ++channel)
        {
            auto* inputSamples  = inputBlock .getChannelPointer (channel);
            auto* outputSamples = outputBlock.getChannelPointer (channel);

            for (size_t i = 0; i < numSamples; ++i){
                
                inputDB = juce::Decibels::gainToDecibels(std::abs(inputSamples[i]));
                
                outputSamples[i] = processSample((int) channel, inputSamples[i], rmsLevels, peak, rms, attackCoef, releaseCoef);
                
                outputDB = juce::Decibels::gainToDecibels(std::abs(outputSamples[i]));
                
                gainReductionDb = inputDB - outputDB;
                
                outputSamples[i] *= juce::Decibels::decibelsToGain(makeupGainValue);
            }
        }
    }

    /** Performs the processing operation on a single sample at a time. */
    SampleType processSample (int channel, SampleType inputValue, const std::vector<float>& rmsLevels, bool peak, bool rms, double attackCoef, double releaseCoef);
    
    float gainReductionFunc()
    {
        return gainReductionDb;
    }
       
    void setMakeupGain(float newMakeUp)
    {
        makeupGainValue = newMakeUp;
    }
       
    void setBypass(bool bypass)
    {
        bypassed = bypass;
    }

private:
    //==============================================================================
    void update();
    
    float makeupGainValue = 0.0f;
    float gainReductionDb;
    double inputDB;
    double outputDB;
    bool bypassed = false;

    //==============================================================================
    SampleType threshold, thresholdInverse, ratioInverse;
    BallisticsFilter<SampleType> envelopeFilter;

    double sampleRate = 44100.0;
    SampleType thresholddB = 0.0, ratio = 1.0, attackTime = 15.0, releaseTime = 50.0;
};

} // namespace juce::dsp
