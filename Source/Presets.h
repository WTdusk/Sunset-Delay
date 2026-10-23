#pragma once

namespace guitar_delay
{

// Factory guitar patches. Mode is 0 tape, 1 analog, 2 digital.
// Division index 8 is a dotted eighth. Unused engine knobs stay at values
// that are still a guitar sound if the player flips mode afterward.
struct FactoryPreset
{
    const char* name;
    int mode;
    float timeMs;
    float feedback;
    float mix;
    float width;
    bool sync;
    int division;
    float wow;
    float flutter;
    float age;
    float tone;
    float drive;
    float hiCutHz;
    float diffuse;
};

inline constexpr int kNumFactoryPresets = 11;

inline constexpr FactoryPreset kFactoryPresets[kNumFactoryPresets] = {
    { "Pedal", 0, 375.0f, 0.34f, 0.28f, 0.20f, false, 8, 0.32f, 0.20f, 0.38f, 0.58f, 0.20f, 7200.0f, 0.12f },
    { "Rockabilly Slap", 0, 96.0f, 0.14f, 0.36f, 0.04f, false, 8, 0.10f, 0.05f, 0.12f, 0.58f, 0.20f, 7200.0f, 0.12f },
    { "Dark Slap", 1, 118.0f, 0.26f, 0.38f, 0.16f, false, 8, 0.32f, 0.20f, 0.38f, 0.22f, 0.48f, 7200.0f, 0.12f },
    { "Studio Slap", 2, 88.0f, 0.10f, 0.30f, 0.00f, false, 8, 0.32f, 0.20f, 0.38f, 0.58f, 0.20f, 15000.0f, 0.00f },
    { "Dotted 8th", 2, 375.0f, 0.38f, 0.30f, 0.14f, true, 8, 0.32f, 0.20f, 0.38f, 0.58f, 0.20f, 9200.0f, 0.02f },
    { "Bright Dotted", 2, 375.0f, 0.58f, 0.36f, 0.46f, true, 8, 0.32f, 0.20f, 0.38f, 0.58f, 0.20f, 12800.0f, 0.16f },
    { "Worn Tape", 0, 420.0f, 0.46f, 0.34f, 0.36f, false, 8, 0.84f, 0.64f, 0.94f, 0.58f, 0.20f, 7200.0f, 0.12f },
    { "Tape Wash", 0, 640.0f, 0.64f, 0.40f, 0.66f, false, 8, 0.44f, 0.22f, 0.55f, 0.58f, 0.20f, 7200.0f, 0.12f },
    { "Dark Analog Lead", 1, 340.0f, 0.38f, 0.40f, 0.08f, false, 8, 0.32f, 0.20f, 0.38f, 0.14f, 0.74f, 7200.0f, 0.12f },
    { "Ambient Swell", 1, 820.0f, 0.74f, 0.46f, 0.84f, false, 8, 0.32f, 0.20f, 0.38f, 0.28f, 0.16f, 7200.0f, 0.12f },
    { "Clean Ping-Pong", 2, 280.0f, 0.52f, 0.34f, 1.00f, false, 8, 0.32f, 0.20f, 0.38f, 0.58f, 0.20f, 14500.0f, 0.00f },
};

} // namespace guitar_delay
