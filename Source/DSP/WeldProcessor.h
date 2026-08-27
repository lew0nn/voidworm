#pragma once

#include <JuceHeader.h>

namespace voidworm
{
class WeldProcessor
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept;
    float process (juce::AudioBuffer<float>& buffer, float weld) noexcept;
    uint32_t getAndClearStateFaultCount() noexcept;

private:
    double sampleRate = 44100.0;
    float envelope = 0.0f;
    float gain = 1.0f;
    uint32_t stateFaultCount = 0;
};
}
