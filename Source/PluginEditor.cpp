/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ExpectDelaysAudioProcessorEditor::ExpectDelaysAudioProcessorEditor (ExpectDelaysAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (600, 400);
    
    rateSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
//    delaySlider.setRange(0, 2);
//    delaySlider.setValue(1);
    rateSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    rateSlider.addListener(this);
    addAndMakeVisible(&rateSlider);
    
    rateValue = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.treeState, "rate", rateSlider);
    
    
    
    fbSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    fbSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    fbSlider.addListener(this);
    addAndMakeVisible(&fbSlider);
    
    fbValue = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.treeState, "feedback", fbSlider);
    
    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    fbSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    mixSlider.addListener(this);
    addAndMakeVisible(&mixSlider);
    
    mixValue = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.treeState, "mix", mixSlider);
    
    pingPongButton.setColour(juce::TextButton::buttonColourId, juce::Colours::black);
    pingPongButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::darkred);
    pingPongButton.setClickingTogglesState (true);
    pingPongButton.onClick = [this]() {};
    addAndMakeVisible(&pingPongButton);
    
    pingPongValue = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.treeState, "pingpong", pingPongButton);
    
}

ExpectDelaysAudioProcessorEditor::~ExpectDelaysAudioProcessorEditor()
{
}

//==============================================================================
void ExpectDelaysAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Expect Delays", 0,30,getWidth(), 30, juce::Justification::centred, 1);
}

void ExpectDelaysAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    rateSlider.setBounds(30, getHeight()/2, 110, 110);
    fbSlider.setBounds(140, getHeight()/2, 110, 110);
    mixSlider.setBounds(250, getHeight()/2, 110, 115);
    pingPongButton.setBounds(60, 25, 90, 60);

}


void ExpectDelaysAudioProcessorEditor::sliderValueChanged (juce::Slider* slider)
{
    if (slider == &rateSlider){
        rateSlider.snapValue(4.0, juce::Slider::DragMode::absoluteDrag);
    }

}
