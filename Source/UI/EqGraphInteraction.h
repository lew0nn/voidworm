#pragma once

#include <JuceHeader.h>
#include "../DSP/ReactorPreEq.h"

namespace voidworm::ui
{
inline constexpr float graphMinimumFrequency = 20.0f;
inline constexpr float graphMaximumFrequency = 20000.0f;
inline constexpr float reactorGraphInset = 3.5f;
inline constexpr float reactorGraphDbScale = 0.36f / 12.0f;

inline float frequencyToProportion (float frequency) noexcept
{
    const auto safe = std::isfinite (frequency) ? frequency : graphMinimumFrequency;
    return juce::jlimit (0.0f, 1.0f,
        std::log10 (juce::jmax (graphMinimumFrequency, safe) / graphMinimumFrequency)
            / std::log10 (graphMaximumFrequency / graphMinimumFrequency));
}

inline float proportionToFrequency (float proportion) noexcept
{
    const auto safe = std::isfinite (proportion) ? proportion : 0.0f;
    return graphMinimumFrequency * std::pow (graphMaximumFrequency / graphMinimumFrequency,
                                               juce::jlimit (0.0f, 1.0f, safe));
}

inline float frequencyToX (juce::Rectangle<float> plot, float frequency) noexcept
{
    return juce::jlimit (plot.getX() + reactorGraphInset, plot.getRight() - reactorGraphInset,
                         plot.getX() + frequencyToProportion (frequency) * plot.getWidth());
}

inline float responseDbToY (juce::Rectangle<float> plot, float responseDb) noexcept
{
    const auto safeDb = std::isfinite (responseDb) ? responseDb : 0.0f;
    return juce::jlimit (plot.getY() + reactorGraphInset, plot.getBottom() - reactorGraphInset,
                         plot.getCentreY() - safeDb * plot.getHeight() * reactorGraphDbScale);
}

inline float yToGainDb (juce::Rectangle<float> plot, float y) noexcept
{
    const auto proportion = juce::jlimit (0.0f, 1.0f, (y - plot.getY()) / plot.getHeight());
    return juce::jmap (proportion, 12.0f, -12.0f);
}

struct ReactorEqGraphModel
{
    ReactorEqGraphModel (double processingRate, ReactorEqSettings parameterState,
                         juce::Rectangle<float> plotBounds) noexcept
        : sampleRate (std::isfinite (processingRate) ? juce::jmax (1.0, processingRate) : 44100.0),
          settings (ReactorPreEq::sanitise (sampleRate, parameterState)), plot (plotBounds)
    {
    }

    float evaluateResponseDb (float frequency) const noexcept
    {
        const auto magnitude = ReactorPreEq::getResponseMagnitude (sampleRate, frequency, settings);
        return juce::Decibels::gainToDecibels (juce::jmax (0.0001f, magnitude));
    }

    juce::Point<float> responsePoint (float frequency) const noexcept
    {
        return { frequencyToX (plot, frequency), responseDbToY (plot, evaluateResponseDb (frequency)) };
    }

    std::array<float, 4> nodeFrequencies() const noexcept
    {
        return { settings.hp, settings.focusFrequency, settings.focus2Frequency, settings.lp };
    }

    std::array<juce::Point<float>, 4> nodePositions() const noexcept
    {
        const auto frequencies = nodeFrequencies();
        return { responsePoint (frequencies[0]), responsePoint (frequencies[1]),
                 responsePoint (frequencies[2]), responsePoint (frequencies[3]) };
    }

    double sampleRate;
    ReactorEqSettings settings;
    juce::Rectangle<float> plot;
};

template <size_t nodeCount>
inline int hitTestEqNode (juce::Point<float> point,
                          const std::array<juce::Point<float>, nodeCount>& nodes,
                          float radius) noexcept
{
    auto hitNode = -1;
    auto closestDistanceSquared = radius * radius;
    for (int node = 0; node < static_cast<int> (nodes.size()); ++node)
    {
        const auto distanceSquared = point.getDistanceSquaredFrom (nodes[static_cast<size_t> (node)]);
        // First node wins an exact tie, making overlap selection deterministic.
        if (distanceSquared <= radius * radius
            && (hitNode < 0 || distanceSquared < closestDistanceSquared))
        {
            hitNode = node;
            closestDistanceSquared = distanceSquared;
        }
    }
    return hitNode;
}
}
