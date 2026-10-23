#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace guitar_delay
{
namespace
{
constexpr float kHalfPi = 1.5707963267948966f;
}

GuitarDelayProcessor::GuitarDelayProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    modeRaw = apvts.getRawParameterValue (id::mode);
    timeRaw = apvts.getRawParameterValue (id::time);
    feedbackRaw = apvts.getRawParameterValue (id::feedback);
    mixRaw = apvts.getRawParameterValue (id::mix);
    widthRaw = apvts.getRawParameterValue (id::width);
    syncRaw = apvts.getRawParameterValue (id::sync);
    divisionRaw = apvts.getRawParameterValue (id::division);
    bypassRaw = apvts.getRawParameterValue (id::bypass);
    wowRaw = apvts.getRawParameterValue (id::tapeWow);
    flutterRaw = apvts.getRawParameterValue (id::tapeFlutter);
    ageRaw = apvts.getRawParameterValue (id::tapeAge);
    toneRaw = apvts.getRawParameterValue (id::analogTone);
    driveRaw = apvts.getRawParameterValue (id::analogDrive);
    hiCutRaw = apvts.getRawParameterValue (id::digitalHiCut);
    diffuseRaw = apvts.getRawParameterValue (id::digitalDiffuse);
}

GuitarDelayProcessor::~GuitarDelayProcessor() = default;

const juce::String GuitarDelayProcessor::getName() const { return "Sunset Delay"; }

bool GuitarDelayProcessor::acceptsMidi() const { return false; }
bool GuitarDelayProcessor::producesMidi() const { return false; }
bool GuitarDelayProcessor::isMidiEffect() const { return false; }
double GuitarDelayProcessor::getTailLengthSeconds() const { return 12.0; }

int GuitarDelayProcessor::getNumPrograms() { return kNumFactoryPresets; }

int GuitarDelayProcessor::getCurrentProgram()
{
    return juce::jlimit (0, kNumFactoryPresets - 1, currentProgram.load());
}

void GuitarDelayProcessor::setRealParameter (const char* paramId, float realValue)
{
    if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (paramId)))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (realValue));
        parameter->endChangeGesture();
    }
}

void GuitarDelayProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= kNumFactoryPresets)
        return;

    currentProgram.store (index);
    const FactoryPreset& preset = kFactoryPresets[index];
    // Knobs land first, then the mode. The next audio block sees one patch,
    // and the processor crossfades the engines so the recall does not click.
    setRealParameter (id::time, preset.timeMs);
    setRealParameter (id::feedback, preset.feedback);
    setRealParameter (id::mix, preset.mix);
    setRealParameter (id::width, preset.width);
    setRealParameter (id::sync, preset.sync ? 1.0f : 0.0f);
    setRealParameter (id::division, static_cast<float> (preset.division));
    setRealParameter (id::tapeWow, preset.wow);
    setRealParameter (id::tapeFlutter, preset.flutter);
    setRealParameter (id::tapeAge, preset.age);
    setRealParameter (id::analogTone, preset.tone);
    setRealParameter (id::analogDrive, preset.drive);
    setRealParameter (id::digitalHiCut, preset.hiCutHz);
    setRealParameter (id::digitalDiffuse, preset.diffuse);
    setRealParameter (id::mode, static_cast<float> (preset.mode));
}

bool GuitarDelayProcessor::programMatchesCurrent() const noexcept
{
    const int index = juce::jlimit (0, kNumFactoryPresets - 1, currentProgram.load());
    const FactoryPreset& preset = kFactoryPresets[index];

    const auto near = [] (float value, float target, float tolerance)
    {
        return std::abs (value - target) <= tolerance;
    };

    if (getModeIndex() != preset.mode)
        return false;
    if (isSyncEnabled() != preset.sync)
        return false;
    if (std::lround (divisionRaw->load()) != preset.division)
        return false;
    if (! near (timeRaw->load(), preset.timeMs, 1.0f))
        return false;
    if (! near (feedbackRaw->load(), preset.feedback, 0.006f))
        return false;
    if (! near (mixRaw->load(), preset.mix, 0.006f))
        return false;
    if (! near (widthRaw->load(), preset.width, 0.006f))
        return false;
    if (! near (wowRaw->load(), preset.wow, 0.006f))
        return false;
    if (! near (flutterRaw->load(), preset.flutter, 0.006f))
        return false;
    if (! near (ageRaw->load(), preset.age, 0.006f))
        return false;
    if (! near (toneRaw->load(), preset.tone, 0.006f))
        return false;
    if (! near (driveRaw->load(), preset.drive, 0.006f))
        return false;
    if (! near (hiCutRaw->load(), preset.hiCutHz, 12.0f))
        return false;
    if (! near (diffuseRaw->load(), preset.diffuse, 0.006f))
        return false;
    return true;
}

const juce::String GuitarDelayProcessor::getProgramName (int index)
{
    if (index < 0 || index >= kNumFactoryPresets)
        return {};
    return kFactoryPresets[index].name;
}

void GuitarDelayProcessor::changeProgramName (int, const juce::String&) {}

bool GuitarDelayProcessor::hasEditor() const { return true; }

namespace
{
class QuietEditor final : public juce::AudioProcessorEditor
{
public:
    explicit QuietEditor (juce::AudioProcessor& processor)
        : juce::AudioProcessorEditor (processor)
    {
        setSize (480, 240);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xffE7E2DB));
        g.setColour (juce::Colour (0xff514C46));
        g.setFont (juce::FontOptions (18.0f));
        g.drawFittedText ("Sunset Delay", getLocalBounds().reduced (24), juce::Justification::centred, 2);
    }
};
} // namespace

juce::AudioProcessorEditor* GuitarDelayProcessor::createEditor()
{
    // The faceplate is the normal editor. If constructing it throws, the host
    // still receives a view so the audio processor stays loaded.
    try
    {
        return new GuitarDelayEditor (*this);
    }
    catch (...)
    {
        return new QuietEditor (*this);
    }
}

juce::AudioProcessorParameter* GuitarDelayProcessor::getBypassParameter() const
{
    return apvts.getParameter (id::bypass);
}

int GuitarDelayProcessor::getModeIndex() const noexcept
{
    return juce::jlimit (0, 2, static_cast<int> (std::lround (modeRaw->load())));
}

float GuitarDelayProcessor::getDisplayedBpm() const noexcept
{
    return displayedBpm.load();
}

bool GuitarDelayProcessor::isSyncEnabled() const noexcept
{
    return syncRaw->load() >= 0.5f;
}

void GuitarDelayProcessor::prepareToPlay (double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate > 1000.0 ? newSampleRate : 48000.0;
    blockCapacity = std::max (samplesPerBlock, 512);

    sendBuffer.setSize (2, blockCapacity, false, false, true);
    wetBuffer.setSize (2, blockCapacity, false, false, true);
    wetTailBuffer.setSize (2, blockCapacity, false, false, true);
    silenceBuffer.setSize (2, blockCapacity, false, false, true);
    bypassEnv.assign (static_cast<size_t> (blockCapacity), 0.0f);
    mixEnv.assign (static_cast<size_t> (blockCapacity), 0.28f);
    fadeEnv.assign (static_cast<size_t> (blockCapacity), 1.0f);

    tape.prepare (sampleRate);
    analog.prepare (sampleRate);
    digital.prepare (sampleRate);

    mixSmooth.reset (sampleRate, 0.020);
    bypassSmooth.reset (sampleRate, 0.025);
    crossfade.reset (sampleRate, 0.55);

    const auto settings = readSettings();
    mixSmooth.setCurrentAndTargetValue (std::clamp (mixRaw->load(), 0.0f, 1.0f));
    bypassSmooth.setCurrentAndTargetValue (bypassRaw->load() >= 0.5f ? 1.0f : 0.0f);
    crossfade.setCurrentAndTargetValue (1.0f);

    currentMode = getModeIndex();
    outgoingMode = -1;
    snapMode (0, settings);
    snapMode (1, settings);
    snapMode (2, settings);
    reset();
}

void GuitarDelayProcessor::releaseResources()
{
    reset();
}

void GuitarDelayProcessor::reset()
{
    tape.reset();
    analog.reset();
    digital.reset();
    if (sampleRate > 0.0)
    {
        const auto settings = readSettings();
        snapMode (currentMode, settings);
    }
}

bool GuitarDelayProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    const auto in = layouts.getMainInputChannelSet();

    const bool outOk = out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
    const bool inOk = in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
    if (! outOk || ! inOk)
        return false;

    // Stereo in to mono out would throw away a channel. Guitar mono-in / stereo-out stays.
    return in.size() <= out.size();
}

void GuitarDelayProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);
    render (buffer, false);
}

void GuitarDelayProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);
    render (buffer, true);
}

void GuitarDelayProcessor::updateTransport()
{
    float bpm = 120.0f;
    if (auto* head = getPlayHead())
    {
        if (auto position = head->getPosition())
        {
            if (auto bpmValue = position->getBpm())
            {
                if (*bpmValue >= 20.0 && *bpmValue <= 400.0)
                    bpm = static_cast<float> (*bpmValue);
            }
        }
    }
    displayedBpm.store (bpm);
}

float GuitarDelayProcessor::resolveDelayMs() const
{
    if (syncRaw->load() < 0.5f)
        return std::clamp (timeRaw->load(), 50.0f, 1000.0f);

    const int division = juce::jlimit (0, kNumDivisions - 1, static_cast<int> (std::lround (divisionRaw->load())));
    return clampDelayMs (delayMsForDivision (displayedBpm.load(), division));
}

GuitarDelayProcessor::BlockSettings GuitarDelayProcessor::readSettings() const
{
    BlockSettings settings;
    settings.delayMs = resolveDelayMs();
    settings.feedback = std::clamp (feedbackRaw->load(), 0.0f, 1.0f);
    settings.width = std::clamp (widthRaw->load(), 0.0f, 1.0f);
    settings.wow = std::clamp (wowRaw->load(), 0.0f, 1.0f);
    settings.flutter = std::clamp (flutterRaw->load(), 0.0f, 1.0f);
    settings.age = std::clamp (ageRaw->load(), 0.0f, 1.0f);
    settings.tone = std::clamp (toneRaw->load(), 0.0f, 1.0f);
    settings.drive = std::clamp (driveRaw->load(), 0.0f, 1.0f);
    settings.hiCutHz = std::clamp (hiCutRaw->load(), 1000.0f, 16000.0f);
    settings.diffuse = std::clamp (diffuseRaw->load(), 0.0f, 1.0f);
    return settings;
}

void GuitarDelayProcessor::resetMode (int mode)
{
    if (mode == 0)
        tape.reset();
    else if (mode == 1)
        analog.reset();
    else if (mode == 2)
        digital.reset();
}

void GuitarDelayProcessor::snapMode (int mode, const BlockSettings& settings)
{
    if (mode == 0)
    {
        tape.snap ({ settings.delayMs, settings.feedback, settings.width, settings.wow, settings.flutter, settings.age });
    }
    else if (mode == 1)
    {
        analog.snap ({ settings.delayMs, settings.feedback, settings.width, settings.tone, settings.drive });
    }
    else if (mode == 2)
    {
        digital.snap ({ settings.delayMs, settings.feedback, settings.width, settings.hiCutHz, settings.diffuse });
    }
}

void GuitarDelayProcessor::handleModeChange (int requested, const BlockSettings& settings)
{
    if (requested == currentMode)
        return;

    if (requested == outgoingMode)
    {
        std::swap (currentMode, outgoingMode);
        const float flipped = 1.0f - crossfade.getCurrentValue();
        crossfade.setCurrentAndTargetValue (flipped);
        crossfade.setTargetValue (1.0f);
        return;
    }

    if (currentMode >= 0)
        outgoingMode = currentMode;

    currentMode = requested;
    crossfade.setCurrentAndTargetValue (0.0f);
    crossfade.setTargetValue (1.0f);
    resetMode (currentMode);
    snapMode (currentMode, settings);
}

void GuitarDelayProcessor::renderMode (int mode, const float* const* input, float* const* output, int channels, int count, const BlockSettings& settings, bool)
{
    if (mode == 0)
    {
        const TapeSettings tapeSettings { settings.delayMs, settings.feedback, settings.width, settings.wow, settings.flutter, settings.age };
        tape.process (input, output, channels, count, tapeSettings);
    }
    else if (mode == 1)
    {
        const AnalogSettings analogSettings { settings.delayMs, settings.feedback, settings.width, settings.tone, settings.drive };
        analog.process (input, output, channels, count, analogSettings);
    }
    else
    {
        const DigitalSettings digitalSettings { settings.delayMs, settings.feedback, settings.width, settings.hiCutHz, settings.diffuse };
        digital.process (input, output, channels, count, digitalSettings);
    }
}

void GuitarDelayProcessor::render (juce::AudioBuffer<float>& buffer, bool hostBypass)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0 || sampleRate <= 0.0 || blockCapacity <= 0)
        return;

    const int numIn = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    const int bufferChannels = buffer.getNumChannels();
    const int channels = juce::jmin (2, numOut, bufferChannels);
    if (channels <= 0)
        return;

    // The host's insert buffer already holds the track. Spare outputs (mono
    // in, stereo out) start empty, then the mono channel is copied across.
    // Clearing from channel 0 when the input count is 0 erases that track.
    if (numIn > 0 && numIn < channels)
    {
        for (int channel = numIn; channel < channels; ++channel)
            buffer.clear (channel, 0, numSamples);

        if (numIn == 1)
            buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
    }

    updateTransport();
    const auto settings = readSettings();
    handleModeChange (getModeIndex(), settings);

    int offset = 0;
    while (offset < numSamples)
    {
        const int count = std::min (blockCapacity, numSamples - offset);
        processChunk (buffer, offset, count, channels, hostBypass, settings);
        offset += count;
    }

    if (outgoingMode >= 0 && crossfade.getCurrentValue() >= 0.999f)
        outgoingMode = -1;
}

void GuitarDelayProcessor::processChunk (juce::AudioBuffer<float>& buffer, int offset, int count, int channels, bool hostBypass, const BlockSettings& settings)
{
    const bool bypassed = hostBypass || bypassRaw->load() >= 0.5f;
    bypassSmooth.setTargetValue (bypassed ? 1.0f : 0.0f);
    mixSmooth.setTargetValue (std::clamp (mixRaw->load(), 0.0f, 1.0f));

    float* send[2] = { sendBuffer.getWritePointer (0), sendBuffer.getWritePointer (1) };
    const float* dry[2] = { nullptr, nullptr };

    for (int channel = 0; channel < channels; ++channel)
        dry[channel] = buffer.getReadPointer (channel) + offset;

    for (int i = 0; i < count; ++i)
    {
        const float bypassAmt = bypassSmooth.getNextValue();
        const float sendGain = 1.0f - bypassAmt;
        bypassEnv[static_cast<size_t> (i)] = bypassAmt;
        mixEnv[static_cast<size_t> (i)] = mixSmooth.getNextValue();
        fadeEnv[static_cast<size_t> (i)] = crossfade.getNextValue();

        for (int channel = 0; channel < channels; ++channel)
        {
            const float sample = dry[channel] != nullptr ? dry[channel][i] : 0.0f;
            send[channel][i] = sample * sendGain;
        }
    }

    float* wet[2] = { wetBuffer.getWritePointer (0), wetBuffer.getWritePointer (1) };
    const float* sendIn[2] = { send[0], channels > 1 ? send[1] : send[0] };
    renderMode (currentMode, sendIn, wet, channels, count, settings, false);

    float* tail[2] = { wetTailBuffer.getWritePointer (0), wetTailBuffer.getWritePointer (1) };
    if (outgoingMode >= 0)
    {
        silenceBuffer.clear (0, count);
        const float* silent[2] = { silenceBuffer.getReadPointer (0), silenceBuffer.getReadPointer (1) };
        renderMode (outgoingMode, silent, tail, channels, count, settings, false);
    }

    for (int i = 0; i < count; ++i)
    {
        const float mix = mixEnv[static_cast<size_t> (i)];
        const float bypassAmt = bypassEnv[static_cast<size_t> (i)];
        const float fade = outgoingMode >= 0 ? fadeEnv[static_cast<size_t> (i)] : 1.0f;

        const float engagedDry = std::cos (mix * kHalfPi);
        const float wetGain = std::sin (mix * kHalfPi);
        const float dryGain = engagedDry + (1.0f - engagedDry) * bypassAmt;
        const float newAmt = std::sin (fade * kHalfPi);
        const float oldAmt = std::cos (fade * kHalfPi);

        for (int channel = 0; channel < channels; ++channel)
        {
            float wetSample = wet[channel][i] * newAmt;
            if (outgoingMode >= 0)
                wetSample += tail[channel][i] * oldAmt;

            float sample = dry[channel][i] * dryGain + wetSample * wetGain;
            if (! std::isfinite (sample))
                sample = 0.0f;

            buffer.getWritePointer (channel)[offset + i] = sample;
        }
    }
}

void GuitarDelayProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree state = apvts.copyState();
    state.setProperty ("program", getCurrentProgram(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void GuitarDelayProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            const int program = static_cast<int> (state.getProperty ("program", 0));
            apvts.replaceState (state);
            currentProgram.store (juce::jlimit (0, kNumFactoryPresets - 1, program));
        }
    }
}

} // namespace guitar_delay

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new guitar_delay::GuitarDelayProcessor();
}
