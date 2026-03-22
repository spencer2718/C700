#include "PluginEditor.h"
#include "BinaryData.h"
#include "C700Kernel.h"

namespace
{
juce::Image loadBundledImage(const char* data, int size)
{
    return juce::ImageCache::getFromMemory(data, size);
}

juce::Image cropFilmStripFrame(const juce::Image& strip, int frameIndex, int frameCount)
{
    if (!strip.isValid() || frameCount <= 0)
        return {};

    const int frameHeight = strip.getHeight() / frameCount;
    if (frameHeight <= 0)
        return {};

    juce::Image frame(strip.getFormat(), strip.getWidth(), frameHeight, true);
    juce::Graphics g(frame);
    g.drawImageAt(strip, 0, -frameIndex * frameHeight);
    return frame;
}

void configureFilmStripButton(juce::ImageButton& button, const juce::Image& strip)
{
    auto up = cropFilmStripFrame(strip, 0, 2);
    auto down = cropFilmStripFrame(strip, 1, 2);
    button.setImages(false, true, true,
                     up, 1.0f, {},
                     up, 0.85f, {},
                     down, 1.0f, {});
    button.setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void configureUtilityButton(juce::TextButton& button)
{
    button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xaa152032));
    button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xcc243046));
    button.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffd8f8ff));
    button.setColour(juce::TextButton::textColourOnId, juce::Colour(0xffd8f8ff));
}
}

C700AudioProcessorEditor::C700AudioProcessorEditor(C700AudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    mBackgroundImage = loadBundledImage(BinaryData::AUBackground_png, BinaryData::AUBackground_pngSize);
    mHardwareStrip = loadBundledImage(BinaryData::hwconn_png, BinaryData::hwconn_pngSize);

    configureFilmStripButton(mLoadButton,
                             loadBundledImage(BinaryData::bt_load_png, BinaryData::bt_load_pngSize));
    mLoadButton.onClick = [this] { loadSampleClicked(); };
    addAndMakeVisible(mLoadButton);

    configureFilmStripButton(mUnloadButton,
                             loadBundledImage(BinaryData::bt_unload_png, BinaryData::bt_unload_pngSize));
    mUnloadButton.onClick = [this] { unloadSampleClicked(); };
    addAndMakeVisible(mUnloadButton);

    mProgramStepper.setSliderStyle(juce::Slider::IncDecButtons);
    mProgramStepper.setRange(0.0, 127.0, 1.0);
    mProgramStepper.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 24, 14);
    mProgramStepper.setIncDecButtonsMode(juce::Slider::incDecButtonsDraggable_AutoDirection);
    mProgramStepper.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffd8f8ff));
    mProgramStepper.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    mProgramStepper.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xaa1a2430));
    addAndMakeVisible(mProgramStepper);
    mProgramAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), "program", mProgramStepper);

    mEngineBox.addItem("Old", 1);
    mEngineBox.addItem("Relaxed", 2);
    mEngineBox.addItem("Accurate", 3);
    addAndMakeVisible(mEngineBox);
    mEngineAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.getAPVTS(), "engine", mEngineBox);

    mVoiceAllocBox.addItem("Oldest", 1);
    mVoiceAllocBox.addItem("SameCh", 2);
    addAndMakeVisible(mVoiceAllocBox);
    mVoiceAllocAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.getAPVTS(), "voicealloc", mVoiceAllocBox);

    configureUtilityButton(mPlayerCodeButton);
    mPlayerCodeButton.onClick = [this] { loadPlayerCodeClicked(); };
    addAndMakeVisible(mPlayerCodeButton);

    configureUtilityButton(mExportSpcButton);
    mExportSpcButton.onClick = [this] { exportSpcClicked(); };
    addAndMakeVisible(mExportSpcButton);

    mProgramNameLabel.setFont(juce::Font(juce::FontOptions(11.0f)));
    mProgramNameLabel.setColour(juce::Label::textColourId, juce::Colour(0xffd8f8ff));
    mProgramNameLabel.setJustificationType(juce::Justification::centredLeft);
    mProgramNameLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(mProgramNameLabel);

    mStatusLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    mStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffd8f8ff));
    mStatusLabel.setJustificationType(juce::Justification::centredLeft);
    mStatusLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(mStatusLabel);

    mLastBrowseDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
    mLastExportDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory);

    setResizable(false, false);
    setSize(kEditorWidth, kEditorHeight);

    processorRef.forceParamSync();
    startTimerHz(10);
    timerCallback();
}

C700AudioProcessorEditor::~C700AudioProcessorEditor()
{
    stopTimer();
}

void C700AudioProcessorEditor::paint(juce::Graphics& g)
{
    if (mBackgroundImage.isValid())
        g.drawImageAt(mBackgroundImage, 0, 0);
    else
        g.fillAll(juce::Colour(0xff1f2732));

    if (mHardwareStrip.isValid()) {
        auto frame = cropFilmStripFrame(mHardwareStrip, mHardwareConnected ? 1 : 0, 2);
        g.drawImageAt(frame, 520, 376);
    }
}

void C700AudioProcessorEditor::resized()
{
    mProgramStepper.setBounds(110, 87, 40, 19);
    mProgramNameLabel.setBounds(153, 91, 249, 16);
    mEngineBox.setBounds(40, 61, 50, 14);
    mVoiceAllocBox.setBounds(145, 61, 50, 14);

    mLoadButton.setBounds(184, 248, 40, 14);
    mUnloadButton.setBounds(361, 248, 41, 14);

    mPlayerCodeButton.setBounds(8, 382, 112, 14);
    mExportSpcButton.setBounds(180, 382, 160, 14);
    mStatusLabel.setBounds(8, 364, 340, 14);
}

void C700AudioProcessorEditor::timerCallback()
{
    const auto state = processorRef.getRuntimeState();

    mProgramNameLabel.setText(state.programName.isNotEmpty() ? state.programName
                                                             : state.sampleName,
                              juce::dontSendNotification);
    mHardwareConnected = state.isHwConnected;
    repaint(520, 376, 16, 16);

    mExportSpcButton.setEnabled(state.hasPlayerCode);

    if (state.finishedRecording) {
        processorRef.getAdapter().enableSpcRecording(false);
        mStatusLabel.setText("SPC saved!", juce::dontSendNotification);
        mStatusOverrideUntil = juce::Time::currentTimeMillis() + 5000;
    }

    if (juce::Time::currentTimeMillis() < mStatusOverrideUntil)
        return;

    if (!state.hasPlayerCode)
        mStatusLabel.setText("Load playercode.bin to enable SPC export", juce::dontSendNotification);
    else
        mStatusLabel.setText("Shell pass: fixed background and core controls wired", juce::dontSendNotification);
}

void C700AudioProcessorEditor::loadSampleClicked()
{
    int prog = static_cast<int>(processorRef.getAPVTS().getRawParameterValue("program")->load());

    mFileChooser = std::make_unique<juce::FileChooser>(
        "Load Sample to Slot " + juce::String(prog),
        mLastBrowseDir, "*.wav;*.brr");

    mFileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, prog](const juce::FileChooser& fc) {
            auto results = fc.getResults();
            if (results.isEmpty()) return;

            auto file = results.getFirst();
            mLastBrowseDir = file.getParentDirectory();
            if (processorRef.getAdapter().loadSampleToSlot(prog, file.getFullPathName().toStdString())) {
                processorRef.forceParamSync();
                mStatusLabel.setText("Loaded sample into slot " + juce::String(prog),
                                     juce::dontSendNotification);
            } else {
                auto err = processorRef.getAdapter().getLastLoadError();
                mStatusLabel.setText(err.empty() ? "Failed to load sample" : juce::String(err),
                                     juce::dontSendNotification);
            }
            mStatusOverrideUntil = juce::Time::currentTimeMillis() + 4000;
        });
}

void C700AudioProcessorEditor::unloadSampleClicked()
{
    int prog = static_cast<int>(processorRef.getAPVTS().getRawParameterValue("program")->load());
    if (processorRef.getAdapter().unloadSlot(prog)) {
        processorRef.forceParamSync();
        mStatusLabel.setText("Unloaded slot " + juce::String(prog), juce::dontSendNotification);
    } else {
        auto err = processorRef.getAdapter().getLastLoadError();
        mStatusLabel.setText(err.empty() ? "Failed to unload sample" : juce::String(err),
                             juce::dontSendNotification);
    }
    mStatusOverrideUntil = juce::Time::currentTimeMillis() + 3000;
}

void C700AudioProcessorEditor::loadPlayerCodeClicked()
{
    mFileChooser = std::make_unique<juce::FileChooser>(
        "Load Player Code Binary", mLastBrowseDir, "*.bin");

    mFileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc) {
            auto results = fc.getResults();
            if (results.isEmpty()) return;

            auto file = results.getFirst();
            mLastBrowseDir = file.getParentDirectory();
            bool ok = processorRef.getAdapter().loadPlayerCode(file.getFullPathName().toStdString());
            mStatusLabel.setText(ok ? "Player code loaded" : "Failed to load playercode.bin",
                                 juce::dontSendNotification);
            mStatusOverrideUntil = juce::Time::currentTimeMillis() + 3000;
        });
}

void C700AudioProcessorEditor::exportSpcClicked()
{
    if (!processorRef.getAdapter().hasPlayerCode()) {
        mStatusLabel.setText("Load playercode.bin first", juce::dontSendNotification);
        mStatusOverrideUntil = juce::Time::currentTimeMillis() + 3000;
        return;
    }

    float recStart = processorRef.getAPVTS().getRawParameterValue("rec_start")->load();
    float recEnd = processorRef.getAPVTS().getRawParameterValue("rec_end")->load();
    if (recStart >= recEnd) {
        mStatusLabel.setText("Set Record Start/End beats first", juce::dontSendNotification);
        mStatusOverrideUntil = juce::Time::currentTimeMillis() + 4000;
        return;
    }

    mFileChooser = std::make_unique<juce::FileChooser>(
        "Export SPC", mLastExportDir, "*.spc");

    mFileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc) {
            auto results = fc.getResults();
            if (results.isEmpty()) return;

            auto file = results.getFirst();
            mLastExportDir = file.getParentDirectory();

            processorRef.getAdapter().setSpcRecordPath(
                file.getParentDirectory().getFullPathName().toStdString());
            auto* dsp = processorRef.getAdapter().getKernel()->GetDriver()->GetDsp();
            auto title = file.getFileNameWithoutExtension().toStdString();
            dsp->SetSongTitle(title.substr(0, 31).c_str());
            processorRef.getAdapter().enableSpcRecording(true);

            mStatusLabel.setText("Recording armed. Play through the region in REAPER.",
                                 juce::dontSendNotification);
            mStatusOverrideUntil = juce::Time::currentTimeMillis() + 5000;
        });
}
