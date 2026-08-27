#pragma once

#include <JuceHeader.h>

namespace voidworm
{
class InputNoiseGate
{
public:
    void prepare (double newSampleRate) noexcept;
    void reset() noexcept;
    void setParameters (bool shouldBeEnabled, float thresholdDb) noexcept;
    void process (juce::AudioBuffer<float>& buffer) noexcept;
    bool isMuting() const noexcept { return muting.load (std::memory_order_relaxed); }
    bool isFullyClosed() const noexcept { return enabled && effectiveGain <= 1.0e-5f; }

private:
    static float smoothingCoefficient (double sampleRate, float milliseconds) noexcept;

    double sampleRate = 44100.0;
    float detectorAttackCoefficient = 0.0f;
    float detectorReleaseCoefficient = 0.0f;
    float gateAttackCoefficient = 0.0f;
    float gateReleaseCoefficient = 0.0f;
    float bypassCoefficient = 0.0f;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> thresholdDbSmooth;
    float targetThresholdDb = -50.0f;
    float detectorEnvelope = 0.0f;
    float gain = 1.0f;
    float enabledMix = 1.0f;
    float effectiveGain = 1.0f;
    int holdSamples = 1;
    int holdRemaining = 0;
    bool enabled = true;
    bool gateOpen = false;
    std::atomic<bool> muting { false };
};
}
