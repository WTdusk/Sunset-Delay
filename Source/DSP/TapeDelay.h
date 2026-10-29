#pragma once

#include "DSP/Filters.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <vector>

namespace guitar_delay
{

// Echoplex / Space Echo style path.
// A moving read head on an audio-rate tape buffer, with capstan wow and flutter,
// playback-head saturation, and high-frequency loss inside the regeneration loop.
// This is not a clocked BBD and it is not a fixed fractional delay.
struct TapeSettings
{
    float timeMs = 375.0f;
    float feedback = 0.34f;
    float width = 0.20f;
    float wow = 0.32f;
    float flutter = 0.20f;
    float age = 0.38f;
};

class TapeDelay
{
public:
    void prepare (double sampleRate);
    void reset() noexcept;
    void snap (const TapeSettings& settings) noexcept;
    void process (const float* const* input, float* const* output, int numChannels, int numSamples, const TapeSettings& settings);

private:
    struct Head
    {
        std::vector<float> buffer;
        int mask = 0;
        int writeIndex = 0;
        OnePoleLowpass loss;
        OnePoleLowpass presence;
        OnePoleHighpass inputCut;
        Oversampler8x playbackOs;
        Oversampler8x recordOs;
        DcBlocker playbackDc;
        DcBlocker recordDc;
        float compressEnv = 0.0f;
        float wowPhase = 0.0f;
        float flutterPhase = 0.0f;
        float driftPhase = 0.0f;
        float flutterNoise = 0.0f;
        FastRng rng;

        void prepare (int capacity, uint32_t seed);
        void clear() noexcept;
        void push (float x) noexcept;
        float read (float delaySamples) const noexcept;
        float modulate (double sampleRate, float wowAmt, float flutterAmt, float delaySamples) noexcept;
    };

    void arm (const TapeSettings& settings, bool immediate) noexcept;

    double sampleRate = 48000.0;
    float attack = 0.1f;
    float release = 0.001f;
    float currentTimeMs = 375.0f;
    Head heads[2];

    juce::SmoothedValue<float> feedback;
    juce::SmoothedValue<float> width;
    juce::SmoothedValue<float> wow;
    juce::SmoothedValue<float> flutter;
    juce::SmoothedValue<float> age;
    float cachedFeedbackKnob = -1.0f;
    float cachedFeedbackGain = 0.0f;
};

} // namespace guitar_delay
