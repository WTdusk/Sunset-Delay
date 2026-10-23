#pragma once

#include <algorithm>

namespace guitar_delay
{

inline constexpr int kNumDivisions = 13;
inline constexpr float kMinDelayMs = 40.0f;
inline constexpr float kMaxDelayMs = 4000.0f;

// Beats, in quarter notes, for 1/1, 1/2, 1/2., 1/2T, 1/4, 1/4., 1/4T,
// 1/8, 1/8., 1/8T, 1/16, 1/16., 1/16T.
inline constexpr float kDivisionBeats[kNumDivisions] = {
    4.0f, 2.0f, 3.0f, 4.0f / 3.0f,
    1.0f, 1.5f, 2.0f / 3.0f,
    0.5f, 0.75f, 1.0f / 3.0f,
    0.25f, 0.375f, 1.0f / 6.0f
};

inline float delayMsForDivision (float bpm, int division) noexcept
{
    const int index = std::clamp (division, 0, kNumDivisions - 1);
    const float safeBpm = std::clamp (bpm, 20.0f, 400.0f);
    return (60.0f / safeBpm) * kDivisionBeats[index] * 1000.0f;
}

inline float clampDelayMs (float ms) noexcept
{
    return std::clamp (ms, kMinDelayMs, kMaxDelayMs);
}

} // namespace guitar_delay
