#include "NonlinearStage.h"

namespace voidworm
{
void NonlinearStage::reset() noexcept
{
    states = {};
}

void NonlinearStage::process (juce::dsp::AudioBlock<float>& block, float rot,
                              float overload, float surge) noexcept
{
    const auto amount = juce::jlimit (0.0f, 0.72f, 0.10f + 0.30f * rot + 0.14f * overload + 0.10f * surge);
    const auto drive = 1.0f + 1.4f * rot + 0.35f * overload + 0.25f * surge;
    for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
    {
        auto& state = states[juce::jmin (channel, states.size() - 1)];
        auto* output = block.getChannelPointer (channel);
        for (size_t sample = 0; sample < block.getNumSamples(); ++sample)
        {
            const auto input = output[sample];
            const auto driven = input * drive;
            const auto knee = driven >= 0.0f ? 0.48f : 0.72f;
            const auto shaped = driven / (1.0f + knee * std::abs (driven));
            const auto blended = juce::jmap (amount, input, shaped);
            const auto dcBlocked = blended - state.dcInput + 0.9975f * state.dcOutput;
            state.dcInput = blended;
            state.dcOutput = dcBlocked;
            output[sample] = juce::jlimit (-2.5f, 2.5f, dcBlocked);
        }
    }
}
}
