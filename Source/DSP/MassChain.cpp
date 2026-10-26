#include "MassChain.h"

namespace voidworm
{
namespace
{
float onePoleCoefficient (double sampleRate, float frequency) noexcept
{
    return 1.0f - std::exp (-juce::MathConstants<float>::twoPi * frequency
                            / static_cast<float> (sampleRate));
}
}

void MassChain::prepare (double sampleRate) noexcept
{
    const auto safeRate = juce::jmax (1.0, sampleRate);
    // These bands define MASS's body circuit; the user pre-EQ controls its excitation.
    lowCoefficient = onePoleCoefficient (safeRate, 235.0f);
    lowMidCoefficient = onePoleCoefficient (safeRate, 920.0f);
    processingRate = safeRate;
    cleanupCoefficient = onePoleCoefficient (safeRate, 2600.0f);
    saturationSmooth.reset (safeRate, 0.020);
    harmonicsSmooth.reset (safeRate, 0.020);
    compressor.prepare (safeRate);
    preEq.prepare (safeRate);
    reset();
}

void MassChain::reset() noexcept
{
    quench();
    saturationSmooth.setCurrentAndTargetValue (0.5f);
    harmonicsSmooth.setCurrentAndTargetValue (0.5f);
}

void MassChain::quench() noexcept
{
    states = {};
    compressor.reset();
    preEq.reset();
}

float MassChain::asymmetricClip (float input, float hardness) noexcept
{
    if (input >= 0.0f)
        return input / (1.0f + hardness * 0.52f * input);
    return input / (1.0f + hardness * 0.82f * std::abs (input));
}

void MassChain::process (juce::dsp::AudioBlock<float>& block, ReactorEqSettings eq,
                         const SourceFeatures& features,
                         float rot, float overload, float surge,
                         float saturation, float harmonics) noexcept
{
    preEq.process (block, eq);
    saturationSmooth.setTargetValue (character::sanitise (saturation));
    harmonicsSmooth.setTargetValue (character::sanitise (harmonics));
    const auto baseDrive = 1.0f + 1.55f * rot + 0.20f * overload + 0.30f * surge;
    const auto baseHarmonicBlend = juce::jlimit (0.20f, 0.68f,
        0.26f + 0.18f * rot + 0.12f * overload
        + 0.07f * features.lowRatio + 0.06f * features.lowLevel);
    compressor.setParameters (-7.0f - 7.0f * overload, 3.0f + 2.0f * overload,
                              15.0f - 5.0f * overload, 125.0f - 35.0f * overload);
    /* Above its middle, HARMONICS lets the grit out: the cleanup low-pass
       opens from 2.6 to 14 kHz and less of the clean low end is laid back
       under it. Set per block; a one-pole's cutoff does not zipper. */
    const auto openness = character::anger (harmonicsSmooth.getCurrentValue());
    cleanupCoefficient = onePoleCoefficient (processingRate,
        juce::jmin (2600.0f * std::pow (14000.0f / 2600.0f, openness), static_cast<float> (processingRate) * 0.45f));
    for (size_t sample = 0; sample < block.getNumSamples(); ++sample)
    {
        const auto drive = character::massDrive (baseDrive, saturationSmooth.getNextValue());
        const auto harmonics = harmonicsSmooth.getNextValue();
        const auto harmonicBlend = juce::jmax (character::anger (harmonics),
            character::massHarmonicBlend (baseHarmonicBlend, harmonics));
        const auto cleanLow = 0.42f - 0.34f * character::anger (harmonics);
        /* Above its middle, SATURATION clips the lows much harder and then
           squares them off under a ceiling, like a fuzz on a bass: still only
           the lows, so MASS stays the bass reactor, but with teeth. The
           make-up keeps the squared-off body from simply getting quieter. */
        const auto fury = character::anger (saturationSmooth.getCurrentValue());
        const auto furyDrive = 1.0f + 11.0f * fury * fury;
        const auto hardness = 1.0f + 2.0f * fury;
        const auto ceiling = 0.1f + 2.0f * (1.0f - fury) * (1.0f - fury);
        const auto makeUp = 1.0f + 2.0f * fury;
        for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
        {
            auto& state = states[juce::jmin (channel, states.size() - 1)];
            auto* output = block.getChannelPointer (channel);
            const auto input = output[sample];
            state.low += lowCoefficient * (input - state.low);
            state.lowMid += lowMidCoefficient * (input - state.lowMid);
            const auto body = 0.88f * state.low + 0.34f * (state.lowMid - state.low);
            const auto saturated = juce::jlimit (-ceiling, ceiling, asymmetricClip (body * drive * furyDrive, hardness));
            const auto shaped = juce::jmap (harmonicBlend, body, saturated);
            const auto dcBlocked = shaped - state.dcInput + 0.9975f * state.dcOutput;
            state.dcInput = shaped;
            state.dcOutput = dcBlocked;
            state.cleanup += cleanupCoefficient * (dcBlocked - state.cleanup);
            const auto weighted = cleanLow * state.low + 0.82f * state.cleanup;
            output[sample] = juce::jlimit (-2.5f, 2.5f,
                makeUp * compressor.processSample (static_cast<int> (channel), weighted));
        }
    }
}
}
