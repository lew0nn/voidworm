#include "SourceAnalyzer.h"

namespace voidworm
{
float SourceAnalyzer::coefficient (double sampleRate, float timeMs) noexcept
{
    return std::exp (-1.0f / (0.001f * timeMs * static_cast<float> (sampleRate)));
}

float SourceAnalyzer::follow (float current, float target, float attack, float release) noexcept
{
    const auto amount = target > current ? attack : release;
    return amount * current + (1.0f - amount) * target;
}

float SourceAnalyzer::bounded (float value) noexcept
{
    if (! std::isfinite (value) || value <= 0.0f)
        return 0.0f;
    return value / (1.0f + value);
}

void SourceAnalyzer::prepare (double sampleRate) noexcept
{
    const auto safeRate = juce::jmax (1.0, sampleRate);
    fastAttack = coefficient (safeRate, 2.0f);
    fastRelease = coefficient (safeRate, 40.0f);
    slowAttack = coefficient (safeRate, 22.0f);
    slowRelease = coefficient (safeRate, 200.0f);
    energyCoefficient = coefficient (safeRate, 30.0f);
    lowCoefficient = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 200.0f
                                      / static_cast<float> (safeRate));
    midCoefficient = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 2800.0f
                                      / static_cast<float> (safeRate));
    reset();
}

void SourceAnalyzer::reset() noexcept
{
    features = {};
    lowState = {};
    midState = {};
    rmsSquared = lowSquared = midSquared = highSquared = 0.0f;
    fastEnvelopeState = slowEnvelopeState = 0.0f;
    lastLinkedMagnitude = 0.0f;
}

void SourceAnalyzer::processSampleState (float left, float right) noexcept
{
    const auto stateIsFinite = std::isfinite (rmsSquared)
        && std::isfinite (fastEnvelopeState) && std::isfinite (slowEnvelopeState)
        && std::isfinite (lowSquared) && std::isfinite (midSquared) && std::isfinite (highSquared)
        && std::all_of (lowState.begin(), lowState.end(), [] (float value) { return std::isfinite (value); })
        && std::all_of (midState.begin(), midState.end(), [] (float value) { return std::isfinite (value); });
    if (! stateIsFinite)
        reset();

    // Malformed host input must not poison the persistent analyzer state. The
    // generous finite bound is diagnostic protection, not audible clipping.
    left = std::isfinite (left) ? juce::jlimit (-64.0f, 64.0f, left) : 0.0f;
    right = std::isfinite (right) ? juce::jlimit (-64.0f, 64.0f, right) : 0.0f;
    const std::array<float, 2> input { left, right };
    const auto linkedMagnitude = juce::jmax (std::abs (left), std::abs (right));
    lastLinkedMagnitude = linkedMagnitude;
    const auto energy = 0.5f * (left * left + right * right);

    fastEnvelopeState = follow (fastEnvelopeState, linkedMagnitude, fastAttack, fastRelease);
    slowEnvelopeState = follow (slowEnvelopeState, linkedMagnitude, slowAttack, slowRelease);
    rmsSquared = energyCoefficient * rmsSquared + (1.0f - energyCoefficient) * energy;

    auto lowEnergy = 0.0f;
    auto midEnergy = 0.0f;
    auto highEnergy = 0.0f;
    for (size_t channel = 0; channel < input.size(); ++channel)
    {
        lowState[channel] += lowCoefficient * (input[channel] - lowState[channel]);
        midState[channel] += midCoefficient * (input[channel] - midState[channel]);
        const auto low = lowState[channel];
        const auto mid = midState[channel] - low;
        const auto high = input[channel] - midState[channel];
        lowEnergy += 0.5f * low * low;
        midEnergy += 0.5f * mid * mid;
        highEnergy += 0.5f * high * high;
    }

    lowSquared = energyCoefficient * lowSquared + (1.0f - energyCoefficient) * lowEnergy;
    midSquared = energyCoefficient * midSquared + (1.0f - energyCoefficient) * midEnergy;
    highSquared = energyCoefficient * highSquared + (1.0f - energyCoefficient) * highEnergy;
}

const SourceFeatures& SourceAnalyzer::finaliseFeatures() noexcept
{
    const auto lowRoot = std::sqrt (juce::jmax (0.0f, lowSquared));
    const auto midRoot = std::sqrt (juce::jmax (0.0f, midSquared));
    const auto highRoot = std::sqrt (juce::jmax (0.0f, highSquared));
    const auto total = lowRoot + midRoot + highRoot + 1.0e-8f;
    features.peak = bounded (lastLinkedMagnitude);
    features.rms = bounded (std::sqrt (juce::jmax (0.0f, rmsSquared)));
    features.transient = juce::jlimit (0.0f, 1.0f,
        (fastEnvelopeState - slowEnvelopeState) / (slowEnvelopeState + 0.05f));
    features.sustain = bounded (slowEnvelopeState);
    features.lowLevel = bounded (lowRoot);
    features.midLevel = bounded (midRoot);
    features.highLevel = bounded (highRoot);
    features.lowRatio = juce::jlimit (0.0f, 1.0f, lowRoot / total);
    features.midRatio = juce::jlimit (0.0f, 1.0f, midRoot / total);
    features.highRatio = juce::jlimit (0.0f, 1.0f, highRoot / total);
    features.brightness = juce::jlimit (0.0f, 1.0f,
        0.08f * features.lowRatio + 0.50f * features.midRatio + features.highRatio);
    features.fastEnvelope = bounded (fastEnvelopeState);
    features.slowEnvelope = bounded (slowEnvelopeState);
    return features;
}

const SourceFeatures& SourceAnalyzer::processSample (float left, float right) noexcept
{
    processSampleState (left, right);
    return finaliseFeatures();
}
}
