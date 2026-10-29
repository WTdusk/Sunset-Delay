#pragma once

#include "DSP/Filters.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <vector>

namespace guitar_delay
{

// Clean rhythmic delay.
// Fixed fractional read (no wow), a musical hi-cut in the loop, optional
// Schroeder diffusion, and a width control that crossfades into ping-pong.
// No tape saturation and no clocked BBD.
struct DigitalSettings
{
    float timeMs = 375.0f;
    float feedback = 0.34f;
    float width = 0.20f;
    float hiCutHz = 7200.0f;
    float diffuse = 0.12f;
};

class DigitalDelay
{
public:
    void prepare (double sampleRate);
    void reset() noexcept;
    void snap (const DigitalSettings& settings) noexcept;
    void process (const float* const* input, float* const* output, int numChannels, int numSamples, const DigitalSettings& settings);

private:
    struct Allpass
    {
        std::vector<float> buffer;
        int index = 0;
        float gain = 0.0f;

        void prepare (int length);
        void clear() noexcept;
        float process (float x) noexcept;
    };

    struct Line
    {
        std::vector<float> buffer;
        int mask = 0;
        int writeIndex = 0;
        Biquad hiCut;
        DcBlocker dc;
        OnePoleHighpass inputCut;
        Allpass diffuse[4];

        void prepare (int capacity, double sampleRate, float lengthScale);
        void clear() noexcept;
        void push (float x) noexcept;
        float read (float delaySamples) const noexcept;
        float colour (float x, float diffusion) noexcept;
    };

    void arm (const DigitalSettings& settings, bool immediate) noexcept;

    double sampleRate = 48000.0;
    float fadeStep = 0.001f;
    float activeDelay = 0.0f;
    float pendingDelay = 0.0f;
    float fade = 1.0f;
    bool fading = false;
    Line lines[2];

    juce::SmoothedValue<float> feedback;
    juce::SmoothedValue<float> width;
    juce::SmoothedValue<float> hiCutHz;
    juce::SmoothedValue<float> diffuse;
    float cachedFeedbackKnob = -1.0f;
    float cachedFeedbackGain = 0.0f;
};

} // namespace guitar_delay
