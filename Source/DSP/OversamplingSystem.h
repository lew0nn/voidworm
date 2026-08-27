#pragma once

#include <JuceHeader.h>

namespace voidworm
{
class OversamplingSystem
{
public:
    using Oversampler = juce::dsp::Oversampling<float>;

    void prepare (int channels, int maximumBlockSize);
    void reset() noexcept;
    bool select (int factor, bool highQuality) noexcept;
    juce::dsp::AudioBlock<float> processSamplesUp (const juce::dsp::AudioBlock<const float>& input) noexcept;
    void processSamplesDown (juce::dsp::AudioBlock<float>& output) noexcept;
    void applyFixedLatency (juce::AudioBuffer<float>& buffer) noexcept;

    int getFixedLatencySamples() const noexcept { return fixedLatencySamples; }
    int getSelectedFactor() const noexcept;

private:
    static int factorToIndex (int factor) noexcept;

    std::array<std::array<std::unique_ptr<Oversampler>, 4>, 2> configurations;
    std::array<std::vector<float>, 2> latencyBuffer;
    Oversampler* selected = nullptr;
    int selectedIndex = 0;
    int selectedQuality = 0;
    int selectedLatencySamples = 0;
    int fixedLatencySamples = 0;
    int latencyWritePosition = 0;
    int latencyBufferSize = 1;
    int channelCount = 2;
};
}
