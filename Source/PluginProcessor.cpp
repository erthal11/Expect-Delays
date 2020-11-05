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
    auto normRange = juce::NormalisableRange<float>(1.0,13.0,1);
    
    auto rateParam = std::make_unique<juce::AudioParameterFloat>("rate","Rate", normRange,5.0);
    params.push_back(std::move(rateParam));
    
    auto fbLParam = std::make_unique<juce::AudioParameterFloat>("feedbackL","FeedbackL", 0, 1, 0.5);
    params.push_back(std::move(fbLParam));
    
    auto fbRParam = std::make_unique<juce::AudioParameterFloat>("feedbackR","FeedbackR", 0, 1, 0.5);
    params.push_back(std::move(fbRParam));
    
    auto mixParam = std::make_unique<juce::AudioParameterFloat>("mix","Mix", 0, 1, 0.5);
    params.push_back(std::move(mixParam));
    
    auto pingpongParam = std::make_unique<juce::AudioParameterBool>("pingpong","Pingpong", false);
    params.push_back(std::move(pingpongParam));
    
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


//returns note multiplyer for 64th note value (missing dotted notes)
double noteMult(double x)
{
    if (fmod(x,2) != 0)
    {
        if (x==1) return 1;
        else return (2*(noteMult(x-2)));
    }
    else
    {
        if (x==2) return (4.0/3);
        else return (2*(noteMult(x-2)));
    }
}

void ExpectDelaysAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    playHead = this->getPlayHead();
    playHead->getCurrentPosition (currentPositionInfo);
     
    int bpm = currentPositionInfo.bpm;
    
    float const sixtyFourthNote = (60.0/bpm * lastSampleRate)/16.0;
    
    float outputL = 0;
    float outputR = 0;
    
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    float *cleanSignalL = new float[buffer.getNumSamples()];
    float *cleanSignalR = new float[buffer.getNumSamples()];

    
        auto* channelDataL = buffer.getWritePointer (0);
        auto* channelDataR = buffer.getWritePointer (1);
    
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        cleanSignalL[sample] = channelDataL[sample];
        cleanSignalR[sample] = channelDataR[sample];
    }
        
        auto rate = treeState.getRawParameterValue("rate");
        auto feedbackL = treeState.getRawParameterValue("feedbackL");
        auto feedbackR = treeState.getRawParameterValue("feedbackR");
        auto mix = treeState.getRawParameterValue("mix");
        
        delayLine.setDelay(noteMult(rate->load()) * sixtyFourthNote);
        

        for (int sample = 0; sample<buffer.getNumSamples(); ++sample)
        {
            outputL = delayLine.popSample(0);
            delayLine.pushSample(0, channelDataL[sample] + feedbackL->load() * outputL);
            channelDataL[sample] = outputL;
            
            outputR = delayLine.popSample(1);
            delayLine.pushSample(1, channelDataR[sample] + feedbackR->load() * outputR);
            channelDataR[sample] = outputR;
        }
        
        //filtering goes here
        //
        
        // mixing dry + wet
        for (int sample = 0; sample<buffer.getNumSamples(); ++sample)
        {
            channelDataL[sample] = (channelDataL[sample] * mix->load()) + (cleanSignalL[sample] * (1-mix->load()));
            
            channelDataR[sample] = (channelDataR[sample] * mix->load()) + (cleanSignalR[sample] * (1-mix->load()));
        }
        
    

}

//==============================================================================
bool ExpectDelaysAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ExpectDelaysAudioProcessor::createEditor()
{
    //return new ExpectDelaysAudioProcessorEditor (*this);
    return new foleys::MagicPluginEditor (magicState);
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
