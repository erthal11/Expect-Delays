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

//returns note multiplyer for 64th note value
double noteValRec(double x)
{
    if (fmod(x,2) != 0)
    {
        if (x==1) return x;
        else return (2*(noteValRec(x-2)));
    }
    else
    {
        if (x==2) return (4.0/3);
        else return (2*(noteValRec(x-2)));
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
        
//        switch ((int)(sliderRateValue->load()))
//        {
//            // 64th
//            case 1: noteVal = sixtyFourthNote;
//                break;
//            // 32nd triplet
//            case 2: noteVal = sixtyFourthNote*2.0 *(3.0/4);
//                break;
//            // 32nd
//            case 3: noteVal = sixtyFourthNote*2;
//                break;
//            // 16th triplet
//            case 4: noteVal = sixtyFourthNote*4.0 *(3.0/4);
//                break;
//            // 16th
//            case 5: noteVal = sixtyFourthNote*4;
//                break;
//            // 8th triplet
//            case 6: noteVal = sixtyFourthNote*8.0 *(3.0/4);
//                break;
//            // 8th
//            case 7: noteVal = sixtyFourthNote*8;
//                break;
//            // 1/4 triplet
//            case 8: noteVal = sixtyFourthNote*16.0 - (1/3)*(sixtyFourthNote*16 - sixtyFourthNote*8);
//                break;
//            // 1/4
//            case 9: noteVal = sixtyFourthNote*16;
//                break;
//            default: noteVal = sixtyFourthNote*16;
//        }
    

        // ..do something to the data...
        for (int sample = 0; sample<buffer.getNumSamples(); ++sample)
        {
            //delayLine.setDelay(sliderRateValue->load());
            delayLine.setDelay(noteValRec(sliderRateValue->load()) * sixtyFourthNote);
            //delayLine.setDelay(noteVal);
            delayLine.pushSample(channel, channelData[sample]);
            
            channelData[sample] = delayLine.popSample(channel);
        }
    }

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
