#include "InputNoiseGate.h"

#include <cmath>

namespace voidworm
{
float InputNoiseGate::smoothingCoefficient (double rate, float milliseconds) noexcept
{
    const auto samples = juce::jmax (1.0, rate * static_cast<double> (milliseconds) * 0.001);
    return static_cast<float> (std::exp (std::log (0.001) / samples));
}

void InputNoiseGate::prepare (double newSampleRate) noexcept
{
    sampleRate = juce::jmax (1.0, newSampleRate);
    detectorAttackCoefficient = smoothingCoefficient (sampleRate, 2.0f);
    detectorReleaseCoefficient = smoothingCoefficient (sampleRate, 30.0f);
    gateAttackCoefficient = smoothingCoefficient (sampleRate, 8.0f);
    gateReleaseCoefficient = smoothingCoefficient (sampleRate, 200.0f);
    bypassCoefficient = smoothingCoefficient (sampleRate, 45.0f);
    thresholdDbSmooth.reset (sampleRate, 0.045);
    holdSamples = juce::jmax (1, juce::roundToInt (sampleRate * 0.050));
    reset();
}

void InputNoiseGate::reset() noexcept
{
    detectorEnvelope = 0.0f;
    gain = 0.0f;
    enabledMix = enabled ? 1.0f : 0.0f;
    effectiveGain = enabled ? 0.0f : 1.0f;
    thresholdDbSmooth.setCurrentAndTargetValue (targetThresholdDb);
    holdRemaining = 0;
    gateOpen = false;
    muting.store (false, std::memory_order_relaxed);
}

void InputNoiseGate::setParameters (bool shouldBeEnabled, float thresholdDb) noexcept
{
    enabled = shouldBeEnabled;
    if (! enabled)
        muting.store (false, std::memory_order_relaxed);
    const auto safeThreshold = std::isfinite (thresholdDb)
        ? juce::jlimit (-80.0f, -20.0f, thresholdDb) : -50.0f;
    targetThresholdDb = safeThreshold;
    thresholdDbSmooth.setTargetValue (targetThresholdDb);
}

void InputNoiseGate::process (juce::AudioBuffer<float>& buffer) noexcept
{
    const auto channels = juce::jmin (2, buffer.getNumChannels());
    const auto samples = buffer.getNumSamples();
    if (channels <= 0 || samples <= 0)
        return;

    constexpr auto hysteresisGain = 0.5011872336f; // -6 dB
    auto appliedGain = 1.0f;
    const auto thresholdIsSmoothing = thresholdDbSmooth.isSmoothing();
    const auto stableOpenThreshold = thresholdIsSmoothing
        ? 0.0f : juce::Decibels::decibelsToGain (thresholdDbSmooth.getCurrentValue());

    for (int sampleIndex = 0; sampleIndex < samples; ++sampleIndex)
    {
        auto linkedLevel = 0.0f;
        for (int channel = 0; channel < channels; ++channel)
        {
            const auto sample = buffer.getSample (channel, sampleIndex);
            linkedLevel = juce::jmax (linkedLevel, std::isfinite (sample) ? std::abs (sample) : 0.0f);
        }

        const auto detectorCoefficient = linkedLevel > detectorEnvelope
            ? detectorAttackCoefficient : detectorReleaseCoefficient;
        detectorEnvelope = linkedLevel + detectorCoefficient * (detectorEnvelope - linkedLevel);
        if (! std::isfinite (detectorEnvelope) || detectorEnvelope < 1.0e-12f)
            detectorEnvelope = 0.0f;

        auto currentThresholdDb = thresholdDbSmooth.getNextValue();
        if (! std::isfinite (currentThresholdDb))
        {
            currentThresholdDb = targetThresholdDb;
            thresholdDbSmooth.setCurrentAndTargetValue (targetThresholdDb);
        }
        const auto openThreshold = thresholdIsSmoothing
            ? juce::Decibels::decibelsToGain (currentThresholdDb) : stableOpenThreshold;
        const auto closeThreshold = openThreshold * hysteresisGain;

        if (! gateOpen)
        {
            if (detectorEnvelope >= openThreshold)
            {
                gateOpen = true;
                holdRemaining = holdSamples;
            }
        }
        else if (detectorEnvelope >= closeThreshold)
        {
            holdRemaining = holdSamples;
        }
        else if (holdRemaining > 0)
        {
            --holdRemaining;
        }
        else
        {
            gateOpen = false;
        }

        const auto targetGain = gateOpen ? 1.0f : 0.0f;
        const auto gainCoefficient = targetGain > gain ? gateAttackCoefficient : gateReleaseCoefficient;
        gain = targetGain + gainCoefficient * (gain - targetGain);
        if (! std::isfinite (gain))
            gain = targetGain;
        else if (targetGain == 0.0f && gain < 1.0e-5f)
            gain = 0.0f;
        else if (targetGain == 1.0f && gain > 0.99999f)
            gain = 1.0f;

        const auto targetEnabledMix = enabled ? 1.0f : 0.0f;
        enabledMix = targetEnabledMix + bypassCoefficient * (enabledMix - targetEnabledMix);
        if (! std::isfinite (enabledMix))
            enabledMix = targetEnabledMix;
        else if (targetEnabledMix == 0.0f && enabledMix < 1.0e-5f)
            enabledMix = 0.0f;
        else if (targetEnabledMix == 1.0f && enabledMix > 0.99999f)
            enabledMix = 1.0f;
        appliedGain = 1.0f + enabledMix * (gain - 1.0f);
        effectiveGain = appliedGain;

        for (int channel = 0; channel < channels; ++channel)
        {
            const auto sample = buffer.getSample (channel, sampleIndex);
            buffer.setSample (channel, sampleIndex, std::isfinite (sample) ? sample * appliedGain : 0.0f);
        }
    }

    muting.store (enabled && appliedGain < 0.5f, std::memory_order_relaxed);
}
}
