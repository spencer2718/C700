#include "C700Adapter.h"
#include "C700Kernel.h"

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
    // so the plugin produces audible output immediately.
    // Preset 1 = "Testtones" — loads 5 waveforms from samplebrr.h
    if (!mPresetsLoaded) {
        mKernel->SelectPreset(1);
        mPresetsLoaded = true;
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
            mKernel->HandleNoteOn(ch, msg.getNoteNumber(),
                                  msg.getVelocity(), 0, frame);
        }
        else if (msg.isNoteOff()) {
            mKernel->HandleNoteOff(ch, msg.getNoteNumber(), 0, frame);
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
