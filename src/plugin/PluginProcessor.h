#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "C700Adapter.h"

class C700AudioProcessor : public juce::AudioProcessor
{
public:
    C700AudioProcessor();
    ~C700AudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return mParameters; }

private:
    C700Adapter mAdapter;

    juce::AudioProcessorValueTreeState mParameters;
    std::atomic<float>* mProgramParam = nullptr;
    std::atomic<float>* mVolumeParam = nullptr;
    int mLastProgram = -1;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(C700AudioProcessor)
};
