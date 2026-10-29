#pragma once

#include "DSP/Filters.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <vector>

namespace guitar_delay
{

// Memory Man / bucket-brigade path.
// A fixed-length shift register clocked at f = stages / delay, with 4th-order
// anti-alias and reconstruction filters that track the clock, a compander, and
// a soft clip that grits up as feedback pushes the loop. Shorter delays run a
// faster clock and stay brighter. This is not an audio-rate fractional delay.
struct AnalogSettings
{
    float timeMs = 375.0f;
    float feedback = 0.34f;
    float width = 0.20f;
    float tone = 0.58f;
    float drive = 0.20f;
};

class AnalogDelay
{
public:
    static constexpr int kStages = 4096;

    void prepare (double sampleRate);
    void reset() noexcept;
    void snap (const AnalogSettings& settings) noexcept;
    void process (const float* const* input, float* const* output, int numChannels, int numSamples, const AnalogSettings& settings);

private:
    struct Brigade
    {
        std::vector<float> stages;
        int index = 0;
        float phase = 0.0f;
        float held = 0.0f;
        float feedback = 0.0f;
        float compEnv = 0.0f;
        float expEnv = 0.0f;
        OnePoleHighpass inputCut;
        Oversampler8x clipOs;
        Lowpass4 antiAlias;
        Lowpass4 reconstruct;
        DcBlocker dc;

        void prepare();
        void clear() noexcept;
        float clock (float sample, float ticks) noexcept;
    };

    void arm (const AnalogSettings& settings, bool immediate) noexcept;

    double sampleRate = 48000.0;
    float compAttack = 0.02f;
    float compRelease = 0.002f;
    float currentTimeMs = 375.0f;
    Brigade brigades[2];
    FastRng commonNoise;
    FastRng sideNoise;

    juce::SmoothedValue<float> feedback;
    juce::SmoothedValue<float> width;
    juce::SmoothedValue<float> tone;
    juce::SmoothedValue<float> drive;
    float cachedFeedbackKnob = -1.0f;
    float cachedFeedbackGain = 0.0f;
    float cachedDrive = -1.0f;
    float cachedDriveGain = 1.0f;
};

} // namespace guitar_delay
