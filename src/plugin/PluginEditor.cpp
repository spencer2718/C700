#include "PluginEditor.h"

C700AudioProcessorEditor::C700AudioProcessorEditor(C700AudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // Load button
    mLoadButton.onClick = [this] { loadSampleClicked(); };
    addAndMakeVisible(mLoadButton);

    // Slot label
    mSlotLabel.setFont(juce::Font(14.0f));
    mSlotLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    mSlotLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(mSlotLabel);

    // Sample name label
    mSampleNameLabel.setFont(juce::Font(14.0f));
    mSampleNameLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    mSampleNameLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(mSampleNameLabel);

    // Embed the generic editor for all APVTS parameters
    mGenericEditor = std::make_unique<juce::GenericAudioProcessorEditor>(p);
    addAndMakeVisible(mGenericEditor.get());

    // Default browse directory
    mLastBrowseDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory);

    setSize(500, 50 + mGenericEditor->getHeight());

    startTimerHz(10); // update labels at 10Hz
    timerCallback();  // initial sync
}

C700AudioProcessorEditor::~C700AudioProcessorEditor()
{
    stopTimer();
}

void C700AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2a2a));

    // Top bar background
    g.setColour(juce::Colour(0xff3a3a3a));
    g.fillRect(0, 0, getWidth(), 50);
}

void C700AudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    auto topBar = area.removeFromTop(50).reduced(6, 8);

    mLoadButton.setBounds(topBar.removeFromLeft(120));
    topBar.removeFromLeft(8);
    mSlotLabel.setBounds(topBar.removeFromLeft(80));
    topBar.removeFromLeft(4);
    mSampleNameLabel.setBounds(topBar);

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
            if (ok) {
                // Sync params from engine so the editor reflects the loaded sample's settings
                processorRef.forceParamSync();
            }
        });
}
