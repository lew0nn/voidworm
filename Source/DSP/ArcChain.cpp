#include "ArcChain.h"

namespace voidworm
{
namespace
{
float onePoleCoefficient (double sampleRate, float frequency) noexcept
{
    return 1.0f - std::exp (-juce::MathConstants<float>::twoPi * frequency
                            / static_cast<float> (sampleRate));
}
}

void ArcChain::prepare (double sampleRate) noexcept
{
    const auto safeRate = juce::jmax (1.0, sampleRate);
    // 210/2.85k define ARC's collision bands. Cleanup remains internal but is
    // broadened so the user pre-EQ can excite useful upper-spectrum interaction.
    lowCoefficient = onePoleCoefficient (safeRate, 210.0f);
    midCoefficient = onePoleCoefficient (safeRate, 2850.0f);
    cleanupCoefficient = onePoleCoefficient (safeRate, 15000.0f);
    xmodSmooth.reset (safeRate, 0.020);
    foldSmooth.reset (safeRate, 0.020);
    compressor.prepare (safeRate);
    preEq.prepare (safeRate);
    reset();
}

void ArcChain::reset() noexcept
{
    quench();
    xmodSmooth.setCurrentAndTargetValue (0.5f);
    foldSmooth.setCurrentAndTargetValue (0.5f);
}

void ArcChain::quench() noexcept
{
    states = {};
    compressor.reset();
    preEq.reset();
}

float ArcChain::asymmetric (float input) noexcept
{
    const auto magnitude = std::abs (input);
    return input / (1.0f + (input >= 0.0f ? 0.38f : 0.69f) * magnitude);
}

float ArcChain::reflectFold (float input) noexcept
{
    auto wrapped = std::fmod (input + 1.0f, 4.0f);
    if (wrapped < 0.0f)
        wrapped += 4.0f;
    return wrapped <= 2.0f ? wrapped - 1.0f : 3.0f - wrapped;
}

void ArcChain::process (juce::dsp::AudioBlock<float>& block, ReactorEqSettings eq,
                        const SourceFeatures& features,
                        float rot, float overload, float breach, float surge,
                        float xmod, float fold) noexcept
{
    preEq.process (block, eq);
    xmodSmooth.setTargetValue (character::sanitise (xmod));
    foldSmooth.setTargetValue (character::sanitise (fold));
    const auto baseCrossAmount = breach * (0.42f + 0.38f * features.midRatio
                                            + 0.34f * features.highRatio);
    const auto baseFoldAmount = juce::jlimit (0.0f, 0.78f,
        0.06f * rot + (0.28f * features.brightness + 0.18f * features.highRatio)
        * (0.35f + 0.65f * breach) + 0.10f * features.midRatio + 0.12f * surge);
    const auto transientBias = breach * features.transient * (0.04f + 0.10f * overload);
    compressor.setParameters (-11.0f - 8.0f * overload, 4.0f + 4.0f * overload,
                              1.2f + 1.8f * (1.0f - overload), 82.0f - 28.0f * overload);

    for (size_t sample = 0; sample < block.getNumSamples(); ++sample)
    {
        const auto crossAmount = character::arcCrossAmount (
            baseCrossAmount, xmodSmooth.getNextValue());
        const auto foldAmount = character::arcFoldAmount (
            baseFoldAmount, foldSmooth.getNextValue());
        for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
        {
            auto& state = states[juce::jmin (channel, states.size() - 1)];
            auto* output = block.getChannelPointer (channel);
            const auto input = output[sample];
            state.low += lowCoefficient * (input - state.low);
            state.midLow += midCoefficient * (input - state.midLow);
            const auto low = state.low;
            const auto mid = state.midLow - low;
            const auto high = input - state.midLow;

            const auto lowToMid = mid * asymmetric (low * (1.2f + 1.8f * rot));
            const auto midToHigh = high * asymmetric (mid * (1.4f + 2.2f * rot));
            const auto bandCollision = low * high * (1.0f + features.brightness);
            const auto interaction = mid + crossAmount * (3.4f * lowToMid + 2.8f * midToHigh
                                                           + 2.1f * bandCollision);
            const auto biased = interaction * (1.0f + 1.8f * rot + 0.25f * overload)
                              + transientBias * (high >= 0.0f ? 1.0f : -0.55f);
            const auto clipped = asymmetric (biased);
            const auto folded = reflectFold (clipped * (1.0f + 3.4f * foldAmount));
            const auto shaped = juce::jmap (foldAmount, clipped, folded);
            state.cleanup += cleanupCoefficient * (shaped - state.cleanup);
            const auto dcBlocked = state.cleanup - state.dcInput + 0.9965f * state.dcOutput;
            state.dcInput = state.cleanup;
            state.dcOutput = dcBlocked;
            output[sample] = juce::jlimit (-2.5f, 2.5f,
                compressor.processSample (static_cast<int> (channel), dcBlocked));
        }
    }
}
}
