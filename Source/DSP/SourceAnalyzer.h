#pragma once

#include <JuceHeader.h>

namespace voidworm
{
struct SourceFeatures
{
    float peak = 0.0f;
    float rms = 0.0f;
    float fastEnvelope = 0.0f;
    float slowEnvelope = 0.0f;
    float transient = 0.0f;
    float sustain = 0.0f;
    float lowLevel = 0.0f;
    float midLevel = 0.0f;
    float highLevel = 0.0f;
    float lowRatio = 0.0f;
    float midRatio = 0.0f;
    float highRatio = 0.0f;
    float brightness = 0.0f;
};

class SourceAnalyzer
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept;
    const SourceFeatures& processSample (float left, float right) noexcept;
    void processSampleState (float left, float right) noexcept;
    const SourceFeatures& finaliseFeatures() noexcept;
    const SourceFeatures& getFeatures() const noexcept { return features; }

private:
    static float coefficient (double sampleRate, float timeMs) noexcept;
    static float follow (float current, float target, float attack, float release) noexcept;
    static float bounded (float value) noexcept;

    SourceFeatures features;
    std::array<float, 2> lowState {};
    std::array<float, 2> midState {};
    float rmsSquared = 0.0f;
    float fastEnvelopeState = 0.0f;
    float slowEnvelopeState = 0.0f;
    float lowSquared = 0.0f;
    float midSquared = 0.0f;
    float highSquared = 0.0f;
    float fastAttack = 0.0f;
    float fastRelease = 0.0f;
    float slowAttack = 0.0f;
    float slowRelease = 0.0f;
    float energyCoefficient = 0.0f;
    float lowCoefficient = 0.0f;
    float midCoefficient = 0.0f;
    float lastLinkedMagnitude = 0.0f;
};
}
