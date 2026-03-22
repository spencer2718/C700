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
    C700Adapter& getAdapter() { return mAdapter; }
    void forceParamSync();

private:
    C700Adapter mAdapter;

    juce::AudioProcessorValueTreeState mParameters;

    // Raw parameter pointers — per-instrument (slot-relative)
    std::atomic<float>* pProgram = nullptr;
    std::atomic<float>* pVolume = nullptr;
    std::atomic<float>* pAR = nullptr;
    std::atomic<float>* pDR = nullptr;
    std::atomic<float>* pSL = nullptr;
    std::atomic<float>* pSR1 = nullptr;
    std::atomic<float>* pSR2 = nullptr;
    std::atomic<float>* pSustainMode = nullptr;
    std::atomic<float>* pVolL = nullptr;
    std::atomic<float>* pVolR = nullptr;
    std::atomic<float>* pEcho = nullptr;
    std::atomic<float>* pBaseKey = nullptr;
    std::atomic<float>* pLoop = nullptr;
    std::atomic<float>* pRate = nullptr;

    // Raw parameter pointers — global
    std::atomic<float>* pMainVolL = nullptr;
    std::atomic<float>* pMainVolR = nullptr;
    std::atomic<float>* pEchoDelay = nullptr;
    std::atomic<float>* pEchoFB = nullptr;
    std::atomic<float>* pEchoVolL = nullptr;
    std::atomic<float>* pEchoVolR = nullptr;

    // SPC recording params
    std::atomic<float>* pRecStart = nullptr;
    std::atomic<float>* pRecLoop = nullptr;
    std::atomic<float>* pRecEnd = nullptr;

    int mLastProgram = -1;
    bool mSyncingFromEngine = false; // prevent feedback loops
    int mDbgSampleCounter = 0;

    // Deferred state restore
    juce::MemoryBlock mPendingEngineState;
    bool mHasPendingState = false;

    void applyPendingState();
    void syncPerInstrumentParamsFromEngine(int prog);
    void pushPerInstrumentParamsToEngine(int prog);
    void pushGlobalParamsToEngine();

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(C700AudioProcessor)
};
