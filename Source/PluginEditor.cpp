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
    thresholdAttachment(p.getApvts(), "threshold", thresholdSlider),
    attackTimeAttachment(p.getApvts(), "attackTime", attackTimeSlider),
    releaseTimeAttachment(p.getApvts(), "releaseTime", releaseTimeSlider),
    ratioAttachment(p.getApvts(), "ratio", ratioSlider),
    makeupGainAttachment(p.getApvts(), "makeupGain", makeupGainSlider),
    bypassToggleAttachment(p.getApvts(), "bypassButton", bypassToggle),
    peakToggleAttachment(p.getApvts(), "peakButton", peakToggle),
    rmsToggleAttachment(p.getApvts(), "rmsButton", rmsToggle),
    attackCoefficientAttachment(p.getApvts(), "attackCoefficient", attackCoefficientSlider),
    releaseCoefficientAttachment(p.getApvts(), "releaseCoefficient", releaseCoefficientSlider)
{
    
    addAndMakeVisible(gainReductionMeter);
    
    addAndMakeVisible(rmsLevelHeading1);
    addAndMakeVisible(rmsLevelHeading2);
    addAndMakeVisible(currentRmsLabel);
    addAndMakeVisible(maxRmsLabel);
    addAndMakeVisible(currentRmsValue);
    addAndMakeVisible(maxRmsValue);
    addAndMakeVisible(rmsPeriodLabel);
    addAndMakeVisible(ratioSlider);
    addAndMakeVisible(makeupGainSlider);

    addAndMakeVisible(rmsPeriodSlider);
    addAndMakeVisible(enableSmoothingButton);
    
    addAndMakeVisible(thresholdSlider);
    addAndMakeVisible(attackTimeSlider);
    addAndMakeVisible(releaseTimeSlider);
    
    addAndMakeVisible(thresholdLabel);
    addAndMakeVisible(attackTimeLabel);
    addAndMakeVisible(releaseTimeLabel);
    addAndMakeVisible(ratioLabel);
    addAndMakeVisible(makeupGainLabel);
    addAndMakeVisible(bypassToggle);
    addAndMakeVisible(peakToggle);
    addAndMakeVisible(rmsToggle);
    
    addAndMakeVisible(egeMolLabel);
    
    addAndMakeVisible(attackCoefficientSlider);
    addAndMakeVisible(releaseCoefficientSlider);
    addAndMakeVisible(attackCoefficientLabel);
    addAndMakeVisible(releaseCoefficientLabel);
    
    attackCoefficientLabel.setText("Attack Coefficient", juce::dontSendNotification);
    attackCoefficientLabel.setFont(juce::Font{}.withHeight(12.0f).withStyle(juce::Font::FontStyleFlags::bold));
    attackCoefficientLabel.setJustificationType(juce::Justification::centred);
    
    releaseCoefficientLabel.setText("Release Coefficient", juce::dontSendNotification);
    releaseCoefficientLabel.setFont(juce::Font{}.withHeight(12.0f).withStyle(juce::Font::FontStyleFlags::bold));
    releaseCoefficientLabel.setJustificationType(juce::Justification::centred);
    
    rmsToggle.setRadioGroupId(1);
    peakToggle.setRadioGroupId(1);
    
    attackCoefficientSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    attackCoefficientSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 75, 15);
    
    releaseCoefficientSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    releaseCoefficientSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 75, 15);
    
    thresholdSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    thresholdSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 20);
    thresholdSlider.setTextValueSuffix(" dB");
    
    thresholdLabel.setText("Threshold", juce::dontSendNotification);
    thresholdLabel.setFont(juce::Font{}.withStyle(juce::Font::FontStyleFlags::bold));
    thresholdLabel.setJustificationType(juce::Justification::centred);
    
    attackTimeSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    attackTimeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 20);
    attackTimeSlider.setTextValueSuffix(" ms");
    
    attackTimeLabel.setText("Attack", juce::dontSendNotification);
    attackTimeLabel.setFont(juce::Font{}.withStyle(juce::Font::FontStyleFlags::bold));
    attackTimeLabel.setJustificationType(juce::Justification::centred);
    
    releaseTimeSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    releaseTimeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 20);
    releaseTimeSlider.setTextValueSuffix(" ms");
    
    releaseTimeLabel.setText("Release", juce::dontSendNotification);
    releaseTimeLabel.setFont(juce::Font{}.withStyle(juce::Font::FontStyleFlags::bold));
    releaseTimeLabel.setJustificationType(juce::Justification::centred);
    
    egeMolLabel.setText("- Ege Mol Special Edition -", juce::dontSendNotification);
    egeMolLabel.setFont(juce::Font(12.0).withStyle(juce::Font::FontStyleFlags::italic));
    egeMolLabel.setJustificationType(juce::Justification::centred);
    egeMolLabel.setColour(juce::Label::textColourId, juce::Colours::magenta);
    
    
    ratioSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    ratioSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 20);
    
    ratioLabel.setText("Ratio", juce::dontSendNotification);
    ratioLabel.setFont(juce::Font{}.withStyle(juce::Font::FontStyleFlags::bold));
    ratioLabel.setJustificationType(juce::Justification::centred);
    
    makeupGainSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    makeupGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 20);
    makeupGainSlider.setTextValueSuffix(" dB");
    
    makeupGainLabel.setText("Make Up", juce::dontSendNotification);
    makeupGainLabel.setFont(juce::Font{}.withStyle(juce::Font::FontStyleFlags::bold));
    makeupGainLabel.setJustificationType(juce::Justification::centred);
    
    bypassToggle.setButtonText("Bypass");
    peakToggle.setButtonText("Peak");
    rmsToggle.setButtonText("RMS");

    rmsLevelHeading1.setText("dBFS", juce::dontSendNotification);
    rmsLevelHeading1.setFont(juce::Font{}.withStyle(juce::Font::FontStyleFlags::bold));
    rmsLevelHeading2.setText("Left \t Right", juce::dontSendNotification);
    rmsLevelHeading2.setFont(juce::Font{}.withStyle(juce::Font::FontStyleFlags::bold));
    currentRmsLabel.setText("Current RMS:", juce::dontSendNotification);
    maxRmsLabel.setText("Max RMS:", juce::dontSendNotification);
    rmsPeriodLabel.setText("RMS Period", juce::dontSendNotification);
    rmsPeriodLabel.setJustificationType(juce::Justification::right);

    rmsPeriodSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    rmsPeriodSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    rmsPeriodSlider.setPopupDisplayEnabled(true, false, this);
    rmsPeriodSlider.setTextValueSuffix(" ms");
    
    getLookAndFeel().setColour(juce::Slider::thumbColourId, juce::Colours::darkturquoise);
    getLookAndFeel().setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::silver);
    getLookAndFeel().setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::grey);

    enableSmoothingButton.setButtonText("Enable smoothing");

    setSize (600, 600);
    setResizable(true, true);
    setResizeLimits(500, 500, 1000, 1000);
    startTimerHz(24);
}

RMSCompressorAudioProcessorEditor::~RMSCompressorAudioProcessorEditor()
{
    stopTimer();
}

void RMSCompressorAudioProcessorEditor::timerCallback()
{
    // Gain reduction değerini al
    float currentGainReduction = audioProcessor.compressor.gainReductionFunc();

    // Metre güncelle
    gainReductionMeter.setGainReductionDb(currentGainReduction);
    
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
    currentRmsValue.setText(juce::String{ leftGain, 2 } + "   " + juce::String{ rightGain, 2 }, juce::sendNotification);
    maxRmsValue.setText(juce::String{ maxRmsLeft, 2 } + "   " + juce::String{ maxRmsRight, 2 }, juce::sendNotification);
}

//==============================================================================
void RMSCompressorAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.setGradientFill(juce::ColourGradient{ juce::Colours::darkgrey, getLocalBounds().toFloat().getCentre(), juce::Colours::darkgrey.darker(0.7f), {}, true });
    g.fillRect(getLocalBounds());

    g.setColour(juce::Colours::black);
}

void RMSCompressorAudioProcessorEditor::resized()
{
    int newWidth = getWidth();
    int newHeight = getHeight();

    // En ve boy oranını koruyarak pencereyi kare yapın
    if (newWidth != newHeight)
    {
        int size = std::min(newWidth, newHeight);
        setSize(size, size);
    }
    
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
    
    bypassToggle.setBounds(0, getHeight() / 10, getWidth() / 4, getHeight() / 8);
    
    peakToggle.setBounds(0, getHeight() / 2, getWidth() / 4, 25);
    rmsToggle.setBounds(0, getHeight() / 2 - 25, getWidth() / 4, 25);
    
    thresholdSlider.setBounds(0, getHeight() / 1.5, getWidth() / 4, getHeight() / 4);
    thresholdLabel.setBounds(getWidth() / 16, getHeight() / 1.14 , getWidth() / 8 , getHeight() / 8);
    
    ratioSlider.setBounds(getWidth() / 4, getHeight() / 1.5, getWidth() / 4, getHeight() / 4);
    ratioLabel.setBounds(getWidth() / 3.2, getHeight() / 1.14, getWidth() / 8, getHeight() / 8);
    
    attackTimeSlider.setBounds(getWidth() / 2, getHeight() / 1.5, getWidth() / 4, getHeight() / 4);
    attackTimeLabel.setBounds(getWidth() / 1.7777, getHeight() / 1.14 ,getWidth() / 8, getHeight() / 8);
    
    releaseTimeSlider.setBounds(getWidth() / 1.33333333, getHeight() / 1.5, getWidth() / 4, getHeight() / 4);
    releaseTimeLabel.setBounds(getWidth() / 1.2307692, getHeight() / 1.14, getWidth() / 8, getHeight() / 8);
    
    makeupGainSlider.setBounds(getWidth() / 1.33333333, getHeight() / 3, getWidth() / 4, getHeight() / 4);
    makeupGainLabel.setBounds(getWidth() / 1.2307692, getHeight() / 1.84 ,getWidth() / 8, getHeight() / 8);
    
    gainReductionMeter.setBounds((getWidth() / 2) - (getWidth() / 2) / 2, getHeight() / 5, getWidth() / 2, getHeight() / 2);
    
    attackCoefficientSlider.setBounds(getWidth() / 40 , getHeight() / 5, getWidth() / 8, getHeight() / 8);
    
    releaseCoefficientSlider.setBounds(getWidth() / 40 + getWidth() / 8 + 15, getHeight() / 5, getWidth() / 8, getHeight() / 8);
    
    attackCoefficientLabel.setBounds(getWidth() / 40 , getHeight() / 5 + 55, getWidth() / 8, getHeight() / 8);
    
    releaseCoefficientLabel.setBounds(getWidth() / 40 + getWidth() / 8 + 15, getHeight() / 5 + 55, getWidth() / 8, getHeight() / 8);

    egeMolLabel.setBounds(getWidth() / 2 - 60, getHeight() - 60, 120, 100);

}
