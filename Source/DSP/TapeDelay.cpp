#include "DSP/TapeDelay.h"

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

float hermite (float x0, float x1, float x2, float x3, float t) noexcept
{
    const float c0 = x1;
    const float c1 = 0.5f * (x2 - x0);
    const float c2 = x0 - 2.5f * x1 + 2.0f * x2 - 0.5f * x3;
    const float c3 = 0.5f * (x3 - x0) + 1.5f * (x1 - x2);
    return ((c3 * t + c2) * t + c1) * t + c0;
}

float feedbackGain (float knob) noexcept
{
    const float shaped = std::pow (std::clamp (knob, 0.0f, 1.0f), 0.65f);
    return shaped * 0.90f;
}

// Soft tape curve. Small-signal gain is unity, so feedback stays the gain the
// knob asked for and a dimed trail decays instead of sitting at a loud equilibrium.
// Age lowers the ceiling and adds a little even-order bias; the DC blocker
// cleans up the offset.
float tapeShape (float x, float amount) noexcept
{
    const float drive = 1.0f + amount * 2.4f;
    const float biased = x + (0.10f * amount) * (x * x);
    return std::tanh (biased * drive) / drive;
}

} // namespace

void TapeDelay::Head::prepare (int capacity, uint32_t seed)
{
    const int size = nextPowerOfTwo (std::max (capacity, 16));
    buffer.assign (static_cast<size_t> (size), 0.0f);
    mask = size - 1;
    writeIndex = 0;
    rng.reset (seed);
    clear();
}

void TapeDelay::Head::clear() noexcept
{
    std::fill (buffer.begin(), buffer.end(), 0.0f);
    writeIndex = 0;
    loss.reset();
    presence.reset();
    inputCut.reset();
    playbackOs.reset();
    recordOs.reset();
    playbackDc.reset();
    recordDc.reset();
    compressEnv = 0.0f;
    wowPhase = 0.0f;
    flutterPhase = 0.0f;
    driftPhase = 0.0f;
    flutterNoise = 0.0f;
}

void TapeDelay::Head::push (float x) noexcept
{
    buffer[static_cast<size_t> (writeIndex)] = x;
    writeIndex = (writeIndex + 1) & mask;
}

float TapeDelay::Head::read (float delaySamples) const noexcept
{
    const float size = static_cast<float> (buffer.size());
    const float limit = size - 4.0f;
    const float d = std::clamp (delaySamples, 2.0f, limit);
    // d is at most one buffer length, so the read cursor wraps at most once.
    float readPos = static_cast<float> (writeIndex) - d;
    if (readPos < 0.0f)
        readPos += size;

    const int i1 = static_cast<int> (readPos);
    const float t = readPos - static_cast<float> (i1);
    const float x0 = buffer[static_cast<size_t> ((i1 - 1) & mask)];
    const float x1 = buffer[static_cast<size_t> (i1 & mask)];
    const float x2 = buffer[static_cast<size_t> ((i1 + 1) & mask)];
    const float x3 = buffer[static_cast<size_t> ((i1 + 2) & mask)];
    return hermite (x0, x1, x2, x3, t);
}

float TapeDelay::Head::modulate (double sr, float wowAmt, float flutterAmt, float delaySamples) noexcept
{
    driftPhase += static_cast<float> (0.05 / sr);
    if (driftPhase >= 1.0f)
        driftPhase -= 1.0f;

    const float wowHz = 0.28f + 0.20f * std::sin (driftPhase * kTwoPi);
    wowPhase += static_cast<float> (static_cast<double> (wowHz) / sr);
    if (wowPhase >= 1.0f)
        wowPhase -= 1.0f;

    const float flutterHz = 6.2f + 2.4f * std::sin (driftPhase * kTwoPi * 0.41f + 1.7f);
    flutterPhase += static_cast<float> (static_cast<double> (flutterHz) / sr);
    if (flutterPhase >= 1.0f)
        flutterPhase -= 1.0f;

    flutterNoise = flutterNoise * 0.97f + rng.nextBipolar() * 0.03f;
    const float wow = std::sin (wowPhase * kTwoPi);
    const float flutter = std::sin (flutterPhase * kTwoPi) * 0.78f + flutterNoise * 0.22f;

    const float wowDepth = delaySamples * (0.0062f * wowAmt);
    const float flutterDepth = delaySamples * (0.0015f * flutterAmt)
                             + flutterAmt * static_cast<float> (sr * 0.00022);
    return wow * wowDepth + flutter * flutterDepth;
}

void TapeDelay::prepare (double newSampleRate)
{
    sampleRate = newSampleRate > 1000.0 ? newSampleRate : 48000.0;
    const int capacity = static_cast<int> (std::ceil (sampleRate * 4.0)) + 16;
    heads[0].prepare (capacity, 0x51A7C3E1u);
    heads[1].prepare (capacity, 0x9C33B017u);
    heads[1].wowPhase = 0.37f;
    heads[1].flutterPhase = 0.18f;

    attack = 1.0f - std::exp (-1.0f / (0.004f * static_cast<float> (sampleRate)));
    release = 1.0f - std::exp (-1.0f / (0.140f * static_cast<float> (sampleRate)));

    feedback.reset (sampleRate, 0.030);
    width.reset (sampleRate, 0.040);
    wow.reset (sampleRate, 0.050);
    flutter.reset (sampleRate, 0.050);
    age.reset (sampleRate, 0.050);

    for (auto& head : heads)
    {
        head.playbackDc.prepare (sampleRate);
        head.recordDc.prepare (sampleRate);
        head.presence.setCutoff (2400.0f, sampleRate);
        head.inputCut.setCutoff (kRepeatLowCutHz, sampleRate);
    }

    snap ({});
}

void TapeDelay::reset() noexcept
{
    for (auto& head : heads)
    {
        const float savedWow = head.wowPhase;
        const float savedFlutter = head.flutterPhase;
        const uint32_t seed = head.rng.state;
        head.clear();
        head.playbackDc.prepare (sampleRate);
        head.recordDc.prepare (sampleRate);
        head.wowPhase = savedWow;
        head.flutterPhase = savedFlutter;
        head.rng.reset (seed);
    }
}

void TapeDelay::arm (const TapeSettings& settings, bool immediate) noexcept
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
    set (wow, std::clamp (settings.wow, 0.0f, 1.0f));
    set (flutter, std::clamp (settings.flutter, 0.0f, 1.0f));
    set (age, std::clamp (settings.age, 0.0f, 1.0f));
}

void TapeDelay::snap (const TapeSettings& settings) noexcept
{
    arm (settings, true);
    currentTimeMs = std::clamp (settings.timeMs, 40.0f, 4000.0f);
}

void TapeDelay::process (const float* const* input, float* const* output, int numChannels, int numSamples, const TapeSettings& settings)
{
    juce::ScopedNoDenormals noDenormals;
    arm (settings, false);

    const int channels = std::clamp (numChannels, 1, 2);
    const float sr = static_cast<float> (sampleRate);

    for (int i = 0; i < numSamples; ++i)
    {
        // Cap the head speed so a big Time move glides about a fifth, instead of
        // stopping the tape. Small moves still settle quickly.
        const float targetMs = std::clamp (settings.timeMs, 40.0f, 4000.0f);
        const float maxStepMs = 0.30f * 1000.0f / sr;
        const float timeDelta = std::clamp (targetMs - currentTimeMs, -maxStepMs, maxStepMs);
        currentTimeMs += timeDelta;
        const float time = currentTimeMs;
        const float fbKnob = feedback.getNextValue();
        if (! bitsEqual (fbKnob, cachedFeedbackKnob))
        {
            cachedFeedbackKnob = fbKnob;
            cachedFeedbackGain = feedbackGain (fbKnob);
        }
        const float fb = cachedFeedbackGain;
        const float wide = width.getNextValue();
        const float wowAmt = wow.getNextValue();
        const float flutterAmt = flutter.getNextValue();
        const float ageAmt = age.getNextValue();

        const float baseDelay = static_cast<float> (static_cast<double> (time) * 0.001 * sampleRate);
        const float mod0 = heads[0].modulate (sampleRate, wowAmt, flutterAmt, baseDelay);
        const float mod1 = heads[1].modulate (sampleRate, wowAmt, flutterAmt, baseDelay);
        const float spread = wide * 0.0065f * sr;

        const float delayForChannel[2] = {
            baseDelay + mod0,
            baseDelay + spread + mod0 + (mod1 - mod0) * wide
        };

        const float baseFc = lerp (8600.0f, 1500.0f, ageAmt);
        float playback[2] = { 0.0f, 0.0f };

        for (int ch = 0; ch < channels; ++ch)
        {
            Head& head = heads[ch];
            const float raw = head.read (delayForChannel[ch]);
            const float envFollow = std::abs (raw);
            head.compressEnv += (envFollow > head.compressEnv ? attack : release) * (envFollow - head.compressEnv);

            const float dull = 1.0f + std::min (head.compressEnv, 1.2f) * ageAmt * 1.4f;
            head.loss.setCutoff (std::max (850.0f, baseFc / dull), sampleRate);
            float y = head.loss.process (raw);
            const float playbackDrive = 0.12f + ageAmt * 0.88f;
            y = head.playbackOs.process (y, [&] (float sample)
            {
                return tapeShape (sample, playbackDrive);
            });

            const float compAmount = 0.35f + ageAmt * 1.8f;
            y *= 1.0f / (1.0f + head.compressEnv * compAmount);
            y = head.playbackDc.process (y);
            playback[ch] = y;
        }

        if (channels == 1)
            playback[1] = playback[0];

        for (int ch = 0; ch < channels; ++ch)
        {
            const float dryIn = input[ch] != nullptr ? input[ch][i] : 0.0f;
            const float dry = heads[ch].inputCut.process (dryIn);
            const float low = heads[ch].presence.process (dry);
            const float present = dry + (dry - low) * 0.14f;
            const float recordDrive = 0.08f + ageAmt * 0.25f;
            const float recorded = heads[ch].recordOs.process (present + playback[ch] * fb, [&] (float sample)
            {
                return tapeShape (sample, recordDrive);
            });
            heads[ch].push (heads[ch].recordDc.process (recorded));
            output[ch][i] = playback[ch];
        }
    }
}

} // namespace guitar_delay
