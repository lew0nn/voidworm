#pragma once

#include <JuceHeader.h>
#include "SourceAnalyzer.h"

namespace voidworm
{
class TearProcessor
{
public:
    void prepare (double sampleRate, int channels);
    void reset() noexcept;
    void resetForPresetChange() noexcept;
    void quench() noexcept;
    float process (juce::AudioBuffer<float>& buffer, float tear, float overload,
                   float surge, const SourceFeatures& features) noexcept;

private:
    enum class Mode { forward, reverse, freeze, skip };

    float nextRandom() noexcept;
    void scheduleDecision (float tear, float overload, float surge,
                           const SourceFeatures& features) noexcept;
    void writeInputOnly (juce::AudioBuffer<float>& buffer) noexcept;

    std::array<std::vector<float>, 2> history;
    double sampleRate = 44100.0;
    int channelCount = 2;
    int historySize = 2;
    int writePosition = 0;
    int decisionCountdown = 1;
    int fragmentLength = 1;
    int eventRemaining = 0;
    int eventAge = 0;
    int crossfadeSamples = 1;
    double readPosition = 0.0;
    double fragmentStart = 0.0;
    double readStep = 1.0;
    Mode mode = Mode::forward;
    uint32_t randomState = 0x4d37a29bu;
};
}
