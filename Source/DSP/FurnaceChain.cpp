#include "FurnaceChain.h"

namespace voidworm
{
namespace
{
float onePoleCoefficient (double sampleRate, float frequency) noexcept
{
    return 1.0f - std::exp (-juce::MathConstants<float>::twoPi * frequency
                            / static_cast<float> (sampleRate));
}

float envelopeCoefficient (double sampleRate, float timeMs) noexcept
{
    return std::exp (-1.0f / (0.001f * timeMs * static_cast<float> (sampleRate)));
}
}

void FurnaceChain::prepare (double sampleRate) noexcept
{
    const auto safeRate = juce::jmax (1.0, sampleRate);
    // 720/3.3k are circuit-character stages. The cleanup ceiling is deliberately
    // broader so the exposed FURNACE pre-EQ retains authority above the old 7.8k limit.
    preEmphasisCoefficient = onePoleCoefficient (safeRate, 720.0f);
    intermediateCoefficient = onePoleCoefficient (safeRate, 3300.0f);
    cleanupCoefficient = onePoleCoefficient (safeRate, 14000.0f);
    sagAttack = envelopeCoefficient (safeRate, 12.0f);
    sagRelease = envelopeCoefficient (safeRate, 145.0f);
    starveSmooth.reset (safeRate, 0.025);
    foldSmooth.reset (safeRate, 0.020);
    compressor.prepare (safeRate);
    preEq.prepare (safeRate);
    reset();
}

void FurnaceChain::reset() noexcept
{
    quench();
    starveSmooth.setCurrentAndTargetValue (0.5f);
    foldSmooth.setCurrentAndTargetValue (0.5f);
}

void FurnaceChain::quench() noexcept
{
    states = {};
    compressor.reset();
    preEq.reset();
    filterStateFaultCount = 0;
#if VOIDWORM_ENABLE_DIAGNOSTICS
    maximumSagEnvelope = 0.0f;
    minimumSupply = 1.0f;
    maximumFilterStateMagnitude = 0.0f;
#endif
}

DspFaultCounters FurnaceChain::getAndClearFaultCounters() noexcept
{
    DspFaultCounters counters;
    counters.preEqFaultCount = preEq.getAndClearDspFaultCount();
    counters.filterStateFaultCount = filterStateFaultCount;
    filterStateFaultCount = 0;
    return counters;
}

float FurnaceChain::asymmetricClip (float input) noexcept
{
    const auto magnitude = std::abs (input);
    const auto knee = input >= 0.0f ? 0.44f : 0.76f;
    return input / (1.0f + knee * magnitude + 0.08f * magnitude * magnitude);
}

float FurnaceChain::reflectFold (float input) noexcept
{
    auto wrapped = std::fmod (input + 1.0f, 4.0f);
    if (wrapped < 0.0f)
        wrapped += 4.0f;
    return wrapped <= 2.0f ? wrapped - 1.0f : 3.0f - wrapped;
}

void FurnaceChain::process (juce::dsp::AudioBlock<float>& block, ReactorEqSettings eq,
                            const SourceFeatures& features,
                            float rot, float overload, float breach, float surge,
                            float starve, float fold) noexcept
{
#if VOIDWORM_ENABLE_DIAGNOSTICS
    maximumSagEnvelope = 0.0f;
    minimumSupply = 1.0f;
    maximumFilterStateMagnitude = 0.0f;
#endif
    preEq.process (block, eq);
    starveSmooth.setTargetValue (character::sanitise (starve));
    foldSmooth.setTargetValue (character::sanitise (fold));
    const auto baseStarvation = juce::jlimit (0.0f, 0.90f,
        0.16f + 0.44f * rot + 0.18f * overload + 0.08f * features.midRatio
        + 0.08f * breach * features.lowRatio + 0.12f * surge);
    const auto baseDrive = 1.25f + 3.7f * rot + 0.28f * overload
                         + 0.35f * features.midRatio + 0.55f * surge;
    const auto baseFoldBlend = juce::jlimit (0.0f, 0.48f, (rot - 0.52f) * 0.88f);

    compressor.setParameters (-10.0f - 9.0f * overload, 6.0f + 4.0f * overload,
                              5.0f - 2.0f * overload, 82.0f - 30.0f * overload);
    for (size_t sample = 0; sample < block.getNumSamples(); ++sample)
    {
        const auto starvation = character::furnaceStarvation (
            baseStarvation, starveSmooth.getNextValue());
        const auto foldBlend = character::furnaceFoldBlend (
            baseFoldBlend, foldSmooth.getNextValue());
        for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
        {
            auto& state = states[juce::jmin (channel, states.size() - 1)];
            auto* output = block.getChannelPointer (channel);
            const auto input = std::isfinite (output[sample]) ? output[sample] : 0.0f;
            constexpr auto stateLimit = 32.0f;
            const auto validState = std::isfinite (state.preEmphasisLow)
                && std::isfinite (state.intermediateLow) && std::isfinite (state.cleanup)
                && std::isfinite (state.sagEnvelope) && std::isfinite (state.dcInput)
                && std::isfinite (state.dcOutput) && std::abs (state.preEmphasisLow) <= stateLimit
                && std::abs (state.intermediateLow) <= stateLimit && std::abs (state.cleanup) <= stateLimit
                && std::abs (state.sagEnvelope) <= stateLimit && std::abs (state.dcInput) <= stateLimit
                && std::abs (state.dcOutput) <= stateLimit;
            if (! validState || ! std::isfinite (output[sample]))
            {
                state = {};
                output[sample] = input;
                ++filterStateFaultCount;
                continue;
            }
            state.preEmphasisLow += preEmphasisCoefficient * (input - state.preEmphasisLow);
            const auto preEmphasised = input + 0.72f * state.preEmphasisLow;

            const auto magnitude = std::abs (preEmphasised);
            const auto sagCoefficient = magnitude > state.sagEnvelope ? sagAttack : sagRelease;
            state.sagEnvelope = sagCoefficient * state.sagEnvelope + (1.0f - sagCoefficient) * magnitude;
            const auto dynamicSupply = juce::jlimit (0.18f, 1.0f,
                1.0f - starvation * state.sagEnvelope * (0.72f + 0.55f * features.sustain));
            const auto dynamicDrive = juce::jlimit (1.0f, 12.0f, baseDrive / (0.42f + 0.58f * dynamicSupply));
            const auto bias = (0.035f + 0.11f * breach) * state.sagEnvelope * (1.0f - dynamicSupply);
            const auto starved = asymmetricClip (preEmphasised * dynamicDrive + bias) * dynamicSupply;

            state.intermediateLow += intermediateCoefficient * (starved - state.intermediateLow);
            const auto detailed = starved + 0.42f * (starved - state.intermediateLow);
            const auto fuzzed = asymmetricClip (detailed * (1.15f + 2.1f * rot) - 0.045f * state.sagEnvelope);
            const auto folded = reflectFold (fuzzed * (1.0f + 2.4f * foldBlend));
            const auto nonlinear = juce::jmap (foldBlend, fuzzed, folded);
            state.cleanup += cleanupCoefficient * (nonlinear - state.cleanup);
            const auto dcBlocked = state.cleanup - state.dcInput + 0.997f * state.dcOutput;
            state.dcInput = state.cleanup;
            state.dcOutput = dcBlocked;
            output[sample] = juce::jlimit (-2.5f, 2.5f,
                compressor.processSample (static_cast<int> (channel), dcBlocked));
#if VOIDWORM_ENABLE_DIAGNOSTICS
            maximumSagEnvelope = juce::jmax (maximumSagEnvelope, state.sagEnvelope);
            minimumSupply = juce::jmin (minimumSupply, dynamicSupply);
            maximumFilterStateMagnitude = juce::jmax (maximumFilterStateMagnitude,
                juce::jmax (std::abs (state.preEmphasisLow),
                    juce::jmax (std::abs (state.intermediateLow),
                        juce::jmax (std::abs (state.cleanup), std::abs (state.dcOutput)))));
#endif
        }
    }
}
}
