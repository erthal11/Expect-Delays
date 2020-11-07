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
    
    auto rateParam = std::make_unique<juce::AudioParameterFloat>("rate","Rate", normRange,7.0);
    params.push_back(std::move(rateParam));
    
    auto fbLParam = std::make_unique<juce::AudioParameterFloat>("feedbackL","FeedbackL", 0, 1, 0.5);
    params.push_back(std::move(fbLParam));
    
    auto fbRParam = std::make_unique<juce::AudioParameterFloat>("feedbackR","FeedbackR", 0, 1, 0.5);
    params.push_back(std::move(fbRParam));
    
    auto mixParam = std::make_unique<juce::AudioParameterFloat>("mix","Mix", 0, 1, 0.25);
    params.push_back(std::move(mixParam));
    
    //auto pingpongParam = std::make_unique<juce::AudioParameterBool>("pingpong","Pingpong", false);
    //params.push_back(std::move(pingpongParam));
    
    auto widthParam = std::make_unique<juce::AudioParameterFloat>("width","Width", 0, 1, 1);
    params.push_back(std::move(widthParam));
    
    auto pingShiftParam = std::make_unique<juce::AudioParameterFloat>("pingshift","Pingshift", 0, 1, 0);
    params.push_back(std::move(pingShiftParam));
    
    auto pongShiftParam = std::make_unique<juce::AudioParameterFloat>("pongshift","Pongshift", 0, 1, 0);
    params.push_back(std::move(pongShiftParam));
    
    auto outputParam = std::make_unique<juce::AudioParameterFloat>("output","Output", -30, 30, 0);
    params.push_back(std::move(outputParam));
    
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
    
    ping.prepare(spec);
    ping.reset();
    
    pong.prepare(spec);
    pong.reset();
    
    pingShift.prepare(spec);
    pingShift.reset();
    
    pongShift.prepare(spec);
    pongShift.reset();
    
    dummyPing.prepare(spec);
    dummyPing.reset();
    
    dummyPingShift.prepare(spec);
    dummyPingShift.reset();
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
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    float *cleanSignalL = new float[buffer.getNumSamples()];
    float *cleanSignalR = new float[buffer.getNumSamples()];

    auto* channelDataL = buffer.getWritePointer (0);
    auto* channelDataR = buffer.getWritePointer (1);
    
    // copy samples for a clean signal
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        cleanSignalL[sample] = channelDataL[sample];
        cleanSignalR[sample] = channelDataR[sample];
    }
    
    auto rate = treeState.getRawParameterValue("rate");
    auto feedbackL = treeState.getRawParameterValue("feedbackL");
    auto feedbackR = treeState.getRawParameterValue("feedbackR");
    auto mix = treeState.getRawParameterValue("mix");
    auto output =treeState.getRawParameterValue("output")->load();
    
    auto pingshiftValue = (treeState.getRawParameterValue("pingshift"))->load() + 1;
    auto pongshiftValue = (treeState.getRawParameterValue("pongshift"))->load();
    
    //bool pingpong = (treeState.getRawParameterValue("pingpong"))->load();
    
    auto width = (treeState.getRawParameterValue("width"))->load();
    
    float delayTime = noteMult(rate->load()) * sixtyFourthNote;
    
        
//    if (!pingpong)
//    {
//        ping.setDelay(delayTime);
//        pong.setDelay(delayTime);
//
//        for (int sample = 0; sample<buffer.getNumSamples(); ++sample)
//        {
//
//            outputL = ping.popSample(0);
//            ping.pushSample(0, channelDataL[sample] + feedbackL->load() * outputL);
//            channelDataL[sample] = outputL;
//
//            outputR = pong.popSample(1);
//            pong.pushSample(1, channelDataR[sample] + feedbackR->load() * outputR);
//            channelDataR[sample] = outputR;
//
//        }
//    }
    
    //if (pingpong)
    //{
        //pingpong implemented by setting left(ping) and right(pong) delay rates to twice rate value,
        //then dshifting the left delayed signal by rate value,
        //and adding a left dummy delay to fill in gap from delaying signal
        //volumes and feedback were lowered to compensate, in order to simulate a normal delay decay
    
        ping.setDelay(2 * delayTime);
        pong.setDelay(2 * delayTime);
        
        pingShift.setDelay(delayTime * pingshiftValue);
        dummyPingShift.setDelay(delayTime * pingshiftValue - delayTime);
        pongShift.setDelay(delayTime * pongshiftValue);
    
        dummyPing.setDelay(delayTime);
        
        double fbl = feedbackL->load() -.16;
        
        for (int sample = 0; sample<buffer.getNumSamples(); ++sample)
        {
            
            outputLD = dummyPing.popSample(0);
            dummyPing.pushSample(0, channelDataL[sample]);
            
            dummyPingShift.pushSample(0, outputLD);
            outputLDS = dummyPingShift.popSample(0);
            
            outputL = ping.popSample(0);
            ping.pushSample(0, channelDataL[sample] + fbl * outputL);

            pingShift.pushSample(0, outputL);
            outputLS = pingShift.popSample(0) * pow(fbl,1.5);
            
//            if (fbl == 0)
//                outputLS =0;
//            if (feedbackR->load() == 0)
//                outputR =0;
                
            outputR = pong.popSample(1) * feedbackR->load();
            pong.pushSample(1, channelDataR[sample] + feedbackR->load() * outputR);
            
            pongShift.pushSample(0, outputR);
            outputRS = pongShift.popSample(0);
            
            channelDataL[sample] = outputLDS + outputLS + (1-width)*outputRS;
            
            channelDataR[sample] = outputRS + (1-width)*(outputLDS + outputLS);
            
        }
    //}
    
    //filtering goes here
    //
    
    // mixing dry + wet
    for (int sample = 0; sample<buffer.getNumSamples(); ++sample)
    {
        channelDataL[sample] = ((channelDataL[sample] * mix->load()) + (cleanSignalL[sample] * (1-mix->load()))) * juce::Decibels::decibelsToGain(output);
        
        channelDataR[sample] = ((channelDataR[sample] * mix->load()) + (cleanSignalR[sample] * (1-mix->load()))) * juce::Decibels::decibelsToGain(output);
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
    auto state = treeState.copyState();
            std::unique_ptr<juce::XmlElement> xml (state.createXml());
            copyXmlToBinary (*xml, destData);
}

void ExpectDelaysAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
     
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (treeState.state.getType()))
            treeState.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ExpectDelaysAudioProcessor();
}
