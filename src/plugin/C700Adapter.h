#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <string>
#include <vector>

class C700Kernel;

class C700Adapter
{
public:
    C700Adapter();
    ~C700Adapter();

    void init(double sampleRate, int blockSize);
    void process(float** output, int numSamples, juce::MidiBuffer& midi);
    void reset();

    void setProgram(int channel, int program);
    void setProgramForAllChannels(int program);

    // Sample loading — returns true on success
    bool loadSampleToSlot(int slot, const std::string& filePath);

    // State save/load via kernel chunk serialization
    void getStateData(juce::MemoryBlock& destData);
    void setStateData(const void* data, int sizeInBytes);

    C700Kernel* getKernel() { return mKernel.get(); }

private:
    std::unique_ptr<C700Kernel> mKernel;
    double mSampleRate = 44100.0;
    int mBlockSize = 512;
    bool mPresetsLoaded = false;

    bool loadBRR(int slot, const std::string& filePath);
    bool loadWAV(int slot, const std::string& filePath);
};
