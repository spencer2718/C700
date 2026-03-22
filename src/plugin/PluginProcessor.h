#pragma once
#include <array>
#include <juce_audio_processors/juce_audio_processors.h>
#include "C700Adapter.h"

class C700AudioProcessor : public juce::AudioProcessor
{
public:
    struct RuntimeState
    {
        int aramBytes = 0;
        int playerCodeVersion = 0;
        int editingChannel = 0;
        int bank = 0;
        int maxNoteTotal = 0;
        bool hasPlayerCode = false;
        bool isHwConnected = false;
        bool isRecording = false;
        bool finishedRecording = false;
        std::array<int, 16> noteOnTrack {};
        std::array<int, 16> maxNoteTrack {};
        juce::String sampleName;
        juce::String programName;
    };

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
    RuntimeState getRuntimeState() const;

private:
    C700Adapter mAdapter;

    juce::AudioProcessorValueTreeState mParameters;

    // Raw parameter pointers — per-instrument (slot-relative)
    std::atomic<float>* pEditSlot = nullptr;
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
    std::atomic<float>* pLowKey = nullptr;
    std::atomic<float>* pHighKey = nullptr;
    std::atomic<float>* pLoop = nullptr;
    std::atomic<float>* pRate = nullptr;
    std::atomic<float>* pMonoMode = nullptr;
    std::atomic<float>* pPitchMod = nullptr;
    std::atomic<float>* pNoise = nullptr;
    std::atomic<float>* pPortamentoOn = nullptr;
    std::atomic<float>* pPortamentoRate = nullptr;
    std::atomic<float>* pNoteOnPriority = nullptr;
    std::atomic<float>* pReleasePriority = nullptr;

    // Raw parameter pointers — global
    std::atomic<float>* pPoly = nullptr;
    std::atomic<float>* pVibrate = nullptr;
    std::atomic<float>* pVibDepth2 = nullptr;
    std::atomic<float>* pVelocity = nullptr;
    std::atomic<float>* pBendRange = nullptr;
    std::atomic<float>* pEngine = nullptr;
    std::atomic<float>* pBankAMulti = nullptr;
    std::atomic<float>* pBankBMulti = nullptr;
    std::atomic<float>* pBankCMulti = nullptr;
    std::atomic<float>* pBankDMulti = nullptr;
    std::atomic<float>* pVoiceAllocMode = nullptr;
    std::atomic<float>* pMainVolL = nullptr;
    std::atomic<float>* pMainVolR = nullptr;
    std::atomic<float>* pEchoDelay = nullptr;
    std::atomic<float>* pEchoFB = nullptr;
    std::atomic<float>* pEchoVolL = nullptr;
    std::atomic<float>* pEchoVolR = nullptr;
    std::atomic<float>* pFir0 = nullptr;
    std::atomic<float>* pFir1 = nullptr;
    std::atomic<float>* pFir2 = nullptr;
    std::atomic<float>* pFir3 = nullptr;
    std::atomic<float>* pFir4 = nullptr;
    std::atomic<float>* pFir5 = nullptr;
    std::atomic<float>* pFir6 = nullptr;
    std::atomic<float>* pFir7 = nullptr;
    std::atomic<float>* pBand1 = nullptr;
    std::atomic<float>* pBand2 = nullptr;
    std::atomic<float>* pBand3 = nullptr;
    std::atomic<float>* pBand4 = nullptr;
    std::atomic<float>* pBand5 = nullptr;

    // SPC recording params
    std::atomic<float>* pRecStart = nullptr;
    std::atomic<float>* pRecLoop = nullptr;
    std::atomic<float>* pRecEnd = nullptr;
    std::atomic<float>* pRecSaveAsSpc = nullptr;
    std::atomic<float>* pRecSaveAsSmc = nullptr;
    std::atomic<float>* pTimeBaseForSmc = nullptr;
    std::atomic<float>* pRepeatNumForSpc = nullptr;
    std::atomic<float>* pFadeMsForSpc = nullptr;

    int mLastProgram = -1;
    bool mSyncingFromEngine = false; // prevent feedback loops

    // Deferred state restore
    juce::MemoryBlock mPendingEngineState;
    bool mHasPendingState = false;
    RuntimeState mRuntimeState;
    mutable juce::SpinLock mRuntimeStateLock;

    void applyPendingState();
    void syncPerInstrumentParamsFromEngine(int prog);
    void syncGlobalParamsFromEngine();
    void syncRecordingParamsFromEngine();
    void pushPerInstrumentParamsToEngine(int prog);
    void pushGlobalParamsToEngine();
    void updateRuntimeState(int editSlot);

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(C700AudioProcessor)
};
