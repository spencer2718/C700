#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "C700Kernel.h"
#include "C700DSP.h"
#include "C700Properties.h"
#include "C700defines.h"
#include <cstring>

// --- Parameter layout ---

juce::AudioProcessorValueTreeState::ParameterLayout C700AudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    // -- Control --
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("program", 1), "Program", 0, 127, 0));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("volume", 1), "Volume",
        juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

    // -- Per-instrument (reflect current program slot) --
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("ar", 1), "Attack Rate (AR)", 0, 15, 15));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("dr", 1), "Decay Rate (DR)", 0, 7, 7));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("sl", 1), "Sustain Level (SL)", 0, 7, 7));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("sr1", 1), "Sustain Rate (SR1)", 0, 31, 0));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("sr2", 1), "Release Rate (SR2)", 0, 31, 31));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("sustainmode", 1), "Sustain Mode", 0, 1, 1));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("voll", 1), "Volume L", 0, 127, 100));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("volr", 1), "Volume R", 0, 127, 100));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("echo", 1), "Echo Enable", 0, 1, 0));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("basekey", 1), "Base Key", 0, 127, 60));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("loop", 1), "Loop Enable", 0, 1, 0));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("rate", 1), "Sample Rate",
        juce::NormalisableRange<float>(0.0f, 128000.0f, 1.0f), 32000.0f));

    // -- Global DSP --
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("mainvol_l", 1), "Main Vol L", -128, 127, 64));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("mainvol_r", 1), "Main Vol R", -128, 127, 64));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("echodelay", 1), "Echo Delay", 0, 15, 6));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("echofb", 1), "Echo Feedback", -128, 127, -70));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("echovol_l", 1), "Echo Vol L", -128, 127, 50));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("echovol_r", 1), "Echo Vol R", -128, 127, -50));

    // -- SPC Recording --
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("rec_start", 1), "Record Start (beat)",
        juce::NormalisableRange<float>(0.0f, 10000.0f, 0.01f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("rec_loop", 1), "Record Loop (beat)",
        juce::NormalisableRange<float>(0.0f, 10000.0f, 0.01f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("rec_end", 1), "Record End (beat)",
        juce::NormalisableRange<float>(0.0f, 10000.0f, 0.01f), 0.0f));

    // -- Read-only info --
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("aram", 1), "ARAM Used (bytes)",
        juce::NormalisableRange<float>(0.0f, 65536.0f, 1.0f), 0.0f));

    return { p.begin(), p.end() };
}

// --- Constructor / Destructor ---

C700AudioProcessor::C700AudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , mParameters(*this, nullptr, "C700Parameters", createParameterLayout())
{
    pProgram     = mParameters.getRawParameterValue("program");
    pVolume      = mParameters.getRawParameterValue("volume");
    pAR          = mParameters.getRawParameterValue("ar");
    pDR          = mParameters.getRawParameterValue("dr");
    pSL          = mParameters.getRawParameterValue("sl");
    pSR1         = mParameters.getRawParameterValue("sr1");
    pSR2         = mParameters.getRawParameterValue("sr2");
    pSustainMode = mParameters.getRawParameterValue("sustainmode");
    pVolL        = mParameters.getRawParameterValue("voll");
    pVolR        = mParameters.getRawParameterValue("volr");
    pEcho        = mParameters.getRawParameterValue("echo");
    pBaseKey     = mParameters.getRawParameterValue("basekey");
    pLoop        = mParameters.getRawParameterValue("loop");
    pRate        = mParameters.getRawParameterValue("rate");
    pMainVolL    = mParameters.getRawParameterValue("mainvol_l");
    pMainVolR    = mParameters.getRawParameterValue("mainvol_r");
    pEchoDelay   = mParameters.getRawParameterValue("echodelay");
    pEchoFB      = mParameters.getRawParameterValue("echofb");
    pEchoVolL    = mParameters.getRawParameterValue("echovol_l");
    pEchoVolR    = mParameters.getRawParameterValue("echovol_r");
    pRecStart    = mParameters.getRawParameterValue("rec_start");
    pRecLoop     = mParameters.getRawParameterValue("rec_loop");
    pRecEnd      = mParameters.getRawParameterValue("rec_end");
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

// --- Lifecycle ---

void C700AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    mAdapter.init(sampleRate, samplesPerBlock);
    applyPendingState();
    pushGlobalParamsToEngine();
    mLastProgram = -1; // force program sync on first block
}

void C700AudioProcessor::releaseResources() {}

void C700AudioProcessor::applyPendingState()
{
    if (mHasPendingState) {
        mAdapter.setStateData(mPendingEngineState.getData(),
                              static_cast<int>(mPendingEngineState.getSize()));
        mPendingEngineState.reset();
        mHasPendingState = false;
    }
}

// --- Per-instrument sync ---

void C700AudioProcessor::syncPerInstrumentParamsFromEngine(int prog)
{
    // Pull values from kernel slot into JUCE parameters.
    // Set flag to prevent feedback loop.
    mSyncingFromEngine = true;

    auto* kernel = mAdapter.getKernel();
    float savedEdit = kernel->GetPropertyValue(kAudioUnitCustomProperty_EditingProgram);
    kernel->SetPropertyValue(kAudioUnitCustomProperty_EditingProgram, static_cast<float>(prog));

    auto set = [&](juce::StringRef id, float val) {
        if (auto* param = mParameters.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(val));
    };

    set("ar",          kernel->GetPropertyValue(kAudioUnitCustomProperty_AR));
    set("dr",          kernel->GetPropertyValue(kAudioUnitCustomProperty_DR));
    set("sl",          kernel->GetPropertyValue(kAudioUnitCustomProperty_SL));
    set("sr1",         kernel->GetPropertyValue(kAudioUnitCustomProperty_SR1));
    set("sr2",         kernel->GetPropertyValue(kAudioUnitCustomProperty_SR2));
    set("sustainmode", kernel->GetPropertyValue(kAudioUnitCustomProperty_SustainMode));
    set("voll",        kernel->GetPropertyValue(kAudioUnitCustomProperty_VolL));
    set("volr",        kernel->GetPropertyValue(kAudioUnitCustomProperty_VolR));
    set("echo",        kernel->GetPropertyValue(kAudioUnitCustomProperty_Echo));
    set("basekey",     kernel->GetPropertyValue(kAudioUnitCustomProperty_BaseKey));
    set("loop",        kernel->GetPropertyValue(kAudioUnitCustomProperty_Loop));
    set("rate",        kernel->GetPropertyValue(kAudioUnitCustomProperty_Rate));

    kernel->SetPropertyValue(kAudioUnitCustomProperty_EditingProgram, savedEdit);

    mSyncingFromEngine = false;
}

void C700AudioProcessor::pushPerInstrumentParamsToEngine(int prog)
{
    auto* kernel = mAdapter.getKernel();
    float savedEdit = kernel->GetPropertyValue(kAudioUnitCustomProperty_EditingProgram);
    kernel->SetPropertyValue(kAudioUnitCustomProperty_EditingProgram, static_cast<float>(prog));

    kernel->SetPropertyValue(kAudioUnitCustomProperty_AR,           pAR->load());
    kernel->SetPropertyValue(kAudioUnitCustomProperty_DR,           pDR->load());
    kernel->SetPropertyValue(kAudioUnitCustomProperty_SL,           pSL->load());
    kernel->SetPropertyValue(kAudioUnitCustomProperty_SR1,          pSR1->load());
    kernel->SetPropertyValue(kAudioUnitCustomProperty_SR2,          pSR2->load());
    kernel->SetPropertyValue(kAudioUnitCustomProperty_SustainMode,  pSustainMode->load());
    kernel->SetPropertyValue(kAudioUnitCustomProperty_VolL,         pVolL->load());
    kernel->SetPropertyValue(kAudioUnitCustomProperty_VolR,         pVolR->load());
    kernel->SetPropertyValue(kAudioUnitCustomProperty_Echo,         pEcho->load());
    kernel->SetPropertyValue(kAudioUnitCustomProperty_BaseKey,      pBaseKey->load());
    kernel->SetPropertyValue(kAudioUnitCustomProperty_Loop,         pLoop->load());
    kernel->SetPropertyDoubleValue(kAudioUnitCustomProperty_Rate,   static_cast<double>(pRate->load()));

    kernel->SetPropertyValue(kAudioUnitCustomProperty_EditingProgram, savedEdit);
}

void C700AudioProcessor::pushGlobalParamsToEngine()
{
    auto* kernel = mAdapter.getKernel();
    kernel->SetParameter(kParam_mainvol_L,  pMainVolL->load());
    kernel->SetParameter(kParam_mainvol_R,  pMainVolR->load());
    kernel->SetParameter(kParam_echodelay,  pEchoDelay->load());
    kernel->SetParameter(kParam_echoFB,     pEchoFB->load());
    kernel->SetParameter(kParam_echovol_L,  pEchoVolL->load());
    kernel->SetParameter(kParam_echovol_R,  pEchoVolR->load());

    // SPC recording region
    mAdapter.setSpcRecordRegion(
        static_cast<double>(pRecStart->load()),
        static_cast<double>(pRecLoop->load()),
        static_cast<double>(pRecEnd->load()));
}

// --- Process ---

void C700AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    int currentProgram = static_cast<int>(pProgram->load());

    // Program changed — sync per-instrument params from engine
    if (currentProgram != mLastProgram) {
        mAdapter.setProgramForAllChannels(currentProgram);
        syncPerInstrumentParamsFromEngine(currentProgram);
        mLastProgram = currentProgram;
    }
    else if (!mSyncingFromEngine) {
        // User may have changed per-instrument params — push to engine
        pushPerInstrumentParamsToEngine(currentProgram);
    }

    // Push global params every block (cheap, avoids tracking individual changes)
    pushGlobalParamsToEngine();

    // Update ARAM display
    auto* aramParam = mParameters.getParameter("aram");
    if (aramParam) {
        float aram = mAdapter.getKernel()->GetPropertyValue(kAudioUnitCustomProperty_TotalRAM);
        aramParam->setValueNotifyingHost(aramParam->convertTo0to1(aram));
    }

    // Feed host transport info to engine (needed for SPC recording)
    if (auto* playHead = getPlayHead()) {
        auto pos = playHead->getPosition();
        if (pos.hasValue()) {
            double tempo = pos->getBpm().orFallback(120.0);
            double ppq = pos->getPpqPosition().orFallback(0.0);
            bool playing = pos->getIsPlaying();
            mAdapter.setTransportInfo(tempo, ppq, playing);

#ifndef NDEBUG
            // Diagnostic logging (throttled to ~1/sec)
            mDbgSampleCounter += buffer.getNumSamples();
            if (mDbgSampleCounter >= static_cast<int>(getSampleRate())) {
                mDbgSampleCounter = 0;
                double recStart = static_cast<double>(pRecStart->load());
                double recEnd = static_cast<double>(pRecEnd->load());
                if (recEnd > 0.0) {
                    auto* dsp = mAdapter.getKernel()->GetDriver()->GetDsp();
                    DBG("C700 SPC: ppq=" << ppq
                        << " playing=" << playing
                        << " tempo=" << tempo
                        << " recStart=" << recStart
                        << " recEnd=" << recEnd
                        << " spcEnabled=" << dsp->GetRecSaveAsSpc()
                        << " path=[" << dsp->GetSongRecordPath() << "]"
                        << " hasPC=" << mAdapter.hasPlayerCode());
                }
            }
#endif
        }
    }

    // Ensure stereo output
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (int i = 2; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    float* channels[2] = {
        buffer.getWritePointer(0),
        totalNumOutputChannels > 1 ? buffer.getWritePointer(1) : buffer.getWritePointer(0)
    };

    buffer.clear(0, 0, buffer.getNumSamples());
    if (totalNumOutputChannels > 1)
        buffer.clear(1, 0, buffer.getNumSamples());

    mAdapter.process(channels, buffer.getNumSamples(), midiMessages);

    // Apply output volume
    float vol = pVolume->load();
    if (vol < 1.0f) {
        buffer.applyGain(0, 0, buffer.getNumSamples(), vol);
        if (totalNumOutputChannels > 1)
            buffer.applyGain(1, 0, buffer.getNumSamples(), vol);
    }
}

// --- Editor ---

juce::AudioProcessorEditor* C700AudioProcessor::createEditor()
{
    return new C700AudioProcessorEditor(*this);
}

void C700AudioProcessor::forceParamSync()
{
    int prog = static_cast<int>(pProgram->load());
    syncPerInstrumentParamsFromEngine(prog);
}

bool C700AudioProcessor::hasEditor() const { return true; }

// --- State ---

void C700AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::MemoryBlock paramsBlock;
    {
        auto state = mParameters.copyState();
        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        copyXmlToBinary(*xml, paramsBlock);
    }

    juce::MemoryBlock engineBlock;
    mAdapter.getStateData(engineBlock);

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

    {
        std::unique_ptr<juce::XmlElement> xml(
            getXmlFromBinary(bytes + sizeof(paramsSize), paramsSize));
        if (xml && xml->hasTagName(mParameters.state.getType()))
            mParameters.replaceState(juce::ValueTree::fromXml(*xml));
    }

    int engineOffset = sizeof(paramsSize) + paramsSize;
    int engineSize = sizeInBytes - engineOffset;
    if (engineSize > 0) {
        mPendingEngineState.reset();
        mPendingEngineState.append(bytes + engineOffset, engineSize);
        mHasPendingState = true;

        if (getSampleRate() > 0)
            applyPendingState();
    }

    mLastProgram = -1;
}

// --- Factory ---

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new C700AudioProcessor();
}
