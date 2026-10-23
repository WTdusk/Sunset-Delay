#include "DSP/AnalogDelay.h"
#include "DSP/DigitalDelay.h"
#include "DSP/TapeDelay.h"
#include "Tempo.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace
{

int gFailures = 0;

void expect (bool condition, const std::string& message)
{
    if (condition)
    {
        std::cout << "ok   " << message << "\n";
        return;
    }

    std::cout << "FAIL " << message << "\n";
    ++gFailures;
}

bool finiteRange (const std::vector<float>& data)
{
    for (float sample : data)
        if (! std::isfinite (sample))
            return false;
    return true;
}

float rmsRange (const std::vector<float>& data, int start, int end)
{
    const int a = std::max (0, start);
    const int b = std::min (end, static_cast<int> (data.size()));
    if (b <= a)
        return 0.0f;

    double energy = 0.0;
    for (int i = a; i < b; ++i)
        energy += static_cast<double> (data[static_cast<size_t> (i)]) * static_cast<double> (data[static_cast<size_t> (i)]);

    return static_cast<float> (std::sqrt (energy / static_cast<double> (b - a)));
}

int argMaxAbs (const std::vector<float>& data, int start, int end, float& magnitude)
{
    const int a = std::max (0, start);
    const int b = std::min (end, static_cast<int> (data.size()));
    int best = a;
    magnitude = 0.0f;
    for (int i = a; i < b; ++i)
    {
        const float value = std::abs (data[static_cast<size_t> (i)]);
        if (value > magnitude)
        {
            magnitude = value;
            best = i;
        }
    }
    return best;
}

float hfRatio (const std::vector<float>& data, int start, int end, double sampleRate, float cutoff)
{
    guitar_delay::OnePoleHighpass highpass;
    highpass.setCutoff (cutoff, sampleRate);
    double highs = 0.0;
    double total = 0.0;

    for (int i = 0; i < static_cast<int> (data.size()); ++i)
    {
        const float sample = data[static_cast<size_t> (i)];
        const float bright = highpass.process (sample);
        if (i >= start && i < end)
        {
            highs += static_cast<double> (bright) * static_cast<double> (bright);
            total += static_cast<double> (sample) * static_cast<double> (sample);
        }
    }

    if (total < 1.0e-12)
        return 0.0f;
    return static_cast<float> (highs / total);
}

std::vector<float> impulse (int length)
{
    std::vector<float> data (static_cast<size_t> (length), 0.0f);
    data[0] = 1.0f;
    return data;
}

std::vector<float> noiseBurst (int length, int burst, uint32_t seed)
{
    std::vector<float> data (static_cast<size_t> (length), 0.0f);
    guitar_delay::FastRng rng;
    rng.reset (seed);
    const int count = std::min (burst, length);
    for (int i = 0; i < count; ++i)
        data[static_cast<size_t> (i)] = rng.nextBipolar() * 0.55f;
    return data;
}

std::vector<float> sine (int length, double sampleRate, float hz, float amplitude)
{
    std::vector<float> data (static_cast<size_t> (length), 0.0f);
    for (int i = 0; i < length; ++i)
        data[static_cast<size_t> (i)] = amplitude * std::sin (guitar_delay::kTwoPi * hz * static_cast<float> (i) / static_cast<float> (sampleRate));
    return data;
}

std::vector<float> renderDigital (const std::vector<float>& input, double sampleRate, const guitar_delay::DigitalSettings& settings, int channels = 1, std::vector<float>* right = nullptr)
{
    guitar_delay::DigitalDelay delay;
    delay.prepare (sampleRate);
    delay.snap (settings);

    std::vector<float> left (input.size(), 0.0f);
    std::vector<float> rightBuffer (input.size(), 0.0f);
    const float* in[2] = { input.data(), input.data() };
    float* out[2] = { left.data(), rightBuffer.data() };
    delay.process (in, out, channels, static_cast<int> (input.size()), settings);
    if (right != nullptr)
        *right = std::move (rightBuffer);
    return left;
}

std::vector<float> renderTape (const std::vector<float>& input, double sampleRate, const guitar_delay::TapeSettings& settings)
{
    guitar_delay::TapeDelay delay;
    delay.prepare (sampleRate);
    delay.snap (settings);
    std::vector<float> output (input.size(), 0.0f);
    const float* in[1] = { input.data() };
    float* out[1] = { output.data() };
    delay.process (in, out, 1, static_cast<int> (input.size()), settings);
    return output;
}

std::vector<float> renderAnalog (const std::vector<float>& input, double sampleRate, const guitar_delay::AnalogSettings& settings, int channels = 1, std::vector<float>* right = nullptr)
{
    guitar_delay::AnalogDelay delay;
    delay.prepare (sampleRate);
    delay.snap (settings);
    std::vector<float> left (input.size(), 0.0f);
    std::vector<float> rightBuffer (input.size(), 0.0f);
    const float* in[2] = { input.data(), channels > 1 && right != nullptr ? input.data() : input.data() };
    float* out[2] = { left.data(), rightBuffer.data() };

    if (channels > 1 && right != nullptr)
    {
        std::vector<float> rightIn = input;
        const float* stereoIn[2] = { input.data(), rightIn.data() };
        delay.process (stereoIn, out, 2, static_cast<int> (input.size()), settings);
        *right = std::move (rightBuffer);
        return left;
    }

    delay.process (in, out, 1, static_cast<int> (input.size()), settings);
    return left;
}

float maxAdjacent (const std::vector<float>& data, int start, int end)
{
    const int a = std::max (1, start);
    const int b = std::min (end, static_cast<int> (data.size()));
    float peak = 0.0f;
    for (int i = a; i < b; ++i)
        peak = std::max (peak, std::abs (data[static_cast<size_t> (i)] - data[static_cast<size_t> (i - 1)]));
    return peak;
}

float goertzelPower (const std::vector<float>& data, int start, int end, double sampleRate, float freq)
{
    const int a = std::max (0, start);
    const int b = std::min (end, static_cast<int> (data.size()));
    const double coeff = 2.0 * std::cos (guitar_delay::kTwoPi * static_cast<double> (freq) / sampleRate);
    double s0 = 0.0;
    double s1 = 0.0;
    double s2 = 0.0;
    for (int i = a; i < b; ++i)
    {
        s0 = static_cast<double> (data[static_cast<size_t> (i)]) + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    return static_cast<float> (s1 * s1 + s2 * s2 - coeff * s1 * s2);
}

int estimatePeriod (const std::vector<float>& data, int start, int end, int minLag, int maxLag)
{
    int bestLag = minLag;
    double best = -1.0e30;
    const int last = std::min (end, static_cast<int> (data.size()));
    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        double acc = 0.0;
        for (int i = start; i + lag < last; i += 2)
            acc += static_cast<double> (data[static_cast<size_t> (i)]) * static_cast<double> (data[static_cast<size_t> (i + lag)]);
        if (acc > best)
        {
            best = acc;
            bestLag = lag;
        }
    }
    return bestLag;
}

float maxAbsDiff (const std::vector<float>& a, const std::vector<float>& b)
{
    const int n = std::min (static_cast<int> (a.size()), static_cast<int> (b.size()));
    float diff = 0.0f;
    for (int i = 0; i < n; ++i)
        diff = std::max (diff, std::abs (a[static_cast<size_t> (i)] - b[static_cast<size_t> (i)]));
    return diff;
}

} // namespace

int main()
{
    constexpr double sampleRate = 48000.0;
    const float dotted = guitar_delay::delayMsForDivision (120.0f, 8);
    expect (std::abs (dotted - 375.0f) < 0.01f, "dotted eighth at 120 BPM is 375 ms (" + std::to_string (dotted) + ")");

    const auto click = impulse (static_cast<int> (sampleRate));
    const int delaySamples = 12000; // 250 ms

    {
        guitar_delay::DigitalSettings settings;
        settings.timeMs = 250.0f;
        settings.feedback = 0.0f;
        settings.width = 0.0f;
        settings.hiCutHz = 18000.0f;
        settings.diffuse = 0.0f;
        const auto output = renderDigital (click, sampleRate, settings);
        float magnitude = 0.0f;
        const int peak = argMaxAbs (output, 0, static_cast<int> (output.size()), magnitude);
        const int error = std::abs (peak - delaySamples);
        std::cout << "     digital peak @" << peak << " mag " << magnitude << " err " << error << "\n";
        expect (finiteRange (output), "digital impulse is finite");
        expect (rmsRange (output, 0, delaySamples - 64) < 1.0e-4f, "digital stays silent before the repeat");
        expect (error <= 8, "digital repeat lands on the delay time");
        expect (magnitude > 0.4f, "digital repeat keeps the impulse level");
    }

    {
        guitar_delay::DigitalSettings settings;
        settings.timeMs = 250.0f;
        settings.feedback = 0.0f;
        settings.hiCutHz = 18000.0f;
        settings.diffuse = 0.0f;
        const auto dry = renderDigital (click, sampleRate, settings);
        settings.feedback = 0.7f;
        const auto wet = renderDigital (click, sampleRate, settings);
        const int second = delaySamples * 2;
        const float first = rmsRange (wet, delaySamples - 20, delaySamples + 40);
        const float again = rmsRange (wet, second - 20, second + 40);
        const float leaked = rmsRange (dry, second - 20, second + 80);
        std::cout << "     digital fb first " << first << " second " << again << " leaked " << leaked << "\n";
        expect (again > first * 0.25f, "digital feedback produces a second repeat");
        expect (leaked < first * 0.05f, "digital feedback at zero does not print a second repeat");
    }

    {
        std::vector<float> zeros (static_cast<size_t> (sampleRate / 2), 0.0f);
        guitar_delay::DigitalSettings settings;
        settings.feedback = 0.5f;
        const auto output = renderDigital (zeros, sampleRate, settings);
        expect (rmsRange (output, 0, static_cast<int> (output.size())) < 1.0e-5f, "digital silence stays silent");
    }

    {
        const auto tone = sine (static_cast<int> (sampleRate * 2.0), sampleRate, 220.0f, 0.55f);
        guitar_delay::DigitalSettings settings;
        settings.timeMs = 180.0f;
        settings.feedback = 1.0f;
        settings.hiCutHz = 9000.0f;
        settings.diffuse = 0.0f;
        const auto output = renderDigital (tone, sampleRate, settings);
        float magnitude = 0.0f;
        argMaxAbs (output, 0, static_cast<int> (output.size()), magnitude);
        const int tail = static_cast<int> (output.size()) - static_cast<int> (sampleRate * 0.25);
        std::cout << "     digital stress peak " << magnitude << " tail rms " << rmsRange (output, tail, static_cast<int> (output.size())) << "\n";
        expect (finiteRange (output), "digital feedback stays finite");
        expect (magnitude < 1.8f, "digital feedback does not explode");
    }

    {
        std::vector<float> right;
        guitar_delay::DigitalSettings settings;
        settings.timeMs = 200.0f;
        settings.feedback = 0.65f;
        settings.width = 1.0f;
        settings.hiCutHz = 18000.0f;
        settings.diffuse = 0.0f;

        guitar_delay::DigitalDelay delay;
        delay.prepare (sampleRate);
        delay.snap (settings);
        const int length = static_cast<int> (sampleRate);
        std::vector<float> leftIn (static_cast<size_t> (length), 0.0f);
        std::vector<float> rightIn (static_cast<size_t> (length), 0.0f);
        leftIn[0] = 1.0f;
        std::vector<float> left (static_cast<size_t> (length), 0.0f);
        right.assign (static_cast<size_t> (length), 0.0f);
        const float* in[2] = { leftIn.data(), rightIn.data() };
        float* out[2] = { left.data(), right.data() };
        delay.process (in, out, 2, length, settings);

        const int first = static_cast<int> (0.200 * sampleRate);
        const int second = static_cast<int> (0.400 * sampleRate);
        const float leftFirst = rmsRange (left, first - 30, first + 50);
        const float rightFirst = rmsRange (right, first - 30, first + 50);
        const float leftSecond = rmsRange (left, second - 30, second + 50);
        const float rightSecond = rmsRange (right, second - 30, second + 50);
        std::cout << "     pingpong L1 " << leftFirst << " R1 " << rightFirst
                  << " L2 " << leftSecond << " R2 " << rightSecond << "\n";
        expect (leftFirst > rightFirst * 4.0f, "digital width sends the first repeat to the input side");
        expect (rightSecond > leftSecond * 4.0f, "digital width ping-pongs the next repeat");
    }

    {
        guitar_delay::TapeSettings settings;
        settings.timeMs = 250.0f;
        settings.feedback = 0.0f;
        settings.width = 0.0f;
        settings.wow = 0.0f;
        settings.flutter = 0.0f;
        settings.age = 0.15f;
        const auto output = renderTape (click, sampleRate, settings);
        float magnitude = 0.0f;
        const int peak = argMaxAbs (output, 0, static_cast<int> (output.size()), magnitude);
        const int error = std::abs (peak - delaySamples);
        std::cout << "     tape peak @" << peak << " mag " << magnitude << " err " << error << "\n";
        expect (finiteRange (output), "tape impulse is finite");
        expect (rmsRange (output, 0, delaySamples - 400) < 0.01f, "tape stays quiet before the head");
        expect (error <= 500, "tape repeat lands near the delay time");
        expect (magnitude > 0.08f, "tape repeat is audible");
    }

    {
        guitar_delay::TapeSettings steady;
        steady.timeMs = 250.0f;
        steady.feedback = 0.45f;
        steady.wow = 0.0f;
        steady.flutter = 0.0f;
        steady.age = 0.3f;
        guitar_delay::TapeSettings wobbly = steady;
        wobbly.wow = 1.0f;
        wobbly.flutter = 0.8f;
        const auto plain = renderTape (click, sampleRate, steady);
        const auto worn = renderTape (click, sampleRate, wobbly);
        const float diff = maxAbsDiff (plain, worn);
        std::cout << "     tape wow delta " << diff << "\n";
        expect (diff > 0.02f, "tape wow and flutter move the repeat");
    }

    {
        const auto tone = sine (static_cast<int> (sampleRate * 2.0), sampleRate, 196.0f, 0.6f);
        guitar_delay::TapeSettings settings;
        settings.timeMs = 220.0f;
        settings.feedback = 1.0f;
        settings.wow = 0.4f;
        settings.flutter = 0.3f;
        settings.age = 0.7f;
        const auto output = renderTape (tone, sampleRate, settings);
        float magnitude = 0.0f;
        argMaxAbs (output, 0, static_cast<int> (output.size()), magnitude);
        std::cout << "     tape stress peak " << magnitude << "\n";
        expect (finiteRange (output), "tape feedback stays finite");
        expect (magnitude < 2.0f, "tape feedback does not explode");
    }

    {
        guitar_delay::AnalogSettings settings;
        settings.timeMs = 250.0f;
        settings.feedback = 0.0f;
        settings.width = 0.0f;
        settings.tone = 0.7f;
        settings.drive = 0.15f;
        const auto output = renderAnalog (click, sampleRate, settings);
        const int earlyEnd = delaySamples - static_cast<int> (0.020 * sampleRate);
        const float early = rmsRange (output, 0, earlyEnd);
        const float arrival = rmsRange (output, delaySamples - 200, delaySamples + static_cast<int> (0.030 * sampleRate));
        std::cout << "     analog early " << early << " arrival " << arrival << "\n";
        expect (finiteRange (output), "analog impulse is finite");
        expect (early < 0.008f, "analog does not pass the dry guitar");
        expect (arrival > early * 4.0f && arrival > 0.004f, "analog repeat arrives at the clock delay");
    }

    {
        const auto burst = noiseBurst (static_cast<int> (sampleRate), static_cast<int> (0.030 * sampleRate), 0x51u);
        guitar_delay::AnalogSettings bright;
        bright.timeMs = 80.0f;
        bright.feedback = 0.0f;
        bright.tone = 0.75f;
        bright.drive = 0.1f;
        guitar_delay::AnalogSettings dark = bright;
        dark.timeMs = 700.0f;
        const auto shortDelay = renderAnalog (burst, sampleRate, bright);
        const auto longDelay = renderAnalog (burst, sampleRate, dark);
        const int shortAt = static_cast<int> (0.080 * sampleRate);
        const int longAt = static_cast<int> (0.700 * sampleRate);
        const int window = static_cast<int> (0.050 * sampleRate);
        const float shortHf = hfRatio (shortDelay, shortAt, shortAt + window, sampleRate, 2500.0f);
        const float longHf = hfRatio (longDelay, longAt, longAt + window, sampleRate, 2500.0f);
        std::cout << "     analog HF short " << shortHf << " long " << longHf << "\n";
        expect (shortHf > longHf * 1.8f, "analog short delays stay brighter than long BBD delays");
    }

    {
        std::vector<float> zeros (static_cast<size_t> (sampleRate / 2), 0.0f);
        guitar_delay::AnalogSettings settings;
        settings.feedback = 0.6f;
        settings.drive = 0.25f;
        const auto output = renderAnalog (zeros, sampleRate, settings);
        const float level = rmsRange (output, 0, static_cast<int> (output.size()));
        std::cout << "     analog hiss rms " << level << "\n";
        expect (finiteRange (output), "analog hiss is finite");
        expect (level < 0.02f, "analog hiss stays a hint, not a noise source");
    }

    {
        const auto tone = sine (static_cast<int> (sampleRate * 2.0), sampleRate, 146.0f, 0.55f);
        guitar_delay::AnalogSettings settings;
        settings.timeMs = 280.0f;
        settings.feedback = 1.0f;
        settings.tone = 0.45f;
        settings.drive = 0.55f;
        const auto output = renderAnalog (tone, sampleRate, settings);
        float magnitude = 0.0f;
        argMaxAbs (output, 0, static_cast<int> (output.size()), magnitude);
        std::cout << "     analog stress peak " << magnitude << "\n";
        expect (finiteRange (output), "analog feedback stays finite");
        expect (magnitude < 2.0f, "analog feedback does not explode");
    }

    {
        const auto burst = noiseBurst (static_cast<int> (sampleRate), static_cast<int> (0.025 * sampleRate), 99u);
        guitar_delay::TapeSettings tapeSettings;
        tapeSettings.timeMs = 160.0f;
        tapeSettings.feedback = 0.8f;
        tapeSettings.wow = 0.0f;
        tapeSettings.flutter = 0.0f;
        tapeSettings.age = 1.0f;
        guitar_delay::DigitalSettings digitalSettings;
        digitalSettings.timeMs = 160.0f;
        digitalSettings.feedback = 0.8f;
        digitalSettings.hiCutHz = 15000.0f;
        digitalSettings.diffuse = 0.0f;
        const auto tape = renderTape (burst, sampleRate, tapeSettings);
        const auto digital = renderDigital (burst, sampleRate, digitalSettings);
        const int start = static_cast<int> (0.250 * sampleRate);
        const int end = static_cast<int> (0.900 * sampleRate);
        const float tapeHf = hfRatio (tape, start, end, sampleRate, 3500.0f);
        const float digitalHf = hfRatio (digital, start, end, sampleRate, 3500.0f);
        const float difference = maxAbsDiff (tape, digital);
        std::cout << "     HF tape " << tapeHf << " digital " << digitalHf << " delta " << difference << "\n";
        expect (difference > 0.05f, "tape and digital are not the same delay");
        expect (tapeHf < digitalHf * 0.55f, "worn tape loses highs that the digital path keeps");
    }

    {
        const auto burst = noiseBurst (static_cast<int> (sampleRate / 2), static_cast<int> (0.040 * sampleRate), 7u);
        guitar_delay::AnalogSettings analogSettings;
        analogSettings.timeMs = 180.0f;
        analogSettings.feedback = 0.4f;
        analogSettings.tone = 0.5f;
        const auto analog = renderAnalog (burst, sampleRate, analogSettings);
        guitar_delay::DigitalSettings digitalSettings;
        digitalSettings.timeMs = 180.0f;
        digitalSettings.feedback = 0.4f;
        digitalSettings.hiCutHz = 14000.0f;
        const auto digital = renderDigital (burst, sampleRate, digitalSettings);
        expect (maxAbsDiff (analog, digital) > 0.02f, "analog and digital renders differ");
    }

    {
        const auto note = sine (static_cast<int> (sampleRate * 0.8), sampleRate, 220.0f, 0.40f);
        const int at = static_cast<int> (0.300 * sampleRate);
        const int span = static_cast<int> (0.080 * sampleRate);
        guitar_delay::TapeSettings tapeSettings;
        tapeSettings.timeMs = 300.0f;
        tapeSettings.feedback = 0.0f;
        tapeSettings.wow = 0.0f;
        tapeSettings.flutter = 0.0f;
        tapeSettings.age = 0.35f;
        guitar_delay::AnalogSettings analogSettings;
        analogSettings.timeMs = 300.0f;
        analogSettings.feedback = 0.0f;
        analogSettings.tone = 0.55f;
        analogSettings.drive = 0.2f;
        guitar_delay::DigitalSettings digitalSettings;
        digitalSettings.timeMs = 300.0f;
        digitalSettings.feedback = 0.0f;
        digitalSettings.hiCutHz = 7000.0f;
        digitalSettings.diffuse = 0.0f;
        const float tapeLevel = rmsRange (renderTape (note, sampleRate, tapeSettings), at, at + span);
        const float analogLevel = rmsRange (renderAnalog (note, sampleRate, analogSettings), at, at + span);
        const float digitalLevel = rmsRange (renderDigital (note, sampleRate, digitalSettings), at, at + span);
        std::cout << "     note rms tape " << tapeLevel << " analog " << analogLevel << " digital " << digitalLevel << "\n";
        expect (tapeLevel > 0.08f && analogLevel > 0.08f && digitalLevel > 0.08f, "a guitar note repeats at a usable level in every mode");
    }

    {
        guitar_delay::Oversampler8x oversampler;
        const int length = 4000;
        std::vector<float> input (static_cast<size_t> (length), 0.0f);
        std::vector<float> output (static_cast<size_t> (length), 0.0f);
        for (int i = 0; i < length; ++i)
            input[static_cast<size_t> (i)] = 0.50f * std::sin (guitar_delay::kTwoPi * 1000.0f * static_cast<float> (i) / static_cast<float> (sampleRate));
        for (int i = 0; i < length; ++i)
            output[static_cast<size_t> (i)] = oversampler.process (input[static_cast<size_t> (i)], [] (float sample) { return sample; });

        float bestError = 1.0e9f;
        int bestLag = 0;
        for (int lag = 60; lag <= 110; ++lag)
        {
            double energy = 0.0;
            int count = 0;
            for (int i = 1500; i < length - lag; ++i)
            {
                const float error = output[static_cast<size_t> (i + lag)] - input[static_cast<size_t> (i)];
                energy += static_cast<double> (error) * static_cast<double> (error);
                ++count;
            }
            const float rms = static_cast<float> (std::sqrt (energy / static_cast<double> (count)));
            if (rms < bestError)
            {
                bestError = rms;
                bestLag = lag;
            }
        }
        std::cout << "     oversampler lag " << bestLag << " err " << bestError << "\n";
        expect (bestError < 0.02f, "8x oversampler passes a 1 kHz sine");
    }

    {
        const int length = static_cast<int> (sampleRate);
        const float toneHz = 7350.0f;
        std::vector<float> shaped (static_cast<size_t> (length), 0.0f);
        guitar_delay::Oversampler8x oversampler;
        for (int i = 0; i < length; ++i)
        {
            const float x = 0.62f * std::sin (guitar_delay::kTwoPi * toneHz * static_cast<float> (i) / static_cast<float> (sampleRate));
            shaped[static_cast<size_t> (i)] = oversampler.process (x, [] (float sample)
            {
                const float drive = 3.4f;
                const float biased = sample + 0.10f * sample * sample;
                return std::tanh (biased * drive) / std::tanh (drive);
            });
        }
        const int a = length / 3;
        const int b = length * 2 / 3;
        const float fund = goertzelPower (shaped, a, b, sampleRate, toneHz);
        const float alias = std::max (goertzelPower (shaped, a, b, sampleRate, 1000.0f),
                                       goertzelPower (shaped, a, b, sampleRate, 451.0f));
        std::cout << "     tape-curve alias power " << alias << " fund " << fund << "\n";
        expect (fund > 0.0f && alias < fund * 1.0e-4f, "oversampled tape curve keeps 7 kHz aliases down");
    }

    {
        const auto low = sine (static_cast<int> (sampleRate), sampleRate, 60.0f, 0.50f);
        const auto mid = sine (static_cast<int> (sampleRate), sampleRate, 400.0f, 0.50f);
        guitar_delay::DigitalSettings settings;
        settings.timeMs = 200.0f;
        settings.feedback = 0.0f;
        settings.hiCutHz = 16000.0f;
        settings.diffuse = 0.0f;
        const int at = static_cast<int> (0.200 * sampleRate);
        const int span = static_cast<int> (0.080 * sampleRate);
        const float lowLevel = rmsRange (renderDigital (low, sampleRate, settings), at, at + span);
        const float midLevel = rmsRange (renderDigital (mid, sampleRate, settings), at, at + span);
        std::cout << "     low-cut 60 Hz " << lowLevel << " 400 Hz " << midLevel << "\n";
        expect (midLevel > 0.05f && lowLevel < midLevel * 0.45f, "repeats shed guitar low end");
    }

    {
        std::vector<float> right;
        guitar_delay::DigitalSettings settings;
        settings.timeMs = 180.0f;
        settings.feedback = 0.55f;
        settings.width = 1.0f;
        settings.hiCutHz = 14000.0f;
        settings.diffuse = 0.0f;
        guitar_delay::DigitalDelay delay;
        delay.prepare (sampleRate);
        delay.snap (settings);
        const int length = static_cast<int> (sampleRate * 0.7);
        std::vector<float> leftIn (static_cast<size_t> (length), 0.0f);
        std::vector<float> rightIn (static_cast<size_t> (length), 0.0f);
        leftIn[0] = 1.0f;
        rightIn[0] = 1.0f;
        std::vector<float> left (static_cast<size_t> (length), 0.0f);
        right.assign (static_cast<size_t> (length), 0.0f);
        const float* in[2] = { leftIn.data(), rightIn.data() };
        float* out[2] = { left.data(), right.data() };
        delay.process (in, out, 2, length, settings);
        const int first = static_cast<int> (0.180 * sampleRate);
        const int second = static_cast<int> (0.360 * sampleRate);
        const float leftFirst = rmsRange (left, first - 40, first + 80);
        const float rightFirst = rmsRange (right, first - 40, first + 80);
        const float leftSecond = rmsRange (left, second - 40, second + 80);
        const float rightSecond = rmsRange (right, second - 40, second + 80);
        std::cout << "     dual-mono pingpong L1 " << leftFirst << " R1 " << rightFirst
                  << " L2 " << leftSecond << " R2 " << rightSecond << "\n";
        expect (leftFirst > rightFirst * 3.0f, "a centered guitar's first digital repeat stays on the left");
        expect (rightSecond > leftSecond * 3.0f, "the next repeat ping-pongs to the right");
    }

    {
        const float toneHz = 330.0f;
        const int steady = static_cast<int> (sampleRate * 0.70);
        const int glide = static_cast<int> (sampleRate * 0.55);
        const auto tone = sine (steady + glide, sampleRate, toneHz, 0.32f);

        guitar_delay::TapeDelay tape;
        tape.prepare (sampleRate);
        guitar_delay::TapeSettings tapeSettings;
        tapeSettings.timeMs = 150.0f;
        tapeSettings.feedback = 0.0f;
        tapeSettings.wow = 0.0f;
        tapeSettings.flutter = 0.0f;
        tapeSettings.age = 0.25f;
        tape.snap (tapeSettings);
        std::vector<float> tapeOut (tone.size(), 0.0f);
        const float* inPtr = tone.data();
        float* outPtr = tapeOut.data();
        tape.process (&inPtr, &outPtr, 1, steady, tapeSettings);
        tapeSettings.timeMs = 520.0f;
        const float* glideIn = tone.data() + steady;
        float* glideOut = tapeOut.data() + steady;
        tape.process (&glideIn, &glideOut, 1, glide, tapeSettings);

        const int periodBefore = estimatePeriod (tapeOut, static_cast<int> (0.40 * sampleRate), static_cast<int> (0.65 * sampleRate), 90, 220);
        const int periodDuring = estimatePeriod (tapeOut, steady + static_cast<int> (0.08 * sampleRate), steady + static_cast<int> (0.32 * sampleRate), 90, 260);
        const float tapeClick = maxAdjacent (tapeOut, steady - 20, steady + static_cast<int> (0.40 * sampleRate));
        std::cout << "     tape period " << periodBefore << " -> " << periodDuring << " delta " << tapeClick << "\n";
        expect (periodBefore > 120 && periodBefore < 170, "tape holds the note before the glide");
        expect (periodDuring > periodBefore + 12, "tape time change glides the pitch down");
        expect (tapeClick < 0.35f, "tape time glide has no click");

        guitar_delay::DigitalDelay digital;
        digital.prepare (sampleRate);
        guitar_delay::DigitalSettings digitalSettings;
        digitalSettings.timeMs = 150.0f;
        digitalSettings.feedback = 0.0f;
        digitalSettings.hiCutHz = 14000.0f;
        digitalSettings.diffuse = 0.0f;
        digital.snap (digitalSettings);
        std::vector<float> digitalOut (tone.size(), 0.0f);
        float* digitalPtr = digitalOut.data();
        digital.process (&inPtr, &digitalPtr, 1, steady, digitalSettings);
        digitalSettings.timeMs = 520.0f;
        float* digitalGlide = digitalOut.data() + steady;
        digital.process (&glideIn, &digitalGlide, 1, glide, digitalSettings);
        const int digitalPeriod = estimatePeriod (digitalOut, steady + static_cast<int> (0.04 * sampleRate), steady + static_cast<int> (0.22 * sampleRate), 90, 220);
        const float digitalClick = maxAdjacent (digitalOut, steady - 20, steady + static_cast<int> (0.20 * sampleRate));
        std::cout << "     digital period during move " << digitalPeriod << " delta " << digitalClick << "\n";
        expect (std::abs (digitalPeriod - periodBefore) < 10, "digital time change does not pitch-shift");
        expect (digitalClick < 0.12f, "digital time crossfade has no click");
    }

    {
        constexpr float kHalfPi = 1.5707963267948966f;
        const int length = static_cast<int> (sampleRate * 0.90);
        const int switchAt = static_cast<int> (sampleRate * 0.40);
        const auto tone = sine (length, sampleRate, 220.0f, 0.40f);

        guitar_delay::TapeDelay tape;
        guitar_delay::DigitalDelay digital;
        tape.prepare (sampleRate);
        digital.prepare (sampleRate);
        guitar_delay::TapeSettings tapeSettings;
        tapeSettings.timeMs = 160.0f;
        tapeSettings.feedback = 0.55f;
        tapeSettings.wow = 0.0f;
        tapeSettings.flutter = 0.0f;
        tapeSettings.age = 0.30f;
        guitar_delay::DigitalSettings digitalSettings;
        digitalSettings.timeMs = 160.0f;
        digitalSettings.feedback = 0.45f;
        digitalSettings.hiCutHz = 9000.0f;
        digitalSettings.diffuse = 0.0f;
        tape.snap (tapeSettings);
        digital.snap (digitalSettings);
        digital.reset();
        digital.snap (digitalSettings);

        std::vector<float> mixed (static_cast<size_t> (length), 0.0f);
        std::vector<float> trail (static_cast<size_t> (length), 0.0f);
        const float fadeStep = 1.0f / static_cast<float> (0.55 * sampleRate);
        float fade = 0.0f;
        bool switched = false;

        for (int i = 0; i < length; ++i)
        {
            float sample = tone[static_cast<size_t> (i)];
            float tapeSample = 0.0f;
            float digitalSample = 0.0f;
            float* tapeOut = &tapeSample;
            float* digitalOut = &digitalSample;

            if (! switched && i >= switchAt)
            {
                switched = true;
                fade = 0.0f;
                digital.reset();
                digital.snap (digitalSettings);
            }

            if (! switched)
            {
                const float* in = &sample;
                tape.process (&in, &tapeOut, 1, 1, tapeSettings);
                mixed[static_cast<size_t> (i)] = tapeSample;
            }
            else
            {
                float silent = 0.0f;
                const float* tapeIn = &silent;
                const float* digitalIn = &sample;
                tape.process (&tapeIn, &tapeOut, 1, 1, tapeSettings);
                digital.process (&digitalIn, &digitalOut, 1, 1, digitalSettings);
                const float oldGain = std::cos (std::min (fade, 1.0f) * kHalfPi);
                const float newGain = std::sin (std::min (fade, 1.0f) * kHalfPi);
                mixed[static_cast<size_t> (i)] = tapeSample * oldGain + digitalSample * newGain;
                trail[static_cast<size_t> (i)] = tapeSample;
                fade += fadeStep;
            }
        }

        const float switchDelta = maxAdjacent (mixed, switchAt - 8, switchAt + 64);
        const float trailLevel = rmsRange (trail, switchAt + 80, switchAt + static_cast<int> (0.12 * sampleRate));
        std::cout << "     mode switch delta " << switchDelta << " tape trail " << trailLevel << "\n";
        expect (finiteRange (mixed), "mode crossfade stays finite");
        expect (switchDelta < 0.20f, "mode switch is click-free");
        expect (trailLevel > 0.02f, "the previous mode keeps ringing after the switch");
    }

    {
        guitar_delay::DigitalSettings settings;
        settings.timeMs = 200.0f;
        settings.feedback = 0.62f;
        settings.width = 0.48f;
        settings.hiCutHz = 5400.0f;
        settings.diffuse = 0.42f;
        const int length = static_cast<int> (sampleRate * 6.5);
        std::vector<float> leftIn (static_cast<size_t> (length), 0.0f);
        std::vector<float> rightIn (static_cast<size_t> (length), 0.0f);
        leftIn[0] = 1.0f;
        rightIn[0] = 1.0f;
        std::vector<float> left (static_cast<size_t> (length), 0.0f);
        std::vector<float> right (static_cast<size_t> (length), 0.0f);
        guitar_delay::DigitalDelay delay;
        delay.prepare (sampleRate);
        delay.snap (settings);
        const float* in[2] = { leftIn.data(), rightIn.data() };
        float* out[2] = { left.data(), right.data() };
        delay.process (in, out, 2, length, settings);
        std::cout << "     digital stereo decay";
        float first = 0.0f;
        float later = 0.0f;
        for (int step = 1; step <= 8; ++step)
        {
            const int at = static_cast<int> (sampleRate * 0.64 * static_cast<double> (step));
            const float level = rmsRange (left, at, at + static_cast<int> (0.12 * sampleRate));
            if (step == 1)
                first = level;
            later = level;
            std::cout << " " << level;
        }
        std::cout << "\n";
        expect (first > 0.002f && later < first * 0.5f, "digital feedback with diffusion and width still decays");
    }

    if (gFailures == 0)
    {
        std::cout << "All DSP render checks passed.\n";
        return 0;
    }

    std::cout << gFailures << " check(s) failed.\n";
    return 1;
}
