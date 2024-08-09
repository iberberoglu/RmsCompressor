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
                    parameters(*this, nullptr, "LevelMeter", createParameters())
{
    parameters.addParameterListener("rmsPeriod", this);
    parameters.addParameterListener("smoothing", this);
    parameters.addParameterListener("threshold", this);
    parameters.addParameterListener("attackTime", this);
    parameters.addParameterListener("releaseTime", this);
    parameters.addParameterListener("ratio", this);
    parameters.addParameterListener("makeupGain", this);
    parameters.addParameterListener("bypassButton", this);
    parameters.addParameterListener("peakButton", this);
    parameters.addParameterListener("rmsButton", this);
    parameters.addParameterListener("attackCoefficient", this);
    parameters.addParameterListener("releaseCoefficient", this);
}

RMSCompressorAudioProcessor::~RMSCompressorAudioProcessor()
{
    parameters.removeParameterListener("rmsPeriod", this);
    parameters.removeParameterListener("smoothing", this);
    parameters.removeParameterListener("threshold", this);
    parameters.removeParameterListener("attackTime", this);
    parameters.removeParameterListener("releaseTime", this);
    parameters.removeParameterListener("ratio", this);
    parameters.removeParameterListener("makeupGain", this);
    parameters.removeParameterListener("bypassButton", this);
    parameters.removeParameterListener("peakButton", this);
    parameters.removeParameterListener("rmsButton", this);
    parameters.removeParameterListener("attackCoefficient", this);
    parameters.removeParameterListener("releaseCoefficient", this);
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

    rmsFifo.reset(numberOfChannels, (static_cast<int>(sampleRate) * 2) + 1);
    rmsCalculationBuffer.clear();
    rmsCalculationBuffer.setSize(numberOfChannels, (static_cast<int>(sampleRate) * 2) + 1);

    rmsWindowSize =  static_cast<int> (sampleRate * parameters.getRawParameterValue("rmsPeriod")->load()) / 1000;
    isSmoothed = static_cast<bool> (parameters.getRawParameterValue("smoothing")->load());
    thresholdValue = static_cast<float> (parameters.getRawParameterValue("threshold")->load());
    attackTimeValue = static_cast<float> (parameters.getRawParameterValue("attackTime")->load());
    releaseTimeValue = static_cast<float> (parameters.getRawParameterValue("releaseTime")->load());
    ratioValue = static_cast<float> (parameters.getRawParameterValue("ratio")->load());
    makeupGainValue = static_cast<float> (parameters.getRawParameterValue("makeupGain")->load());
    
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    spec.sampleRate = sampleRate;
    
    compressor.prepare(spec);
    compressor.setThreshold(thresholdValue);
    compressor.setRatio(ratioValue);
    
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
    
    auto block = juce::dsp::AudioBlock<float>(buffer);
    auto context = juce::dsp::ProcessContextReplacing<float>(block);
    
    compressor.process(context, rmsLevels, peakValue, rmsValue, attackCoefficientValue, releaseCoefficientValue);

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

juce::AudioProcessorValueTreeState::ParameterLayout RMSCompressorAudioProcessor::createParameters()
{
    // PARAMETRELERIN YARATILDIĞI YER
    
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "threshold",  1 }, "Threshold", juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -20.0f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "attackTime", 1}, "AttackTime", juce::NormalisableRange<float>(0.5f, 300.0f, 0.1f), 15.0f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "releaseTime", 1}, "ReleaseTime", juce::NormalisableRange<float>(5.0f, 1000.0f, 0.1f), 50.0f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "makeupGain", 1}, "MakeupGain", juce::NormalisableRange<float>(-20.0f, 20.0f, 0.1f), 0.0f));
    
    auto attackRange = juce::NormalisableRange<float>(-5.0f, -0.01f, 0.01f);
    attackRange.setSkewForCentre(-2.0f);
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "attackCoefficient",  1 }, "AttackCoefficient", attackRange, -0.1f));
       
    auto releaseRange = juce::NormalisableRange<float>(-5.0f, -0.01f, 0.01f); 
    releaseRange.setSkewForCentre(-2.0f);
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "releaseCoefficient",  1 }, "ReleaseCoefficient", releaseRange, -0.4f));
    
    params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID { "rmsPeriod",  1 }, "Period", 1, 1500, 50));
    
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "smoothing",  1 }, "Enable Smoothing", false));
    
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "bypassButton",  1 }, "Bypass", false));
    
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "peakButton",  1 }, "Peak", false));
    
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "rmsButton",  1 }, "RMS", true));
    
    auto choices = std::vector<float>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 15.0, 20.0, 50.0, 100.0};
    juce::StringArray sa;
    for(auto choice: choices)
    {
        sa.add(juce::String(choice, 1));
    }
    
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID {"ratio", 1}, "Ratio", sa, 4));
    
    return {params.begin(), params.end()};
}

void RMSCompressorAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID.equalsIgnoreCase("rmsPeriod")){
        rmsWindowSize = static_cast<int>(sampleRate * newValue) / 1000;
//        getBPM();
    }
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
    if (parameterID.equalsIgnoreCase("ratio")){
        auto ratioIndex = newValue;
        compressor.setRatio(ratioValues[ratioIndex]);
    }
    if (parameterID.equalsIgnoreCase("makeupGain")){
        makeupGainValue = newValue;
        compressor.setMakeupGain(makeupGainValue);
    }
    if (parameterID.equalsIgnoreCase("bypassButton")){
        bypassValue = newValue;
        compressor.setBypass(bypassValue);
    }
    if (parameterID.equalsIgnoreCase("peakButton")){
        peakValue = newValue;
    }
    if (parameterID.equalsIgnoreCase("rmsButton")){
        rmsValue = newValue;
    }
    
    if (parameterID.equalsIgnoreCase("attackCoefficient")){
        attackCoefficientValue = newValue;
    }
    
    if (parameterID.equalsIgnoreCase("releaseCoefficient")){
        releaseCoefficientValue = newValue;
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

//void RMSCompressorAudioProcessor::getBPM()
//{
//    if (auto* playHead = getPlayHead())
//    {
//        juce::AudioPlayHead::CurrentPositionInfo positionInfo;
//
//        if (playHead->getPosition())
//        {
//            double bpm = positionInfo.bpm;
//            DBG("Current BPM: " << bpm);
//            // bpm bilgisini burada kullanabilirsiniz
//        }
//    }
//}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RMSCompressorAudioProcessor();
}

