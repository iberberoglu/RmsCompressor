/* pluginproceessor.cpp
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
RMSCompressorAudioProcessor::RMSCompressorAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput("Input", juce::AudioChannelSet::stereo(), true)
                       .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
                   parameters(*this, nullptr, "LevelMeter", AudioProcessorValueTreeState::ParameterLayout{
                       std::make_unique<AudioParameterFloat>(ParameterID { "threshold",  1 }, "Threshold", NormalisableRange<float>(-60.0f, 0.0f), -20.0f),
                       std::make_unique<AudioParameterInt>(ParameterID { "rmsPeriod",  1 }, "Period", 1, 3000, 50),
                       std::make_unique<AudioParameterBool>(ParameterID { "smoothing",  1 }, "Enable Smoothing", true)
                   })
{
    parameters.addParameterListener("rmsPeriod", this);
    parameters.addParameterListener("smoothing", this);
    parameters.addParameterListener("threshold", this);
}

RMSCompressorAudioProcessor::~RMSCompressorAudioProcessor()
{
    parameters.removeParameterListener("rmsPeriod", this);
    parameters.removeParameterListener("smoothing", this);
    parameters.removeParameterListener("threshold", this);
}

//==============================================================================
const juce::String RMSCompressorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool RMSCompressorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool RMSCompressorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool RMSCompressorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double RMSCompressorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int RMSCompressorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int RMSCompressorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void RMSCompressorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String RMSCompressorAudioProcessor::getProgramName (int index)
{
    return {};
}

void RMSCompressorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void RMSCompressorAudioProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    sampleRate = sr;
    const auto numberOfChannels = getTotalNumInputChannels();
    
    rmsLevels.clear();
    for (auto i = 0; i < numberOfChannels; i++)
    {
        LinearSmoothedValue<float> rms{ -100.f };
        rms.reset(sampleRate, 0.5);
        rmsLevels.emplace_back(std::move(rms));
    }

    rmsFifo.reset(numberOfChannels, (static_cast<int>(sampleRate) * 4) + 1);
    rmsCalculationBuffer.clear();
    rmsCalculationBuffer.setSize(numberOfChannels, (static_cast<int>(sampleRate) * 4) + 1);

    rmsWindowSize =  static_cast<int> (sampleRate * parameters.getRawParameterValue("rmsPeriod")->load()) / 1000;
    isSmoothed = static_cast<bool> (parameters.getRawParameterValue("smoothing")->load());
    thresholdValue = static_cast<float> (parameters.getRawParameterValue("threshold")->load());
    
}

void RMSCompressorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool RMSCompressorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void RMSCompressorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
//    juce::ScopedNoDenormals noDenormals;
//    const auto numSamples = buffer.getNumSamples();
//
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    
    for (auto& rmsLevel : rmsLevels)
        rmsLevel.skip(numSamples);
    
    rmsFifo.push(buffer);
    
    auto rmsLevels = getRmsLevels(); // RMS seviyelerini al

    for (int channel = 0; channel < getTotalNumInputChannels(); ++channel) {
        auto* channelData = buffer.getWritePointer(channel);

        for (int i = 0; i < numSamples; ++i) {
            // Her sample için RMS seviyesine göre işlem yap
            channelData[i] = compressor.processSample(channelData[i], rmsLevels[channel]);
        }
    }
    
    
    //seviye görmek için değişip değişmedii
//    float threshh = compressor.threshOut();
//    std::cout << threshh << std::endl;
    
}

//==============================================================================
bool RMSCompressorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* RMSCompressorAudioProcessor::createEditor()
{
    return new RMSCompressorAudioProcessorEditor (*this);
}

//==============================================================================
void RMSCompressorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void RMSCompressorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

void RMSCompressorAudioProcessor::parameterChanged(const String& parameterID, float newValue)
{
    if (parameterID.equalsIgnoreCase("rmsPeriod"))
        rmsWindowSize = static_cast<int>(sampleRate * newValue) / 1000;
    if (parameterID.equalsIgnoreCase("smoothing"))
        isSmoothed = static_cast<bool> (newValue);
    if (parameterID.equalsIgnoreCase("threshold")){
        thresholdValue = newValue;  // Değişkeni güncelle
        compressor.setThreshold(newValue);
    }
}


std::vector<float> RMSCompressorAudioProcessor::getRmsLevels()
{
    rmsFifo.pull(rmsCalculationBuffer, rmsWindowSize);
    std::vector<float> levels;
    for (auto channel = 0; channel < rmsCalculationBuffer.getNumChannels(); channel++)
    {
        processLevelValue(rmsLevels[channel], Decibels::gainToDecibels(rmsCalculationBuffer.getRMSLevel(channel, 0, rmsWindowSize)));
        levels.push_back(rmsLevels[channel].getCurrentValue());
    }
    return levels;
}

float RMSCompressorAudioProcessor::getRmsLevel(const int channel)
{
    jassert(channel >= 0 && channel < rmsCalculationBuffer.getNumChannels());
    rmsFifo.pull(rmsCalculationBuffer.getWritePointer(channel), channel, rmsWindowSize);
    processLevelValue(rmsLevels[channel], Decibels::gainToDecibels(rmsCalculationBuffer.getRMSLevel(channel, 0, rmsWindowSize)));
    return rmsLevels[channel].getCurrentValue();
}

void RMSCompressorAudioProcessor::processLevelValue(LinearSmoothedValue<float>& smoothedValue, const float value) const
{
    if (isSmoothed)
    {
        if (value < smoothedValue.getCurrentValue())
        {
            smoothedValue.setTargetValue(value);
            return;
        }
    }
    smoothedValue.setCurrentAndTargetValue(value);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RMSCompressorAudioProcessor();
}

