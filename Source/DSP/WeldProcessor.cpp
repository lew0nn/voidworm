#include "WeldProcessor.h"

namespace voidworm
{
void WeldProcessor::prepare (double newSampleRate) noexcept
{
    sampleRate = juce::jmax (1.0, newSampleRate);
    reset();
}

void WeldProcessor::reset() noexcept
{
    envelope = 0.0f;
    gain = 1.0f;
    stateFaultCount = 0;
}

float WeldProcessor::process (juce::AudioBuffer<float>& buffer, float amount) noexcept
{
    const auto weld = std::isfinite (amount) ? juce::jlimit (0.0f, 1.0f, amount) : 0.30f;
    const auto thresholdDb = juce::jmap (weld, -1.0f, -22.0f);
    const auto threshold = juce::Decibels::decibelsToGain (thresholdDb);
    const auto ratio = 1.0f + 11.0f * weld;
    const auto attackMs = juce::jmap (weld, 14.0f, 1.5f);
    const auto releaseMs = juce::jmap (weld, 180.0f, 52.0f);
    const auto attack = std::exp (-1.0f / (0.001f * attackMs * static_cast<float> (sampleRate)));
    const auto release = std::exp (-1.0f / (0.001f * releaseMs * static_cast<float> (sampleRate)));
    auto minimumArtisticGain = 1.0f;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        auto detector = 0.0f;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            detector = juce::jmax (detector, std::abs (buffer.getSample (channel, sample)));

        if (! std::isfinite (detector) || ! std::isfinite (envelope) || ! std::isfinite (gain)
            || envelope < 0.0f || gain < 0.0f || gain > 1.0001f)
        {
            envelope = 0.0f;
            gain = 1.0f;
            detector = 0.0f;
            ++stateFaultCount;
        }

        const auto detectorCoefficient = detector > envelope ? attack : release;
        envelope = detectorCoefficient * envelope + (1.0f - detectorCoefficient) * detector;
        auto targetGain = 1.0f;
        if (envelope > threshold)
        {
            const auto inputDb = juce::Decibels::gainToDecibels (envelope, -120.0f);
            const auto outputDb = thresholdDb + (inputDb - thresholdDb) / ratio;
            targetGain = juce::Decibels::decibelsToGain (outputDb - inputDb);
        }
        const auto gainCoefficient = targetGain < gain ? attack : release;
        gain = gainCoefficient * gain + (1.0f - gainCoefficient) * targetGain;
        const auto artisticGain = 1.0f - weld * (1.0f - gain);
        const auto containmentStrength = weld * weld;
        const auto kneeStart = juce::jmap (weld, 1.24f, 0.76f);
        const auto ceiling = juce::jmap (weld, 1.34f, 0.96f);

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            const auto compressed = buffer.getSample (channel, sample) * artisticGain;
            const auto magnitude = std::abs (compressed);
            auto contained = compressed;
            if (magnitude > kneeStart)
            {
                const auto knee = juce::jmax (0.04f, ceiling - kneeStart);
                const auto shapedMagnitude = kneeStart + knee * std::tanh ((magnitude - kneeStart) / knee);
                contained = std::copysign (juce::jmin (ceiling, shapedMagnitude), compressed);
            }
            const auto output = compressed + containmentStrength * (contained - compressed);
            buffer.setSample (channel, sample, std::isfinite (output) ? output : 0.0f);
            if (! std::isfinite (output))
                ++stateFaultCount;
        }
        minimumArtisticGain = juce::jmin (minimumArtisticGain, artisticGain);
    }
    return -juce::Decibels::gainToDecibels (juce::jmax (minimumArtisticGain, 1.0e-6f));
}

uint32_t WeldProcessor::getAndClearStateFaultCount() noexcept
{
    const auto result = stateFaultCount;
    stateFaultCount = 0;
    return result;
}
}
