#include "PluginProcessor.h"
#include "PluginEditor.h"

C700AudioProcessor::C700AudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
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

void C700AudioProcessor::prepareToPlay(double, int) {}
void C700AudioProcessor::releaseResources() {}

void C700AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    // Placeholder: output silence
    buffer.clear();
}

juce::AudioProcessorEditor* C700AudioProcessor::createEditor()
{
    return new C700AudioProcessorEditor(*this);
}

bool C700AudioProcessor::hasEditor() const { return true; }

void C700AudioProcessor::getStateInformation(juce::MemoryBlock&) {}
void C700AudioProcessor::setStateInformation(const void*, int) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new C700AudioProcessor();
}
