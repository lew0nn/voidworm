#include "OversamplingSystem.h"

namespace voidworm
{
int OversamplingSystem::factorToIndex (int factor) noexcept
{
    if (factor >= 8) return 3;
    if (factor >= 4) return 2;
    if (factor >= 2) return 1;
    return 0;
}

void OversamplingSystem::prepare (int channels, int maximumBlockSize)
{
    channelCount = juce::jlimit (1, 2, channels);
    fixedLatencySamples = 0;
    for (int quality = 0; quality < 2; ++quality)
    {
        for (int exponent = 0; exponent < 4; ++exponent)
        {
            const auto type = quality == 0 ? Oversampler::filterHalfBandPolyphaseIIR
                                           : Oversampler::filterHalfBandFIREquiripple;
            auto oversampler = std::make_unique<Oversampler> (static_cast<size_t> (channelCount),
                                                               static_cast<size_t> (exponent), type,
                                                               quality != 0, true);
            oversampler->initProcessing (static_cast<size_t> (maximumBlockSize));
            fixedLatencySamples = juce::jmax (fixedLatencySamples,
                juce::roundToInt (oversampler->getLatencyInSamples()));
            configurations[static_cast<size_t> (quality)][static_cast<size_t> (exponent)] = std::move (oversampler);
        }
    }

    latencyBufferSize = juce::jmax (2, fixedLatencySamples + maximumBlockSize + 2);
    for (auto& channel : latencyBuffer)
        channel.assign (static_cast<size_t> (latencyBufferSize), 0.0f);
    selectedQuality = 0;
    selectedIndex = 0;
    selected = configurations[0][0].get();
    selectedLatencySamples = juce::roundToInt (selected->getLatencyInSamples());
    reset();
}

void OversamplingSystem::reset() noexcept
{
    for (auto& quality : configurations)
        for (auto& oversampler : quality)
            if (oversampler != nullptr)
                oversampler->reset();
    for (auto& channel : latencyBuffer)
        std::fill (channel.begin(), channel.end(), 0.0f);
    latencyWritePosition = 0;
}

bool OversamplingSystem::select (int factor, bool highQuality) noexcept
{
    const auto newIndex = factorToIndex (factor);
    const auto newQuality = highQuality ? 1 : 0;
    if (newIndex == selectedIndex && newQuality == selectedQuality)
        return false;
    selectedIndex = newIndex;
    selectedQuality = newQuality;
    selected = configurations[static_cast<size_t> (selectedQuality)][static_cast<size_t> (selectedIndex)].get();
    selected->reset();
    selectedLatencySamples = juce::roundToInt (selected->getLatencyInSamples());
    return true;
}

juce::dsp::AudioBlock<float> OversamplingSystem::processSamplesUp (
    const juce::dsp::AudioBlock<const float>& input) noexcept
{
    jassert (selected != nullptr);
    return selected->processSamplesUp (input);
}

void OversamplingSystem::processSamplesDown (juce::dsp::AudioBlock<float>& output) noexcept
{
    jassert (selected != nullptr);
    selected->processSamplesDown (output);
}

void OversamplingSystem::applyFixedLatency (juce::AudioBuffer<float>& buffer) noexcept
{
    const auto delay = juce::jmax (0, fixedLatencySamples - selectedLatencySamples);
    const auto channels = juce::jmin (channelCount, buffer.getNumChannels());
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        auto readPosition = latencyWritePosition - delay;
        if (readPosition < 0)
            readPosition += latencyBufferSize;
        for (int channel = 0; channel < channels; ++channel)
        {
            auto& ring = latencyBuffer[static_cast<size_t> (channel)];
            ring[static_cast<size_t> (latencyWritePosition)] = buffer.getSample (channel, sample);
            buffer.setSample (channel, sample, ring[static_cast<size_t> (readPosition)]);
        }
        latencyWritePosition = (latencyWritePosition + 1) % latencyBufferSize;
    }
}

int OversamplingSystem::getSelectedFactor() const noexcept
{
    return 1 << selectedIndex;
}
}
