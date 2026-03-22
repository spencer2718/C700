#include "PluginEditor.h"

#include "BinaryData.h"
#include "C700Kernel.h"

namespace
{
const auto kValueTextColour = juce::Colour(0xffd8f8ff);
const auto kStaticTextColour = juce::Colour(0xcc141414);
const auto kFieldColour = juce::Colour(0xaa22303f);

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

juce::Image cropImageRegion(const juce::Image& source, int y, int height)
{
    if (!source.isValid() || height <= 0)
        return {};

    juce::Image frame(source.getFormat(), source.getWidth(), height, true);
    juce::Graphics g(frame);
    g.drawImageAt(source, 0, -y);
    return frame;
}

juce::Image composeImages(const juce::Image& base, const juce::Image& overlay)
{
    if (!base.isValid())
        return overlay;
    if (!overlay.isValid())
        return base;

    juce::Image composed(juce::Image::ARGB,
                         juce::jmax(base.getWidth(), overlay.getWidth()),
                         juce::jmax(base.getHeight(), overlay.getHeight()),
                         true);
    juce::Graphics g(composed);
    g.drawImageAt(base, 0, 0);
    g.drawImageAt(overlay, 0, 0);
    return composed;
}

void configureMomentaryButton(juce::ImageButton& button, const juce::Image& up, const juce::Image& down)
{
    button.setImages(false, true, true,
                     up, 1.0f, {},
                     up, 0.95f, {},
                     down, 1.0f, {});
    button.setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void configureFilmStripButton(juce::ImageButton& button, const juce::Image& strip)
{
    configureMomentaryButton(button,
                             cropFilmStripFrame(strip, 0, 2),
                             cropFilmStripFrame(strip, 1, 2));
}

void configureComboBox(juce::ComboBox& box)
{
    box.setColour(juce::ComboBox::backgroundColourId, kFieldColour);
    box.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    box.setColour(juce::ComboBox::textColourId, kValueTextColour);
    box.setColour(juce::ComboBox::arrowColourId, kValueTextColour);
    box.setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff22303f));
    box.setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff2e4358));
    box.setColour(juce::PopupMenu::textColourId, kValueTextColour);
    box.setTextWhenNothingSelected({});
}

void configureValueLabel(juce::Label& label, juce::Justification justification, float fontSize = 11.0f)
{
    label.setFont(juce::Font(juce::FontOptions(fontSize)));
    label.setColour(juce::Label::textColourId, kValueTextColour);
    label.setJustificationType(justification);
    label.setInterceptsMouseClicks(false, false);
}

void configureUtilityButton(juce::TextButton& button)
{
    button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xaa152032));
    button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xcc243046));
    button.setColour(juce::TextButton::textColourOffId, kValueTextColour);
    button.setColour(juce::TextButton::textColourOnId, kValueTextColour);
}

juce::String sanitiseFileStem(juce::String stem)
{
    stem = stem.trim();
    if (stem.isEmpty())
        return {};

    static const juce::String illegal = "\\/:*?\"<>|";
    for (auto c : illegal)
        stem = stem.replaceCharacter(c, '_');
    return stem;
}

juce::String defaultBrrFileName(int program, const juce::String& programName, const juce::String& sampleName)
{
    auto stem = sanitiseFileStem(programName);
    if (stem.isEmpty())
        stem = sanitiseFileStem(sampleName);
    if (stem.isEmpty())
        stem = juce::String::formatted("program_%03d", program);
    return stem + ".brr";
}

void setDiscreteParameterValue(juce::AudioProcessorValueTreeState& apvts,
                               const juce::String& parameterId,
                               int discreteValue)
{
    if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(parameterId))) {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(discreteValue)));
        parameter->endChangeGesture();
    }
}
}

C700AudioProcessorEditor::C700AudioProcessorEditor(C700AudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    mBackgroundImage = loadBundledImage(BinaryData::AUBackground_png, BinaryData::AUBackground_pngSize);
    mHardwareStrip = loadBundledImage(BinaryData::hwconn_png, BinaryData::hwconn_pngSize);
    mTrackIndicatorStrip = loadBundledImage(BinaryData::ind_track_png, BinaryData::ind_track_pngSize);
    mProgramRockerStrip = loadBundledImage(BinaryData::rocker_sw_png, BinaryData::rocker_sw_pngSize);

    mBankStrips[0] = loadBundledImage(BinaryData::bank_a_png, BinaryData::bank_a_pngSize);
    mBankStrips[1] = loadBundledImage(BinaryData::bank_b_png, BinaryData::bank_b_pngSize);
    mBankStrips[2] = loadBundledImage(BinaryData::bank_c_png, BinaryData::bank_c_pngSize);
    mBankStrips[3] = loadBundledImage(BinaryData::bank_d_png, BinaryData::bank_d_pngSize);

    mTrackNumberStrips[0] = loadBundledImage(BinaryData::track01__png, BinaryData::track01__pngSize);
    mTrackNumberStrips[1] = loadBundledImage(BinaryData::track02__png, BinaryData::track02__pngSize);
    mTrackNumberStrips[2] = loadBundledImage(BinaryData::track03__png, BinaryData::track03__pngSize);
    mTrackNumberStrips[3] = loadBundledImage(BinaryData::track04__png, BinaryData::track04__pngSize);
    mTrackNumberStrips[4] = loadBundledImage(BinaryData::track05__png, BinaryData::track05__pngSize);
    mTrackNumberStrips[5] = loadBundledImage(BinaryData::track06__png, BinaryData::track06__pngSize);
    mTrackNumberStrips[6] = loadBundledImage(BinaryData::track07__png, BinaryData::track07__pngSize);
    mTrackNumberStrips[7] = loadBundledImage(BinaryData::track08__png, BinaryData::track08__pngSize);
    mTrackNumberStrips[8] = loadBundledImage(BinaryData::track09__png, BinaryData::track09__pngSize);
    mTrackNumberStrips[9] = loadBundledImage(BinaryData::track10__png, BinaryData::track10__pngSize);
    mTrackNumberStrips[10] = loadBundledImage(BinaryData::track11__png, BinaryData::track11__pngSize);
    mTrackNumberStrips[11] = loadBundledImage(BinaryData::track12__png, BinaryData::track12__pngSize);
    mTrackNumberStrips[12] = loadBundledImage(BinaryData::track13__png, BinaryData::track13__pngSize);
    mTrackNumberStrips[13] = loadBundledImage(BinaryData::track14__png, BinaryData::track14__pngSize);
    mTrackNumberStrips[14] = loadBundledImage(BinaryData::track15__png, BinaryData::track15__pngSize);
    mTrackNumberStrips[15] = loadBundledImage(BinaryData::track16__png, BinaryData::track16__pngSize);

    configureFilmStripButton(mLoadButton,
                             loadBundledImage(BinaryData::bt_load_png, BinaryData::bt_load_pngSize));
    mLoadButton.onClick = [this] { loadSampleClicked(); };
    addAndMakeVisible(mLoadButton);

    configureFilmStripButton(mUnloadButton,
                             loadBundledImage(BinaryData::bt_unload_png, BinaryData::bt_unload_pngSize));
    mUnloadButton.onClick = [this] { unloadSampleClicked(); };
    addAndMakeVisible(mUnloadButton);

    configureFilmStripButton(mSaveButton,
                             loadBundledImage(BinaryData::bt_save_png, BinaryData::bt_save_pngSize));
    mSaveButton.onClick = [this] { saveSampleClicked(); };
    addAndMakeVisible(mSaveButton);

    configureFilmStripButton(mSaveXiButton,
                             loadBundledImage(BinaryData::bt_save_xi_png, BinaryData::bt_save_xi_pngSize));
    mSaveXiButton.onClick = [this] { saveXiClicked(); };
    addAndMakeVisible(mSaveXiButton);

    for (int bank = 0; bank < static_cast<int>(mBankButtons.size()); ++bank) {
        const auto offImage = cropFilmStripFrame(mBankStrips[bank], 0, 2);
        const auto onImage = cropFilmStripFrame(mBankStrips[bank], 1, 2);
        configureMomentaryButton(mBankButtons[bank], offImage, onImage);
        mBankButtons[bank].setTriggeredOnMouseDown(true);
        mBankButtons[bank].onClick = [this, bank] { selectBank(bank); };
        addAndMakeVisible(mBankButtons[bank]);
    }

    for (int channel = 0; channel < static_cast<int>(mTrackButtons.size()); ++channel) {
        const auto offImage = composeImages(cropFilmStripFrame(mTrackIndicatorStrip, 0, 2),
                                            cropFilmStripFrame(mTrackNumberStrips[channel], 0, 2));
        const auto onImage = composeImages(cropFilmStripFrame(mTrackIndicatorStrip, 1, 2),
                                           cropFilmStripFrame(mTrackNumberStrips[channel], 1, 2));
        configureMomentaryButton(mTrackButtons[channel], offImage, onImage);
        mTrackButtons[channel].setTriggeredOnMouseDown(true);
        mTrackButtons[channel].setTooltip("Edit Track " + juce::String(channel + 1));
        mTrackButtons[channel].onClick = [this, channel] { selectEditingChannel(channel); };
        addAndMakeVisible(mTrackButtons[channel]);
    }

    const auto rockerNeutral = cropFilmStripFrame(mProgramRockerStrip, 1, 3);
    const auto rockerUp = cropFilmStripFrame(mProgramRockerStrip, 0, 3);
    const auto rockerDown = cropFilmStripFrame(mProgramRockerStrip, 2, 3);
    configureMomentaryButton(mProgramUpButton,
                             cropImageRegion(rockerNeutral, 0, 9),
                             cropImageRegion(rockerUp, 0, 9));
    configureMomentaryButton(mProgramDownButton,
                             cropImageRegion(rockerNeutral, 9, 11),
                             cropImageRegion(rockerDown, 9, 11));
    mProgramUpButton.onClick = [this] { adjustProgram(1); };
    mProgramDownButton.onClick = [this] { adjustProgram(-1); };
    addAndMakeVisible(mProgramUpButton);
    addAndMakeVisible(mProgramDownButton);

    configureValueLabel(mProgramValueLabel, juce::Justification::centredRight);
    mProgramValueLabel.setColour(juce::Label::backgroundColourId, kFieldColour);
    mProgramValueLabel.setText("0", juce::dontSendNotification);
    addAndMakeVisible(mProgramValueLabel);

    mEngineBox.addItem("Old", 1);
    mEngineBox.addItem("Relaxed", 2);
    mEngineBox.addItem("Accurate", 3);
    configureComboBox(mEngineBox);
    addAndMakeVisible(mEngineBox);
    mEngineAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.getAPVTS(), "engine", mEngineBox);

    mVoiceAllocBox.addItem("Oldest", 1);
    mVoiceAllocBox.addItem("SameCh", 2);
    configureComboBox(mVoiceAllocBox);
    addAndMakeVisible(mVoiceAllocBox);
    mVoiceAllocAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.getAPVTS(), "voicealloc", mVoiceAllocBox);

    configureUtilityButton(mPlayerCodeButton);
    mPlayerCodeButton.onClick = [this] { loadPlayerCodeClicked(); };
    addAndMakeVisible(mPlayerCodeButton);

    configureUtilityButton(mExportSpcButton);
    mExportSpcButton.onClick = [this] { exportSpcClicked(); };
    addAndMakeVisible(mExportSpcButton);

    configureValueLabel(mProgramNameLabel, juce::Justification::centredLeft);
    addAndMakeVisible(mProgramNameLabel);

    configureValueLabel(mAramLabel, juce::Justification::centredRight, 10.0f);
    addAndMakeVisible(mAramLabel);

    configureValueLabel(mStatusLabel, juce::Justification::centredLeft, 10.0f);
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
        g.fillAll(juce::Colour(0xffc4c8ce));

    g.setColour(kStaticTextColour);
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText("Engine", 6, 63, 40, 11, juce::Justification::centredLeft);
    g.drawText("VoiceAlloc", 96, 63, 50, 11, juce::Justification::centredLeft);
    g.drawText("Bank", 12, 91, 30, 11, juce::Justification::centredLeft);

    if (mHardwareStrip.isValid()) {
        auto frame = cropFilmStripFrame(mHardwareStrip, mHardwareConnected ? 1 : 0, 2);
        g.drawImageAt(frame, 520, 376);
    }
}

void C700AudioProcessorEditor::resized()
{
    for (int channel = 0; channel < static_cast<int>(mTrackButtons.size()); ++channel)
        mTrackButtons[channel].setBounds(60 + channel * 20, 13, 20, 20);

    for (int bank = 0; bank < static_cast<int>(mBankButtons.size()); ++bank)
        mBankButtons[bank].setBounds(51 + bank * 15, 91, 12, 12);

    mEngineBox.setBounds(40, 61, 50, 12);
    mVoiceAllocBox.setBounds(145, 61, 50, 12);

    mProgramValueLabel.setBounds(114, 92, 23, 14);
    mProgramUpButton.setBounds(137, 87, 13, 9);
    mProgramDownButton.setBounds(137, 96, 13, 10);
    mProgramNameLabel.setBounds(153, 92, 249, 14);

    mLoadButton.setBounds(184, 248, 40, 14);
    mSaveButton.setBounds(230, 248, 68, 14);
    mSaveXiButton.setBounds(304, 248, 49, 14);
    mUnloadButton.setBounds(361, 248, 41, 14);

    mPlayerCodeButton.setBounds(8, 382, 112, 14);
    mExportSpcButton.setBounds(180, 382, 160, 14);
    mAramLabel.setBounds(452, 392, 84, 14);
    mStatusLabel.setBounds(8, 364, 340, 14);
}

void C700AudioProcessorEditor::timerCallback()
{
    const auto state = processorRef.getRuntimeState();
    const auto program = juce::roundToInt(processorRef.getAPVTS().getRawParameterValue("program")->load());

    mProgramValueLabel.setText(juce::String(program), juce::dontSendNotification);
    mProgramNameLabel.setText(state.programName.isNotEmpty() ? state.programName
                                                             : state.sampleName,
                              juce::dontSendNotification);

    mDisplayedBank = state.bank;
    mDisplayedChannel = state.editingChannel;
    mDisplayedTrackActivity = state.noteOnTrack;
    refreshSelectionButtons();

    mHardwareConnected = state.isHwConnected;
    repaint(520, 376, 16, 16);

    const int ramMax = BRR_ENDADDR - BRR_STARTADDR;
    mAramLabel.setText(juce::String(state.aramBytes), juce::dontSendNotification);
    mAramLabel.setColour(juce::Label::textColourId,
                         state.aramBytes > ramMax ? juce::Colours::red : kValueTextColour);

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
        mStatusLabel.setText("Pass 3: bitmap selectors and sample buttons restored", juce::dontSendNotification);
}

void C700AudioProcessorEditor::loadSampleClicked()
{
    const int prog = juce::roundToInt(processorRef.getAPVTS().getRawParameterValue("program")->load());

    mFileChooser = std::make_unique<juce::FileChooser>(
        "Load Sample to Slot " + juce::String(prog),
        mLastBrowseDir,
        "*.wav;*.brr");

    mFileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, prog](const juce::FileChooser& fc) {
            const auto results = fc.getResults();
            if (results.isEmpty())
                return;

            const auto file = results.getFirst();
            mLastBrowseDir = file.getParentDirectory();
            if (processorRef.getAdapter().loadSampleToSlot(prog, file.getFullPathName().toStdString())) {
                processorRef.forceParamSync();
                mStatusLabel.setText("Loaded sample into slot " + juce::String(prog),
                                     juce::dontSendNotification);
            } else {
                const auto err = processorRef.getAdapter().getLastLoadError();
                mStatusLabel.setText(err.empty() ? "Failed to load sample" : juce::String(err),
                                     juce::dontSendNotification);
            }

            mStatusOverrideUntil = juce::Time::currentTimeMillis() + 4000;
        });
}

void C700AudioProcessorEditor::unloadSampleClicked()
{
    const int prog = juce::roundToInt(processorRef.getAPVTS().getRawParameterValue("program")->load());
    if (processorRef.getAdapter().unloadSlot(prog)) {
        processorRef.forceParamSync();
        mStatusLabel.setText("Unloaded slot " + juce::String(prog), juce::dontSendNotification);
    } else {
        const auto err = processorRef.getAdapter().getLastLoadError();
        mStatusLabel.setText(err.empty() ? "Failed to unload sample" : juce::String(err),
                             juce::dontSendNotification);
    }

    mStatusOverrideUntil = juce::Time::currentTimeMillis() + 3000;
}

void C700AudioProcessorEditor::saveSampleClicked()
{
    const int prog = juce::roundToInt(processorRef.getAPVTS().getRawParameterValue("program")->load());
    auto data = processorRef.getAdapter().copyBRRData(prog);
    if (data.empty()) {
        mStatusLabel.setText("No BRR data in the current slot", juce::dontSendNotification);
        mStatusOverrideUntil = juce::Time::currentTimeMillis() + 3000;
        return;
    }

    const auto runtime = processorRef.getRuntimeState();
    const auto defaultName = defaultBrrFileName(prog, runtime.programName, runtime.sampleName);

    mFileChooser = std::make_unique<juce::FileChooser>(
        "Save BRR Sample",
        mLastExportDir.getChildFile(defaultName),
        "*.brr");

    mFileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this, data = std::move(data)](const juce::FileChooser& fc) mutable {
            const auto results = fc.getResults();
            if (results.isEmpty())
                return;

            auto file = results.getFirst();
            if (file.getFileExtension().isEmpty())
                file = file.withFileExtension(".brr");

            mLastExportDir = file.getParentDirectory();
            const bool ok = file.replaceWithData(data.data(), data.size());
            mStatusLabel.setText(ok ? "Saved BRR sample"
                                    : "Failed to save BRR sample",
                                 juce::dontSendNotification);
            mStatusOverrideUntil = juce::Time::currentTimeMillis() + 3000;
        });
}

void C700AudioProcessorEditor::saveXiClicked()
{
    mStatusLabel.setText("XI export is not wired yet in the JUCE editor", juce::dontSendNotification);
    mStatusOverrideUntil = juce::Time::currentTimeMillis() + 4000;
}

void C700AudioProcessorEditor::loadPlayerCodeClicked()
{
    mFileChooser = std::make_unique<juce::FileChooser>(
        "Load Player Code Binary",
        mLastBrowseDir,
        "*.bin");

    mFileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc) {
            const auto results = fc.getResults();
            if (results.isEmpty())
                return;

            const auto file = results.getFirst();
            mLastBrowseDir = file.getParentDirectory();
            const bool ok = processorRef.getAdapter().loadPlayerCode(file.getFullPathName().toStdString());
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

    const float recStart = processorRef.getAPVTS().getRawParameterValue("rec_start")->load();
    const float recEnd = processorRef.getAPVTS().getRawParameterValue("rec_end")->load();
    if (recStart >= recEnd) {
        mStatusLabel.setText("Set Record Start/End beats first", juce::dontSendNotification);
        mStatusOverrideUntil = juce::Time::currentTimeMillis() + 4000;
        return;
    }

    mFileChooser = std::make_unique<juce::FileChooser>(
        "Export SPC",
        mLastExportDir,
        "*.spc");

    mFileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc) {
            const auto results = fc.getResults();
            if (results.isEmpty())
                return;

            auto file = results.getFirst();
            if (file.getFileExtension().isEmpty())
                file = file.withFileExtension(".spc");

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

void C700AudioProcessorEditor::adjustProgram(int delta)
{
    const int current = juce::roundToInt(processorRef.getAPVTS().getRawParameterValue("program")->load());
    const int next = juce::jlimit(0, 127, current + delta);
    if (next == current)
        return;

    setDiscreteParameterValue(processorRef.getAPVTS(), "program", next);
    processorRef.forceParamSync();
    mProgramValueLabel.setText(juce::String(next), juce::dontSendNotification);
}

void C700AudioProcessorEditor::selectBank(int bank)
{
    bank = juce::jlimit(0, 3, bank);
    if (processorRef.getAdapter().setPropertyValue(kAudioUnitCustomProperty_Bank, static_cast<float>(bank))) {
        mDisplayedBank = bank;
        refreshSelectionButtons();
    }
}

void C700AudioProcessorEditor::selectEditingChannel(int channel)
{
    channel = juce::jlimit(0, 15, channel);
    if (processorRef.getAdapter().setPropertyValue(kAudioUnitCustomProperty_EditingChannel,
                                                   static_cast<float>(channel))) {
        mDisplayedChannel = channel;
        refreshSelectionButtons();
    }
}

void C700AudioProcessorEditor::refreshSelectionButtons()
{
    for (int bank = 0; bank < static_cast<int>(mBankButtons.size()); ++bank) {
        const auto offImage = cropFilmStripFrame(mBankStrips[bank], 0, 2);
        const auto onImage = cropFilmStripFrame(mBankStrips[bank], 1, 2);
        const auto normalImage = (mDisplayedBank == bank) ? onImage : offImage;
        configureMomentaryButton(mBankButtons[bank], normalImage, onImage);
    }

    for (int channel = 0; channel < static_cast<int>(mTrackButtons.size()); ++channel) {
        const auto offImage = composeImages(cropFilmStripFrame(mTrackIndicatorStrip, 0, 2),
                                            cropFilmStripFrame(mTrackNumberStrips[channel], 0, 2));
        const auto onImage = composeImages(cropFilmStripFrame(mTrackIndicatorStrip, 1, 2),
                                           cropFilmStripFrame(mTrackNumberStrips[channel], 1, 2));
        const bool isActive = mDisplayedChannel == channel || mDisplayedTrackActivity[channel] > 0;
        configureMomentaryButton(mTrackButtons[channel], isActive ? onImage : offImage, onImage);
    }
}
