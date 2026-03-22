#pragma once
#include "PluginProcessor.h"

struct ParamSlider {
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

struct ParamToggle {
    juce::ToggleButton button;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};

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

    void addSlider(ParamSlider& ps, const juce::String& paramID, const juce::String& name);
    void addToggle(ParamToggle& pt, const juce::String& paramID, const juce::String& name);
    void addSectionHeader(const juce::String& text);
    void layoutRow(juce::Rectangle<int>& area, juce::Component& label, juce::Component& control);
    void layoutHeader(juce::Rectangle<int>& area, juce::Label& header);

    C700AudioProcessor& processorRef;

    // Top bar
    juce::TextButton mLoadButton { "Load Sample" };
    juce::TextButton mLoadPlayerCodeButton { "Load Player Code" };
    juce::TextButton mExportSpcButton { "Export SPC..." };
    juce::Label mSlotLabel, mSampleNameLabel, mStatusLabel;

    // Scrollable param panel
    juce::Viewport mViewport;
    juce::Component mParamPanel;

    // Section headers
    std::vector<std::unique_ptr<juce::Label>> mHeaders;

    // Instrument params
    ParamSlider sProgram, sAR, sDR, sSL, sSR1, sSR2, sVolL, sVolR, sBaseKey, sRate;
    ParamToggle tSustainMode, tEcho, tLoop;

    // Volume
    ParamSlider sVolume;

    // Global DSP
    ParamSlider sMainVolL, sMainVolR, sEchoDelay, sEchoFB, sEchoVolL, sEchoVolR;

    // Recording
    ParamSlider sRecStart, sRecLoop, sRecEnd;

    // Info
    ParamSlider sARAM;

    std::unique_ptr<juce::FileChooser> mFileChooser;
    juce::File mLastBrowseDir, mLastExportDir;
    int mDisplayedProgram = -1;
    juce::int64 mStatusOverrideUntil = 0;

    static constexpr int kTopBarHeight = 90;
    static constexpr int kRowHeight = 28;
    static constexpr int kHeaderHeight = 30;
    static constexpr int kLabelWidth = 130;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(C700AudioProcessorEditor)
};
