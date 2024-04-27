/* plugineditor . H
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class RMSCompressorAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    RMSCompressorAudioProcessorEditor (RMSCompressorAudioProcessor&);
    ~RMSCompressorAudioProcessorEditor() override;

    //==============================================================================
    void timerCallback() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    RMSCompressorAudioProcessor& audioProcessor;
    
    juce::Slider rmsPeriodSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment rmsPeriodAttachment;
    juce::ToggleButton enableSmoothingButton;
    juce::AudioProcessorValueTreeState::ButtonAttachment enableSmoothingAttachment;
    juce::Slider thresholdSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment thresholdAttachment;
    juce::Slider attackTimeSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment attackTimeAttachment;
    juce::Slider releaseTimeSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment releaseTimeAttachment;
    
    juce::Label rmsLevelHeading1, rmsLevelHeading2;
    juce::Label currentRmsLabel, maxRmsLabel;
    juce::Label currentRmsValue, maxRmsValue;
    juce::Label rmsPeriodLabel;
    float maxRmsLeft{}, maxRmsRight{};
    int framesElapsed = 0;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RMSCompressorAudioProcessorEditor)
};
