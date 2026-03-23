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
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

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
    void setStatusMessage(const juce::String& message, int durationMs = 3000);
    void configureEditorField(juce::TextEditor& editor,
                              juce::Justification justification,
                              int maxChars,
                              const juce::String& allowedChars = {});
    int currentProgram() const;
    int resolveEditProgram(const juce::TextEditor& editor, int trackedProgram) const;
    void commitPendingFieldEdits();
    void commitIntegerPropertyEditor(int propertyId,
                                     juce::TextEditor& editor,
                                     int minValue,
                                     int maxValue,
                                     int& trackedProgram);
    void commitDoublePropertyEditor(int propertyId,
                                    juce::TextEditor& editor,
                                    double minValue,
                                    double maxValue,
                                    int decimals,
                                    int& trackedProgram);
    void commitProgramNameEditor();
    void commitLoopPointEditor();
    void shiftLoopPoint(int deltaBlocks);
    void autoDetectCurrentRate();
    void autoDetectCurrentBaseKey();
    void syncEditorFields(int program);
    void syncValueLabels();

    C700AudioProcessor& processorRef;

    std::unique_ptr<juce::LookAndFeel_V4> mBitmapLookAndFeel;
    juce::Image mBackgroundImage;
    juce::Image mHardwareStrip;
    juce::Image mTrackIndicatorStrip;
    juce::Image mProgramRockerStrip;
    juce::Image mCheckStrip;
    juce::Image mKnobImage;
    juce::Image mSliderThumbImage;
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
    juce::ImageButton mLoopPointUpButton;
    juce::ImageButton mLoopPointDownButton;
    juce::ImageButton mAutoRateButton;
    juce::ImageButton mAutoKeyButton;
    juce::Label mProgramValueLabel;
    juce::ComboBox mEngineBox;
    juce::ComboBox mVoiceAllocBox;
    juce::TextButton mPlayerCodeButton { "Player Code..." };
    juce::TextButton mExportSpcButton { "Export SPC..." };
    juce::Label mProgramNameLabel;
    juce::TextEditor mProgramNameEditor;
    juce::TextEditor mLoopPointEditor;
    juce::TextEditor mRateEditor;
    juce::TextEditor mBaseKeyEditor;
    juce::TextEditor mLowKeyEditor;
    juce::TextEditor mHighKeyEditor;
    std::array<juce::Slider, 5> mEnvelopeSliders;
    std::array<juce::Label, 5> mEnvelopeValueLabels;
    juce::Slider mVolLKnob;
    juce::Slider mVolRKnob;
    juce::Label mVolLValueLabel;
    juce::Label mVolRValueLabel;
    juce::Slider mVibrateKnob;
    juce::Slider mVibDepthKnob;
    juce::Label mVibrateValueLabel;
    juce::Label mVibDepthValueLabel;
    juce::ToggleButton mLoopToggle { "Loop" };
    juce::ToggleButton mEchoToggle { "Echo" };
    juce::ToggleButton mPitchModToggle { "PMOD" };
    juce::ToggleButton mNoiseToggle { "Noise" };
    juce::ToggleButton mMonoToggle { "Mono" };
    juce::ToggleButton mGlideToggle { "Glide" };
    juce::Label mPortamentoRateLabel;
    juce::Label mNotePriorityValueLabel;
    juce::Label mReleasePriorityValueLabel;
    juce::Label mAramLabel;
    juce::Label mStatusLabel;

    std::unique_ptr<SliderAttachment> mArAttachment;
    std::unique_ptr<SliderAttachment> mDrAttachment;
    std::unique_ptr<SliderAttachment> mSlAttachment;
    std::unique_ptr<SliderAttachment> mSr1Attachment;
    std::unique_ptr<SliderAttachment> mSr2Attachment;
    std::unique_ptr<SliderAttachment> mVolLAttachment;
    std::unique_ptr<SliderAttachment> mVolRAttachment;
    std::unique_ptr<SliderAttachment> mVibrateAttachment;
    std::unique_ptr<SliderAttachment> mVibDepthAttachment;
    std::unique_ptr<ButtonAttachment> mLoopAttachment;
    std::unique_ptr<ButtonAttachment> mEchoAttachment;
    std::unique_ptr<ButtonAttachment> mPitchModAttachment;
    std::unique_ptr<ButtonAttachment> mNoiseAttachment;
    std::unique_ptr<ButtonAttachment> mMonoAttachment;
    std::unique_ptr<ButtonAttachment> mGlideAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> mEngineAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> mVoiceAllocAttachment;

    std::unique_ptr<juce::FileChooser> mFileChooser;
    juce::File mLastBrowseDir;
    juce::File mLastExportDir;
    juce::int64 mStatusOverrideUntil = 0;
    int mProgramNameEditProgram = -1;
    int mLoopPointEditProgram = -1;
    int mRateEditProgram = -1;
    int mBaseKeyEditProgram = -1;
    int mLowKeyEditProgram = -1;
    int mHighKeyEditProgram = -1;
    bool mHardwareConnected = false;
    int mDisplayedBank = 0;
    int mDisplayedChannel = 0;
    std::array<int, 16> mDisplayedTrackActivity {};

    static constexpr int kEditorWidth = 536;
    static constexpr int kEditorHeight = 406;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(C700AudioProcessorEditor)
};
