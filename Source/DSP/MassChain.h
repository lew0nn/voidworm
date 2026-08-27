#pragma once

#include <JuceHeader.h>
#include "Dynamics.h"
#include "ReactorPreEq.h"
#include "ReactorCharacter.h"
#include "SourceAnalyzer.h"

namespace voidworm
{
class MassChain
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept;
    void quench() noexcept;
    void process (juce::dsp::AudioBlock<float>& block, ReactorEqSettings eq,
                  const SourceFeatures& features,
                  float rot, float overload, float surge,
                  float saturation, float harmonics) noexcept;
    uint32_t getAndClearDspFaultCount() noexcept { return preEq.getAndClearDspFaultCount(); }

private:
    struct State
    {
        float low = 0.0f;
        float lowMid = 0.0f;
        float cleanup = 0.0f;
        float dcInput = 0.0f;
        float dcOutput = 0.0f;
    };

    static float asymmetricClip (float input) noexcept;
    std::array<State, 2> states {};
    Dynamics compressor;
    ReactorPreEq preEq;
    float lowCoefficient = 0.0f;
    float lowMidCoefficient = 0.0f;
    float cleanupCoefficient = 0.0f;
    juce::SmoothedValue<float> saturationSmooth;
    juce::SmoothedValue<float> harmonicsSmooth;
};
}
