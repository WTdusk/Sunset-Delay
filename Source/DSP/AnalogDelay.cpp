#include "DSP/AnalogDelay.h"

#include <cmath>

namespace guitar_delay
{
namespace
{

float feedbackGain (float knob) noexcept
{
    const float shaped = std::pow (std::clamp (knob, 0.0f, 1.0f), 0.65f);
    return shaped * 0.84f;
}

// C1 soft knee. The value and the slope match a straight wire at the
// threshold, then the excess compresses. A hard derivative jump here
// was its own alias source on hot feedback.
float softLimit (float x) noexcept
{
    constexpr float threshold = 0.90f;
    const float a = std::abs (x);
    if (a <= threshold)
        return x;
    const float sign = x < 0.0f ? -1.0f : 1.0f;
    const float excess = a - threshold;
    return sign * (threshold + excess / (1.0f + excess * 3.0f));
}

} // namespace

void AnalogDelay::Brigade::prepare()
{
    stages.assign (static_cast<size_t> (kStages), 0.0f);
    clear();
}

void AnalogDelay::Brigade::clear() noexcept
{
    std::fill (stages.begin(), stages.end(), 0.0f);
    index = 0;
    phase = 0.0f;
    held = 0.0f;
    feedback = 0.0f;
    compEnv = 0.0f;
    expEnv = 0.0f;
    inputCut.reset();
    clipOs.reset();
    antiAlias.reset();
    reconstruct.reset();
    dc.reset();
}

float AnalogDelay::Brigade::clock (float sample, float ticks) noexcept
{
    phase += ticks;
    int guard = 0;
    while (phase >= 1.0f && guard < 8)
    {
        phase -= 1.0f;
        held = stages[static_cast<size_t> (index)];
        stages[static_cast<size_t> (index)] = sample;
        ++index;
        if (index >= kStages)
            index = 0;
        ++guard;
    }
    return held;
}

void AnalogDelay::prepare (double newSampleRate)
{
    sampleRate = newSampleRate > 1000.0 ? newSampleRate : 48000.0;
    brigades[0].prepare();
    brigades[1].prepare();
    commonNoise.reset (0x14D2B91Du);
    sideNoise.reset (0x77E31A05u);

    const float sr = static_cast<float> (sampleRate);
    compAttack = 1.0f - std::exp (-1.0f / (0.006f * sr));
    compRelease = 1.0f - std::exp (-1.0f / (0.090f * sr));

    feedback.reset (sampleRate, 0.040);
    width.reset (sampleRate, 0.040);
    tone.reset (sampleRate, 0.050);
    drive.reset (sampleRate, 0.030);

    for (auto& brigade : brigades)
    {
        brigade.dc.prepare (sampleRate);
        brigade.inputCut.setCutoff (kRepeatLowCutHz, sampleRate);
    }

    snap ({});
}

void AnalogDelay::reset() noexcept
{
    for (auto& brigade : brigades)
    {
        brigade.clear();
        brigade.dc.prepare (sampleRate);
    }
    commonNoise.reset (0x14D2B91Du);
    sideNoise.reset (0x77E31A05u);
}

void AnalogDelay::arm (const AnalogSettings& settings, bool immediate) noexcept
{
    const auto set = [immediate] (juce::SmoothedValue<float>& smoother, float value)
    {
        if (immediate)
            smoother.setCurrentAndTargetValue (value);
        else
            smoother.setTargetValue (value);
    };

    set (feedback, std::clamp (settings.feedback, 0.0f, 1.0f));
    set (width, std::clamp (settings.width, 0.0f, 1.0f));
    set (tone, std::clamp (settings.tone, 0.0f, 1.0f));
    set (drive, std::clamp (settings.drive, 0.0f, 1.0f));
}

void AnalogDelay::snap (const AnalogSettings& settings) noexcept
{
    arm (settings, true);
    currentTimeMs = std::clamp (settings.timeMs, 40.0f, 4000.0f);
}

void AnalogDelay::process (const float* const* input, float* const* output, int numChannels, int numSamples, const AnalogSettings& settings)
{
    juce::ScopedNoDenormals noDenormals;
    arm (settings, false);

    const int channels = std::clamp (numChannels, 1, 2);
    const float minDelayMs = (1000.0f * static_cast<float> (kStages)) / (8.0f * static_cast<float> (sampleRate));

    for (int i = 0; i < numSamples; ++i)
    {
        const float targetMs = std::clamp (settings.timeMs, 40.0f, 4000.0f);
        const float maxStepMs = 0.34f * 1000.0f / static_cast<float> (sampleRate);
        const float timeDelta = std::clamp (targetMs - currentTimeMs, -maxStepMs, maxStepMs);
        currentTimeMs += timeDelta;
        const float time = std::max (currentTimeMs, minDelayMs);
        const float fbKnob = feedback.getNextValue();
        if (! bitsEqual (fbKnob, cachedFeedbackKnob))
        {
            cachedFeedbackKnob = fbKnob;
            cachedFeedbackGain = feedbackGain (fbKnob);
        }
        const float fb = cachedFeedbackGain;
        const float wide = width.getNextValue();
        const float toneAmt = tone.getNextValue();
        const float driveAmt = drive.getNextValue();
        if (! bitsEqual (driveAmt, cachedDrive))
        {
            cachedDrive = driveAmt;
            cachedDriveGain = std::pow (10.0f, driveAmt * 16.0f / 20.0f);
        }
        const float driveGain = cachedDriveGain;
        const float compAmount = 1.2f + driveAmt * 3.5f;

        const float common = commonNoise.nextBipolar();
        const float side = sideNoise.nextBipolar();
        const float hissL = (common) * 0.00022f;
        const float hissR = (common + side * wide) * 0.00022f;
        const float hiss[2] = { hissL, channels == 1 ? hissL : hissR };

        float wet[2] = { 0.0f, 0.0f };

        for (int ch = 0; ch < channels; ++ch)
        {
            const float delayMs = time + (ch == 1 ? wide * 7.0f : 0.0f);
            const float delaySec = std::max (delayMs, minDelayMs) * 0.001f;
            const float tickRate = static_cast<float> (kStages) / delaySec;
            const float ticks = tickRate / static_cast<float> (sampleRate);

            // Clock Nyquist falls as the delay gets longer. Tone opens or closes
            // that filter as a fraction of the clock, which is the BBD bandwidth law.
            const float nyquist = std::max (180.0f, tickRate * 0.5f);
            const float ratio = 0.16f + toneAmt * 0.46f;
            float fc = std::clamp (nyquist * ratio, 180.0f, 11000.0f);
            if (ch == 1)
                fc *= 1.0f + wide * 0.035f;

            Brigade& brigade = brigades[ch];
            brigade.antiAlias.setCutoff (fc, sampleRate);
            brigade.reconstruct.setCutoff (fc * 0.86f, sampleRate);

            const float dryIn = input[ch] != nullptr ? input[ch][i] : 0.0f;
            const float in = brigade.inputCut.process (dryIn);
            float sum = in * driveGain + brigade.feedback;

            const float rect = std::abs (sum);
            brigade.compEnv += (rect > brigade.compEnv ? compAttack : compRelease) * (rect - brigade.compEnv);
            const float compG = 1.0f / std::sqrt (1.0f + brigade.compEnv * compAmount);
            sum *= compG;

            const float clipDrive = 1.15f + driveAmt * 1.8f + fbKnob * 0.55f;
            sum = brigade.clipOs.process (sum, [&] (float sample)
            {
                return std::tanh (sample * clipDrive) / clipDrive;
            });
            sum += hiss[ch];

            const float bandlimited = brigade.antiAlias.process (sum);
            const float shifted = brigade.clock (bandlimited, ticks);
            float rec = brigade.reconstruct.process (shifted);

            const float outRect = std::abs (rec);
            brigade.expEnv += (outRect > brigade.expEnv ? compAttack : compRelease) * (outRect - brigade.expEnv);
            rec *= std::sqrt (1.0f + brigade.expEnv * compAmount);

            rec = brigade.dc.process (rec);
            // A little hiss after the reconstruction filter, so long dark delays
            // don't erase the noise floor the way the anti-alias filter does.
            rec += hiss[ch] * 2.5f;
            rec = softLimit (rec);
            brigade.feedback = rec * fb;
            wet[ch] = rec;
        }

        for (int ch = 0; ch < channels; ++ch)
            output[ch][i] = wet[ch];
    }
}

} // namespace guitar_delay
