#pragma once

#include <JuceHeader.h>
#include "Dynamics.h"
#include "ReactorPreEq.h"
#include "ReactorCharacter.h"
#include "SourceAnalyzer.h"

namespace voidworm
{
class ArcChain
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept;
    void quench() noexcept;
    void process (juce::dsp::AudioBlock<float>& block, ReactorEqSettings eq,
                  const SourceFeatures& features,
                  float rot, float overload, float breach, float surge,
                  float xmod, float fold) noexcept;
    uint32_t getAndClearDspFaultCount() noexcept { return preEq.getAndClearDspFaultCount(); }

private:
    struct State
    {
        float low = 0.0f;
        float midLow = 0.0f;
        float cleanup = 0.0f;
        float dcInput = 0.0f;
        float dcOutput = 0.0f;
    };

    static float asymmetric (float input) noexcept;
    static float reflectFold (float input) noexcept;
    std::array<State, 2> states {};
    Dynamics compressor;
    ReactorPreEq preEq;
    float lowCoefficient = 0.0f;
    float midCoefficient = 0.0f;
    float cleanupCoefficient = 0.0f;
    juce::SmoothedValue<float> xmodSmooth;
    juce::SmoothedValue<float> foldSmooth;
};
}
