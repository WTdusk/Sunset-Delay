#include "Parameters.h"
#include "PluginProcessor.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <cmath>
#include <iostream>
#include <vector>

namespace
{

constexpr double kSampleRate = 48000.0;

void setReal (juce::AudioProcessorValueTreeState& state, const char* paramId, float realValue)
{
    if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (state.getParameter (paramId)))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (realValue));
}

struct Voice
{
    std::vector<float> buffer;
    std::vector<float> comb;
    int index = 0;
    int combIndex = 0;
    int age = 0;
    int life = 1;
    float previous = 0.0f;
    float damp = 0.5f;
    float loss = 0.999f;
    bool alive = false;

    void pluck (float frequency, float velocity, float seconds, float brightness, uint32_t seed)
    {
        const int length = std::max (8, static_cast<int> (std::lround (kSampleRate / static_cast<double> (frequency))));
        buffer.assign (static_cast<size_t> (length), 0.0f);
        guitar_delay::FastRng rng;
        rng.reset (seed == 0u ? 1u : seed);
        float lowpass = 0.0f;
        for (int i = 0; i < length; ++i)
        {
            const float noise = rng.nextBipolar();
            lowpass = lowpass * 0.55f + noise * 0.45f;
            const float window = std::sin (guitar_delay::kPi * (static_cast<float> (i) + 0.5f) / static_cast<float> (length));
            buffer[static_cast<size_t> (i)] = lowpass * window * velocity;
        }
        index = 0;
        previous = 0.0f;
        age = 0;
        life = std::max (1, static_cast<int> (std::lround (static_cast<double> (seconds) * kSampleRate)));
        damp = std::clamp (brightness, 0.05f, 0.95f);
        // The string itself thins out. An envelope in tick() finishes the note
        // so the phrase actually goes quiet and the delay trail can be heard.
        loss = std::exp (std::log (0.45f) / static_cast<float> (life));
        const int combDelay = std::max (1, length / 8);
        comb.assign (static_cast<size_t> (combDelay), 0.0f);
        combIndex = 0;
        alive = true;
    }

    float tick() noexcept
    {
        if (! alive || buffer.empty())
            return 0.0f;

        const float current = buffer[static_cast<size_t> (index)];
        const float averaged = (current + previous) * 0.5f;
        const float filtered = current + (averaged - current) * (1.0f - damp);
        previous = filtered;
        buffer[static_cast<size_t> (index)] = filtered * loss;
        ++index;
        if (index >= static_cast<int> (buffer.size()))
            index = 0;

        const float delayed = comb[static_cast<size_t> (combIndex)];
        comb[static_cast<size_t> (combIndex)] = filtered;
        ++combIndex;
        if (combIndex >= static_cast<int> (comb.size()))
            combIndex = 0;

        const float env = std::exp (-4.2f * static_cast<float> (age) / static_cast<float> (life));
        ++age;
        if (age >= life)
            alive = false;
        return (filtered - 0.55f * delayed) * env;
    }
};

struct Event
{
    int sample = 0;
    float frequency = 110.0f;
    float velocity = 0.6f;
    float seconds = 0.8f;
    float brightness = 0.42f;
    uint32_t seed = 1u;
};

void addNote (std::vector<Event>& events, float timeSeconds, float frequency, float velocity, float seconds, float brightness, uint32_t seed)
{
    events.push_back ({ static_cast<int> (std::lround (timeSeconds * kSampleRate)), frequency, velocity, seconds, brightness, seed });
}

void addStrum (std::vector<Event>& events, float timeSeconds, const float* frequencies, int count, float velocity, float seconds, uint32_t seed)
{
    for (int i = 0; i < count; ++i)
    {
        const float detune = 1.0f + (static_cast<float> (i) - 1.5f) * 0.0015f;
        addNote (events,
                 timeSeconds + static_cast<float> (i) * 0.011f,
                 frequencies[i] * detune,
                 velocity * (1.0f - static_cast<float> (i) * 0.08f),
                 seconds,
                 0.40f + static_cast<float> (i) * 0.03f,
                 seed + static_cast<uint32_t> (i) * 17u);
    }
}

std::vector<float> makeGuitarPhrase (int length)
{
    std::vector<Event> events;
    addNote (events, 0.15f, 110.00f, 0.72f, 0.85f, 0.46f, 11u);
    addNote (events, 0.72f, 130.81f, 0.62f, 0.70f, 0.50f, 23u);
    addNote (events, 1.25f, 146.83f, 0.66f, 0.75f, 0.48f, 31u);
    addNote (events, 1.78f, 164.81f, 0.60f, 0.70f, 0.52f, 47u);
    addNote (events, 2.32f, 196.00f, 0.64f, 0.80f, 0.44f, 59u);

    const float aMajor[] = { 110.00f, 164.81f, 220.00f, 277.18f };
    const float dMajor[] = { 146.83f, 220.00f, 293.66f, 369.99f };
    const float eMinor[] = { 82.41f, 123.47f, 164.81f, 196.00f };
    addStrum (events, 3.05f, aMajor, 4, 0.70f, 1.35f, 101u);
    addStrum (events, 4.40f, dMajor, 4, 0.68f, 1.40f, 202u);
    addStrum (events, 5.75f, eMinor, 4, 0.74f, 1.55f, 303u);
    addNote (events, 7.20f, 110.00f, 0.78f, 1.80f, 0.40f, 404u);

    std::vector<Voice> voices (12);
    int nextVoice = 0;
    std::vector<float> dry (static_cast<size_t> (length), 0.0f);
    guitar_delay::Biquad pickup;
    {
        const float hz = 2600.0f;
        const float q = 0.85f;
        const float gainDb = 3.2f;
        const float a = std::pow (10.0f, gainDb / 40.0f);
        const float w0 = guitar_delay::kTwoPi * hz / static_cast<float> (kSampleRate);
        const float alpha = std::sin (w0) / (2.0f * q);
        const float cosw = std::cos (w0);
        const float b0 = 1.0f + alpha * a;
        const float b1 = -2.0f * cosw;
        const float b2 = 1.0f - alpha * a;
        const float a0 = 1.0f + alpha / a;
        const float inv = 1.0f / a0;
        pickup.b0 = b0 * inv;
        pickup.b1 = b1 * inv;
        pickup.b2 = b2 * inv;
        pickup.a1 = (-2.0f * cosw) * inv;
        pickup.a2 = (1.0f - alpha / a) * inv;
    }
    guitar_delay::OnePoleHighpass rumble;
    rumble.setCutoff (70.0f, kSampleRate);

    size_t eventIndex = 0;
    float peak = 0.0f;
    for (int i = 0; i < length; ++i)
    {
        while (eventIndex < events.size() && events[eventIndex].sample <= i)
        {
            Voice& voice = voices[static_cast<size_t> (nextVoice % static_cast<int> (voices.size()))];
            ++nextVoice;
            const Event& event = events[eventIndex];
            voice.pluck (event.frequency, event.velocity, event.seconds, event.brightness, event.seed);
            ++eventIndex;
        }

        float sum = 0.0f;
        for (auto& voice : voices)
            sum += voice.tick();
        if (eventIndex > 0)
        {
            const int newest = events[eventIndex - 1].sample;
            const int age = i - newest;
            if (age >= 0 && age < 180)
                sum += (static_cast<float> (180 - age) / 180.0f) * 0.08f * std::sin (age * 0.7f);
        }
        const float sample = rumble.process (pickup.process (sum * 0.22f));
        dry[static_cast<size_t> (i)] = sample;
        peak = std::max (peak, std::abs (sample));
    }

    if (peak > 1.0e-6f)
    {
        const float gain = 0.45f / peak;
        for (float& sample : dry)
            sample *= gain;
    }
    return dry;
}

struct Demo
{
    const char* name;
    int mode;
    float timeMs;
    float feedback;
    float mix;
    float width;
    float wow;
    float flutter;
    float age;
    float tone;
    float drive;
    float hiCutHz;
    float diffuse;
    int changeAt = -1;
    int changeMode = -1;
    float changeTime = -1.0f;
};

void applyPatch (guitar_delay::GuitarDelayProcessor& processor, const Demo& demo)
{
    auto& state = processor.apvts;
    setReal (state, guitar_delay::id::mode, static_cast<float> (demo.mode));
    setReal (state, guitar_delay::id::time, demo.timeMs);
    setReal (state, guitar_delay::id::feedback, demo.feedback);
    setReal (state, guitar_delay::id::mix, demo.mix);
    setReal (state, guitar_delay::id::width, demo.width);
    setReal (state, guitar_delay::id::sync, 0.0f);
    setReal (state, guitar_delay::id::division, 8.0f);
    setReal (state, guitar_delay::id::bypass, 0.0f);
    setReal (state, guitar_delay::id::tapeWow, demo.wow);
    setReal (state, guitar_delay::id::tapeFlutter, demo.flutter);
    setReal (state, guitar_delay::id::tapeAge, demo.age);
    setReal (state, guitar_delay::id::analogTone, demo.tone);
    setReal (state, guitar_delay::id::analogDrive, demo.drive);
    setReal (state, guitar_delay::id::digitalHiCut, demo.hiCutHz);
    setReal (state, guitar_delay::id::digitalDiffuse, demo.diffuse);
}

juce::AudioBuffer<float> renderDemo (const std::vector<float>& phrase, const Demo& demo)
{
    const int length = static_cast<int> (phrase.size());
    guitar_delay::GuitarDelayProcessor processor;
    applyPatch (processor, demo);
    processor.prepareToPlay (kSampleRate, 256);

    juce::AudioBuffer<float> rendered (2, length);
    rendered.clear();
    bool changed = demo.changeAt < 0;
    int offset = 0;
    while (offset < length)
    {
        if (! changed && offset >= demo.changeAt)
        {
            if (demo.changeMode >= 0)
                setReal (processor.apvts, guitar_delay::id::mode, static_cast<float> (demo.changeMode));
            if (demo.changeTime > 0.0f)
                setReal (processor.apvts, guitar_delay::id::time, demo.changeTime);
            changed = true;
        }

        const int count = std::min (256, length - offset);
        juce::AudioBuffer<float> block (2, count);
        for (int i = 0; i < count; ++i)
        {
            const float sample = phrase[static_cast<size_t> (offset + i)];
            block.setSample (0, i, sample);
            block.setSample (1, i, sample);
        }
        juce::MidiBuffer midi;
        processor.processBlock (block, midi);
        for (int channel = 0; channel < 2; ++channel)
            rendered.copyFrom (channel, offset, block, channel, 0, count);
        offset += count;
    }
    return rendered;
}

bool writeWav (const juce::File& file, const juce::AudioBuffer<float>& buffer)
{
    file.deleteFile();
    auto stream = std::unique_ptr<juce::OutputStream> (file.createOutputStream());
    if (stream == nullptr)
        return false;

    juce::WavAudioFormat format;
    auto writer = format.createWriterFor (stream,
                                           juce::AudioFormatWriterOptions{}
                                               .withSampleRate (kSampleRate)
                                               .withNumChannels (buffer.getNumChannels())
                                               .withBitsPerSample (24));
    if (writer == nullptr)
        return false;
    return writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples());
}

float peakOf (const juce::AudioBuffer<float>& buffer)
{
    float peak = 0.0f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        peak = std::max (peak, buffer.getMagnitude (channel, 0, buffer.getNumSamples()));
    return peak;
}

} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI libraryInitialiser;

    const juce::File directory (argc > 1 ? argv[1] : "/tmp/guitar-delay-demos");
    directory.createDirectory();

    const int length = static_cast<int> (kSampleRate * 12.0);
    const auto phrase = makeGuitarPhrase (length);
    juce::AudioBuffer<float> dry (2, length);
    for (int i = 0; i < length; ++i)
    {
        dry.setSample (0, i, phrase[static_cast<size_t> (i)]);
        dry.setSample (1, i, phrase[static_cast<size_t> (i)]);
    }

    const Demo demos[] = {
        { "guitar_tape_slapback", 0, 105.0f, 0.18f, 0.34f, 0.08f, 0.16f, 0.10f, 0.26f, 0.58f, 0.18f, 9000.0f, 0.04f, -1, -1, -1.0f },
        { "guitar_tape_dotted_eighth", 0, 375.0f, 0.40f, 0.32f, 0.22f, 0.30f, 0.18f, 0.40f, 0.58f, 0.20f, 8000.0f, 0.08f, -1, -1, -1.0f },
        { "guitar_tape_ambient", 0, 680.0f, 0.68f, 0.44f, 0.58f, 0.36f, 0.20f, 0.52f, 0.40f, 0.24f, 6000.0f, 0.20f, -1, -1, -1.0f },
        { "guitar_analog_slapback", 1, 100.0f, 0.20f, 0.34f, 0.10f, 0.20f, 0.12f, 0.30f, 0.64f, 0.16f, 9000.0f, 0.02f, -1, -1, -1.0f },
        { "guitar_analog_dotted_eighth", 1, 375.0f, 0.42f, 0.33f, 0.24f, 0.20f, 0.12f, 0.30f, 0.48f, 0.30f, 7000.0f, 0.10f, -1, -1, -1.0f },
        { "guitar_analog_ambient", 1, 720.0f, 0.70f, 0.46f, 0.66f, 0.20f, 0.12f, 0.30f, 0.32f, 0.28f, 5000.0f, 0.20f, -1, -1, -1.0f },
        { "guitar_digital_slapback", 2, 100.0f, 0.16f, 0.32f, 0.06f, 0.20f, 0.12f, 0.30f, 0.58f, 0.16f, 10000.0f, 0.00f, -1, -1, -1.0f },
        { "guitar_digital_dotted_eighth", 2, 375.0f, 0.42f, 0.30f, 0.16f, 0.20f, 0.12f, 0.30f, 0.58f, 0.16f, 8600.0f, 0.08f, -1, -1, -1.0f },
        { "guitar_digital_ambient", 2, 640.0f, 0.55f, 0.40f, 0.48f, 0.20f, 0.12f, 0.30f, 0.58f, 0.16f, 5400.0f, 0.36f, -1, -1, -1.0f },
        { "guitar_tape_time_glide", 0, 130.0f, 0.36f, 0.38f, 0.16f, 0.18f, 0.10f, 0.34f, 0.50f, 0.18f, 8000.0f, 0.05f, static_cast<int> (2.8 * kSampleRate), -1, 620.0f },
        { "guitar_digital_time_crossfade", 2, 130.0f, 0.40f, 0.36f, 0.12f, 0.20f, 0.12f, 0.30f, 0.55f, 0.16f, 11000.0f, 0.04f, static_cast<int> (2.8 * kSampleRate), -1, 540.0f },
        { "guitar_mode_switch_trails", 0, 300.0f, 0.52f, 0.40f, 0.20f, 0.22f, 0.12f, 0.36f, 0.50f, 0.22f, 9000.0f, 0.10f, static_cast<int> (3.2 * kSampleRate), 2, -1.0f },
    };

    if (! writeWav (directory.getChildFile ("guitar_di_dry.wav"), dry))
    {
        std::cerr << "could not write dry wav\n";
        return 1;
    }
    std::cout << "wrote guitar_di_dry.wav peak " << peakOf (dry) << "\n";

    for (const Demo& demo : demos)
    {
        const auto rendered = renderDemo (phrase, demo);
        const juce::File file = directory.getChildFile (juce::String (demo.name) + ".wav");
        if (! writeWav (file, rendered))
        {
            std::cerr << "could not write " << demo.name << "\n";
            return 1;
        }
        std::cout << "wrote " << demo.name << ".wav peak " << peakOf (rendered) << "\n";
    }

    return 0;
}
