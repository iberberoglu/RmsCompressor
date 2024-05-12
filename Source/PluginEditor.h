/* plugineditor . H
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GainReductionMeter.h"

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
    juce::Label thresholdLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment thresholdAttachment;
    juce::Slider attackTimeSlider;
    juce::Label attackTimeLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment attackTimeAttachment;
    juce::Slider releaseTimeSlider;
    juce::Label releaseTimeLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment releaseTimeAttachment;
    juce::Slider ratioSlider;
    juce::Label ratioLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment ratioAttachment;
    juce::Slider makeupGainSlider;
    juce::Label makeupGainLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment makeupGainAttachment;
    juce::ToggleButton bypassToggle;
    juce::AudioProcessorValueTreeState::ButtonAttachment bypassToggleAttachment;
    
    juce::ToggleButton peakButton;
    juce::AudioProcessorValueTreeState::ButtonAttachment peakButtonAttachment;
    
    juce::ToggleButton rmsButton;
    juce::AudioProcessorValueTreeState::ButtonAttachment rmsButtonAttachment;
    
    juce::ToggleButton variableSizedRmsButton;
    juce::AudioProcessorValueTreeState::ButtonAttachment variableSizedRmsButtonAttachment;
    
    juce::Label rmsLevelHeading1, rmsLevelHeading2;
    juce::Label currentRmsLabel, maxRmsLabel;
    juce::Label currentRmsValue, maxRmsValue;
    juce::Label rmsPeriodLabel;
    float maxRmsLeft{}, maxRmsRight{};
    int framesElapsed = 0;
    
    GainReductionMeter gainReductionMeter;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RMSCompressorAudioProcessorEditor)
};
