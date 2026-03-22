#pragma once
#include "PluginProcessor.h"

class C700AudioProcessorEditor : public juce::AudioProcessorEditor,
                                  private juce::Timer
{
public:
    explicit C700AudioProcessorEditor(C700AudioProcessor&);
    ~C700AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void loadSampleClicked();

    C700AudioProcessor& processorRef;

    juce::TextButton mLoadButton { "Load Sample" };
    juce::Label mSlotLabel;
    juce::Label mSampleNameLabel;

    std::unique_ptr<juce::GenericAudioProcessorEditor> mGenericEditor;
    std::unique_ptr<juce::FileChooser> mFileChooser;

    juce::File mLastBrowseDir;
    int mDisplayedProgram = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(C700AudioProcessorEditor)
};
