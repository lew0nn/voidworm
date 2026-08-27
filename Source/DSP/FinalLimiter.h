#pragma once

#include <JuceHeader.h>

namespace voidworm
{
class FinalLimiter
{
public:
    void prepare (double sampleRate, int maximumBlockSize, int channels);
    void reset() noexcept;
    float process (juce::AudioBuffer<float>& buffer, bool enabled,
                   juce::SmoothedValue<float>& thresholdDb,
                   juce::SmoothedValue<float>& ceilingDb) noexcept;
    int getLatencySamples() const noexcept { return lookaheadSamples; }
    uint32_t getAndClearStateFaultCount() noexcept;

private:
    std::array<std::vector<float>, 2> delayBuffer;
    double sampleRate = 44100.0;
    int channelCount = 2;
    int bufferSize = 2;
    int writePosition = 0;
    int lookaheadSamples = 1;
    int releaseHoldSamples = 0;
    float gain = 1.0f;
    float releaseCoefficient = 0.0f;
    uint32_t stateFaultCount = 0;
};
}
