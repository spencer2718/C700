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
    void saveSampleClicked();
    void saveXiClicked();
    void loadPlayerCodeClicked();
    void exportSpcClicked();
    void adjustProgram(int delta);
    void selectBank(int bank);
    void selectEditingChannel(int channel);
    void refreshSelectionButtons();

    C700AudioProcessor& processorRef;

    juce::Image mBackgroundImage;
    juce::Image mHardwareStrip;
    juce::Image mTrackIndicatorStrip;
    juce::Image mProgramRockerStrip;
    std::array<juce::Image, 4> mBankStrips;
    std::array<juce::Image, 16> mTrackNumberStrips;

    juce::ImageButton mLoadButton;
    juce::ImageButton mUnloadButton;
    juce::ImageButton mSaveButton;
    juce::ImageButton mSaveXiButton;
    std::array<juce::ImageButton, 4> mBankButtons;
    std::array<juce::ImageButton, 16> mTrackButtons;
    juce::ImageButton mProgramUpButton;
    juce::ImageButton mProgramDownButton;
    juce::Label mProgramValueLabel;
    juce::ComboBox mEngineBox;
    juce::ComboBox mVoiceAllocBox;
    juce::TextButton mPlayerCodeButton { "Player Code..." };
    juce::TextButton mExportSpcButton { "Export SPC..." };
    juce::Label mProgramNameLabel;
    juce::Label mAramLabel;
    juce::Label mStatusLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> mEngineAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> mVoiceAllocAttachment;

    std::unique_ptr<juce::FileChooser> mFileChooser;
    juce::File mLastBrowseDir;
    juce::File mLastExportDir;
    juce::int64 mStatusOverrideUntil = 0;
    bool mHardwareConnected = false;
    int mDisplayedBank = 0;
    int mDisplayedChannel = 0;
    std::array<int, 16> mDisplayedTrackActivity {};

    static constexpr int kEditorWidth = 536;
    static constexpr int kEditorHeight = 406;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(C700AudioProcessorEditor)
};
