/* editor .cpp
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
RMSCompressorAudioProcessorEditor::RMSCompressorAudioProcessorEditor (RMSCompressorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    rmsPeriodAttachment(p.getApvts(), "rmsPeriod", rmsPeriodSlider),
    enableSmoothingAttachment(p.getApvts(), "smoothing", enableSmoothingButton),
    thresholdAttachment(p.getApvts(), "threshold", thresholdSlider)
{
    addAndMakeVisible(rmsLevelHeading1);
    addAndMakeVisible(rmsLevelHeading2);
    addAndMakeVisible(currentRmsLabel);
    addAndMakeVisible(maxRmsLabel);
    addAndMakeVisible(currentRmsValue);
    addAndMakeVisible(maxRmsValue);
    addAndMakeVisible(rmsPeriodLabel);

    addAndMakeVisible(rmsPeriodSlider);
    addAndMakeVisible(enableSmoothingButton);
    
    addAndMakeVisible(thresholdSlider);
    
    thresholdSlider.setRange(-60.0, 0.0, 0.1);
    thresholdSlider.setSliderStyle(Slider::LinearHorizontal);
    thresholdSlider.setTextBoxStyle(Slider::TextBoxRight, false, 100, 20);
    thresholdSlider.setPopupDisplayEnabled(true, true, this);
    thresholdSlider.setTextValueSuffix(" dB");

    rmsLevelHeading1.setText("dBFS", dontSendNotification);
    rmsLevelHeading1.setFont(Font{}.withStyle(Font::FontStyleFlags::bold));
    rmsLevelHeading2.setText("Left \t Right", dontSendNotification);
    rmsLevelHeading2.setFont(Font{}.withStyle(Font::FontStyleFlags::bold));
    currentRmsLabel.setText("Current RMS:", dontSendNotification);
    maxRmsLabel.setText("Max RMS:", dontSendNotification);
    rmsPeriodLabel.setText("RMS Period", dontSendNotification);
    rmsPeriodLabel.setJustificationType(Justification::right);

    rmsPeriodSlider.setSliderStyle(Slider::LinearHorizontal);
    rmsPeriodSlider.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
    rmsPeriodSlider.setPopupDisplayEnabled(true, false, this);
    rmsPeriodSlider.setTextValueSuffix(" ms");

    enableSmoothingButton.setButtonText("Enable smoothing");

    setSize (400, 500);
    setResizable(true, true);
    setResizeLimits(400, 400, 1000, 1000);
    startTimerHz(24);
}

RMSCompressorAudioProcessorEditor::~RMSCompressorAudioProcessorEditor()
{
    stopTimer();
}

void RMSCompressorAudioProcessorEditor::timerCallback()
{
    if (++framesElapsed > 100)
    {
        framesElapsed = 0;
        maxRmsLeft = -100.f;
        maxRmsRight = -100.f;
    }

    const auto leftGain = audioProcessor.getRmsLevel(0);
    const auto rightGain = audioProcessor.getRmsLevel(1);
    if (leftGain > maxRmsLeft)
        maxRmsLeft = leftGain;
    if (rightGain > maxRmsRight)
        maxRmsRight = rightGain;
    currentRmsValue.setText(String{ leftGain, 2 } + "   " + String{ rightGain, 2 }, sendNotification);
    maxRmsValue.setText(String{ maxRmsLeft, 2 } + "   " + String{ maxRmsRight, 2 }, sendNotification);
}

//==============================================================================
void RMSCompressorAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.setGradientFill(ColourGradient{ Colours::darkgrey, getLocalBounds().toFloat().getCentre(), Colours::darkgrey.darker(0.7f), {}, true });
    g.fillRect(getLocalBounds());

    g.setColour(Colours::black);
}

void RMSCompressorAudioProcessorEditor::resized()
{
    const auto container = getBounds().reduced(20);
    auto bounds = container;

    auto labelBounds = bounds.removeFromTop(container.proportionOfHeight(0.12f));
    auto controlBounds = labelBounds.removeFromRight(container.proportionOfWidth(0.35f));

    const auto labelHeight = labelBounds.proportionOfHeight(0.33f);

    auto labelRow1 = labelBounds.removeFromTop(labelHeight);
    rmsLevelHeading1.setBounds(labelRow1.removeFromLeft(labelRow1.proportionOfWidth(0.5f)));
    rmsLevelHeading2.setBounds(labelRow1);

    auto labelRow2 = labelBounds.removeFromTop(labelHeight);
    maxRmsLabel.setBounds(labelRow2.removeFromLeft(labelRow2.proportionOfWidth(0.5f)));
    maxRmsValue.setBounds(labelRow2);

    auto labelRow3 = labelBounds;
    currentRmsLabel.setBounds(labelRow3.removeFromLeft(labelRow3.proportionOfWidth(0.5f)));
    currentRmsValue.setBounds(labelRow3);

    rmsPeriodLabel.setBounds(controlBounds.removeFromTop(labelHeight));
    rmsPeriodSlider.setBounds(controlBounds.removeFromTop(labelHeight));
    enableSmoothingButton.setBounds(controlBounds);
    
    thresholdSlider.setBounds(50, 250, 300, 100);
}
