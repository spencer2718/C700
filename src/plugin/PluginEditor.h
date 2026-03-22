#pragma once
#include "PluginProcessor.h"

class C700AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit C700AudioProcessorEditor(C700AudioProcessor&);
    ~C700AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    C700AudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(C700AudioProcessorEditor)
};
