#include "Dynamics.h"
#include "FastMath.h"

namespace voidworm
{
void Dynamics::prepare (double newSampleRate) noexcept
{
    sampleRate = juce::jmax (1.0, newSampleRate);
    parametersInitialised = false;
    setParameters (-12.0f, 4.0f, 8.0f, 90.0f);
    reset();
}

void Dynamics::reset() noexcept
{
    states = {};
    for (auto& state : states)
        state.gain = 1.0f;
}

void Dynamics::setParameters (float thresholdDb, float newRatio, float attackMs, float releaseMs) noexcept
{
    thresholdDb = juce::jlimit (-40.0f, 0.0f, thresholdDb);
    newRatio = juce::jlimit (1.0f, 20.0f, newRatio);
    attackMs = juce::jmax (0.1f, attackMs);
    releaseMs = juce::jmax (1.0f, releaseMs);
    if (parametersInitialised && thresholdDb == lastThresholdDb && newRatio == lastRatio
        && attackMs == lastAttackMs && releaseMs == lastReleaseMs)
        return;

    lastThresholdDb = thresholdDb;
    lastRatio = newRatio;
    lastAttackMs = attackMs;
    lastReleaseMs = releaseMs;
    parametersInitialised = true;
    threshold = juce::Decibels::decibelsToGain (thresholdDb);
    ratio = newRatio;
    log2Threshold = std::log2 (threshold);
    slope = 1.0f - 1.0f / ratio;
    attackCoefficient = std::exp (-1.0f / (0.001f * attackMs * static_cast<float> (sampleRate)));
    releaseCoefficient = std::exp (-1.0f / (0.001f * releaseMs * static_cast<float> (sampleRate)));
}

float Dynamics::updateGain (State& state, float magnitude) noexcept
{
    if (! fastmath::isFinite (magnitude) || ! fastmath::isFinite (state.envelope) || ! fastmath::isFinite (state.gain))
    {
        state = {};
        state.gain = 1.0f;
        magnitude = 0.0f;
    }
    const auto detectorCoefficient = magnitude > state.envelope ? attackCoefficient : releaseCoefficient;
    state.envelope = detectorCoefficient * state.envelope + (1.0f - detectorCoefficient) * magnitude;

    auto targetGain = 1.0f;
    /* Above threshold the gain is 10^((out - in) / 20 dB), and out - in
       reduces to (threshold - in)(1 - 1/ratio), so the gain is
       (threshold / envelope)^(1 - 1/ratio): one log and one exp in base 2,
       with the threshold's log held from setParameters. It was two log10s
       (one of a constant) and a pow per sample, in five compressors at the
       oversampled rate. FastMath's versions hold the gain to ~0.0001 dB. */
    if (state.envelope > threshold)
        targetGain = fastmath::exp2 (slope * (log2Threshold - fastmath::log2 (state.envelope)));
    const auto gainCoefficient = targetGain < state.gain ? attackCoefficient : releaseCoefficient;
    state.gain = gainCoefficient * state.gain + (1.0f - gainCoefficient) * targetGain;
    return state.gain;
}

float Dynamics::processSample (int channel, float input) noexcept
{
    auto& state = states[static_cast<size_t> (juce::jlimit (0, 1, channel))];
    return input * updateGain (state, std::abs (input));
}

float Dynamics::processLinkedGain (float detectorMagnitude) noexcept
{
    return updateGain (states[0], std::abs (detectorMagnitude));
}
}
