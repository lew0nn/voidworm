#pragma once

#include <JuceHeader.h>
#include "Dynamics.h"
#include "ReactorPreEq.h"
#include "ReactorCharacter.h"
#include "RealtimeDiagnostics.h"
#include "SourceAnalyzer.h"

namespace voidworm
{
class FurnaceChain
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept;
    void quench() noexcept;
    void process (juce::dsp::AudioBlock<float>& block, ReactorEqSettings eq,
                  const SourceFeatures& features,
                  float rot, float overload, float breach, float surge,
                  float starve, float fold) noexcept;
    DspFaultCounters getAndClearFaultCounters() noexcept;
#if VOIDWORM_ENABLE_DIAGNOSTICS
    float getMaximumSagEnvelope() const noexcept { return maximumSagEnvelope; }
    float getMinimumSupply() const noexcept { return minimumSupply; }
    float getMaximumFilterStateMagnitude() const noexcept { return maximumFilterStateMagnitude; }
    void poisonStateForTest() noexcept { states[0].cleanup = std::numeric_limits<float>::infinity(); }
#endif

private:
    struct State
    {
        float preEmphasisLow = 0.0f;
        float intermediateLow = 0.0f;
        float cleanup = 0.0f;
        float sagEnvelope = 0.0f;
        float dcInput = 0.0f;
        float dcOutput = 0.0f;
    };

    static float asymmetricClip (float input) noexcept;
    static float reflectFold (float input) noexcept;
    std::array<State, 2> states {};
    Dynamics compressor;
    ReactorPreEq preEq;
    float preEmphasisCoefficient = 0.0f;
    float intermediateCoefficient = 0.0f;
    float cleanupCoefficient = 0.0f;
    float sagAttack = 0.0f;
    float sagRelease = 0.0f;
    juce::SmoothedValue<float> starveSmooth;
    juce::SmoothedValue<float> foldSmooth;
    uint32_t filterStateFaultCount = 0;
#if VOIDWORM_ENABLE_DIAGNOSTICS
    float maximumSagEnvelope = 0.0f;
    float minimumSupply = 1.0f;
    float maximumFilterStateMagnitude = 0.0f;
#endif
};
}
