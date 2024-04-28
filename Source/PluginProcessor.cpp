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
                    parameters(*this, nullptr, "LevelMeter", juce::AudioProcessorValueTreeState::ParameterLayout{
                       std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "threshold",  1 }, "Threshold", juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -20.0f),
                       std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "attackTime", 1}, "AttackTime", juce::NormalisableRange<float>(0.0f, 200.0f, 0.1f), 15.0f),
                       std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "releaseTime", 1}, "ReleaseTime", juce::NormalisableRange<float>(5.0f, 5000.0f, 0.1f), 50.0f),
                       std::make_unique<juce::AudioParameterInt>(juce::ParameterID { "rmsPeriod",  1 }, "Period", 1, 3000, 50),
                       std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "smoothing",  1 }, "Enable Smoothing", true)
                   })
{
    parameters.addParameterListener("rmsPeriod", this);
    parameters.addParameterListener("smoothing", this);
    parameters.addParameterListener("threshold", this);
    parameters.addParameterListener("attackTime", this);
    parameters.addParameterListener("releaseTime", this);
}

RMSCompressorAudioProcessor::~RMSCompressorAudioProcessor()
{
    parameters.removeParameterListener("rmsPeriod", this);
    parameters.removeParameterListener("smoothing", this);
    parameters.removeParameterListener("threshold", this);
    parameters.removeParameterListener("attackTime", this);
    parameters.removeParameterListener("releaseTime", this);
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
        juce::LinearSmoothedValue<float> rms{ -100.f };
        rms.reset(sampleRate, 0.5);
        rmsLevels.emplace_back(std::move(rms));
    }

    rmsFifo.reset(numberOfChannels, (static_cast<int>(sampleRate) * 4) + 1);
    rmsCalculationBuffer.clear();
    rmsCalculationBuffer.setSize(numberOfChannels, (static_cast<int>(sampleRate) * 4) + 1);

    rmsWindowSize =  static_cast<int> (sampleRate * parameters.getRawParameterValue("rmsPeriod")->load()) / 1000;
    isSmoothed = static_cast<bool> (parameters.getRawParameterValue("smoothing")->load());
    thresholdValue = static_cast<float> (parameters.getRawParameterValue("threshold")->load());
    attackTimeValue = static_cast<float> (parameters.getRawParameterValue("attackTime")->load());
    releaseTimeValue = static_cast<float> (parameters.getRawParameterValue("releaseTime")->load());
    
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    spec.sampleRate = sampleRate;
    
    compressor.prepare(spec);
    
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
    
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    
    for (auto& rmsLevel : rmsLevels)
        rmsLevel.skip(numSamples);
    
    rmsFifo.push(buffer);
    
    auto rmsLevels = getRmsLevels(); // RMS seviyelerini al
    
    compressor.setRatio(10);
    
    auto block = juce::dsp::AudioBlock<float>(buffer);
    auto context = juce::dsp::ProcessContextReplacing<float>(block);

    compressor.process(context, rmsLevels);

    
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
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void RMSCompressorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

void RMSCompressorAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID.equalsIgnoreCase("rmsPeriod"))
        rmsWindowSize = static_cast<int>(sampleRate * newValue) / 1000;
    if (parameterID.equalsIgnoreCase("smoothing"))
        isSmoothed = static_cast<bool> (newValue);
    if (parameterID.equalsIgnoreCase("threshold")){
        thresholdValue = newValue;  // Değişkeni güncelle
        compressor.setThreshold(newValue);
    }
    if (parameterID.equalsIgnoreCase("attackTime")){
        attackTimeValue = newValue;  // Değişkeni güncelle
        compressor.setAttack(newValue);
    }
    if (parameterID.equalsIgnoreCase("releaseTime")){
        releaseTimeValue = newValue;  // Değişkeni güncelle
        compressor.setRelease(newValue);
    }
}


std::vector<float> RMSCompressorAudioProcessor::getRmsLevels()
{
    rmsFifo.pull(rmsCalculationBuffer, rmsWindowSize);
    std::vector<float> levels;
    for (auto channel = 0; channel < rmsCalculationBuffer.getNumChannels(); channel++)
    {
        processLevelValue(rmsLevels[channel], juce::Decibels::gainToDecibels(rmsCalculationBuffer.getRMSLevel(channel, 0, rmsWindowSize)));
        levels.push_back(rmsLevels[channel].getCurrentValue());
    }
    return levels;
}

float RMSCompressorAudioProcessor::getRmsLevel(const int channel)
{
    jassert(channel >= 0 && channel < rmsCalculationBuffer.getNumChannels());
    rmsFifo.pull(rmsCalculationBuffer.getWritePointer(channel), channel, rmsWindowSize);
    processLevelValue(rmsLevels[channel], juce::Decibels::gainToDecibels(rmsCalculationBuffer.getRMSLevel(channel, 0, rmsWindowSize)));
    return rmsLevels[channel].getCurrentValue();
}

void RMSCompressorAudioProcessor::processLevelValue(juce::LinearSmoothedValue<float>& smoothedValue, const float value) const
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

