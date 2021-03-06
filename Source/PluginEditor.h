/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class ExpectDelaysAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                          public juce::Slider::Listener
{
public:
    ExpectDelaysAudioProcessorEditor (ExpectDelaysAudioProcessor&);
    ~ExpectDelaysAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    void sliderValueChanged (juce::Slider* sliderGain) override;
    
//    virtual double snapValues (std::vector<double> attemptedValues, juce::Slider::DragMode dragMode) override;

    //virtual double snapValue (double attemptedValue, juce::Slider::DragMode dragMode) override;
    
private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ExpectDelaysAudioProcessor& audioProcessor;
    
    juce::Slider rateSlider;
    juce::Slider fbSlider;
    juce::Slider mixSlider;
    
    juce::TextButton pingPongButton {"ping-pong"};
    
public:
    
    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> rateValue;
    
    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> fbValue;
    
    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> mixValue;
    
    std::unique_ptr <juce::AudioProcessorValueTreeState::ButtonAttachment> pingPongValue;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExpectDelaysAudioProcessorEditor)
};
