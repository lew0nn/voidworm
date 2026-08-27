#pragma once

#include <JuceHeader.h>

namespace voidworm
{
struct DspFaultCounters
{
    uint32_t preEqFaultCount = 0;
    uint32_t feedbackStateFaultCount = 0;
    uint32_t filterStateFaultCount = 0;
    uint32_t nonFiniteRepairCount = 0;
    uint32_t weldStateFaultCount = 0;
    uint32_t limiterStateFaultCount = 0;

    uint32_t total() const noexcept
    {
        return preEqFaultCount + feedbackStateFaultCount
             + filterStateFaultCount + nonFiniteRepairCount
             + weldStateFaultCount + limiterStateFaultCount;
    }

    DspFaultCounters& operator+= (const DspFaultCounters& other) noexcept
    {
        preEqFaultCount += other.preEqFaultCount;
        feedbackStateFaultCount += other.feedbackStateFaultCount;
        filterStateFaultCount += other.filterStateFaultCount;
        nonFiniteRepairCount += other.nonFiniteRepairCount;
        weldStateFaultCount += other.weldStateFaultCount;
        limiterStateFaultCount += other.limiterStateFaultCount;
        return *this;
    }
};

#if VOIDWORM_ENABLE_DIAGNOSTICS
struct StageMetrics
{
    float peak = 0.0f;
    float largestDelta = 0.0f;
    double sumSquares = 0.0;
    uint64_t sampleCount = 0;
    uint32_t nonFiniteCount = 0;
    uint32_t repairedCount = 0;
    uint32_t clippedCount = 0;
    std::array<float, 2> previous {};
    std::array<bool, 2> hasPrevious {};

    void observe (float value, int channel = 0) noexcept
    {
        const auto safeChannel = static_cast<size_t> (juce::jlimit (0, 1, channel));
        if (! std::isfinite (value))
        {
            ++nonFiniteCount;
            value = 0.0f;
        }
        const auto magnitude = std::abs (value);
        peak = juce::jmax (peak, magnitude);
        sumSquares += static_cast<double> (value) * static_cast<double> (value);
        ++sampleCount;
        if (hasPrevious[safeChannel])
            largestDelta = juce::jmax (largestDelta, std::abs (value - previous[safeChannel]));
        previous[safeChannel] = value;
        hasPrevious[safeChannel] = true;
    }

    float rms() const noexcept
    {
        return sampleCount == 0 ? 0.0f
            : static_cast<float> (std::sqrt (sumSquares / static_cast<double> (sampleCount)));
    }
};

inline void observeBlock (StageMetrics& metrics,
                          const juce::dsp::AudioBlock<float>& block,
                          float hardClipLevel = std::numeric_limits<float>::infinity()) noexcept
{
    for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
        for (size_t sample = 0; sample < block.getNumSamples(); ++sample)
        {
            const auto value = block.getSample (static_cast<int> (channel),
                                                static_cast<int> (sample));
            metrics.observe (value, static_cast<int> (channel));
            if (std::isfinite (hardClipLevel) && std::isfinite (value)
                && std::abs (value) >= hardClipLevel - 1.0e-6f)
                ++metrics.clippedCount;
        }
}

inline void observeBuffer (StageMetrics& metrics,
                           const juce::AudioBuffer<float>& buffer) noexcept
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            metrics.observe (buffer.getSample (channel, sample), channel);
}
#endif
}
