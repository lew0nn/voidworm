#include "FeedbackChain.h"

namespace voidworm
{
void FeedbackChain::prepare (double sampleRate) noexcept
{
    const auto safeRate = juce::jmax (1.0, sampleRate);
    processingRate = safeRate;
    // Inside-loop damping remains independent of the exposed excitation PRE EQ.
    returnSmooth.reset (safeRate, 0.025);
    dampSmooth.reset (safeRate, 0.025);
    safetyDynamics.prepare (safeRate);
    preEq.prepare (safeRate);
    reset();
}

void FeedbackChain::reset() noexcept
{
    quench();
    returnSmooth.setCurrentAndTargetValue (0.5f);
    dampSmooth.setCurrentAndTargetValue (0.5f);
}

void FeedbackChain::quench() noexcept
{
    states = {};
    safetyDynamics.reset();
    preEq.reset();
    feedbackStateFaultCount = 0;
#if VOIDWORM_ENABLE_DIAGNOSTICS
    maximumStateMagnitude = 0.0f;
#endif
}

DspFaultCounters FeedbackChain::getAndClearFaultCounters() noexcept
{
    DspFaultCounters counters;
    counters.preEqFaultCount = preEq.getAndClearDspFaultCount();
    counters.feedbackStateFaultCount = feedbackStateFaultCount;
    feedbackStateFaultCount = 0;
    return counters;
}

float FeedbackChain::boundedShape (float input) noexcept
{
    const auto magnitude = std::abs (input);
    return input / (1.0f + 0.64f * magnitude + 0.11f * magnitude * magnitude);
}

void FeedbackChain::process (juce::dsp::AudioBlock<float>& block, ReactorEqSettings eq,
                             const SourceFeatures& features,
                             float rot, float overload, float surge, float activation,
                             float returnControl, float damp) noexcept
{
#if VOIDWORM_ENABLE_DIAGNOSTICS
    maximumStateMagnitude = 0.0f;
#endif
    preEq.process (block, eq);
    returnSmooth.setTargetValue (character::sanitise (returnControl));
    dampSmooth.setTargetValue (character::sanitise (damp));
    const auto maximum = juce::jlimit (0.0f, 0.62f, 0.34f + 0.09f * overload + 0.18f * surge);
    const auto inputDrive = 1.0f + 1.4f * rot + 0.22f * overload;
    safetyDynamics.setParameters (-14.0f - 7.0f * overload, 9.0f + 7.0f * overload,
                                  0.8f, 42.0f);

    const auto returnIsSmoothing = returnSmooth.isSmoothing();
    const auto dampIsSmoothing = dampSmooth.isSmoothing();
    const auto stableFeedbackAmount = returnIsSmoothing ? 0.0f
        : character::feedbackReturnAmount (maximum, activation, returnSmooth.getCurrentValue());
    const auto stableCutoff = dampIsSmoothing ? 0.0f
        : juce::jmin (character::feedbackDampingCutoff (dampSmooth.getCurrentValue()),
                      static_cast<float> (processingRate * 0.45));
    const auto stableDampingCoefficient = dampIsSmoothing ? 0.0f : 1.0f - std::exp (
        -juce::MathConstants<float>::twoPi * stableCutoff / static_cast<float> (processingRate));

    for (size_t sample = 0; sample < block.getNumSamples(); ++sample)
    {
        const auto feedbackAmount = returnIsSmoothing
            ? character::feedbackReturnAmount (maximum, activation, returnSmooth.getNextValue())
            : stableFeedbackAmount;
        auto dampingCoefficient = stableDampingCoefficient;
        if (dampIsSmoothing)
        {
            const auto cutoff = juce::jmin (character::feedbackDampingCutoff (dampSmooth.getNextValue()),
                                            static_cast<float> (processingRate * 0.45));
            dampingCoefficient = 1.0f - std::exp (
                -juce::MathConstants<float>::twoPi * cutoff / static_cast<float> (processingRate));
        }
        for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
        {
            auto& state = states[juce::jmin (channel, states.size() - 1)];
            auto* output = block.getChannelPointer (channel);
            const auto input = std::isfinite (output[sample]) ? output[sample] : 0.0f;
            constexpr auto stateLimit = 8.0f;
            const auto stateIsValid = std::isfinite (state.recursive) && std::isfinite (state.damping)
                && std::isfinite (state.dcInput) && std::isfinite (state.dcOutput)
                && std::abs (state.recursive) <= stateLimit && std::abs (state.damping) <= stateLimit
                && std::abs (state.dcInput) <= stateLimit && std::abs (state.dcOutput) <= stateLimit;
            if (! stateIsValid || ! std::isfinite (output[sample]))
            {
                state = {};
                output[sample] = input;
                ++feedbackStateFaultCount;
                continue;
            }
            const auto recursiveInput = input * inputDrive + state.recursive * feedbackAmount;
            const auto shaped = boundedShape (recursiveInput);
            state.damping += dampingCoefficient * (shaped - state.damping);
            const auto silenceDecay = std::abs (input) < 1.0e-6f ? 0.92f : 0.985f;
            state.recursive = juce::jlimit (-1.2f, 1.2f, state.damping * silenceDecay);
            const auto density = state.recursive - 0.58f * input;
            const auto dcBlocked = density - state.dcInput + 0.995f * state.dcOutput;
            state.dcInput = density;
            state.dcOutput = dcBlocked;
            const auto processed = safetyDynamics.processSample (static_cast<int> (channel), dcBlocked);
            if (! std::isfinite (processed) || ! std::isfinite (state.recursive)
                || ! std::isfinite (state.damping) || ! std::isfinite (state.dcOutput)
                || std::abs (state.recursive) > stateLimit || std::abs (state.damping) > stateLimit
                || std::abs (state.dcOutput) > stateLimit)
            {
                state = {};
                output[sample] = input;
                ++feedbackStateFaultCount;
                continue;
            }
            output[sample] = processed;
#if VOIDWORM_ENABLE_DIAGNOSTICS
            maximumStateMagnitude = juce::jmax (maximumStateMagnitude,
                juce::jmax (std::abs (state.recursive),
                    juce::jmax (std::abs (state.damping), std::abs (state.dcOutput))));
#endif
        }
    }

    if (features.sustain < 1.0e-5f && features.slowEnvelope < 1.0e-5f)
        for (auto& state : states)
            state.recursive *= 0.80f;
}
}
