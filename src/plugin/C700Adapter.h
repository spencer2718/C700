#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class C700Kernel;

class C700Adapter
{
public:
    C700Adapter();
    ~C700Adapter();

    void init(double sampleRate, int blockSize);
    void process(float** output, int numSamples, juce::MidiBuffer& midi);
    void reset();

private:
    std::unique_ptr<C700Kernel> mKernel;
    double mSampleRate = 44100.0;
    int mBlockSize = 512;
    bool mPresetsLoaded = false;
};
