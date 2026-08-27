#include "FinalLimiter.h"

namespace voidworm
{
void FinalLimiter::prepare (double newSampleRate, int maximumBlockSize, int channels)
{
    sampleRate = juce::jmax (1.0, newSampleRate);
    channelCount = juce::jlimit (1, 2, channels);
    lookaheadSamples = juce::jmax (1, juce::roundToInt (0.001 * sampleRate));
    bufferSize = juce::jmax (2, lookaheadSamples + maximumBlockSize + 2);
    for (auto& channel : delayBuffer)
        channel.assign (static_cast<size_t> (bufferSize), 0.0f);
    releaseCoefficient = std::exp (-1.0f / (0.070f * static_cast<float> (sampleRate)));
    reset();
}

void FinalLimiter::reset() noexcept
{
    for (auto& channel : delayBuffer)
        std::fill (channel.begin(), channel.end(), 0.0f);
    writePosition = 0;
    releaseHoldSamples = 0;
    gain = 1.0f;
    stateFaultCount = 0;
}

float FinalLimiter::process (juce::AudioBuffer<float>& buffer, bool enabled,
                             juce::SmoothedValue<float>& thresholdDbSmooth,
                             juce::SmoothedValue<float>& ceilingDbSmooth) noexcept
{
    auto minimumAppliedGain = 1.0f;
    const auto channels = juce::jmin (channelCount, buffer.getNumChannels());
    const auto parametersAreSmoothing = thresholdDbSmooth.isSmoothing() || ceilingDbSmooth.isSmoothing();
    const auto stableCeiling = parametersAreSmoothing ? 1.0f
        : juce::Decibels::decibelsToGain (juce::jlimit (-6.0f, 0.0f, ceilingDbSmooth.getCurrentValue()));
    const auto stablePreGain = parametersAreSmoothing ? 1.0f : stableCeiling
        / juce::Decibels::decibelsToGain (juce::jlimit (-18.0f, 0.0f, thresholdDbSmooth.getCurrentValue()));
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        auto ceiling = stableCeiling;
        auto preGain = stablePreGain;
        if (parametersAreSmoothing)
        {
            const auto thresholdDb = juce::jlimit (-18.0f, 0.0f, thresholdDbSmooth.getNextValue());
            const auto ceilingDb = juce::jlimit (-6.0f, 0.0f, ceilingDbSmooth.getNextValue());
            ceiling = juce::Decibels::decibelsToGain (ceilingDb);
            preGain = ceiling / juce::Decibels::decibelsToGain (thresholdDb);
        }
        auto detector = 0.0f;
        for (int channel = 0; channel < channels; ++channel)
        {
            const auto input = buffer.getSample (channel, sample);
            delayBuffer[static_cast<size_t> (channel)][static_cast<size_t> (writePosition)] = input;
            detector = juce::jmax (detector, std::abs (input * preGain));
        }

        if (! std::isfinite (detector) || ! std::isfinite (gain)
            || gain < 0.0f || gain > 1.0001f)
        {
            detector = 0.0f;
            gain = 1.0f;
            releaseHoldSamples = 0;
            ++stateFaultCount;
        }
        const auto requiredGain = detector > ceiling ? ceiling / juce::jmax (detector, 1.0e-12f) : 1.0f;
        if (requiredGain < gain)
        {
            gain = requiredGain;
            releaseHoldSamples = lookaheadSamples;
        }
        else if (releaseHoldSamples > 0)
            --releaseHoldSamples;
        else
            gain = releaseCoefficient * gain + (1.0f - releaseCoefficient) * requiredGain;

        auto readPosition = writePosition - lookaheadSamples;
        if (readPosition < 0)
            readPosition += bufferSize;
        for (int channel = 0; channel < channels; ++channel)
        {
            const auto delayed = delayBuffer[static_cast<size_t> (channel)][static_cast<size_t> (readPosition)];
            auto output = delayed;
            auto appliedGain = 1.0f;
            if (enabled)
            {
                const auto driven = delayed * preGain;
                output = driven * gain;
                if (std::abs (output) > ceiling)
                    output = std::copysign (ceiling, output);
                if (std::abs (driven) > 1.0e-12f)
                    appliedGain = juce::jmin (1.0f, std::abs (output / driven));
            }
            if (! std::isfinite (output))
            {
                output = 0.0f;
                ++stateFaultCount;
            }
            buffer.setSample (channel, sample, output);
            minimumAppliedGain = juce::jmin (minimumAppliedGain, appliedGain);
        }
        writePosition = (writePosition + 1) % bufferSize;
    }
    return enabled
        ? -juce::Decibels::gainToDecibels (juce::jmax (minimumAppliedGain, 1.0e-6f))
        : 0.0f;
}

uint32_t FinalLimiter::getAndClearStateFaultCount() noexcept
{
    const auto result = stateFaultCount;
    stateFaultCount = 0;
    return result;
}
}
