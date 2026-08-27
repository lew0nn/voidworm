#pragma once

#include <JuceHeader.h>
#include "ArcChain.h"
#include "Dynamics.h"
#include "FurnaceChain.h"
#include "FeedbackChain.h"
#include "MassChain.h"
#include "NonlinearStage.h"
#include "ReactorCharacter.h"
#include "RealtimeDiagnostics.h"

namespace voidworm
{
struct ReactorActivity
{
    float mass = 0.0f;
    float furnace = 0.0f;
    float arc = 0.0f;
    float feedback = 0.0f;
};

#if VOIDWORM_ENABLE_DIAGNOSTICS
struct ReactorDiagnostics
{
    StageMetrics mass, furnace, arc, feedback;
    StageMetrics sum, bus, weldSaturation;
    float feedbackStateMaximum = 0.0f;
    float furnaceSagMaximum = 0.0f;
    float furnaceSupplyMinimum = 1.0f;
    float filterStateMaximum = 0.0f;
    SourceFeatures featureMaxima;
};
#endif

class ReactorRack
{
public:
    void prepare (double hostSampleRate, int maximumBlockSize, int channels);
    void reset() noexcept;
    void resetForPresetChange() noexcept;
    ReactorActivity process (juce::dsp::AudioBlock<float>& block, int oversamplingFactor,
                             const SourceFeatures& features, float rot, float overload,
                             float breach, float surge, float weld, const std::array<bool, 4>& enabled,
                             const std::array<float, 4>& amounts,
                             const ReactorCharacterSettings& character, int soloTarget,
                             const ReactorEqSettings& massEq,
                             const ReactorEqSettings& furnaceEq, const ReactorEqSettings& arcEq,
                             const ReactorEqSettings& feedbackEq) noexcept;
    DspFaultCounters getAndClearFaultCounters() noexcept;
#if VOIDWORM_ENABLE_DIAGNOSTICS
    void beginDiagnosticsBlock() noexcept { diagnostics = {}; }
    ReactorDiagnostics getDiagnostics() const noexcept { return diagnostics; }
#endif

private:
    struct WeightState
    {
        float mass = 0.60f;
        float furnace = 0.0f;
        float arc = 0.0f;
        float feedback = 0.0f;
    };

    struct RoutingState
    {
        std::array<float, 4> gains { 1.0f, 1.0f, 1.0f, 1.0f };
    };

    static int factorToIndex (int factor) noexcept;
    static float smoothstep (float low, float high, float value) noexcept;

    std::array<MassChain, 4> massChains;
    std::array<FurnaceChain, 4> furnaceChains;
    std::array<ArcChain, 4> arcChains;
    std::array<FeedbackChain, 4> feedbackChains;
    std::array<Dynamics, 4> busDynamics;
    std::array<NonlinearStage, 4> busNonlinearStages;
    std::array<juce::AudioBuffer<float>, 4> massBuffers;
    std::array<juce::AudioBuffer<float>, 4> furnaceBuffers;
    std::array<juce::AudioBuffer<float>, 4> arcBuffers;
    std::array<juce::AudioBuffer<float>, 4> feedbackBuffers;
    std::array<WeightState, 4> weightStates;
    std::array<RoutingState, 4> routingStates;
    std::array<std::array<bool, 4>, 4> pathProcessingActive {};
    std::array<bool, 4> busProcessingActive {};
    std::array<double, 4> processingRates {};
    std::array<float, 4> routingSmoothingCoefficients {};
    int channelCount = 2;
    DspFaultCounters faultCounters;
#if VOIDWORM_ENABLE_DIAGNOSTICS
    ReactorDiagnostics diagnostics;
#endif
};
}
