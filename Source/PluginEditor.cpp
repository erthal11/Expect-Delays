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
    setSize (400, 300);
    
    rateSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
//    delaySlider.setRange(0, 2);
//    delaySlider.setValue(1);
    rateSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    rateSlider.addListener(this);
    addAndMakeVisible(&rateSlider);
    
    rateValue = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.treeState, "rate", rateSlider);
    
    
    
    fbSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    fbSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
//    fbSlider.setRange(0, 2);
//    fbSlider.setValue(1);
    fbSlider.addListener(this);
    addAndMakeVisible(&fbSlider);
    
    fbValue = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.treeState, "feedback", fbSlider);
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
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void ExpectDelaysAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    rateSlider.setBounds(getWidth()/2-70, getHeight()/2, 110, 115);
    fbSlider.setBounds(getWidth()/2+70, getHeight()/2, 110, 115);
}


void ExpectDelaysAudioProcessorEditor::sliderValueChanged (juce::Slider* slider)
{
    if (slider == &rateSlider){
        rateSlider.snapValue(4.0, juce::Slider::DragMode::absoluteDrag);
    }

}
