#pragma once

#include "DSP/AnalogDelay.h"
#include "DSP/DigitalDelay.h"
#include "DSP/TapeDelay.h"
#include "Parameters.h"
#include "Presets.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <vector>

namespace guitar_delay
{

class GuitarDelayProcessor final : public juce::AudioProcessor
{
public:
    GuitarDelayProcessor();
    ~GuitarDelayProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    using AudioProcessor::processBlockBypassed;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorParameter* getBypassParameter() const override;

    int getModeIndex() const noexcept;
    float getDisplayedBpm() const noexcept;
    bool isSyncEnabled() const noexcept;
    bool programMatchesCurrent() const noexcept;

    juce::AudioProcessorValueTreeState apvts;

private:
    struct BlockSettings
    {
        float delayMs = 375.0f;
        float feedback = 0.34f;
        float width = 0.20f;
        float wow = 0.32f;
        float flutter = 0.20f;
        float age = 0.38f;
        float tone = 0.58f;
        float drive = 0.20f;
        float hiCutHz = 7200.0f;
        float diffuse = 0.12f;
    };

    void render (juce::AudioBuffer<float>& buffer, bool hostBypass);
    void processChunk (juce::AudioBuffer<float>& buffer, int offset, int count, int channels, bool hostBypass, const BlockSettings& settings);
    void updateTransport();
    float resolveDelayMs() const;
    BlockSettings readSettings() const;
    void handleModeChange (int requested, const BlockSettings& settings);
    void renderMode (int mode, const float* const* input, float* const* output, int channels, int count, const BlockSettings& settings, bool snapFirst);
    void resetMode (int mode);
    void snapMode (int mode, const BlockSettings& settings);

    std::atomic<float>* modeRaw = nullptr;
    std::atomic<float>* timeRaw = nullptr;
    std::atomic<float>* feedbackRaw = nullptr;
    std::atomic<float>* mixRaw = nullptr;
    std::atomic<float>* widthRaw = nullptr;
    std::atomic<float>* syncRaw = nullptr;
    std::atomic<float>* divisionRaw = nullptr;
    std::atomic<float>* bypassRaw = nullptr;
    std::atomic<float>* wowRaw = nullptr;
    std::atomic<float>* flutterRaw = nullptr;
    std::atomic<float>* ageRaw = nullptr;
    std::atomic<float>* toneRaw = nullptr;
    std::atomic<float>* driveRaw = nullptr;
    std::atomic<float>* hiCutRaw = nullptr;
    std::atomic<float>* diffuseRaw = nullptr;

    TapeDelay tape;
    AnalogDelay analog;
    DigitalDelay digital;

    juce::AudioBuffer<float> sendBuffer;
    juce::AudioBuffer<float> wetBuffer;
    juce::AudioBuffer<float> wetTailBuffer;
    juce::AudioBuffer<float> silenceBuffer;
    std::vector<float> bypassEnv;
    std::vector<float> mixEnv;
    std::vector<float> fadeEnv;

    juce::SmoothedValue<float> mixSmooth;
    juce::SmoothedValue<float> bypassSmooth;
    juce::SmoothedValue<float> crossfade;

    double sampleRate = 0.0;
    int blockCapacity = 0;
    int currentMode = 0;
    int outgoingMode = -1;
    std::atomic<int> currentProgram { 0 };
    std::atomic<float> displayedBpm { 120.0f };

    void setRealParameter (const char* paramId, float realValue);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GuitarDelayProcessor)
};

} // namespace guitar_delay
