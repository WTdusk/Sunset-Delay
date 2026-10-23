#include "PluginProcessor.h"

#include <cmath>
#include <iostream>
#include <vector>

namespace
{

int gFailures = 0;

void expect (bool condition, const char* message)
{
    std::cout << (condition ? "ok   " : "FAIL ") << message << "\n";
    if (! condition)
        ++gFailures;
}

juce::AudioProcessor::BusesLayout layout (juce::AudioChannelSet input, juce::AudioChannelSet output)
{
    juce::AudioProcessor::BusesLayout result;
    result.inputBuses.add (input);
    result.outputBuses.add (output);
    return result;
}

bool bufferIsFinite (const juce::AudioBuffer<float>& buffer)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            if (! std::isfinite (buffer.getSample (channel, sample)))
                return false;
    return true;
}

void setReal (guitar_delay::GuitarDelayProcessor& processor, const char* paramId, float value)
{
    if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (processor.apvts.getParameter (paramId)))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

float energyRange (const std::vector<float>& data, int start, int end)
{
    const int a = std::max (0, start);
    const int b = std::min (end, static_cast<int> (data.size()));
    double energy = 0.0;
    for (int i = a; i < b; ++i)
        energy += static_cast<double> (data[static_cast<size_t> (i)]) * static_cast<double> (data[static_cast<size_t> (i)]);
    return static_cast<float> (energy);
}

std::vector<float> renderBlocks (guitar_delay::GuitarDelayProcessor& processor, int blockSize, int numBlocks, float impulse, int impulseBlock, bool hostBypass)
{
    juce::MidiBuffer midi;
    std::vector<float> left (static_cast<size_t> (blockSize * numBlocks), 0.0f);

    for (int block = 0; block < numBlocks; ++block)
    {
        juce::AudioBuffer<float> buffer (2, blockSize);
        buffer.clear();
        if (block == impulseBlock)
        {
            buffer.setSample (0, 0, impulse);
            buffer.setSample (1, 0, impulse);
        }

        if (hostBypass)
            processor.processBlockBypassed (buffer, midi);
        else
            processor.processBlock (buffer, midi);

        for (int i = 0; i < blockSize; ++i)
            left[static_cast<size_t> (block * blockSize + i)] = buffer.getSample (0, i);
    }

    return left;
}

} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    guitar_delay::GuitarDelayProcessor processor;
    juce::MidiBuffer midi;

    expect (processor.getName() == "Sunset Delay", "host name is Sunset Delay");

    const auto stereo = layout (juce::AudioChannelSet::stereo(), juce::AudioChannelSet::stereo());
    const auto mono = layout (juce::AudioChannelSet::mono(), juce::AudioChannelSet::mono());
    const auto monoToStereo = layout (juce::AudioChannelSet::mono(), juce::AudioChannelSet::stereo());
    const auto stereoToMono = layout (juce::AudioChannelSet::stereo(), juce::AudioChannelSet::mono());
    const auto noInput = layout (juce::AudioChannelSet::disabled(), juce::AudioChannelSet::stereo());
    const auto emptyInput = layout (juce::AudioChannelSet(), juce::AudioChannelSet::stereo());
    const auto surround = layout (juce::AudioChannelSet::create5point1(), juce::AudioChannelSet::create5point1());

    expect (processor.isBusesLayoutSupported (stereo), "stereo in / stereo out");
    expect (processor.isBusesLayoutSupported (mono), "mono in / mono out");
    expect (processor.isBusesLayoutSupported (monoToStereo), "mono in / stereo out");
    expect (! processor.isBusesLayoutSupported (stereoToMono), "stereo in / mono out is rejected");
    expect (! processor.isBusesLayoutSupported (noInput), "disabled audio input is rejected");
    expect (! processor.isBusesLayoutSupported (emptyInput), "empty audio input is rejected");
    expect (! processor.isBusesLayoutSupported (surround), "surround is rejected");
    expect (! processor.setBusesLayout (noInput), "host cannot open an insert with no input");

    constexpr float kHalfPi = 1.5707963267948966f;
    constexpr float kDefaultMix = 0.28f;
    constexpr float kPick = 0.8f;
    const float expectedDry = kPick * std::cos (kDefaultMix * kHalfPi);

    expect (processor.setBusesLayout (stereo), "stereo effect layout applies");
    processor.prepareToPlay (48000.0, 256);
    juce::AudioBuffer<float> impulse (2, 256);
    impulse.clear();
    impulse.setSample (0, 0, kPick);
    impulse.setSample (1, 0, kPick);
    processor.processBlock (impulse, midi);
    std::cout << "default dry L/R " << impulse.getSample (0, 0) << " " << impulse.getSample (1, 0)
              << " expected " << expectedDry << "\n";
    expect (bufferIsFinite (impulse), "default preset output is finite");
    expect (std::abs (impulse.getSample (0, 0) - expectedDry) < 0.05f
                && std::abs (impulse.getSample (1, 0) - expectedDry) < 0.05f,
            "default preset keeps the dry pick");
    processor.releaseResources();

    setReal (processor, guitar_delay::id::mix, 0.0f);
    processor.prepareToPlay (48000.0, 256);
    impulse.clear();
    impulse.setSample (0, 0, kPick);
    impulse.setSample (1, 0, kPick);
    processor.processBlock (impulse, midi);
    std::cout << "mix 0 dry " << impulse.getSample (0, 0) << "\n";
    expect (std::abs (impulse.getSample (0, 0) - kPick) < 0.04f
                && std::abs (impulse.getSample (1, 0) - kPick) < 0.04f,
            "mix at zero is full dry");
    processor.releaseResources();

    setReal (processor, guitar_delay::id::mix, 1.0f);
    setReal (processor, guitar_delay::id::mode, 2.0f);
    setReal (processor, guitar_delay::id::time, 50.0f);
    setReal (processor, guitar_delay::id::feedback, 0.0f);
    setReal (processor, guitar_delay::id::width, 0.0f);
    processor.prepareToPlay (48000.0, 512);
    auto wet = renderBlocks (processor, 512, 8, 1.0f, 0, false);
    const float early = energyRange (wet, 64, 1600);
    const float echo = energyRange (wet, 2000, 3600);
    std::cout << "mix up early energy " << early << " echo energy " << echo
              << " first sample " << wet[0] << "\n";
    expect (std::abs (wet[0]) < 0.08f, "mix at full leaves the dry pick out");
    expect (early < 0.02f && echo > 0.05f, "mix at full produces the delayed wet");
    processor.releaseResources();

    setReal (processor, guitar_delay::id::mix, 1.0f);
    setReal (processor, guitar_delay::id::mode, 2.0f);
    setReal (processor, guitar_delay::id::time, 50.0f);
    setReal (processor, guitar_delay::id::feedback, 0.45f);
    setReal (processor, guitar_delay::id::width, 0.0f);
    setReal (processor, guitar_delay::id::bypass, 0.0f);
    processor.prepareToPlay (48000.0, 512);
    {
        juce::AudioBuffer<float> primed (2, 512);
        primed.clear();
        primed.setSample (0, 0, 1.0f);
        primed.setSample (1, 0, 1.0f);
        processor.processBlock (primed, midi);
        expect (std::abs (primed.getSample (0, 0)) < 0.08f, "pre-bypass wet path is not a dry copy");
    }
    for (int i = 0; i < 2; ++i)
    {
        juce::AudioBuffer<float> gap (2, 512);
        gap.clear();
        processor.processBlock (gap, midi);
    }
    setReal (processor, guitar_delay::id::bypass, 1.0f);
    float trail = 0.0f;
    for (int i = 0; i < 6; ++i)
    {
        juce::AudioBuffer<float> gap (2, 512);
        gap.clear();
        processor.processBlock (gap, midi);
        for (int sample = 0; sample < gap.getNumSamples(); ++sample)
            trail += gap.getSample (0, sample) * gap.getSample (0, sample);
    }
    std::cout << "bypass trail energy " << trail << "\n";
    expect (trail > 0.02f, "bypass lets the delay trail ring");

    for (int i = 0; i < 4; ++i)
    {
        juce::AudioBuffer<float> settle (2, 512);
        settle.clear();
        processor.processBlock (settle, midi);
    }
    juce::AudioBuffer<float> bypassedPick (2, 512);
    bypassedPick.clear();
    bypassedPick.setSample (0, 0, kPick);
    bypassedPick.setSample (1, 0, kPick);
    processor.processBlock (bypassedPick, midi);
    std::cout << "bypassed dry " << bypassedPick.getSample (0, 0) << "\n";
    expect (std::abs (bypassedPick.getSample (0, 0) - kPick) < 0.12f
                && std::abs (bypassedPick.getSample (1, 0) - kPick) < 0.12f,
            "bypass passes the dry pick");
    processor.releaseResources();

    setReal (processor, guitar_delay::id::bypass, 0.0f);
    setReal (processor, guitar_delay::id::mix, 1.0f);
    processor.prepareToPlay (48000.0, 512);
    for (int i = 0; i < 4; ++i)
    {
        juce::AudioBuffer<float> settle (2, 512);
        settle.clear();
        processor.processBlockBypassed (settle, midi);
    }
    juce::AudioBuffer<float> hostBypass (2, 512);
    hostBypass.clear();
    hostBypass.setSample (0, 0, kPick);
    hostBypass.setSample (1, 0, kPick);
    processor.processBlockBypassed (hostBypass, midi);
    std::cout << "host bypass dry " << hostBypass.getSample (0, 0) << "\n";
    expect (std::abs (hostBypass.getSample (0, 0) - kPick) < 0.08f, "host bypass passes dry");
    processor.releaseResources();

    setReal (processor, guitar_delay::id::mix, kDefaultMix);
    setReal (processor, guitar_delay::id::mode, 0.0f);
    setReal (processor, guitar_delay::id::time, 375.0f);
    expect (processor.setBusesLayout (monoToStereo), "mono-to-stereo insert applies");
    processor.prepareToPlay (48000.0, 256);
    juce::AudioBuffer<float> guitar (2, 256);
    guitar.clear();
    guitar.setSample (0, 0, kPick);
    processor.processBlock (guitar, midi);
    std::cout << "mono-to-stereo L/R " << guitar.getSample (0, 0) << " " << guitar.getSample (1, 0) << "\n";
    expect (std::abs (guitar.getSample (0, 0) - expectedDry) < 0.05f
                && std::abs (guitar.getSample (1, 0) - expectedDry) < 0.05f,
            "mono guitar is copied to both outputs and stays dry");
    processor.releaseResources();

    auto* editor = processor.createEditor();
    expect (editor != nullptr, "editor instantiates on the message thread");
    if (editor != nullptr)
    {
        expect (editor->getWidth() >= 400 && editor->getHeight() >= 200, "editor reports a size");
        delete editor;
        editor = nullptr;
    }

    if (gFailures == 0)
        std::cout << "All host compatibility checks passed.\n";
    else
        std::cout << gFailures << " host compatibility checks failed.\n";

    return gFailures == 0 ? 0 : 1;
}
