#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "C700Kernel.h"
#include "C700Parameters.h"
#include "C700Properties.h"
#include "C700defines.h"
#include <cstring>

// --- Parameter layout ---

namespace
{
void addIntParam(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                 const char* id, const char* name,
                 int minValue, int maxValue, int defaultValue)
{
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID(id, 1), name, minValue, maxValue, defaultValue));
}

void addFloatParam(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                   const char* id, const char* name,
                   float minValue, float maxValue, float step, float defaultValue)
{
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(id, 1), name,
        juce::NormalisableRange<float>(minValue, maxValue, step),
        defaultValue));
}
}

juce::AudioProcessorValueTreeState::ParameterLayout C700AudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    // -- Control --
    addIntParam(p, "program", "Edit Slot", 0, 127, 0);
    addFloatParam(p, "volume", "Volume", 0.0f, 1.0f, 0.001f, 1.0f);

    // -- Per-instrument (reflect current program slot) --
    addIntParam(p, "ar", "Attack Rate (AR)", 0, 15, 15);
    addIntParam(p, "dr", "Decay Rate (DR)", 0, 7, 7);
    addIntParam(p, "sl", "Sustain Level (SL)", 0, 7, 7);
    addIntParam(p, "sr1", "Sustain Rate (SR1)", 0, 31, 0);
    addIntParam(p, "sr2", "Release Rate (SR2)", 0, 31, 31);
    addIntParam(p, "sustainmode", "Sustain Mode", 0, 1, 1);
    addIntParam(p, "voll", "Volume L", 0, 127, 100);
    addIntParam(p, "volr", "Volume R", 0, 127, 100);
    addIntParam(p, "echo", "Echo Enable", 0, 1, 0);
    addIntParam(p, "basekey", "Base Key", 0, 127, 60);
    addIntParam(p, "lowkey", "Low Key", 0, 127, 0);
    addIntParam(p, "highkey", "High Key", 0, 127, 127);
    addIntParam(p, "loop", "Loop Enable", 0, 1, 0);
    addFloatParam(p, "rate", "Sample Rate", 0.0f, 128000.0f, 1.0f, 32000.0f);
    addIntParam(p, "monomode", "Mono Mode", 0, 1, 0);
    addIntParam(p, "pitchmod", "Pitch Modulation", 0, 1, 0);
    addIntParam(p, "noise", "Noise Enable", 0, 1, 0);
    addIntParam(p, "porta", "Portamento", 0, 1, 0);
    addIntParam(p, "portarate", "Portamento Rate", 0, 127, 0);
    addIntParam(p, "noteprio", "Note On Priority", 0, 127, 0);
    addIntParam(p, "releaseprio", "Release Priority", 0, 127, 0);

    // -- Global DSP --
    addIntParam(p, "poly", "Voices", 1, 16, 8);
    addFloatParam(p, "vibrate", "Vibrato Rate",
                  C700Parameters::GetParameterMin(kParam_vibrate),
                  C700Parameters::GetParameterMax(kParam_vibrate),
                  0.001f, C700Parameters::GetParameterDefault(kParam_vibrate));
    addFloatParam(p, "vibdepth2", "Vibrato Depth 2",
                  C700Parameters::GetParameterMin(kParam_vibdepth2),
                  C700Parameters::GetParameterMax(kParam_vibdepth2),
                  0.001f, C700Parameters::GetParameterDefault(kParam_vibdepth2));
    addIntParam(p, "velocity", "Velocity Curve", 0, 2, 1);
    addIntParam(p, "bendrange", "Bend Range", 1, 24, 2);
    addIntParam(p, "engine", "Engine", 0, 2, 2);
    addIntParam(p, "banka_multi", "Multi Bank A", 0, 1, 0);
    addIntParam(p, "bankb_multi", "Multi Bank B", 0, 1, 0);
    addIntParam(p, "bankc_multi", "Multi Bank C", 0, 1, 0);
    addIntParam(p, "bankd_multi", "Multi Bank D", 0, 1, 0);
    addIntParam(p, "voicealloc", "Voice Allocation", 0, 1, 0);
    addIntParam(p, "mainvol_l", "Main Vol L", -128, 127, 64);
    addIntParam(p, "mainvol_r", "Main Vol R", -128, 127, 64);
    addIntParam(p, "echodelay", "Echo Delay", 0, 15, 6);
    addIntParam(p, "echofb", "Echo Feedback", -128, 127, -70);
    addIntParam(p, "echovol_l", "Echo Vol L", -128, 127, 50);
    addIntParam(p, "echovol_r", "Echo Vol R", -128, 127, -50);
    addIntParam(p, "fir0", "FIR 0", -128, 127, 127);
    addIntParam(p, "fir1", "FIR 1", -128, 127, 0);
    addIntParam(p, "fir2", "FIR 2", -128, 127, 0);
    addIntParam(p, "fir3", "FIR 3", -128, 127, 0);
    addIntParam(p, "fir4", "FIR 4", -128, 127, 0);
    addIntParam(p, "fir5", "FIR 5", -128, 127, 0);
    addIntParam(p, "fir6", "FIR 6", -128, 127, 0);
    addIntParam(p, "fir7", "FIR 7", -128, 127, 0);
    addFloatParam(p, "band1", "Echo Band 1", 0.0f, 1.0f, 0.001f, 1.0f);
    addFloatParam(p, "band2", "Echo Band 2", 0.0f, 1.0f, 0.001f, 1.0f);
    addFloatParam(p, "band3", "Echo Band 3", 0.0f, 1.0f, 0.001f, 1.0f);
    addFloatParam(p, "band4", "Echo Band 4", 0.0f, 1.0f, 0.001f, 1.0f);
    addFloatParam(p, "band5", "Echo Band 5", 0.0f, 1.0f, 0.001f, 1.0f);

    // -- SPC Recording --
    addFloatParam(p, "rec_start", "Record Start (beat)", 0.0f, 10000.0f, 0.01f, 0.0f);
    addFloatParam(p, "rec_loop", "Record Loop (beat)", 0.0f, 10000.0f, 0.01f, 0.0f);
    addFloatParam(p, "rec_end", "Record End (beat)", 0.0f, 10000.0f, 0.01f, 0.0f);
    addIntParam(p, "rec_save_spc", "Record Save As SPC", 0, 1, 0);
    addIntParam(p, "rec_save_smc", "Record Save As SMC", 0, 1, 0);
    addIntParam(p, "timebase_smc", "SMC Time Base", 0, 1, 0);
    addFloatParam(p, "repeat_spc", "Repeat Count (SPC)", 0.0f, 9.9f, 0.1f, 1.0f);
    addIntParam(p, "fade_spc_ms", "Fade Time (SPC ms)", 0, 99999, 5000);

    // -- Read-only info --
    addFloatParam(p, "aram", "ARAM Used (bytes)", 0.0f, 65536.0f, 1.0f, 0.0f);

    return { p.begin(), p.end() };
}

// --- Constructor / Destructor ---

C700AudioProcessor::C700AudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , mParameters(*this, nullptr, "C700Parameters", createParameterLayout())
{
    pEditSlot    = mParameters.getRawParameterValue("program");
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
    pLowKey      = mParameters.getRawParameterValue("lowkey");
    pHighKey     = mParameters.getRawParameterValue("highkey");
    pLoop        = mParameters.getRawParameterValue("loop");
    pRate        = mParameters.getRawParameterValue("rate");
    pMonoMode    = mParameters.getRawParameterValue("monomode");
    pPitchMod    = mParameters.getRawParameterValue("pitchmod");
    pNoise       = mParameters.getRawParameterValue("noise");
    pPortamentoOn = mParameters.getRawParameterValue("porta");
    pPortamentoRate = mParameters.getRawParameterValue("portarate");
    pNoteOnPriority = mParameters.getRawParameterValue("noteprio");
    pReleasePriority = mParameters.getRawParameterValue("releaseprio");
    pPoly        = mParameters.getRawParameterValue("poly");
    pVibrate     = mParameters.getRawParameterValue("vibrate");
    pVibDepth2   = mParameters.getRawParameterValue("vibdepth2");
    pVelocity    = mParameters.getRawParameterValue("velocity");
    pBendRange   = mParameters.getRawParameterValue("bendrange");
    pEngine      = mParameters.getRawParameterValue("engine");
    pBankAMulti  = mParameters.getRawParameterValue("banka_multi");
    pBankBMulti  = mParameters.getRawParameterValue("bankb_multi");
    pBankCMulti  = mParameters.getRawParameterValue("bankc_multi");
    pBankDMulti  = mParameters.getRawParameterValue("bankd_multi");
    pVoiceAllocMode = mParameters.getRawParameterValue("voicealloc");
    pMainVolL    = mParameters.getRawParameterValue("mainvol_l");
    pMainVolR    = mParameters.getRawParameterValue("mainvol_r");
    pEchoDelay   = mParameters.getRawParameterValue("echodelay");
    pEchoFB      = mParameters.getRawParameterValue("echofb");
    pEchoVolL    = mParameters.getRawParameterValue("echovol_l");
    pEchoVolR    = mParameters.getRawParameterValue("echovol_r");
    pFir0        = mParameters.getRawParameterValue("fir0");
    pFir1        = mParameters.getRawParameterValue("fir1");
    pFir2        = mParameters.getRawParameterValue("fir2");
    pFir3        = mParameters.getRawParameterValue("fir3");
    pFir4        = mParameters.getRawParameterValue("fir4");
    pFir5        = mParameters.getRawParameterValue("fir5");
    pFir6        = mParameters.getRawParameterValue("fir6");
    pFir7        = mParameters.getRawParameterValue("fir7");
    pBand1       = mParameters.getRawParameterValue("band1");
    pBand2       = mParameters.getRawParameterValue("band2");
    pBand3       = mParameters.getRawParameterValue("band3");
    pBand4       = mParameters.getRawParameterValue("band4");
    pBand5       = mParameters.getRawParameterValue("band5");
    pRecStart    = mParameters.getRawParameterValue("rec_start");
    pRecLoop     = mParameters.getRawParameterValue("rec_loop");
    pRecEnd      = mParameters.getRawParameterValue("rec_end");
    pRecSaveAsSpc = mParameters.getRawParameterValue("rec_save_spc");
    pRecSaveAsSmc = mParameters.getRawParameterValue("rec_save_smc");
    pTimeBaseForSmc = mParameters.getRawParameterValue("timebase_smc");
    pRepeatNumForSpc = mParameters.getRawParameterValue("repeat_spc");
    pFadeMsForSpc = mParameters.getRawParameterValue("fade_spc_ms");
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
    syncGlobalParamsFromEngine();
    syncRecordingParamsFromEngine();
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
        syncGlobalParamsFromEngine();
        syncRecordingParamsFromEngine();
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
    set("lowkey",      kernel->GetPropertyValue(kAudioUnitCustomProperty_LowKey));
    set("highkey",     kernel->GetPropertyValue(kAudioUnitCustomProperty_HighKey));
    set("loop",        kernel->GetPropertyValue(kAudioUnitCustomProperty_Loop));
    set("rate",        kernel->GetPropertyValue(kAudioUnitCustomProperty_Rate));
    set("monomode",    kernel->GetPropertyValue(kAudioUnitCustomProperty_MonoMode));
    set("pitchmod",    kernel->GetPropertyValue(kAudioUnitCustomProperty_PitchModulationOn));
    set("noise",       kernel->GetPropertyValue(kAudioUnitCustomProperty_NoiseOn));
    set("porta",       kernel->GetPropertyValue(kAudioUnitCustomProperty_PortamentoOn));
    set("portarate",   kernel->GetPropertyValue(kAudioUnitCustomProperty_PortamentoRate));
    set("noteprio",    kernel->GetPropertyValue(kAudioUnitCustomProperty_NoteOnPriority));
    set("releaseprio", kernel->GetPropertyValue(kAudioUnitCustomProperty_ReleasePriority));

    kernel->SetPropertyValue(kAudioUnitCustomProperty_EditingProgram, savedEdit);

    mSyncingFromEngine = false;
}

void C700AudioProcessor::syncGlobalParamsFromEngine()
{
    mSyncingFromEngine = true;

    auto* kernel = mAdapter.getKernel();
    auto set = [&](juce::StringRef id, float val) {
        if (auto* param = mParameters.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(val));
    };

    set("poly",        kernel->GetParameter(kParam_poly));
    set("vibrate",     kernel->GetParameter(kParam_vibrate));
    set("vibdepth2",   kernel->GetParameter(kParam_vibdepth2));
    set("velocity",    kernel->GetParameter(kParam_velocity));
    set("bendrange",   kernel->GetParameter(kParam_bendrange));
    set("engine",      kernel->GetParameter(kParam_engine));
    set("banka_multi", kernel->GetParameter(kParam_bankAmulti));
    set("bankb_multi", kernel->GetParameter(kParam_bankBmulti));
    set("bankc_multi", kernel->GetParameter(kParam_bankCmulti));
    set("bankd_multi", kernel->GetParameter(kParam_bankDmulti));
    set("voicealloc",  kernel->GetParameter(kParam_voiceAllocMode));
    set("mainvol_l",   kernel->GetParameter(kParam_mainvol_L));
    set("mainvol_r",   kernel->GetParameter(kParam_mainvol_R));
    set("echodelay",   kernel->GetParameter(kParam_echodelay));
    set("echofb",      kernel->GetParameter(kParam_echoFB));
    set("echovol_l",   kernel->GetParameter(kParam_echovol_L));
    set("echovol_r",   kernel->GetParameter(kParam_echovol_R));
    set("fir0",        kernel->GetParameter(kParam_fir0));
    set("fir1",        kernel->GetParameter(kParam_fir1));
    set("fir2",        kernel->GetParameter(kParam_fir2));
    set("fir3",        kernel->GetParameter(kParam_fir3));
    set("fir4",        kernel->GetParameter(kParam_fir4));
    set("fir5",        kernel->GetParameter(kParam_fir5));
    set("fir6",        kernel->GetParameter(kParam_fir6));
    set("fir7",        kernel->GetParameter(kParam_fir7));
    set("band1",       kernel->GetPropertyValue(kAudioUnitCustomProperty_Band1));
    set("band2",       kernel->GetPropertyValue(kAudioUnitCustomProperty_Band2));
    set("band3",       kernel->GetPropertyValue(kAudioUnitCustomProperty_Band3));
    set("band4",       kernel->GetPropertyValue(kAudioUnitCustomProperty_Band4));
    set("band5",       kernel->GetPropertyValue(kAudioUnitCustomProperty_Band5));

    mSyncingFromEngine = false;
}

void C700AudioProcessor::syncRecordingParamsFromEngine()
{
    mSyncingFromEngine = true;

    auto* kernel = mAdapter.getKernel();
    auto set = [&](juce::StringRef id, float val) {
        if (auto* param = mParameters.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(val));
    };

    set("rec_start",    static_cast<float>(kernel->GetPropertyDoubleValue(kAudioUnitCustomProperty_RecordStartBeatPos)));
    set("rec_loop",     static_cast<float>(kernel->GetPropertyDoubleValue(kAudioUnitCustomProperty_RecordLoopStartBeatPos)));
    set("rec_end",      static_cast<float>(kernel->GetPropertyDoubleValue(kAudioUnitCustomProperty_RecordEndBeatPos)));
    set("rec_save_spc", kernel->GetPropertyValue(kAudioUnitCustomProperty_RecSaveAsSpc));
    set("rec_save_smc", kernel->GetPropertyValue(kAudioUnitCustomProperty_RecSaveAsSmc));
    set("timebase_smc", kernel->GetPropertyValue(kAudioUnitCustomProperty_TimeBaseForSmc));
    set("repeat_spc",   kernel->GetPropertyValue(kAudioUnitCustomProperty_RepeatNumForSpc));
    set("fade_spc_ms",  kernel->GetPropertyValue(kAudioUnitCustomProperty_FadeMsTimeForSpc));

    mSyncingFromEngine = false;
}

void C700AudioProcessor::pushPerInstrumentParamsToEngine(int prog)
{
    auto* kernel = mAdapter.getKernel();
    float savedEdit = kernel->GetPropertyValue(kAudioUnitCustomProperty_EditingProgram);
    kernel->SetPropertyValue(kAudioUnitCustomProperty_EditingProgram, static_cast<float>(prog));

    auto setIfChanged = [&](int propertyId, float value) {
        if (kernel->GetPropertyValue(propertyId) != value)
            kernel->SetPropertyValue(propertyId, value);
    };
    auto setDoubleIfChanged = [&](int propertyId, double value) {
        if (kernel->GetPropertyDoubleValue(propertyId) != value)
            kernel->SetPropertyDoubleValue(propertyId, value);
    };

    setIfChanged(kAudioUnitCustomProperty_AR,           pAR->load());
    setIfChanged(kAudioUnitCustomProperty_DR,           pDR->load());
    setIfChanged(kAudioUnitCustomProperty_SL,           pSL->load());
    setIfChanged(kAudioUnitCustomProperty_SR1,          pSR1->load());
    setIfChanged(kAudioUnitCustomProperty_SR2,          pSR2->load());
    setIfChanged(kAudioUnitCustomProperty_SustainMode,  pSustainMode->load());
    setIfChanged(kAudioUnitCustomProperty_VolL,         pVolL->load());
    setIfChanged(kAudioUnitCustomProperty_VolR,         pVolR->load());
    setIfChanged(kAudioUnitCustomProperty_Echo,         pEcho->load());
    setIfChanged(kAudioUnitCustomProperty_BaseKey,      pBaseKey->load());
    setIfChanged(kAudioUnitCustomProperty_LowKey,       pLowKey->load());
    setIfChanged(kAudioUnitCustomProperty_HighKey,      pHighKey->load());
    setIfChanged(kAudioUnitCustomProperty_Loop,         pLoop->load());
    setDoubleIfChanged(kAudioUnitCustomProperty_Rate,   static_cast<double>(pRate->load()));
    setIfChanged(kAudioUnitCustomProperty_MonoMode,     pMonoMode->load());
    setIfChanged(kAudioUnitCustomProperty_PitchModulationOn, pPitchMod->load());
    setIfChanged(kAudioUnitCustomProperty_NoiseOn,      pNoise->load());
    setIfChanged(kAudioUnitCustomProperty_PortamentoOn, pPortamentoOn->load());
    setIfChanged(kAudioUnitCustomProperty_PortamentoRate, pPortamentoRate->load());
    setIfChanged(kAudioUnitCustomProperty_NoteOnPriority, pNoteOnPriority->load());
    setIfChanged(kAudioUnitCustomProperty_ReleasePriority, pReleasePriority->load());

    kernel->SetPropertyValue(kAudioUnitCustomProperty_EditingProgram, savedEdit);
}

void C700AudioProcessor::pushGlobalParamsToEngine()
{
    auto* kernel = mAdapter.getKernel();
    auto setParamIfChanged = [&](int paramId, float value) {
        if (kernel->GetParameter(paramId) != value)
            kernel->SetParameter(paramId, value);
    };
    auto setPropIfChanged = [&](int propertyId, float value) {
        if (kernel->GetPropertyValue(propertyId) != value)
            kernel->SetPropertyValue(propertyId, value);
    };
    auto setPropDoubleIfChanged = [&](int propertyId, double value) {
        if (kernel->GetPropertyDoubleValue(propertyId) != value)
            kernel->SetPropertyDoubleValue(propertyId, value);
    };

    setParamIfChanged(kParam_poly,       pPoly->load());
    setParamIfChanged(kParam_vibrate,    pVibrate->load());
    setParamIfChanged(kParam_vibdepth2,  pVibDepth2->load());
    setParamIfChanged(kParam_velocity,   pVelocity->load());
    setParamIfChanged(kParam_bendrange,  pBendRange->load());
    setParamIfChanged(kParam_engine,     pEngine->load());
    setParamIfChanged(kParam_bankAmulti, pBankAMulti->load());
    setParamIfChanged(kParam_bankBmulti, pBankBMulti->load());
    setParamIfChanged(kParam_bankCmulti, pBankCMulti->load());
    setParamIfChanged(kParam_bankDmulti, pBankDMulti->load());
    setParamIfChanged(kParam_voiceAllocMode, pVoiceAllocMode->load());
    setParamIfChanged(kParam_mainvol_L,  pMainVolL->load());
    setParamIfChanged(kParam_mainvol_R,  pMainVolR->load());
    setParamIfChanged(kParam_echodelay,  pEchoDelay->load());
    setParamIfChanged(kParam_echoFB,     pEchoFB->load());
    setParamIfChanged(kParam_echovol_L,  pEchoVolL->load());
    setParamIfChanged(kParam_echovol_R,  pEchoVolR->load());
    setParamIfChanged(kParam_fir0,       pFir0->load());
    setParamIfChanged(kParam_fir1,       pFir1->load());
    setParamIfChanged(kParam_fir2,       pFir2->load());
    setParamIfChanged(kParam_fir3,       pFir3->load());
    setParamIfChanged(kParam_fir4,       pFir4->load());
    setParamIfChanged(kParam_fir5,       pFir5->load());
    setParamIfChanged(kParam_fir6,       pFir6->load());
    setParamIfChanged(kParam_fir7,       pFir7->load());
    setPropIfChanged(kAudioUnitCustomProperty_Band1, pBand1->load());
    setPropIfChanged(kAudioUnitCustomProperty_Band2, pBand2->load());
    setPropIfChanged(kAudioUnitCustomProperty_Band3, pBand3->load());
    setPropIfChanged(kAudioUnitCustomProperty_Band4, pBand4->load());
    setPropIfChanged(kAudioUnitCustomProperty_Band5, pBand5->load());

    setPropDoubleIfChanged(kAudioUnitCustomProperty_RecordStartBeatPos, static_cast<double>(pRecStart->load()));
    setPropDoubleIfChanged(kAudioUnitCustomProperty_RecordLoopStartBeatPos, static_cast<double>(pRecLoop->load()));
    setPropDoubleIfChanged(kAudioUnitCustomProperty_RecordEndBeatPos, static_cast<double>(pRecEnd->load()));
    setPropIfChanged(kAudioUnitCustomProperty_RecSaveAsSpc, pRecSaveAsSpc->load());
    setPropIfChanged(kAudioUnitCustomProperty_RecSaveAsSmc, pRecSaveAsSmc->load());
    setPropIfChanged(kAudioUnitCustomProperty_TimeBaseForSmc, pTimeBaseForSmc->load());
    setPropIfChanged(kAudioUnitCustomProperty_RepeatNumForSpc, pRepeatNumForSpc->load());
    setPropIfChanged(kAudioUnitCustomProperty_FadeMsTimeForSpc, pFadeMsForSpc->load());
}

void C700AudioProcessor::updateRuntimeState(int editSlot)
{
    RuntimeState state;
    auto* kernel = mAdapter.getKernel();

    state.aramBytes = static_cast<int>(kernel->GetPropertyValue(kAudioUnitCustomProperty_TotalRAM));
    state.playerCodeVersion = static_cast<int>(kernel->GetPropertyValue(kAudioUnitCustomProperty_SongPlayerCodeVer));
    state.hasPlayerCode = state.playerCodeVersion > 0;
    state.isHwConnected = kernel->GetPropertyValue(kAudioUnitCustomProperty_IsHwConnected) != 0.0f;
    state.isRecording = mAdapter.isRecording();
    state.finishedRecording = mAdapter.hasFinishedRecording();
    state.editingChannel = static_cast<int>(kernel->GetPropertyValue(kAudioUnitCustomProperty_EditingChannel));
    state.maxNoteTotal = static_cast<int>(kernel->GetPropertyValue(kAudioUnitCustomProperty_MaxNoteOnTotal));
    state.sampleName = juce::String(mAdapter.getSampleName(editSlot));
    state.programName = juce::String(mAdapter.getProgramName(editSlot));

    float savedEdit = kernel->GetPropertyValue(kAudioUnitCustomProperty_EditingProgram);
    kernel->SetPropertyValue(kAudioUnitCustomProperty_EditingProgram, static_cast<float>(editSlot));
    state.bank = static_cast<int>(kernel->GetPropertyValue(kAudioUnitCustomProperty_Bank));
    kernel->SetPropertyValue(kAudioUnitCustomProperty_EditingProgram, savedEdit);

    for (int i = 0; i < 16; ++i) {
        state.noteOnTrack[static_cast<size_t>(i)] =
            static_cast<int>(kernel->GetPropertyValue(kAudioUnitCustomProperty_NoteOnTrack_1 + i));
        state.maxNoteTrack[static_cast<size_t>(i)] =
            static_cast<int>(kernel->GetPropertyValue(kAudioUnitCustomProperty_MaxNoteTrack_1 + i));
    }

    const juce::SpinLock::ScopedLockType lock(mRuntimeStateLock);
    mRuntimeState = state;
}

// --- Process ---

void C700AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Edit Slot selects which instrument slot's params are displayed/edited
    // It does NOT change what any MIDI channel plays (that's per-channel program change)
    int editSlot = static_cast<int>(pEditSlot->load());

    if (editSlot != mLastProgram) {
        // Slot changed — pull this slot's values into the JUCE parameters
        syncPerInstrumentParamsFromEngine(editSlot);
        mLastProgram = editSlot;
    }
    else if (!mSyncingFromEngine) {
        // User may have changed per-instrument params — push to engine
        pushPerInstrumentParamsToEngine(editSlot);
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

    updateRuntimeState(editSlot);

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
    int prog = static_cast<int>(pEditSlot->load());
    syncPerInstrumentParamsFromEngine(prog);
    syncGlobalParamsFromEngine();
    syncRecordingParamsFromEngine();
    updateRuntimeState(prog);
}

C700AudioProcessor::RuntimeState C700AudioProcessor::getRuntimeState() const
{
    const juce::SpinLock::ScopedLockType lock(mRuntimeStateLock);
    return mRuntimeState;
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
