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

    // Sample loading — returns true on success
    bool loadSampleToSlot(int slot, const std::string& filePath);
    bool unloadSlot(int slot);

    // Query
    std::string getSampleName(int slot) const;
    std::string getProgramName(int slot) const;
    std::vector<unsigned char> copyBRRData(int slot) const;
    const std::string& getLastLoadError() const { return mLastLoadError; }
    float getProgramPropertyValue(int slot, int propertyId);
    double getProgramPropertyDoubleValue(int slot, int propertyId);
    bool setProgramPropertyValue(int slot, int propertyId, float value);
    bool setProgramPropertyDoubleValue(int slot, int propertyId, double value);
    std::string getProgramStringProperty(int slot, int propertyId);
    bool setProgramStringProperty(int slot, int propertyId, const std::string& value);

    // Generic engine accessors for the reconstructed UI
    float getParameterValue(int paramId) const;
    bool setParameterValue(int paramId, float value);
    float getPropertyValue(int propertyId) const;
    double getPropertyDoubleValue(int propertyId) const;
    bool setPropertyValue(int propertyId, float value);
    bool setPropertyDoubleValue(int propertyId, double value);
    std::string getStringProperty(int propertyId) const;
    bool setStringProperty(int propertyId, const std::string& value);
    std::string getFilePathProperty(int propertyId) const;
    bool setFilePathProperty(int propertyId, const std::string& path);

    // Transport (call from processBlock)
    void setTransportInfo(double tempo, double ppqPos, bool isPlaying);

    // SPC recording
    void setSpcRecordPath(const std::string& path);
    void setSpcRecordRegion(double startBeat, double loopBeat, double endBeat);
    void enableSpcRecording(bool enable);
    bool loadPlayerCode(const std::string& path);
    bool hasPlayerCode() const;
    bool isRecording() const;
    bool hasFinishedRecording();

    // State save/load via kernel chunk serialization
    void getStateData(juce::MemoryBlock& destData);
    void setStateData(const void* data, int sizeInBytes);

    C700Kernel* getKernel() { return mKernel.get(); }
    const C700Kernel* getKernel() const { return mKernel.get(); }

private:
    std::unique_ptr<C700Kernel> mKernel;
    double mSampleRate = 44100.0;
    int mBlockSize = 512;
    bool mPresetsLoaded = false;
    int mChannelProgram[16] = {}; // track per-channel program assignments
    std::string mLastLoadError;

    bool loadBRR(int slot, const std::string& filePath);
    bool loadWAV(int slot, const std::string& filePath);
};
