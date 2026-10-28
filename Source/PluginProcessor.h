#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Params.h"

class SatLabProcessor : public juce::AudioProcessor
{
public:
    SatLabProcessor();
    ~SatLabProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SatLab"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    satlab::ParamCache params;

    std::atomic<float> inLevel { 0.0f }, outLevel { 0.0f };   // crete, pour les vu-metres

private:
    static constexpr int kMaxFactorLog2 = 4;      // 16x
    void applyPreset (int index);

    std::array<satlab::SatEngine, kMaxFactorLog2 + 1> engines;
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, kMaxFactorLog2> oversamplers;  // 2x, 4x, 8x, 16x
    juce::AudioBuffer<float> work;
    int  maxBlock = 512;
    double sr = 44100.0;
    int  currentOs = -1;
    int  currentProgram = 0;
    bool prepared = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SatLabProcessor)
};
