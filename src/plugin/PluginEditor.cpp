#include "PluginEditor.h"

C700AudioProcessorEditor::C700AudioProcessorEditor(C700AudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(400, 200);
}

C700AudioProcessorEditor::~C700AudioProcessorEditor() {}

void C700AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    g.drawFittedText("C700 — Placeholder (M1)", getLocalBounds(), juce::Justification::centred, 1);
}

void C700AudioProcessorEditor::resized() {}
