#pragma once

#include "Tempo.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace guitar_delay
{

namespace id
{
inline constexpr const char* mode = "mode";
inline constexpr const char* time = "time";
inline constexpr const char* feedback = "feedback";
inline constexpr const char* mix = "mix";
inline constexpr const char* width = "width";
inline constexpr const char* sync = "sync";
inline constexpr const char* division = "division";
inline constexpr const char* bypass = "bypass";
inline constexpr const char* tapeWow = "tapeWow";
inline constexpr const char* tapeFlutter = "tapeFlutter";
inline constexpr const char* tapeAge = "tapeAge";
inline constexpr const char* analogTone = "analogTone";
inline constexpr const char* analogDrive = "analogDrive";
inline constexpr const char* digitalHiCut = "digitalHiCut";
inline constexpr const char* digitalDiffuse = "digitalDiffuse";
} // namespace id

inline juce::StringArray divisionNames()
{
    return { "1/1", "1/2", "1/2.", "1/2T",
             "1/4", "1/4.", "1/4T",
             "1/8", "1/8.", "1/8T",
             "1/16", "1/16.", "1/16T" };
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;

    auto percent = [] (float value, int) -> String
    {
        return String (roundToInt (value * 100.0f)) + " %";
    };

    auto percentFromText = [] (const String& text) -> float
    {
        return jlimit (0.0f, 1.0f, text.getFloatValue() / 100.0f);
    };

    NormalisableRange<float> timeRange (50.0f, 1000.0f, 0.01f);
    timeRange.setSkewForCentre (320.0f);

    NormalisableRange<float> cutRange (1000.0f, 16000.0f, 0.1f);
    cutRange.setSkewForCentre (5500.0f);

    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { id::mode, 1 },
        "Mode",
        StringArray { "Tape", "Analog", "Digital" },
        0));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { id::time, 1 },
        "Time",
        timeRange,
        375.0f,
        AudioParameterFloatAttributes()
            .withLabel ("ms")
            .withStringFromValueFunction ([] (float value, int) { return String (value, 1) + " ms"; })
            .withValueFromStringFunction ([] (const String& text) { return text.getFloatValue(); })));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { id::feedback, 1 },
        "Feedback",
        NormalisableRange<float> (0.0f, 1.0f, 0.0001f),
        0.34f,
        AudioParameterFloatAttributes()
            .withLabel ("%")
            .withStringFromValueFunction (percent)
            .withValueFromStringFunction (percentFromText)));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { id::mix, 1 },
        "Mix",
        NormalisableRange<float> (0.0f, 1.0f, 0.0001f),
        0.28f,
        AudioParameterFloatAttributes()
            .withLabel ("%")
            .withStringFromValueFunction (percent)
            .withValueFromStringFunction (percentFromText)));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { id::width, 1 },
        "Width",
        NormalisableRange<float> (0.0f, 1.0f, 0.0001f),
        0.20f,
        AudioParameterFloatAttributes()
            .withLabel ("%")
            .withStringFromValueFunction (percent)
            .withValueFromStringFunction (percentFromText)));

    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { id::sync, 1 },
        "Sync",
        false,
        AudioParameterBoolAttributes().withStringFromValueFunction (
            [] (bool value, int) { return value ? String ("Sync") : String ("Free"); })));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { id::division, 1 },
        "Division",
        divisionNames(),
        8));

    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { id::bypass, 1 },
        "Bypass",
        false,
        AudioParameterBoolAttributes().withStringFromValueFunction (
            [] (bool value, int) { return value ? String ("Bypassed") : String ("Active"); })));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { id::tapeWow, 1 },
        "Wow",
        NormalisableRange<float> (0.0f, 1.0f, 0.0001f),
        0.32f,
        AudioParameterFloatAttributes()
            .withLabel ("%")
            .withStringFromValueFunction (percent)
            .withValueFromStringFunction (percentFromText)));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { id::tapeFlutter, 1 },
        "Flutter",
        NormalisableRange<float> (0.0f, 1.0f, 0.0001f),
        0.20f,
        AudioParameterFloatAttributes()
            .withLabel ("%")
            .withStringFromValueFunction (percent)
            .withValueFromStringFunction (percentFromText)));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { id::tapeAge, 1 },
        "Age",
        NormalisableRange<float> (0.0f, 1.0f, 0.0001f),
        0.38f,
        AudioParameterFloatAttributes()
            .withLabel ("%")
            .withStringFromValueFunction (percent)
            .withValueFromStringFunction (percentFromText)));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { id::analogTone, 1 },
        "Tone",
        NormalisableRange<float> (0.0f, 1.0f, 0.0001f),
        0.58f,
        AudioParameterFloatAttributes()
            .withLabel ("%")
            .withStringFromValueFunction (percent)
            .withValueFromStringFunction (percentFromText)));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { id::analogDrive, 1 },
        "Drive",
        NormalisableRange<float> (0.0f, 1.0f, 0.0001f),
        0.20f,
        AudioParameterFloatAttributes()
            .withLabel ("%")
            .withStringFromValueFunction (percent)
            .withValueFromStringFunction (percentFromText)));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { id::digitalHiCut, 1 },
        "Hi-Cut",
        cutRange,
        7200.0f,
        AudioParameterFloatAttributes()
            .withLabel ("Hz")
            .withStringFromValueFunction ([] (float value, int)
                                           { return String (value / 1000.0f, 2) + " kHz"; })
            .withValueFromStringFunction ([] (const String& text) { return text.getFloatValue(); })));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { id::digitalDiffuse, 1 },
        "Diffuse",
        NormalisableRange<float> (0.0f, 1.0f, 0.0001f),
        0.12f,
        AudioParameterFloatAttributes()
            .withLabel ("%")
            .withStringFromValueFunction (percent)
            .withValueFromStringFunction (percentFromText)));

    return layout;
}

} // namespace guitar_delay
