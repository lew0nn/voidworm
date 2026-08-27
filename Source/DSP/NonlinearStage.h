#pragma once

#include <JuceHeader.h>

namespace voidworm
{
class NonlinearStage
{
public:
    void reset() noexcept;
    void process (juce::dsp::AudioBlock<float>& block, float rot, float overload, float surge) noexcept;

private:
    struct State
    {
        float dcInput = 0.0f;
        float dcOutput = 0.0f;
    };
    std::array<State, 2> states {};
};
}
