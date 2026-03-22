#include "PluginEditor.h"
#include "C700Kernel.h"

static const int kTopBarHeight = 90;

C700AudioProcessorEditor::C700AudioProcessorEditor(C700AudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // Row 1: Load Sample
    mLoadButton.onClick = [this] { loadSampleClicked(); };
    addAndMakeVisible(mLoadButton);

    mSlotLabel.setFont(juce::Font(14.0f));
    mSlotLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    mSlotLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(mSlotLabel);

    mSampleNameLabel.setFont(juce::Font(14.0f));
    mSampleNameLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    mSampleNameLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(mSampleNameLabel);

    // Row 2: SPC export
    mLoadPlayerCodeButton.onClick = [this] { loadPlayerCodeClicked(); };
    addAndMakeVisible(mLoadPlayerCodeButton);

    mExportSpcButton.onClick = [this] { exportSpcClicked(); };
    addAndMakeVisible(mExportSpcButton);

    mStatusLabel.setFont(juce::Font(13.0f));
    mStatusLabel.setColour(juce::Label::textColourId, juce::Colours::lightyellow);
    mStatusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(mStatusLabel);

    // Generic editor for all APVTS parameters
    mGenericEditor = std::make_unique<juce::GenericAudioProcessorEditor>(p);
    addAndMakeVisible(mGenericEditor.get());

    mLastBrowseDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
    mLastExportDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory);

    setSize(500, kTopBarHeight + mGenericEditor->getHeight());

    startTimerHz(10);
    timerCallback();
}

C700AudioProcessorEditor::~C700AudioProcessorEditor()
{
    stopTimer();
}

void C700AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2a2a));
    g.setColour(juce::Colour(0xff3a3a3a));
    g.fillRect(0, 0, getWidth(), kTopBarHeight);
}

void C700AudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    auto topBar = area.removeFromTop(kTopBarHeight);

    // Row 1: sample loading
    auto row1 = topBar.removeFromTop(kTopBarHeight / 2).reduced(6, 4);
    mLoadButton.setBounds(row1.removeFromLeft(120));
    row1.removeFromLeft(8);
    mSlotLabel.setBounds(row1.removeFromLeft(70));
    row1.removeFromLeft(4);
    mSampleNameLabel.setBounds(row1);

    // Row 2: SPC export
    auto row2 = topBar.reduced(6, 4);
    mLoadPlayerCodeButton.setBounds(row2.removeFromLeft(140));
    row2.removeFromLeft(6);
    mExportSpcButton.setBounds(row2.removeFromLeft(110));
    row2.removeFromLeft(6);
    mStatusLabel.setBounds(row2);

    if (mGenericEditor)
        mGenericEditor->setBounds(area);
}

void C700AudioProcessorEditor::timerCallback()
{
    int prog = static_cast<int>(processorRef.getAPVTS().getRawParameterValue("program")->load());

    if (prog != mDisplayedProgram) {
        mDisplayedProgram = prog;
        mSlotLabel.setText("Slot " + juce::String(prog) + ":", juce::dontSendNotification);
    }

    auto name = processorRef.getAdapter().getSampleName(prog);
    mSampleNameLabel.setText(juce::String(name), juce::dontSendNotification);

    // Update SPC button state (skip if a timed status message is showing)
    bool hasPC = processorRef.getAdapter().hasPlayerCode();
    mExportSpcButton.setEnabled(hasPC);
    if (juce::Time::currentTimeMillis() < mStatusOverrideUntil)
        return; // don't overwrite timed status messages
    if (!hasPC) {
        mStatusLabel.setText("Load playercode.bin first", juce::dontSendNotification);
    } else if (mStatusLabel.getText() == "Load playercode.bin first") {
        mStatusLabel.setText("SPC export ready", juce::dontSendNotification);
    }
}

void C700AudioProcessorEditor::loadSampleClicked()
{
    int prog = static_cast<int>(processorRef.getAPVTS().getRawParameterValue("program")->load());

    mFileChooser = std::make_unique<juce::FileChooser>(
        "Load Sample to Slot " + juce::String(prog),
        mLastBrowseDir,
        "*.wav;*.brr");

    mFileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, prog](const juce::FileChooser& fc)
        {
            auto results = fc.getResults();
            if (results.isEmpty()) return;

            auto file = results.getFirst();
            mLastBrowseDir = file.getParentDirectory();

            bool ok = processorRef.getAdapter().loadSampleToSlot(prog, file.getFullPathName().toStdString());
            if (ok)
                processorRef.forceParamSync();
        });
}

void C700AudioProcessorEditor::loadPlayerCodeClicked()
{
    mFileChooser = std::make_unique<juce::FileChooser>(
        "Load Player Code Binary",
        mLastBrowseDir,
        "*.bin");

    mFileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
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
        return;
    }

    // Verify record region is set
    float recStart = processorRef.getAPVTS().getRawParameterValue("rec_start")->load();
    float recEnd = processorRef.getAPVTS().getRawParameterValue("rec_end")->load();
    if (recStart >= recEnd) {
        mStatusLabel.setText("Set Record Start/End beats first (End must be > Start)",
                            juce::dontSendNotification);
        mStatusOverrideUntil = juce::Time::currentTimeMillis() + 4000;
        return;
    }

    mFileChooser = std::make_unique<juce::FileChooser>(
        "Export SPC — set record region in REAPER, then play through it",
        mLastExportDir,
        "*.spc");

    mFileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            auto results = fc.getResults();
            if (results.isEmpty()) return;

            auto file = results.getFirst();
            mLastExportDir = file.getParentDirectory();

            // Set the record output directory to the chosen file's parent
            processorRef.getAdapter().setSpcRecordPath(
                file.getParentDirectory().getFullPathName().toStdString());

            // Set song title from filename
            auto* dsp = processorRef.getAdapter().getKernel()->GetDriver()->GetDsp();
            auto title = file.getFileNameWithoutExtension().toStdString();
            dsp->SetSongTitle(title.substr(0, 31).c_str());

            // Enable SPC recording
            processorRef.getAdapter().enableSpcRecording(true);

            mStatusLabel.setText("Recording armed. Set record region & play in REAPER.",
                                juce::dontSendNotification);
            mStatusOverrideUntil = juce::Time::currentTimeMillis() + 5000;

            // Show instructions in an alert
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::InfoIcon,
                "SPC Recording Armed",
                "1. In C700's parameters, set RecordStartBeat and RecordEndBeat\n"
                "   (use REAPER's transport position as reference)\n"
                "2. Play through the region in REAPER\n"
                "3. The .spc file will be saved automatically when playback\n"
                "   reaches the end beat position.\n\n"
                "Output: " + file.getParentDirectory().getFullPathName());
        });
}
