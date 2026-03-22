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
    void loadPlayerCodeClicked();
    void exportSpcClicked();

    C700AudioProcessor& processorRef;

    juce::TextButton mLoadButton { "Load Sample" };
    juce::TextButton mLoadPlayerCodeButton { "Load Player Code" };
    juce::TextButton mExportSpcButton { "Export SPC..." };
    juce::Label mSlotLabel;
    juce::Label mSampleNameLabel;
    juce::Label mStatusLabel;

    std::unique_ptr<juce::GenericAudioProcessorEditor> mGenericEditor;
    std::unique_ptr<juce::FileChooser> mFileChooser;

    juce::File mLastBrowseDir;
    juce::File mLastExportDir;
    int mDisplayedProgram = -1;
    juce::int64 mStatusOverrideUntil = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(C700AudioProcessorEditor)
};
