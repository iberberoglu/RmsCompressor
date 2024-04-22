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
class RMSCompressorAudioProcessorEditor  : public juce::AudioProcessorEditor, public Timer
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
    
    Slider rmsPeriodSlider;
    AudioProcessorValueTreeState::SliderAttachment rmsPeriodAttachment;
    ToggleButton enableSmoothingButton;
    AudioProcessorValueTreeState::ButtonAttachment enableSmoothingAttachment;
    Slider thresholdSlider;
    AudioProcessorValueTreeState::SliderAttachment thresholdAttachment;
    
    Label rmsLevelHeading1, rmsLevelHeading2;
    Label currentRmsLabel, maxRmsLabel;
    Label currentRmsValue, maxRmsValue;
    Label rmsPeriodLabel;
    float maxRmsLeft{}, maxRmsRight{};
    int framesElapsed = 0;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RMSCompressorAudioProcessorEditor)
};
