#include "PluginProcessor.h"
#include <cstring>

juce::AudioProcessorValueTreeState::ParameterLayout C700AudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("program", 1), "Program", 0, 127, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("volume", 1), "Volume",
        juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

    return { params.begin(), params.end() };
}

C700AudioProcessor::C700AudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , mParameters(*this, nullptr, "C700Parameters", createParameterLayout())
{
    mProgramParam = mParameters.getRawParameterValue("program");
    mVolumeParam  = mParameters.getRawParameterValue("volume");
}

C700AudioProcessor::~C700AudioProcessor() {}

const juce::String C700AudioProcessor::getName() const { return "C700"; }
bool C700AudioProcessor::acceptsMidi() const { return true; }
bool C700AudioProcessor::producesMidi() const { return false; }
bool C700AudioProcessor::isMidiEffect() const { return false; }
double C700AudioProcessor::getTailLengthSeconds() const { return 0.0; }

int C700AudioProcessor::getNumPrograms() { return 1; }
int C700AudioProcessor::getCurrentProgram() { return 0; }
void C700AudioProcessor::setCurrentProgram(int) {}
const juce::String C700AudioProcessor::getProgramName(int) { return {}; }
void C700AudioProcessor::changeProgramName(int, const juce::String&) {}

void C700AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    mAdapter.init(sampleRate, samplesPerBlock);
    mLastProgram = -1; // force program sync on first block
}

void C700AudioProcessor::releaseResources() {}

void C700AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Sync program parameter to engine
    int currentProgram = static_cast<int>(mProgramParam->load());
    if (currentProgram != mLastProgram) {
        mAdapter.setProgramForAllChannels(currentProgram);
        mLastProgram = currentProgram;
    }

    // Ensure stereo output
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (int i = 2; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Get write pointers for L/R
    float* channels[2] = {
        buffer.getWritePointer(0),
        totalNumOutputChannels > 1 ? buffer.getWritePointer(1) : buffer.getWritePointer(0)
    };

    // Clear before rendering (engine accumulates into buffer)
    buffer.clear(0, 0, buffer.getNumSamples());
    if (totalNumOutputChannels > 1)
        buffer.clear(1, 0, buffer.getNumSamples());

    mAdapter.process(channels, buffer.getNumSamples(), midiMessages);

    // Apply volume parameter
    float vol = mVolumeParam->load();
    if (vol < 1.0f) {
        buffer.applyGain(0, 0, buffer.getNumSamples(), vol);
        if (totalNumOutputChannels > 1)
            buffer.applyGain(1, 0, buffer.getNumSamples(), vol);
    }
}

juce::AudioProcessorEditor* C700AudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

bool C700AudioProcessor::hasEditor() const { return true; }

void C700AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Save APVTS parameters as XML
    juce::MemoryBlock paramsBlock;
    {
        auto state = mParameters.copyState();
        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        copyXmlToBinary(*xml, paramsBlock);
    }

    // Save engine state (instrument data)
    juce::MemoryBlock engineBlock;
    mAdapter.getStateData(engineBlock);

    // Write combined: [4 bytes paramsSize][paramsData][engineData]
    uint32_t paramsSize = static_cast<uint32_t>(paramsBlock.getSize());
    destData.append(&paramsSize, sizeof(paramsSize));
    destData.append(paramsBlock.getData(), paramsBlock.getSize());
    destData.append(engineBlock.getData(), engineBlock.getSize());
}

void C700AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (sizeInBytes < 4) return;

    auto* bytes = static_cast<const uint8_t*>(data);
    uint32_t paramsSize;
    std::memcpy(&paramsSize, bytes, sizeof(paramsSize));

    if (static_cast<int>(paramsSize + sizeof(paramsSize)) > sizeInBytes) return;

    // Restore APVTS parameters
    {
        std::unique_ptr<juce::XmlElement> xml(
            getXmlFromBinary(bytes + sizeof(paramsSize), paramsSize));
        if (xml && xml->hasTagName(mParameters.state.getType()))
            mParameters.replaceState(juce::ValueTree::fromXml(*xml));
    }

    // Restore engine state
    int engineOffset = sizeof(paramsSize) + paramsSize;
    int engineSize = sizeInBytes - engineOffset;
    if (engineSize > 0) {
        mAdapter.setStateData(bytes + engineOffset, engineSize);
    }

    mLastProgram = -1; // force re-sync
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new C700AudioProcessor();
}
