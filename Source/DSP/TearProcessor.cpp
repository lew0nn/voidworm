#include "TearProcessor.h"

namespace voidworm
{
void TearProcessor::prepare (double newSampleRate, int channels)
{
    sampleRate = juce::jmax (1.0, newSampleRate);
    channelCount = juce::jlimit (1, 2, channels);
    historySize = juce::jmax (256, static_cast<int> (sampleRate * 0.30));
    for (auto& channel : history)
        channel.assign (static_cast<size_t> (historySize), 0.0f);
    reset();
}

void TearProcessor::reset() noexcept
{
    for (auto& channel : history)
        std::fill (channel.begin(), channel.end(), 0.0f);
    writePosition = 0;
    decisionCountdown = 1;
    fragmentLength = 1;
    eventRemaining = 0;
    eventAge = 0;
    crossfadeSamples = 1;
    readPosition = fragmentStart = 0.0;
    readStep = 1.0;
    mode = Mode::forward;
}

void TearProcessor::resetForPresetChange() noexcept
{
    // End the active fragment without clearing or reallocating prepared history.
    decisionCountdown = 1;
    fragmentLength = 1;
    eventRemaining = 0;
    eventAge = 0;
    crossfadeSamples = 1;
    readPosition = fragmentStart = static_cast<double> (writePosition);
    readStep = 1.0;
    mode = Mode::forward;
}

void TearProcessor::quench() noexcept
{
    // Cancel any active fragment without touching the prepared history. Waiting
    // one full history cycle prevents pre-quench material from being selected
    // after the gate opens again.
    decisionCountdown = historySize;
    fragmentLength = 1;
    eventRemaining = 0;
    eventAge = 0;
    crossfadeSamples = 1;
    readPosition = fragmentStart = static_cast<double> (writePosition);
    readStep = 1.0;
    mode = Mode::forward;
}

float TearProcessor::nextRandom() noexcept
{
    randomState ^= randomState << 13;
    randomState ^= randomState >> 17;
    randomState ^= randomState << 5;
    return static_cast<float> (randomState & 0x00ffffffu) / static_cast<float> (0x01000000u);
}

void TearProcessor::scheduleDecision (float tear, float overload, float surge,
                                      const SourceFeatures& features) noexcept
{
    const auto intervalMs = juce::jmap (tear, 170.0f, 22.0f);
    decisionCountdown = juce::jmax (1, static_cast<int> (sampleRate * 0.001
        * intervalMs * juce::jmap (nextRandom(), 0.72f, 1.35f)));

    const auto transientBias = juce::jmap (tear, 0.22f + 0.78f * features.transient, 0.45f);
    const auto probability = juce::jlimit (0.0f, 0.82f,
        std::pow (tear, 1.35f) * transientBias * (1.0f + 0.22f * surge));
    if (nextRandom() >= probability)
        return;

    const auto maximumMs = juce::jmap (tear, 7.0f, 25.0f);
    const auto fragmentMs = juce::jmap (nextRandom(), 3.0f, maximumMs);
    fragmentLength = juce::jlimit (8, historySize / 3,
        static_cast<int> (sampleRate * 0.001 * fragmentMs));
    const auto repeatCount = 1 + static_cast<int> (nextRandom() * (1.0f + 2.0f * tear));
    eventRemaining = fragmentLength * repeatCount;
    eventAge = 0;
    crossfadeSamples = juce::jlimit (4, fragmentLength / 3,
        static_cast<int> (sampleRate * 0.001 * juce::jmap (tear, 0.7f, 2.2f)));

    const auto offsetMs = juce::jmap (nextRandom(), 4.0f, juce::jmap (tear, 14.0f, 55.0f));
    const auto offset = fragmentLength + static_cast<int> (sampleRate * 0.001 * offsetMs);
    readPosition = static_cast<double> ((writePosition - offset + historySize) % historySize);
    fragmentStart = readPosition;

    const auto choice = nextRandom();
    mode = Mode::forward;
    if (tear > 0.58f && choice > 0.84f)
        mode = Mode::reverse;
    if (tear > 0.74f && choice > 0.93f)
        mode = Mode::freeze;
    if (tear > 0.86f && choice > 0.975f)
        mode = Mode::skip;
    readStep = mode == Mode::reverse ? -1.0 : mode == Mode::freeze ? 0.0 : mode == Mode::skip ? 2.0 : 1.0;
    juce::ignoreUnused (overload);
}

void TearProcessor::writeInputOnly (juce::AudioBuffer<float>& buffer) noexcept
{
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        for (int channel = 0; channel < channelCount; ++channel)
            history[static_cast<size_t> (channel)][static_cast<size_t> (writePosition)] =
                buffer.getSample (juce::jmin (channel, buffer.getNumChannels() - 1), sample);
        writePosition = (writePosition + 1) % historySize;
    }
}

float TearProcessor::process (juce::AudioBuffer<float>& buffer, float tear, float overload,
                              float surge, const SourceFeatures& features) noexcept
{
    tear = juce::jlimit (0.0f, 1.0f, tear);
    if (tear <= 0.000001f)
    {
        eventRemaining = 0;
        decisionCountdown = 1;
        writeInputOnly (buffer);
        return 0.0f;
    }

    auto peakActivity = 0.0f;
    const auto wetAmount = juce::jlimit (0.0f, 0.88f,
        (0.08f + 0.68f * tear) * (1.0f + 0.16f * surge));
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        auto linkedInput = 0.0f;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            linkedInput = juce::jmax (linkedInput, std::abs (buffer.getSample (channel, sample)));

        for (int channel = 0; channel < channelCount; ++channel)
            history[static_cast<size_t> (channel)][static_cast<size_t> (writePosition)] =
                buffer.getSample (juce::jmin (channel, buffer.getNumChannels() - 1), sample);

        if (linkedInput < 1.0e-7f)
            eventRemaining = 0;
        else if (--decisionCountdown <= 0)
            scheduleDecision (tear, overload, surge, features);

        if (eventRemaining > 0)
        {
            auto readIndex = static_cast<int> (readPosition) % historySize;
            if (readIndex < 0)
                readIndex += historySize;
            const auto fadeIn = juce::jlimit (0.0f, 1.0f,
                static_cast<float> (eventAge) / static_cast<float> (crossfadeSamples));
            const auto fadeOut = juce::jlimit (0.0f, 1.0f,
                static_cast<float> (eventRemaining) / static_cast<float> (crossfadeSamples));
            const auto fragmentPosition = eventAge % fragmentLength;
            const auto loopFadeIn = juce::jlimit (0.0f, 1.0f,
                static_cast<float> (fragmentPosition) / static_cast<float> (crossfadeSamples));
            const auto loopFadeOut = juce::jlimit (0.0f, 1.0f,
                static_cast<float> (fragmentLength - fragmentPosition) / static_cast<float> (crossfadeSamples));
            const auto mix = wetAmount * juce::jmin (juce::jmin (fadeIn, fadeOut),
                                                      juce::jmin (loopFadeIn, loopFadeOut));
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            {
                const auto fragment = history[static_cast<size_t> (juce::jmin (channel, channelCount - 1))]
                                             [static_cast<size_t> (readIndex)];
                buffer.setSample (channel, sample, juce::jmap (mix, buffer.getSample (channel, sample), fragment));
            }
            peakActivity = juce::jmax (peakActivity, mix);
            readPosition += readStep;
            while (readPosition < 0.0) readPosition += static_cast<double> (historySize);
            while (readPosition >= static_cast<double> (historySize)) readPosition -= static_cast<double> (historySize);
            ++eventAge;
            if (eventAge % fragmentLength == 0)
                readPosition = fragmentStart;
            --eventRemaining;
        }

        writePosition = (writePosition + 1) % historySize;
    }
    return peakActivity;
}
}
