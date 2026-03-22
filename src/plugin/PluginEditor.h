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
    void unloadSampleClicked();
    void loadPlayerCodeClicked();
    void exportSpcClicked();

    C700AudioProcessor& processorRef;

    juce::Image mBackgroundImage;
    juce::Image mHardwareStrip;

    juce::ImageButton mLoadButton;
    juce::ImageButton mUnloadButton;
    juce::Slider mProgramStepper;
    juce::ComboBox mEngineBox;
    juce::ComboBox mVoiceAllocBox;
    juce::TextButton mPlayerCodeButton { "Player Code..." };
    juce::TextButton mExportSpcButton { "Export SPC..." };
    juce::Label mProgramNameLabel;
    juce::Label mStatusLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mProgramAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> mEngineAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> mVoiceAllocAttachment;

    std::unique_ptr<juce::FileChooser> mFileChooser;
    juce::File mLastBrowseDir;
    juce::File mLastExportDir;
    juce::int64 mStatusOverrideUntil = 0;
    bool mHardwareConnected = false;

    static constexpr int kEditorWidth = 536;
    static constexpr int kEditorHeight = 406;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(C700AudioProcessorEditor)
};
