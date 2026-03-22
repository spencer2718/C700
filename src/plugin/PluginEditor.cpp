#include "PluginEditor.h"
#include "C700Kernel.h"

// --- Helpers ---

void C700AudioProcessorEditor::addSlider(ParamSlider& ps, const juce::String& paramID, const juce::String& name)
{
    ps.label.setText(name, juce::dontSendNotification);
    ps.label.setFont(juce::Font(13.0f));
    ps.label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    ps.label.setJustificationType(juce::Justification::centredRight);
    mParamPanel.addAndMakeVisible(ps.label);

    ps.slider.setSliderStyle(juce::Slider::LinearHorizontal);
    ps.slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 24);
    ps.slider.setTextBoxIsEditable(true);
    mParamPanel.addAndMakeVisible(ps.slider);

    ps.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), paramID, ps.slider);
}

void C700AudioProcessorEditor::addToggle(ParamToggle& pt, const juce::String& paramID, const juce::String& name)
{
    pt.button.setButtonText(name);
    pt.button.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    mParamPanel.addAndMakeVisible(pt.button);

    pt.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getAPVTS(), paramID, pt.button);
}

void C700AudioProcessorEditor::addSectionHeader(const juce::String& text)
{
    auto h = std::make_unique<juce::Label>();
    h->setText(text, juce::dontSendNotification);
    h->setFont(juce::Font(14.0f, juce::Font::bold));
    h->setColour(juce::Label::textColourId, juce::Colours::white);
    h->setColour(juce::Label::backgroundColourId, juce::Colour(0xff404040));
    h->setJustificationType(juce::Justification::centredLeft);
    mParamPanel.addAndMakeVisible(h.get());
    mHeaders.push_back(std::move(h));
}

void C700AudioProcessorEditor::layoutRow(juce::Rectangle<int>& area, juce::Component& label, juce::Component& control)
{
    auto row = area.removeFromTop(kRowHeight).reduced(4, 1);
    label.setBounds(row.removeFromLeft(kLabelWidth));
    row.removeFromLeft(4);
    control.setBounds(row);
}

void C700AudioProcessorEditor::layoutHeader(juce::Rectangle<int>& area, juce::Label& header)
{
    header.setBounds(area.removeFromTop(kHeaderHeight).reduced(2, 2));
}

// --- Constructor ---

C700AudioProcessorEditor::C700AudioProcessorEditor(C700AudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // Top bar buttons
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

    mLoadPlayerCodeButton.onClick = [this] { loadPlayerCodeClicked(); };
    addAndMakeVisible(mLoadPlayerCodeButton);

    mExportSpcButton.onClick = [this] { exportSpcClicked(); };
    addAndMakeVisible(mExportSpcButton);

    mStatusLabel.setFont(juce::Font(13.0f));
    mStatusLabel.setColour(juce::Label::textColourId, juce::Colours::lightyellow);
    mStatusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(mStatusLabel);

    // --- Parameter panel (scrollable) ---
    addAndMakeVisible(mViewport);
    mViewport.setViewedComponent(&mParamPanel, false);
    mViewport.setScrollBarsShown(true, false);

    // Instrument
    addSectionHeader("INSTRUMENT");
    addSlider(sProgram, "program", "Program");
    addSlider(sAR, "ar", "Attack (AR)");
    addSlider(sDR, "dr", "Decay (DR)");
    addSlider(sSL, "sl", "Sustain Lv (SL)");
    addSlider(sSR1, "sr1", "Sustain Rate");
    addSlider(sSR2, "sr2", "Release (SR2)");
    addToggle(tSustainMode, "sustainmode", "Sustain Mode");
    addSlider(sBaseKey, "basekey", "Base Key");
    addSlider(sRate, "rate", "Sample Rate");
    addToggle(tLoop, "loop", "Loop Enable");
    addToggle(tEcho, "echo", "Echo Enable");

    // Volume
    addSectionHeader("VOLUME");
    addSlider(sVolume, "volume", "Output Volume");
    addSlider(sVolL, "voll", "Inst Vol L");
    addSlider(sVolR, "volr", "Inst Vol R");
    addSlider(sMainVolL, "mainvol_l", "Main Vol L");
    addSlider(sMainVolR, "mainvol_r", "Main Vol R");

    // Echo
    addSectionHeader("ECHO");
    addSlider(sEchoDelay, "echodelay", "Echo Delay");
    addSlider(sEchoFB, "echofb", "Echo Feedback");
    addSlider(sEchoVolL, "echovol_l", "Echo Vol L");
    addSlider(sEchoVolR, "echovol_r", "Echo Vol R");

    // Recording
    addSectionHeader("SPC RECORDING");
    addSlider(sRecStart, "rec_start", "Record Start");
    addSlider(sRecLoop, "rec_loop", "Record Loop");
    addSlider(sRecEnd, "rec_end", "Record End");

    // Info
    addSectionHeader("INFO");
    addSlider(sARAM, "aram", "ARAM Used");
    sARAM.slider.setEnabled(false);

    // Defaults
    mLastBrowseDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
    mLastExportDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory);

    setResizable(true, true);
    setResizeLimits(450, 400, 1200, 1200);
    setSize(600, 700);

    startTimerHz(10);
    timerCallback();
}

C700AudioProcessorEditor::~C700AudioProcessorEditor()
{
    stopTimer();
}

// --- Paint / Layout ---

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

    // Row 1
    auto row1 = topBar.removeFromTop(kTopBarHeight / 2).reduced(6, 4);
    mLoadButton.setBounds(row1.removeFromLeft(120));
    row1.removeFromLeft(8);
    mSlotLabel.setBounds(row1.removeFromLeft(70));
    row1.removeFromLeft(4);
    mSampleNameLabel.setBounds(row1);

    // Row 2
    auto row2 = topBar.reduced(6, 4);
    mLoadPlayerCodeButton.setBounds(row2.removeFromLeft(140));
    row2.removeFromLeft(6);
    mExportSpcButton.setBounds(row2.removeFromLeft(110));
    row2.removeFromLeft(6);
    mStatusLabel.setBounds(row2);

    // Scrollable param panel
    mViewport.setBounds(area);

    int panelWidth = mViewport.getMaximumVisibleWidth();
    auto cursor = juce::Rectangle<int>(0, 0, panelWidth, 10000);

    // Count total height needed
    int headerIdx = 0;
    auto layRow = [&](juce::Component& lbl, juce::Component& ctrl) {
        layoutRow(cursor, lbl, ctrl);
    };
    auto layToggle = [&](juce::Component& ctrl) {
        auto row = cursor.removeFromTop(kRowHeight).reduced(4, 1);
        row.removeFromLeft(kLabelWidth + 4);
        ctrl.setBounds(row);
    };
    auto layHdr = [&]() {
        if (headerIdx < static_cast<int>(mHeaders.size()))
            layoutHeader(cursor, *mHeaders[static_cast<size_t>(headerIdx++)]);
    };

    // Instrument
    layHdr();
    layRow(sProgram.label, sProgram.slider);
    layRow(sAR.label, sAR.slider);
    layRow(sDR.label, sDR.slider);
    layRow(sSL.label, sSL.slider);
    layRow(sSR1.label, sSR1.slider);
    layRow(sSR2.label, sSR2.slider);
    layToggle(tSustainMode.button);
    layRow(sBaseKey.label, sBaseKey.slider);
    layRow(sRate.label, sRate.slider);
    layToggle(tLoop.button);
    layToggle(tEcho.button);

    // Volume
    layHdr();
    layRow(sVolume.label, sVolume.slider);
    layRow(sVolL.label, sVolL.slider);
    layRow(sVolR.label, sVolR.slider);
    layRow(sMainVolL.label, sMainVolL.slider);
    layRow(sMainVolR.label, sMainVolR.slider);

    // Echo
    layHdr();
    layRow(sEchoDelay.label, sEchoDelay.slider);
    layRow(sEchoFB.label, sEchoFB.slider);
    layRow(sEchoVolL.label, sEchoVolL.slider);
    layRow(sEchoVolR.label, sEchoVolR.slider);

    // Recording
    layHdr();
    layRow(sRecStart.label, sRecStart.slider);
    layRow(sRecLoop.label, sRecLoop.slider);
    layRow(sRecEnd.label, sRecEnd.slider);

    // Info
    layHdr();
    layRow(sARAM.label, sARAM.slider);

    mParamPanel.setSize(panelWidth, 10000 - cursor.getHeight());
}

// --- Timer ---

void C700AudioProcessorEditor::timerCallback()
{
    int prog = static_cast<int>(processorRef.getAPVTS().getRawParameterValue("program")->load());

    if (prog != mDisplayedProgram) {
        mDisplayedProgram = prog;
        mSlotLabel.setText("Slot " + juce::String(prog) + ":", juce::dontSendNotification);
    }

    auto name = processorRef.getAdapter().getSampleName(prog);
    mSampleNameLabel.setText(juce::String(name), juce::dontSendNotification);

    bool hasPC = processorRef.getAdapter().hasPlayerCode();
    mExportSpcButton.setEnabled(hasPC);
    if (juce::Time::currentTimeMillis() < mStatusOverrideUntil)
        return;
    if (!hasPC) {
        mStatusLabel.setText("Load playercode.bin first", juce::dontSendNotification);
    } else if (mStatusLabel.getText() == "Load playercode.bin first") {
        mStatusLabel.setText("SPC export ready", juce::dontSendNotification);
    }
}

// --- File dialogs ---

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
            if (processorRef.getAdapter().loadSampleToSlot(prog, file.getFullPathName().toStdString()))
                processorRef.forceParamSync();
        });
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
        return;
    }

    float recStart = processorRef.getAPVTS().getRawParameterValue("rec_start")->load();
    float recEnd = processorRef.getAPVTS().getRawParameterValue("rec_end")->load();
    if (recStart >= recEnd) {
        mStatusLabel.setText("Set Record Start/End beats first (End > Start)",
                            juce::dontSendNotification);
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
