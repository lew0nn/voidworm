#pragma once

#include <JuceHeader.h>

namespace voidworm
{
struct ReactorCharacterSettings
{
    float massSaturation = 0.5f;
    float massHarmonics = 0.5f;
    float furnaceStarve = 0.5f;
    float furnaceFold = 0.5f;
    float arcXmod = 0.5f;
    float arcFold = 0.5f;
    float feedbackReturn = 0.5f;
    float feedbackDamp = 0.5f;
};

namespace character
{
inline float sanitise (float value) noexcept
{
    return std::isfinite (value) ? juce::jlimit (0.0f, 1.0f, value) : 0.5f;
}

inline ReactorCharacterSettings sanitise (ReactorCharacterSettings settings) noexcept
{
    settings.massSaturation = sanitise (settings.massSaturation);
    settings.massHarmonics = sanitise (settings.massHarmonics);
    settings.furnaceStarve = sanitise (settings.furnaceStarve);
    settings.furnaceFold = sanitise (settings.furnaceFold);
    settings.arcXmod = sanitise (settings.arcXmod);
    settings.arcFold = sanitise (settings.arcFold);
    settings.feedbackReturn = sanitise (settings.feedbackReturn);
    settings.feedbackDamp = sanitise (settings.feedbackDamp);
    return settings;
}

inline float neutralScale (float value, float maximumScale) noexcept
{
    const auto safe = sanitise (value);
    if (safe <= 0.5f)
        return safe * 2.0f;
    return 1.0f + (safe - 0.5f) * 2.0f * (juce::jmax (1.0f, maximumScale) - 1.0f);
}

inline float massDrive (float baseDrive, float saturation) noexcept
{
    return 1.0f + juce::jmax (0.0f, baseDrive - 1.0f) * neutralScale (saturation, 1.55f);
}

inline float massHarmonicBlend (float baseBlend, float harmonics) noexcept
{
    return juce::jlimit (0.0f, 0.88f, baseBlend * neutralScale (harmonics, 1.35f));
}

inline float furnaceStarvation (float baseStarvation, float starve) noexcept
{
    return juce::jlimit (0.0f, 0.95f, baseStarvation * neutralScale (starve, 1.30f));
}

inline float furnaceFoldBlend (float baseBlend, float fold) noexcept
{
    return juce::jlimit (0.0f, 0.68f, baseBlend * neutralScale (fold, 1.42f));
}

inline float arcCrossAmount (float baseCrossAmount, float xmod) noexcept
{
    return juce::jlimit (0.0f, 1.70f, baseCrossAmount * neutralScale (xmod, 1.45f));
}

inline float arcFoldAmount (float baseFoldAmount, float fold) noexcept
{
    return juce::jlimit (0.0f, 0.92f, baseFoldAmount * neutralScale (fold, 1.18f));
}

inline float feedbackReturnAmount (float baseMaximum, float activation, float returnControl) noexcept
{
    return juce::jlimit (0.0f, 0.62f,
        baseMaximum * juce::jlimit (0.0f, 1.0f, activation) * neutralScale (returnControl, 1.16f));
}

inline float feedbackDampingCutoff (float damp) noexcept
{
    const auto safe = sanitise (damp);
    if (safe <= 0.5f)
        return 12000.0f * std::pow (4200.0f / 12000.0f, safe * 2.0f);
    return 4200.0f * std::pow (1200.0f / 4200.0f, (safe - 0.5f) * 2.0f);
}
}
}
