#include "DSP/DigitalDelay.h"

#include <cmath>

namespace guitar_delay
{
namespace
{

int nextPowerOfTwo (int value)
{
    int p = 1;
    while (p < value)
        p <<= 1;
    return p;
}

// 3rd-order Lagrange through samples at offsets -1, 0, +1, +2.
// t = 0 returns y1 (the integer delay). t = 1 returns y2.
float lagrange3 (float y0, float y1, float y2, float y3, float t) noexcept
{
    const float l0 = (t * (t - 1.0f) * (t - 2.0f)) / -6.0f;
    const float l1 = ((t + 1.0f) * (t - 1.0f) * (t - 2.0f)) / 2.0f;
    const float l2 = -((t + 1.0f) * t * (t - 2.0f)) / 2.0f;
    const float l3 = ((t + 1.0f) * t * (t - 1.0f)) / 6.0f;
    return y0 * l0 + y1 * l1 + y2 * l2 + y3 * l3;
}

float feedbackGain (float knob) noexcept
{
    const float shaped = std::pow (std::clamp (knob, 0.0f, 1.0f), 0.65f);
    return shaped * 0.93f;
}

float safetyLimit (float x) noexcept
{
    const float a = std::abs (x);
    if (a <= 1.0f)
        return x;
    const float sign = x < 0.0f ? -1.0f : 1.0f;
    return sign * (1.0f + 0.12f * std::tanh ((a - 1.0f) * 2.0f));
}

} // namespace

void DigitalDelay::Allpass::prepare (int length)
{
    buffer.assign (static_cast<size_t> (std::max (length, 1)), 0.0f);
    index = 0;
    gain = 0.0f;
}

void DigitalDelay::Allpass::clear() noexcept
{
    std::fill (buffer.begin(), buffer.end(), 0.0f);
    index = 0;
}

float DigitalDelay::Allpass::process (float x) noexcept
{
    if (buffer.empty() || std::abs (gain) < 1.0e-4f)
        return x;

    const float delayed = buffer[static_cast<size_t> (index)];
    const float y = -gain * x + delayed;
    buffer[static_cast<size_t> (index)] = x + gain * delayed;
    ++index;
    if (index >= static_cast<int> (buffer.size()))
        index = 0;
    return y;
}

void DigitalDelay::Line::prepare (int capacity, double sr, float lengthScale)
{
    const int size = nextPowerOfTwo (std::max (capacity, 16));
    buffer.assign (static_cast<size_t> (size), 0.0f);
    mask = size - 1;
    writeIndex = 0;

    const int bases[4] = { 97, 149, 211, 293 };
    for (int i = 0; i < 4; ++i)
    {
        const int length = std::max (1, static_cast<int> (std::lround (static_cast<double> (bases[i]) * lengthScale * sr / 48000.0)));
        diffuse[i].prepare (length);
    }
    inputCut.setCutoff (kRepeatLowCutHz, sr);
    clear();
}

void DigitalDelay::Line::clear() noexcept
{
    std::fill (buffer.begin(), buffer.end(), 0.0f);
    writeIndex = 0;
    hiCut.reset();
    dc.reset();
    inputCut.reset();
    for (auto& stage : diffuse)
        stage.clear();
}

void DigitalDelay::Line::push (float x) noexcept
{
    buffer[static_cast<size_t> (writeIndex)] = x;
    writeIndex = (writeIndex + 1) & mask;
}

float DigitalDelay::Line::read (float delaySamples) const noexcept
{
    const float size = static_cast<float> (buffer.size());
    const float d = std::clamp (delaySamples, 2.0f, size - 4.0f);
    // d is at most one buffer length, so the read cursor wraps at most once.
    float readPos = static_cast<float> (writeIndex) - d;
    if (readPos < 0.0f)
        readPos += size;

    const int i1 = static_cast<int> (readPos);
    const float t = readPos - static_cast<float> (i1);
    const float y0 = buffer[static_cast<size_t> ((i1 - 1) & mask)];
    const float y1 = buffer[static_cast<size_t> (i1 & mask)];
    const float y2 = buffer[static_cast<size_t> ((i1 + 1) & mask)];
    const float y3 = buffer[static_cast<size_t> ((i1 + 2) & mask)];
    return lagrange3 (y0, y1, y2, y3, t);
}

float DigitalDelay::Line::colour (float x, float diffusion) noexcept
{
    float y = x;
    const float g = std::clamp (diffusion, 0.0f, 1.0f) * 0.62f;
    for (auto& stage : diffuse)
    {
        stage.gain = g;
        y = stage.process (y);
    }
    return y;
}

void DigitalDelay::prepare (double newSampleRate)
{
    sampleRate = newSampleRate > 1000.0 ? newSampleRate : 48000.0;
    const int capacity = static_cast<int> (std::ceil (sampleRate * 4.0)) + 16;
    lines[0].prepare (capacity, sampleRate, 1.0f);
    lines[1].prepare (capacity, sampleRate, 1.0f);

    fadeStep = 1.0f / static_cast<float> (0.028 * sampleRate);
    feedback.reset (sampleRate, 0.025);
    width.reset (sampleRate, 0.030);
    hiCutHz.reset (sampleRate, 0.030);
    diffuse.reset (sampleRate, 0.040);

    for (auto& line : lines)
        line.dc.prepare (sampleRate);

    snap ({});
}

void DigitalDelay::reset() noexcept
{
    for (auto& line : lines)
    {
        line.clear();
        line.dc.prepare (sampleRate);
    }
    fading = false;
    fade = 1.0f;
}

void DigitalDelay::arm (const DigitalSettings& settings, bool immediate) noexcept
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
    set (hiCutHz, std::clamp (settings.hiCutHz, 800.0f, 20000.0f));
    set (diffuse, std::clamp (settings.diffuse, 0.0f, 1.0f));
}

void DigitalDelay::snap (const DigitalSettings& settings) noexcept
{
    arm (settings, true);
    activeDelay = static_cast<float> (static_cast<double> (std::clamp (settings.timeMs, 40.0f, 4000.0f)) * 0.001 * sampleRate);
    pendingDelay = activeDelay;
    fade = 1.0f;
    fading = false;
}

void DigitalDelay::process (const float* const* input, float* const* output, int numChannels, int numSamples, const DigitalSettings& settings)
{
    juce::ScopedNoDenormals noDenormals;
    arm (settings, false);

    const int channels = std::clamp (numChannels, 1, 2);
    constexpr float kHalfPi = 1.5707963267948966f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float fbKnob = feedback.getNextValue();
        if (! bitsEqual (fbKnob, cachedFeedbackKnob))
        {
            cachedFeedbackKnob = fbKnob;
            cachedFeedbackGain = feedbackGain (fbKnob);
        }
        const float fb = cachedFeedbackGain;
        const float wide = width.getNextValue();
        const float cut = hiCutHz.getNextValue();
        const float diffusion = diffuse.getNextValue();
        const float target = static_cast<float> (static_cast<double> (std::clamp (settings.timeMs, 40.0f, 4000.0f)) * 0.001 * sampleRate);

        // Moving Time crossfades two fixed reads. A moving read would pitch-shift,
        // which is the tape/BBD behaviour and the wrong one for a rhythmic digital delay.
        if (! fading)
        {
            if (std::abs (target - activeDelay) > 0.5f)
            {
                pendingDelay = target;
                fade = 0.0f;
                fading = true;
            }
            else
            {
                activeDelay = target;
            }
        }

        const float newGain = fading ? std::sin (std::min (fade, 1.0f) * kHalfPi) : 0.0f;
        const float oldGain = fading ? std::cos (std::min (fade, 1.0f) * kHalfPi) : 1.0f;

        auto take = [&] (Line& line) -> float
        {
            float y = line.read (activeDelay) * oldGain;
            if (fading)
                y += line.read (pendingDelay) * newGain;
            line.hiCut.setLowpass (cut, 0.7071f, sampleRate);
            y = line.hiCut.process (y);
            y = line.colour (y, diffusion);
            y = safetyLimit (y);
            return line.dc.process (y);
        };

        float wet[2] = { take (lines[0]), 0.0f };
        if (channels > 1)
            wet[1] = take (lines[1]);

        if (fading)
        {
            fade += fadeStep;
            if (fade >= 1.0f)
            {
                fade = 1.0f;
                fading = false;
                activeDelay = pendingDelay;
            }
        }

        if (channels == 1)
        {
            const float dryIn = input[0] != nullptr ? input[0][i] : 0.0f;
            const float in = lines[0].inputCut.process (dryIn);
            lines[0].push (in + wet[0] * fb);
            output[0][i] = wet[0];
            continue;
        }

        const float local = 1.0f - wide;
        const float cross = wide;
        const float inL = input[0] != nullptr ? input[0][i] : 0.0f;
        const float inR = input[1] != nullptr ? input[1][i] : 0.0f;
        const float mono = 0.5f * (inL + inR);
        // Width opens into ping-pong: the mono guitar feeds one line, and feedback swaps sides.
        const float feedL = lines[0].inputCut.process (inL * local + mono * wide);
        const float feedR = lines[1].inputCut.process (inR * local);
        lines[0].push (feedL + (wet[0] * local + wet[1] * cross) * fb);
        lines[1].push (feedR + (wet[1] * local + wet[0] * cross) * fb);
        output[0][i] = wet[0];
        output[1][i] = wet[1];
    }
}

} // namespace guitar_delay
