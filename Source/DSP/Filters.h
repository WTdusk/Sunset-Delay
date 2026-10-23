#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace guitar_delay
{

inline constexpr float kPi = 3.14159265358979323846f;
inline constexpr float kTwoPi = 6.28318530717958647692f;

// Gentle highpass on the signal entering a delay loop. The dry guitar
// stays full-range; the repeats shed low end so the pick and the amp's
// low strings don't pile up.
inline constexpr float kRepeatLowCutHz = 120.0f;

inline float lerp (float a, float b, float t) noexcept
{
    return a + (b - a) * t;
}

inline float flushDenormal (float x) noexcept
{
    return std::abs (x) < 1.0e-15f ? 0.0f : x;
}

// One-pole lowpass. Cheap tape-loss and gentle tone control.
struct OnePoleLowpass
{
    float coeff = 1.0f;
    float state = 0.0f;

    void setCutoff (float hz, double sampleRate) noexcept
    {
        const float sr = static_cast<float> (sampleRate);
        const float fc = std::clamp (hz, 10.0f, sr * 0.49f);
        coeff = 1.0f - std::exp (-kTwoPi * fc / sr);
    }

    float process (float x) noexcept
    {
        state += coeff * (x - state);
        state = flushDenormal (state);
        return state;
    }

    void reset() noexcept { state = 0.0f; }
};

struct OnePoleHighpass
{
    OnePoleLowpass lowpass;

    void setCutoff (float hz, double sampleRate) noexcept { lowpass.setCutoff (hz, sampleRate); }

    float process (float x) noexcept { return x - lowpass.process (x); }

    void reset() noexcept { lowpass.reset(); }
};

struct DcBlocker
{
    float x1 = 0.0f;
    float y1 = 0.0f;
    float r = 0.995f;

    void prepare (double sampleRate) noexcept
    {
        // ~30 Hz at 48 kHz, scaled so the corner stays put at other rates.
        const float sr = static_cast<float> (sampleRate);
        r = std::exp (-kTwoPi * 30.0f / sr);
    }

    float process (float x) noexcept
    {
        const float y = x - x1 + r * y1;
        x1 = x;
        y1 = flushDenormal (y);
        return y1;
    }

    void reset() noexcept
    {
        x1 = 0.0f;
        y1 = 0.0f;
    }
};

// RBJ biquad, transposed direct form II.
struct Biquad
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;

    void setLowpass (float hz, float q, double sampleRate) noexcept
    {
        const float sr = static_cast<float> (sampleRate);
        const float fc = std::clamp (hz, 20.0f, sr * 0.45f);
        const float w0 = kTwoPi * fc / sr;
        const float cosw = std::cos (w0);
        const float sinw = std::sin (w0);
        const float alpha = sinw / (2.0f * std::max (q, 0.05f));
        const float a0 = 1.0f + alpha;
        const float inv = 1.0f / a0;
        b0 = ((1.0f - cosw) * 0.5f) * inv;
        b1 = (1.0f - cosw) * inv;
        b2 = b0;
        a1 = (-2.0f * cosw) * inv;
        a2 = (1.0f - alpha) * inv;
    }

    float process (float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        z1 = flushDenormal (z1);
        z2 = flushDenormal (z2);
        return y;
    }

    void reset() noexcept
    {
        z1 = 0.0f;
        z2 = 0.0f;
    }
};

// 4th-order Butterworth via two Q-staggered biquads.
// Used as the anti-alias and reconstruction pair around the BBD.
struct Lowpass4
{
    Biquad stage1;
    Biquad stage2;

    void setCutoff (float hz, double sampleRate) noexcept
    {
        stage1.setLowpass (hz, 0.54119610f, sampleRate);
        stage2.setLowpass (hz, 1.30656296f, sampleRate);
    }

    float process (float x) noexcept { return stage2.process (stage1.process (x)); }

    void reset() noexcept
    {
        stage1.reset();
        stage2.reset();
    }
};

// 8x oversampler for memoryless nonlinearities (tape curve, BBD clip).
// Three cascaded 2x halfband stages. Each lowpass is a 95-tap Kaiser
// windowed sinc with cutoff 0.22 cycles at that stage's rate, so images
// fall before the next decimator while guitar harmonics pass.
// Group delay is about 82 samples at the base rate (~1.7 ms at 48 kHz),
// which only shifts the wet path.
inline constexpr int kHalfbandTaps = 95;
inline constexpr float kHalfband[kHalfbandTaps] = {
    2.0308331073e-06f, 3.9771000862e-06f, -6.5751265785e-06f, -1.7377142452e-05f, 7.6290667063e-06f,
    4.6478609360e-05f, 8.5259025611e-06f, -9.1655105986e-05f, -6.4141111436e-05f, 1.3875690992e-04f,
    1.8429457650e-04f, -1.5063117086e-04f, -3.8331327052e-04f, 6.4301987633e-05f, 6.4447433169e-04f,
    2.0006856183e-04f, -8.9843649478e-04f, -7.1420986177e-04f, 1.0099034687e-03f, 1.5014414669e-03f,
    -7.8380069196e-04f, -2.4887250185e-03f, 0.0f, 3.4656299839e-03f, 1.5212257697e-03f,
    -4.0685612881e-03f, -3.8310374287e-03f, 3.8066731769e-03f, 6.7595745234e-03f, -2.1371883907e-03f,
    -9.8438693110e-03f, -1.4163146581e-03f, 1.2298413241e-02f, 7.1249211392e-03f, -1.3035914203e-02f,
    -1.4929237515e-02f, 1.0714037644e-02f, 2.4366274633e-02f, -3.7194342883e-03f, -3.4578740524e-02f,
    -1.0173641207e-02f, 4.4418574572e-02f, 3.5458784014e-02f, -5.2631758826e-02f, -8.7869738974e-02f,
    5.8087560775e-02f, 3.1200087212e-01f, 4.3999975439e-01f, 3.1200087212e-01f, 5.8087560775e-02f,
    -8.7869738974e-02f, -5.2631758826e-02f, 3.5458784014e-02f, 4.4418574572e-02f, -1.0173641207e-02f,
    -3.4578740524e-02f, -3.7194342883e-03f, 2.4366274633e-02f, 1.0714037644e-02f, -1.4929237515e-02f,
    -1.3035914203e-02f, 7.1249211392e-03f, 1.2298413241e-02f, -1.4163146581e-03f, -9.8438693110e-03f,
    -2.1371883907e-03f, 6.7595745234e-03f, 3.8066731769e-03f, -3.8310374287e-03f, -4.0685612881e-03f,
    1.5212257697e-03f, 3.4656299839e-03f, 0.0f, -2.4887250185e-03f, -7.8380069196e-04f,
    1.5014414669e-03f, 1.0099034687e-03f, -7.1420986177e-04f, -8.9843649478e-04f, 2.0006856183e-04f,
    6.4447433169e-04f, 6.4301987633e-05f, -3.8331327052e-04f, -1.5063117086e-04f, 1.8429457650e-04f,
    1.3875690992e-04f, -6.4141111436e-05f, -9.1655105986e-05f, 8.5259025611e-06f, 4.6478609360e-05f,
    7.6290667063e-06f, -1.7377142452e-05f, -6.5751265785e-06f, 3.9771000862e-06f, 2.0308331073e-06f
};

struct HalfbandStage
{
    float up[kHalfbandTaps] {};
    float down[kHalfbandTaps] {};
    int upHead = 0;
    int downHead = 0;

    void reset() noexcept
    {
        std::fill (up, up + kHalfbandTaps, 0.0f);
        std::fill (down, down + kHalfbandTaps, 0.0f);
        upHead = 0;
        downHead = 0;
    }

    static float push (float* hist, int& head, float x) noexcept
    {
        hist[head] = x;
        float y = 0.0f;
        for (int i = 0; i < kHalfbandTaps; ++i)
        {
            int index = head - i;
            if (index < 0)
                index += kHalfbandTaps;
            y += kHalfband[i] * hist[index];
        }
        ++head;
        if (head >= kHalfbandTaps)
            head = 0;
        return y;
    }

    // One base-rate sample in, one out. `fn` runs at 2x the stage input rate.
    template <typename Fn>
    float process (float x, Fn&& fn) noexcept
    {
        const float even = push (up, upHead, x) * 2.0f;
        const float odd = push (up, upHead, 0.0f) * 2.0f;
        const float kept = push (down, downHead, fn (even));
        (void) push (down, downHead, fn (odd));
        return flushDenormal (kept);
    }
};

struct Oversampler8x
{
    HalfbandStage outer;
    HalfbandStage mid;
    HalfbandStage inner;

    void reset() noexcept
    {
        outer.reset();
        mid.reset();
        inner.reset();
    }

    template <typename Fn>
    float process (float x, Fn&& fn) noexcept
    {
        return outer.process (x, [&] (float midSample)
        {
            return mid.process (midSample, [&] (float innerSample)
            {
                return inner.process (innerSample, fn);
            });
        });
    }
};

struct FastRng
{
    uint32_t state = 0xA341316Cu;

    float nextBipolar() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float> (state & 0x00FFFFFFu) * (1.0f / 8388608.0f) - 1.0f;
    }

    void reset (uint32_t seed) noexcept { state = seed == 0u ? 1u : seed; }
};

} // namespace guitar_delay
