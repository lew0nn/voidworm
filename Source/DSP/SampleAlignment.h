#pragma once

#include <array>

namespace voidworm
{
/* FURNACE's and ARC's nonlinearities are anti-aliased by averaging each one
   over the step between samples instead of sampling it. Each such stage
   delays its output by half a sample, and both reactors carry two in series,
   so both come out exactly one (oversampled) sample late. Everything they
   are summed or mixed with takes the same delay, so nothing drifts out of
   step: MASS, FEEDBACK and the dry path through SampleDelay, and inside ARC
   the unfolded branch through HalfSampleDelay to meet its folded twin. */
struct SampleDelay
{
    float process (float input) noexcept
    {
        const auto output = previous;
        previous = input;
        return output;
    }
    void reset() noexcept { previous = 0.0f; }
    float previous = 0.0f;
};

/* Half a sample with a flat magnitude response: a first-order Thiran
   all-pass, a = (1 - D) / (1 + D) with D = 0.5. An average of two samples
   would delay as much but dull the top end of a branch that was never
   averaged. */
struct HalfSampleDelay
{
    float process (float input) noexcept
    {
        constexpr float a = 1.0f / 3.0f;
        const auto output = a * input + previousInput - a * previousOutput;
        previousInput = input;
        previousOutput = output;
        return output;
    }
    void reset() noexcept { previousInput = previousOutput = 0.0f; }
    float previousInput = 0.0f;
    float previousOutput = 0.0f;
};

using StereoSampleDelay = std::array<SampleDelay, 2>;
}
