#pragma once

#include <algorithm>
#include <cmath>

namespace voidworm::ui
{
inline float advanceFooterAnimationPhase (float phase) noexcept
{
    return std::fmod (phase + 0.0065f, 1.0f);
}

inline float footerStripeIllumination (int stripe, float phase, int stripeCount = 16) noexcept
{
    const auto position = phase * static_cast<float> (stripeCount);
    auto distance = std::abs (static_cast<float> (stripe) - position);
    distance = std::min (distance, static_cast<float> (stripeCount) - distance);
    return std::exp (-1.45f * distance * distance);
}
}
