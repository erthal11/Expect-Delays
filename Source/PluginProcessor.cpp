/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ExpectDelaysAudioProcessor::ExpectDelaysAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
treeState (*this, nullptr, "PARAMETER", createParameterLayout())
#endif
{
}

ExpectDelaysAudioProcessor::~ExpectDelaysAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout ExpectDelaysAudioProcessor::createParameterLayout()
{
    std::vector <std::unique_ptr<juce::RangedAudioParameter>> params;
    
    auto rateParam = std::make_unique<juce::AudioParameterFloat>("rate","Rate", 0.01f,192000.f,192000.f);
    params.push_back(std::move(rateParam));
    
    auto fbParam = std::make_unique<juce::AudioParameterFloat>("feedback","Feedback", -100.f,0.f,-100.f);
    params.push_back(std::move(fbParam));
    
    return { params.begin(), params.end() };
}

const juce::String ExpectDelaysAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ExpectDelaysAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ExpectDelaysAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ExpectDelaysAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ExpectDelaysAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ExpectDelaysAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ExpectDelaysAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ExpectDelaysAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ExpectDelaysAudioProcessor::getProgramName (int index)
{
    return {};
}

void ExpectDelaysAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ExpectDelaysAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    lastSampleRate = sampleRate;
    
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    spec.numChannels = getTotalNumOutputChannels();
    
    delayLine.prepare(spec);
    delayLine.reset();
}

void ExpectDelaysAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ExpectDelaysAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
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

void ExpectDelaysAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    playHead = this->getPlayHead();
    playHead->getCurrentPosition (currentPositionInfo);
     
    int bpm = currentPositionInfo.bpm;
    
    float quarterNoteValue = (double)60/bpm * (double)lastSampleRate;

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        auto sliderRateValue = treeState.getRawParameterValue("rate");

        // ..do something to the data...
        for (int sample = 0; sample<buffer.getNumSamples(); ++sample)
        {
            //delayLine.setDelay(sliderRateValue->load());
            delayLine.setDelay(quarterNoteValue);
            delayLine.pushSample(channel, channelData[sample]);
            
            channelData[sample] = delayLine.popSample(channel);
        }
    }
    //std::cout << delayLine.getDelay();

}

//==============================================================================
bool ExpectDelaysAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ExpectDelaysAudioProcessor::createEditor()
{
    return new ExpectDelaysAudioProcessorEditor (*this);
}

//==============================================================================
void ExpectDelaysAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void ExpectDelaysAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ExpectDelaysAudioProcessor();
}
