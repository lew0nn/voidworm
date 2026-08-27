#pragma once

#include <JuceHeader.h>
#include "Dynamics.h"
#include "ReactorPreEq.h"
#include "ReactorCharacter.h"
#include "RealtimeDiagnostics.h"
#include "SourceAnalyzer.h"

namespace voidworm
{
class FeedbackChain
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept;
    void quench() noexcept;
    void process (juce::dsp::AudioBlock<float>& block, ReactorEqSettings eq,
                  const SourceFeatures& features,
                  float rot, float overload, float surge, float activation,
                  float returnControl, float damp) noexcept;
    DspFaultCounters getAndClearFaultCounters() noexcept;
#if VOIDWORM_ENABLE_DIAGNOSTICS
    float getMaximumStateMagnitude() const noexcept { return maximumStateMagnitude; }
    void poisonStateForTest() noexcept { states[0].recursive = std::numeric_limits<float>::quiet_NaN(); }
#endif

private:
    struct State
    {
        float recursive = 0.0f;
        float damping = 0.0f;
        float dcInput = 0.0f;
        float dcOutput = 0.0f;
    };

    static float boundedShape (float input) noexcept;
    std::array<State, 2> states {};
    Dynamics safetyDynamics;
    ReactorPreEq preEq;
    double processingRate = 44100.0;
    juce::SmoothedValue<float> returnSmooth;
    juce::SmoothedValue<float> dampSmooth;
    uint32_t feedbackStateFaultCount = 0;
#if VOIDWORM_ENABLE_DIAGNOSTICS
    float maximumStateMagnitude = 0.0f;
#endif
};
}
