#include "C700Adapter.h"
#include "C700Kernel.h"
#include "C700Properties.h"
#include "RawBRRFile.h"
#include "PlayerCodeReader.h"
#include "brrcodec.h"
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <algorithm>

C700Adapter::C700Adapter()
    : mKernel(std::make_unique<C700Kernel>())
{
}

C700Adapter::~C700Adapter() = default;

void C700Adapter::init(double sampleRate, int blockSize)
{
    mSampleRate = sampleRate;
    mBlockSize = blockSize;
    mKernel->Reset();
    mKernel->SetSampleRate(sampleRate);

    // Load built-in test tones (sine, square, pulses) into slots 0-4
    // so the plugin has audible content on first use.
    if (!mPresetsLoaded) {
        mKernel->SelectPreset(1);
        mPresetsLoaded = true;
    }

    // Auto-load playercode.bin for SPC export support
    if (!hasPlayerCode()) {
        const char* home = getenv("HOME");
        if (home) {
            std::string pcPath = std::string(home) + "/.config/C700/playercode.bin";
            loadPlayerCode(pcPath);
        }
    }
}

void C700Adapter::process(float** output, int numSamples, juce::MidiBuffer& midi)
{
    // Feed MIDI events into the engine
    for (const auto metadata : midi) {
        auto msg = metadata.getMessage();
        int frame = metadata.samplePosition;
        int ch = msg.getChannel() - 1; // JUCE uses 1-based channels
        if (ch < 0) ch = 0;

        if (msg.isNoteOn()) {
            int note = msg.getNoteNumber();
            int uniqueID = note + ch * 256;
            mKernel->HandleNoteOn(ch, note,
                                  msg.getVelocity(), uniqueID, frame);
        }
        else if (msg.isNoteOff()) {
            int note = msg.getNoteNumber();
            int uniqueID = note + ch * 256;
            mKernel->HandleNoteOff(ch, note, uniqueID, frame);
        }
        else if (msg.isPitchWheel()) {
            int pw = msg.getPitchWheelValue();
            mKernel->HandlePitchBend(ch, pw & 0x7f, (pw >> 7) & 0x7f, frame);
        }
        else if (msg.isController()) {
            mKernel->HandleControlChange(ch, msg.getControllerNumber(),
                                         msg.getControllerValue(), frame);
        }
        else if (msg.isProgramChange()) {
            mKernel->HandleProgramChange(ch, msg.getProgramChangeNumber(), frame);
        }
        else if (msg.isAllNotesOff()) {
            mKernel->HandleAllNotesOff(ch, frame);
        }
        else if (msg.isAllSoundOff()) {
            mKernel->HandleAllSoundOff(ch, frame);
        }
        else if (msg.isResetAllControllers()) {
            mKernel->HandleResetAllControllers(ch, frame);
        }
    }

    // Render audio — C700Kernel::Render outputs float stereo
    float* outputPtrs[2] = { output[0], output[1] };
    mKernel->Render(static_cast<unsigned int>(numSamples), outputPtrs);
}

void C700Adapter::reset()
{
    mKernel->Reset();
    mKernel->SetSampleRate(mSampleRate);
}

void C700Adapter::setProgram(int channel, int program)
{
    mKernel->HandleProgramChange(channel, program, 0);
}

void C700Adapter::setProgramForAllChannels(int program)
{
    for (int ch = 0; ch < 16; ch++)
        mKernel->HandleProgramChange(ch, program, 0);
}

// --- Query ---

std::string C700Adapter::getSampleName(int slot)
{
    if (slot < 0 || slot > 127) return "";
    const InstParams* vp = &mKernel->GetVP()[slot];
    if (!vp->hasBrrData()) return "(empty)";
    return std::string(vp->pgname);
}

// --- Sample loading ---

bool C700Adapter::loadSampleToSlot(int slot, const std::string& filePath)
{
    if (slot < 0 || slot > 127) return false;

    // Detect file type by extension
    std::string ext;
    auto dotPos = filePath.rfind('.');
    if (dotPos != std::string::npos) {
        ext = filePath.substr(dotPos);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    }

    if (ext == ".brr")
        return loadBRR(slot, filePath);
    else if (ext == ".wav")
        return loadWAV(slot, filePath);

    return false;
}

bool C700Adapter::loadBRR(int slot, const std::string& filePath)
{
    RawBRRFile brrFile(filePath.c_str(), false);
    if (!brrFile.Load()) return false;

    const InstParams* inst = brrFile.GetLoadedInst();
    if (!inst || !inst->hasBrrData()) return false;

    // Copy instrument parameters to the kernel's slot
    InstParams* vp = const_cast<InstParams*>(&mKernel->GetVP()[slot]);
    unsigned int hasFlag = brrFile.GetHasFlag();

    if (hasFlag & HAS_PGNAME)   std::strncpy(vp->pgname, inst->pgname, PROGRAMNAME_MAX_LEN);
    if (hasFlag & HAS_RATE)     vp->rate = inst->rate;
    if (hasFlag & HAS_BASEKEY)  vp->basekey = inst->basekey;
    if (hasFlag & HAS_LOWKEY)   vp->lowkey = inst->lowkey;
    if (hasFlag & HAS_HIGHKEY)  vp->highkey = inst->highkey;
    if (hasFlag & HAS_AR)       vp->ar = inst->ar;
    if (hasFlag & HAS_DR)       vp->dr = inst->dr;
    if (hasFlag & HAS_SL)       vp->sl = inst->sl;
    if (hasFlag & HAS_SR1)      vp->sr1 = inst->sr1;
    if (hasFlag & HAS_SR2)      vp->sr2 = inst->sr2;
    if (hasFlag & HAS_VOLL)     vp->volL = inst->volL;
    if (hasFlag & HAS_VOLR)     vp->volR = inst->volR;
    if (hasFlag & HAS_ECHO)     vp->echo = inst->echo;
    if (hasFlag & HAS_BANK)     vp->bank = inst->bank;
    if (hasFlag & HAS_SUSTAINMODE) vp->sustainMode = inst->sustainMode;
    vp->loop = inst->loop;
    vp->lp = inst->lp;

    // Set mEditProg so SetBRRData's internal SetPropertyValue targets the right slot
    float savedEditProg = mKernel->GetPropertyValue(kAudioUnitCustomProperty_EditingProgram);
    mKernel->SetPropertyValue(kAudioUnitCustomProperty_EditingProgram, static_cast<float>(slot));

    // Register BRR data in the kernel (this copies and transfers to DSP)
    mKernel->SetBRRData(inst->brrData(), inst->brrSize(), slot, false, false);

    mKernel->SetPropertyValue(kAudioUnitCustomProperty_EditingProgram, savedEditProg);
    mKernel->GetDriver()->RefreshKeyMap();

    return true;
}

bool C700Adapter::loadWAV(int slot, const std::string& filePath)
{
    // Portable WAV loader — reads PCM 16-bit or 8-bit mono/stereo WAV files
    std::ifstream file(filePath, std::ios::binary);
    if (!file) return false;

    // Read RIFF header
    char riff[4];
    file.read(riff, 4);
    if (std::strncmp(riff, "RIFF", 4) != 0) return false;

    uint32_t fileSize;
    file.read(reinterpret_cast<char*>(&fileSize), 4);

    char wave[4];
    file.read(wave, 4);
    if (std::strncmp(wave, "WAVE", 4) != 0) return false;

    // Parse chunks
    uint16_t numChannels = 0, bitsPerSample = 0;
    uint32_t sampleRate = 0;
    std::vector<short> pcmData;
    int basekey = 60;
    bool hasLoop = false;
    uint32_t loopStart = 0, loopEnd = 0;

    while (file.good()) {
        char chunkId[4];
        uint32_t chunkSize;
        file.read(chunkId, 4);
        file.read(reinterpret_cast<char*>(&chunkSize), 4);
        if (!file.good()) break;

        auto chunkStart = file.tellg();

        if (std::strncmp(chunkId, "fmt ", 4) == 0) {
            uint16_t formatTag;
            file.read(reinterpret_cast<char*>(&formatTag), 2);
            if (formatTag != 1) return false; // Only PCM
            file.read(reinterpret_cast<char*>(&numChannels), 2);
            file.read(reinterpret_cast<char*>(&sampleRate), 4);
            uint32_t byteRate;
            file.read(reinterpret_cast<char*>(&byteRate), 4);
            uint16_t blockAlign;
            file.read(reinterpret_cast<char*>(&blockAlign), 2);
            file.read(reinterpret_cast<char*>(&bitsPerSample), 2);
        }
        else if (std::strncmp(chunkId, "data", 4) == 0) {
            if (numChannels == 0 || bitsPerSample == 0) return false;
            int bytesPerSample = bitsPerSample / 8;
            int totalSamples = chunkSize / (bytesPerSample * numChannels);
            pcmData.resize(totalSamples);

            for (int i = 0; i < totalSamples; i++) {
                int sum = 0;
                for (int ch = 0; ch < numChannels; ch++) {
                    if (bytesPerSample == 2) {
                        int16_t s;
                        file.read(reinterpret_cast<char*>(&s), 2);
                        sum += s;
                    } else if (bytesPerSample == 1) {
                        uint8_t s;
                        file.read(reinterpret_cast<char*>(&s), 1);
                        sum += (static_cast<int>(s) - 128) * 256;
                    } else if (bytesPerSample == 3) {
                        uint8_t b[3];
                        file.read(reinterpret_cast<char*>(b), 3);
                        int32_t s = (b[2] << 24) | (b[1] << 16) | (b[0] << 8);
                        sum += s >> 16;
                    }
                }
                pcmData[i] = static_cast<short>(sum / numChannels);
            }
        }
        else if (std::strncmp(chunkId, "smpl", 4) == 0 && chunkSize >= 36) {
            // WAV sampler chunk
            uint32_t smplFields[9];
            file.read(reinterpret_cast<char*>(smplFields), 36);
            uint32_t midiNote = smplFields[3]; // note field
            uint32_t numLoops = smplFields[7];
            if (midiNote > 0 && midiNote < 128) basekey = midiNote;
            if (numLoops > 0 && chunkSize >= 60) {
                uint32_t loopFields[6];
                file.read(reinterpret_cast<char*>(loopFields), 24);
                loopStart = loopFields[2]; // start
                loopEnd = loopFields[3] + 1; // end (inclusive in WAV)
                hasLoop = true;
            }
        }

        // Seek to next chunk (handle odd-sized chunks)
        file.seekg(chunkStart);
        file.seekg(chunkSize + (chunkSize & 1), std::ios::cur);
    }

    if (pcmData.empty() || sampleRate == 0) return false;

    // BRR encode the PCM data
    int numSamples = static_cast<int>(pcmData.size());
    int brrLoopPoint = hasLoop ? static_cast<int>((loopStart / 16) * 9) : 0;
    // BRR output buffer: each 16 PCM samples → 9 BRR bytes
    int maxBrrSize = ((numSamples + 15) / 16) * 9 + 9;
    std::vector<unsigned char> brrData(maxBrrSize);

    int brrSize = brrencode(pcmData.data(), brrData.data(), numSamples,
                            hasLoop, hasLoop ? loopStart : 0, 0);
    if (brrSize <= 0) return false;

    // Set instrument parameters
    InstParams* vp = const_cast<InstParams*>(&mKernel->GetVP()[slot]);
    vp->basekey = basekey;
    vp->lowkey = 0;
    vp->highkey = 127;
    vp->rate = static_cast<double>(sampleRate);
    vp->loop = hasLoop;
    vp->lp = brrLoopPoint;
    vp->ar = 15;
    vp->dr = 7;
    vp->sl = 7;
    vp->sr1 = 0;
    vp->sr2 = 31;
    vp->sustainMode = true;
    vp->volL = 100;
    vp->volR = 100;
    vp->echo = false;

    // Extract filename for program name
    auto slashPos = filePath.rfind('/');
    std::string fname = (slashPos != std::string::npos) ? filePath.substr(slashPos + 1) : filePath;
    auto dot = fname.rfind('.');
    if (dot != std::string::npos) fname = fname.substr(0, dot);
    std::strncpy(vp->pgname, fname.c_str(), PROGRAMNAME_MAX_LEN - 1);
    vp->pgname[PROGRAMNAME_MAX_LEN - 1] = '\0';

    // Set mEditProg so SetBRRData's internal SetPropertyValue targets the right slot
    float savedEditProg = mKernel->GetPropertyValue(kAudioUnitCustomProperty_EditingProgram);
    mKernel->SetPropertyValue(kAudioUnitCustomProperty_EditingProgram, static_cast<float>(slot));

    // Register BRR data in the kernel
    mKernel->SetBRRData(brrData.data(), brrSize, slot, false, false);

    mKernel->SetPropertyValue(kAudioUnitCustomProperty_EditingProgram, savedEditProg);
    mKernel->GetDriver()->RefreshKeyMap();

    return true;
}

// --- Transport ---

void C700Adapter::setTransportInfo(double tempo, double ppqPos, bool isPlaying)
{
    mKernel->SetTempo(tempo);
    mKernel->SetCurrentSampleInTimeLine(ppqPos);
    mKernel->SetIsPlaying(isPlaying);
}

// --- SPC recording ---

void C700Adapter::setSpcRecordPath(const std::string& path)
{
    mKernel->GetDriver()->GetDsp()->SetSongRecordPath(path.c_str());
}

void C700Adapter::setSpcRecordRegion(double startBeat, double loopBeat, double endBeat)
{
    mKernel->SetPropertyDoubleValue(kAudioUnitCustomProperty_RecordStartBeatPos, startBeat);
    mKernel->SetPropertyDoubleValue(kAudioUnitCustomProperty_RecordLoopStartBeatPos, loopBeat);
    mKernel->SetPropertyDoubleValue(kAudioUnitCustomProperty_RecordEndBeatPos, endBeat);
}

void C700Adapter::enableSpcRecording(bool enable)
{
    mKernel->GetDriver()->GetDsp()->SetRecSaveAsSpc(enable);
}

bool C700Adapter::loadPlayerCode(const std::string& path)
{
    PlayerCodeReader codeFile(path.c_str());
    if (!codeFile.IsLoaded()) return false;

    auto* dsp = mKernel->GetDriver()->GetDsp();
    dsp->SetSpcPlayerCode(codeFile.getSpcPlayerCode(), codeFile.getSpcPlayerCodeSize());
    dsp->SetSmcPlayerCode(codeFile.getSmcPlayerCode(), codeFile.getSmcPlayerCodeSize());
    dsp->SetSmcNativeVector(codeFile.getSmcNativeVector());
    dsp->SetSmcEmulationVector(codeFile.getSmcEmulationVector());
    dsp->SetSongPlayCodeVer(codeFile.getVersion());
    return true;
}

bool C700Adapter::hasPlayerCode() const
{
    return mKernel->GetDriver()->GetDsp()->GetSongPlayCodeVer() > 0;
}

bool C700Adapter::isRecording() const
{
    return mKernel->GetDriver()->GetDsp()->GetRecSaveAsSpc();
}

bool C700Adapter::hasFinishedRecording() const
{
    // After recording completes, the logger is ended and RecSaveAsSpc is still true
    // but we can check if the log has ended
    return false; // TODO: expose logger state if needed
}

// --- State save/load using kernel chunk serialization ---
// Format matches legacy C700VST.cpp: outer chunks keyed by
// CKID_PROGRAM_DATA+pgnum, each wrapping the inner property data.

void C700Adapter::getStateData(juce::MemoryBlock& destData)
{
    ChunkReader saveChunk(1024 * 32);
    saveChunk.SetAllowExtend(true);

    int totalProgs = 0;
    for (int pg = 0; pg < 128; pg++) {
        const InstParams* vp = &mKernel->GetVP()[pg];
        if (vp->hasBrrData()) {
            int pgChunkSize = mKernel->GetPGChunkSize(pg);
            if (pgChunkSize > 0) {
                ChunkReader pgChunk(pgChunkSize);
                mKernel->SetPGDataToChunk(&pgChunk, pg);
                saveChunk.addChunk(CKID_PROGRAM_DATA + pg,
                                   pgChunk.GetDataPtr(), pgChunk.GetDataUsed());
                totalProgs++;
            }
        }
    }
    saveChunk.addChunk(CKID_PROGRAM_TOTAL, &totalProgs, sizeof(int));

    if (saveChunk.GetDataUsed() > 0) {
        destData.append(saveChunk.GetDataPtr(), saveChunk.GetDataUsed());
    }
}

void C700Adapter::setStateData(const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0) return;

    // Clear existing instruments
    mKernel->SelectPreset(0);

    ChunkReader chunk(data, sizeInBytes);

    while (chunk.GetLeftSize() >= static_cast<int>(sizeof(ChunkReader::MyChunkHead))) {
        int ckType;
        long ckSize;
        if (!chunk.readChunkHead(&ckType, &ckSize)) break;

        if (ckType == CKID_PROGRAM_TOTAL) {
            int totalProgs = 0;
            chunk.readData(&totalProgs, sizeof(int));
        }
        else if (ckType >= CKID_PROGRAM_DATA && ckType < CKID_PROGRAM_DATA + 128) {
            // Inner data is a nested chunk — create a sub-reader for it
            int pgnum = ckType - CKID_PROGRAM_DATA;
            ChunkReader pgChunk(chunk.GetDataPtr() + chunk.GetDataPos(),
                                static_cast<int>(ckSize));
            mKernel->RestorePGDataFromChunk(&pgChunk, pgnum);
            chunk.AdvDataPos(static_cast<int>(ckSize));
        }
        else {
            chunk.AdvDataPos(static_cast<int>(ckSize));
        }
    }

    mKernel->GetDriver()->RefreshKeyMap();
    mPresetsLoaded = true; // Don't overwrite with test tones
}
