/* plugin processor.h
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Fifo.h"
#include "Compressor.h"

//==============================================================================
/**
*/
class RMSCompressorAudioProcessor  : public juce::AudioProcessor,
                                     public juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    RMSCompressorAudioProcessor();
    ~RMSCompressorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    juce::AudioProcessorValueTreeState& getApvts() { return parameters; }
    
    std::vector<float> getRmsLevels();
    float getRmsLevel(const int channel);
    
    Compressor compressor;
    juce::AudioProcessorValueTreeState parameters;
    
private:
    
    void processLevelValue(juce::LinearSmoothedValue<float>&, const float value) const;
    
    std::vector<juce::LinearSmoothedValue<float>> rmsLevels;
    Utility::Fifo rmsFifo;
    juce::AudioBuffer<float> rmsCalculationBuffer;
    
    float thresholdValue = -20.0f;
    float attackTimeValue = 15.0f;
    float releaseTimeValue = 50.0f;
    int rmsWindowSize = 50;
    double sampleRate = 44'100.0;
    bool isSmoothed = true;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RMSCompressorAudioProcessor)
};
