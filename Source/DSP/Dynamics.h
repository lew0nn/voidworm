#pragma once

#include <JuceHeader.h>

namespace voidworm
{
class Dynamics
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept;
    void setParameters (float thresholdDb, float ratio, float attackMs, float releaseMs) noexcept;
    float processSample (int channel, float input) noexcept;
    float processLinkedGain (float detectorMagnitude) noexcept;

private:
    struct State
    {
        float envelope = 0.0f;
        float gain = 1.0f;
    };

    float updateGain (State& state, float magnitude) noexcept;

    std::array<State, 2> states {};
    double sampleRate = 44100.0;
    float threshold = 0.25f;
    float ratio = 4.0f;
    float attackCoefficient = 0.0f;
    float releaseCoefficient = 0.0f;
    float lastThresholdDb = 0.0f;
    float lastRatio = 0.0f;
    float lastAttackMs = 0.0f;
    float lastReleaseMs = 0.0f;
    bool parametersInitialised = false;
};
}
