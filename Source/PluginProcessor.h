/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class ExpectDelaysAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    ExpectDelaysAudioProcessor();
    ~ExpectDelaysAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    juce::AudioProcessorValueTreeState treeState;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    foleys::MagicProcessorState magicState { *this, treeState };
    
    void updateFilter();


private:
    
    
    juce::dsp::DelayLine <float> ping {400000};
    juce::dsp::DelayLine <float> pong {400000};
    
    juce::dsp::DelayLine <float> dummyPing {192000};
    
    juce::dsp::DelayLine <float> pingShift {192000};
    juce::dsp::DelayLine <float> pongShift {192000};
    
    juce::dsp::DelayLine <float> dummyPingShift {192000};
    
    float outputL = 0;
    float outputR = 0;
    
    float outputLD = 0;
    float outputLDS = 0;
    
    float outputLS = 0;
    float outputRS = 0;
    
    
    juce::AudioPlayHead* playHead;
    juce::AudioPlayHead::CurrentPositionInfo currentPositionInfo;
    
    juce::dsp::ProcessorDuplicator <juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients <float>> lowPassFilter;
    
    juce::dsp::ProcessorDuplicator <juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients <float>> highPassFilter;
    
    float lastSampleRate;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExpectDelaysAudioProcessor)
};
