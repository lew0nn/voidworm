#pragma once

#include <JuceHeader.h>
#include "OutputTone.h"
#include "FinalLimiter.h"
#include "InputNoiseGate.h"
#include "OversamplingSystem.h"
#include "ReactorRack.h"
#include "ReactorCharacter.h"
#include "RealtimeDiagnostics.h"
#include "SourceAnalyzer.h"
#include "TearProcessor.h"
#include "WeldProcessor.h"

namespace voidworm
{
struct Parameters
{
    float breach = 0.35f;
    float tear = 0.04f;
    float rot = 0.42f;
    float driveDb = 4.0f;
    float overload = 0.35f;
    float mix = 0.72f;
    float range = 0.92f;
    float lowDb = 0.0f;
    float midDb = 0.0f;
    float highDb = 0.0f;
    float outputDb = -4.0f;
    float weld = 0.30f;
    bool limiterEnabled = true;
    float limiterThresholdDb = -3.0f;
    float limiterCeilingDb = -0.8f;
    bool gateEnabled = true;
    float gateThresholdDb = -50.0f;
    bool surge = false;
    int oversampleFactor = 4;
    bool hqMode = true;
    std::array<bool, 4> reactorEnabled { true, true, true, true };
    std::array<float, 4> reactorAmounts { 1.0f, 1.0f, 1.0f, 1.0f };
    ReactorCharacterSettings reactorCharacter;
    int reactorSolo = 0;
    ReactorEqSettings massEq = massEqDefaults();
    ReactorEqSettings furnaceEq = furnaceEqDefaults();
    ReactorEqSettings arcEq = arcEqDefaults();
    ReactorEqSettings feedbackEq = feedbackEqDefaults();
};

#if VOIDWORM_ENABLE_DIAGNOSTICS
struct EngineDiagnostics
{
    StageMetrics input;
    StageMetrics afterDrive;
    ReactorDiagnostics reactor;
    StageMetrics afterTear;
    StageMetrics afterWeld;
    StageMetrics afterMix;
    StageMetrics masterEq;
    StageMetrics afterOutputGain;
    StageMetrics afterLimiter;
    StageMetrics finalOutput;
    float outputFilterStateMaximum = 0.0f;
    DspFaultCounters faults;
};
#endif

class VoidEngine
{
public:
    void prepare (double newSampleRate, int maximumBlockSize, int channels);
    void reset() noexcept;
    void setTargets (const Parameters& parameters) noexcept;
    void beginPresetTransition (const Parameters& parameters) noexcept;
    void process (juce::AudioBuffer<float>& buffer) noexcept;
    bool isPresetTransitionActive() const noexcept
    {
        return presetTransitionState != PresetTransitionState::normal;
    }
    uint32_t getPresetCommitCount() const noexcept { return presetCommitCount; }
    SourceFeatures getSourceFeatures() const noexcept { return sourceAnalyzer.getFeatures(); }
#if VOIDWORM_ENABLE_DIAGNOSTICS
    EngineDiagnostics getDiagnostics() const noexcept { return diagnostics; }
#endif
    int getLatencySamples() const noexcept
    {
        return oversampling.getFixedLatencySamples() + finalLimiter.getLatencySamples();
    }

    float getTearActivity() const noexcept { return tearActivity.load(); }
    bool isInputGateMuting() const noexcept { return inputNoiseGate.isMuting(); }
    float getWeldGainReductionDb() const noexcept { return weldGainReductionDb.load (std::memory_order_relaxed); }
    float getLimiterGainReductionDb() const noexcept { return limiterGainReductionDb.load (std::memory_order_relaxed); }
    uint32_t getDspFaultCount() const noexcept { return dspFaultCount.load (std::memory_order_relaxed); }
    DspFaultCounters getDspFaultCounters() const noexcept;
    ReactorActivity getReactorActivity() const noexcept;

private:
    float emergencyLimit (float sample) noexcept;
    uint32_t repairNonFiniteBuffer (juce::AudioBuffer<float>& buffer) noexcept;
    void recordFaults (const DspFaultCounters&) noexcept;
    void recordNonFiniteRepairs (uint32_t count = 1) noexcept;
    void setTargetsImmediately (const Parameters& parameters) noexcept;
    void resetForPresetChange() noexcept;
    void quenchClosedGateState() noexcept;
    float nextPresetTransitionGain() noexcept;
    void processChunk (juce::AudioBuffer<float>& buffer) noexcept;

    static constexpr int sourceAnalysisChunkSize = 32;

    enum class PresetTransitionState { normal, fadeOut, commitPending, fadeIn };

    OutputTone outputTone;
    InputNoiseGate inputNoiseGate;
    SourceAnalyzer sourceAnalyzer;
    OversamplingSystem oversampling;
    ReactorRack reactorRack;
    TearProcessor tearProcessor;
    WeldProcessor weldProcessor;
    FinalLimiter finalLimiter;
    std::unique_ptr<juce::dsp::DryWetMixer<float>> dryWetMixer;

    juce::SmoothedValue<float> breachSmooth;
    juce::SmoothedValue<float> tearSmooth;
    juce::SmoothedValue<float> rotSmooth;
    juce::SmoothedValue<float> driveSmooth;
    juce::SmoothedValue<float> overloadSmooth;
    juce::SmoothedValue<float> mixSmooth;
    juce::SmoothedValue<float> rangeSmooth;
    juce::SmoothedValue<float> lowSmooth;
    juce::SmoothedValue<float> midSmooth;
    juce::SmoothedValue<float> highSmooth;
    juce::SmoothedValue<float> outputSmooth;
    juce::SmoothedValue<float> weldSmooth;
    juce::SmoothedValue<float> limiterThresholdSmooth;
    juce::SmoothedValue<float> limiterCeilingSmooth;
    juce::SmoothedValue<float> gateContainmentSmooth;

    double sampleRate = 44100.0;
    int channelCount = 2;
    int oversampleFactor = 4;
    bool hqMode = true;
    bool limiterEnabled = true;
    std::array<bool, 4> reactorEnabled { true, true, true, true };
    std::array<float, 4> reactorAmounts { 1.0f, 1.0f, 1.0f, 1.0f };
    ReactorCharacterSettings reactorCharacter;
    int reactorSolo = 0;
    ReactorEqSettings massEq = massEqDefaults();
    ReactorEqSettings furnaceEq = furnaceEqDefaults();
    ReactorEqSettings arcEq = arcEqDefaults();
    ReactorEqSettings feedbackEq = feedbackEqDefaults();
    float surgeState = 0.0f;
    float surgeTarget = 0.0f;
    float surgeAttackCoefficient = 0.0f;
    float surgeReleaseCoefficient = 0.0f;
    std::array<float, 2> oversamplingTransitionStart {};
    std::array<float, 2> lastWetSamples {};
    int oversamplingTransitionSamples = 1;
    int oversamplingTransitionRemaining = 0;
    Parameters pendingPreset;
    PresetTransitionState presetTransitionState = PresetTransitionState::normal;
    float presetTransitionGain = 1.0f;
    int presetFadeOutSamples = 1;
    int presetFadeInSamples = 1;
    int presetTransitionSamplesRemaining = 0;
    uint32_t presetCommitCount = 0;
    bool targetsInitialised = false;
    bool gateWasFullyClosed = false;
    bool gateStateQuenched = false;
    std::atomic<float> tearActivity { 0.0f };
    std::atomic<float> weldGainReductionDb { 0.0f };
    std::atomic<float> limiterGainReductionDb { 0.0f };
    std::array<std::atomic<float>, 4> reactorActivity {
        std::atomic<float> { 0.0f }, std::atomic<float> { 0.0f },
        std::atomic<float> { 0.0f }, std::atomic<float> { 0.0f }
    };
    std::atomic<uint32_t> dspFaultCount { 0 };
    std::atomic<uint32_t> preEqFaultCount { 0 };
    std::atomic<uint32_t> feedbackStateFaultCount { 0 };
    std::atomic<uint32_t> filterStateFaultCount { 0 };
    std::atomic<uint32_t> nonFiniteRepairCount { 0 };
    std::atomic<uint32_t> weldStateFaultCount { 0 };
    std::atomic<uint32_t> limiterStateFaultCount { 0 };
#if VOIDWORM_ENABLE_DIAGNOSTICS
    EngineDiagnostics diagnostics;
#endif
};
}
