#include "SawProcessor.h"

namespace voidworm
{
namespace
{
constexpr int harmonicCount = 6;
constexpr float harmonicTaper = 0.78f;
constexpr float phaseBias = 0.38f;
}

void SawProcessor::prepare (double sampleRate, int maximumBlockSize, int channels)
{
    sampleRate = juce::jmax (1.0, sampleRate);
    preparedChannels = juce::jlimit (1, 2, channels);
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        static_cast<size_t> (preparedChannels), 2,
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    oversampling->initProcessing (static_cast<size_t> (juce::jmax (1, maximumBlockSize)));
    latencySamples = juce::roundToInt (oversampling->getLatencyInSamples());
    delayedDry.setSize (preparedChannels, juce::jmax (1, maximumBlockSize));
    for (auto& delay : dryDelay)
        delay.assign (static_cast<size_t> (latencySamples + 1), 0.0f);

    const auto rate = static_cast<float> (sampleRate * 4.0);
    attack = std::exp (-1.0f / (0.0015f * rate));
    release = std::exp (-1.0f / (0.040f * rate));
    dcCoefficient = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 25.0f / rate);
    const auto cutoff = juce::jmin (8500.0f, static_cast<float> (sampleRate) * 0.2f);
    lowPassCoefficient = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * cutoff / rate);
    amountSmooth.reset (sampleRate, 0.035);
    zeroOffset = shape (phaseBias, selectedWaveform);
    reset();
}

void SawProcessor::reset() noexcept
{
    if (oversampling != nullptr)
        oversampling->reset();
    delayedDry.clear();
    for (auto& delay : dryDelay)
        std::fill (delay.begin(), delay.end(), 0.0f);
    dcState = {}; lowPass1 = {}; lowPass2 = {};
    envelope = 0.0f;
    residualActive = false;
    delayPosition = 0;
    amountSmooth.setCurrentAndTargetValue (targetAmount);
}

void SawProcessor::setParameters (bool enabled, float amount, Waveform waveform, bool immediate) noexcept
{
    if (waveform != selectedWaveform)
    {
        selectedWaveform = waveform;
        // Each shape has its own value at the bias point, so the offset that
        // keeps a silent input silent has to be re-measured on every change.
        zeroOffset = shape (phaseBias, selectedWaveform);
    }
    targetAmount = enabled && std::isfinite (amount) ? juce::jlimit (0.0f, 1.0f, amount) : 0.0f;
    if (immediate)
        amountSmooth.setCurrentAndTargetValue (targetAmount);
    else
        amountSmooth.setTargetValue (targetAmount);
}

float SawProcessor::shape (float phase, Waveform waveform) noexcept
{
    // Tapered Fourier partials form a smooth periodic amplitude transfer, not a
    // time-domain oscillator. The taper keeps the slope finite so there is no
    // hard wrap discontinuity to alias, and each waveform is simply a different
    // weighting of the same partials.
    const auto sine = std::sin (phase);
    const auto cosine = std::cos (phase);
    auto previous = 0.0f;
    auto current = sine;
    auto taper = harmonicTaper;
    auto sum = 0.0f;

    for (int harmonic = 1; harmonic <= harmonicCount; ++harmonic)
    {
        const auto odd = (harmonic % 2) != 0;
        switch (waveform)
        {
            case Waveform::saw:
                sum += taper * current / static_cast<float> (harmonic);
                break;
            case Waveform::square:
                if (odd)
                    sum += taper * current / static_cast<float> (harmonic);
                break;
            case Waveform::triangle:
                if (odd)
                {
                    const auto alternating = ((harmonic - 1) / 2) % 2 == 0 ? 1.0f : -1.0f;
                    sum += alternating * taper * current
                         / static_cast<float> (harmonic * harmonic);
                }
                break;
        }
        const auto next = 2.0f * cosine * current - previous;
        previous = current;
        current = next;
        taper *= harmonicTaper;
    }

    switch (waveform)
    {
        case Waveform::square:   return sum * (4.0f / juce::MathConstants<float>::pi);
        case Waveform::triangle: return sum * (8.0f / (juce::MathConstants<float>::pi
                                                     * juce::MathConstants<float>::pi));
        case Waveform::saw:
        default:                 return sum * (2.0f / juce::MathConstants<float>::pi);
    }
}

void SawProcessor::process (juce::AudioBuffer<float>& buffer) noexcept
{
    if (oversampling == nullptr || buffer.getNumSamples() == 0)
        return;
    const auto channels = juce::jmin (preparedChannels, buffer.getNumChannels());
    const auto samples = buffer.getNumSamples();
    jassert (samples <= delayedDry.getNumSamples());
    if (samples > delayedDry.getNumSamples() || channels == 0)
        return;

    // The reference only ever sees an integer delay. The filtered path carries
    // ONLY the nonlinear difference, so a settled bypass is sample exact after
    // that delay and the reported latency never changes as SAW is switched.
    for (int sample = 0; sample < samples; ++sample)
    {
        const auto readPosition = (delayPosition + 1) % (latencySamples + 1);
        for (int channel = 0; channel < channels; ++channel)
        {
            auto input = buffer.getSample (channel, sample);
            if (! std::isfinite (input)) input = 0.0f;
            auto& delay = dryDelay[static_cast<size_t> (channel)];
            delay[static_cast<size_t> (delayPosition)] = input;
            delayedDry.setSample (channel, sample, delay[static_cast<size_t> (readPosition)]);
            // Bound the nonlinear branch's analysis only, never the reference.
            buffer.setSample (channel, sample, juce::jlimit (-32.0f, 32.0f, input));
        }
        delayPosition = readPosition;
    }

    if (! amountSmooth.isSmoothing() && amountSmooth.getCurrentValue() == 0.0f)
    {
        if (residualActive)
        {
            oversampling->reset();
            dcState = {}; lowPass1 = {}; lowPass2 = {};
            envelope = 0.0f;
            residualActive = false;
        }
        for (int channel = 0; channel < channels; ++channel)
            buffer.copyFrom (channel, 0, delayedDry, channel, 0, samples);
        return;
    }
    // A fresh enable starts the filters from silence underneath the 35 ms fade.
    residualActive = true;

    const juce::AudioBuffer<float>& inputBuffer = buffer;
    auto up = oversampling->processSamplesUp (juce::dsp::AudioBlock<const float> (inputBuffer));
    for (size_t sample = 0; sample < up.getNumSamples(); ++sample)
    {
        auto peak = 0.0f;
        for (int channel = 0; channel < channels; ++channel)
            peak = juce::jmax (peak, std::abs (up.getSample (channel, static_cast<int> (sample))));
        const auto coefficient = peak > envelope ? attack : release;
        envelope = peak + coefficient * (envelope - peak);
        const auto scale = juce::jmax (envelope, 0.025f);
        for (int channel = 0; channel < channels; ++channel)
        {
            const auto index = static_cast<size_t> (channel);
            const auto input = up.getSample (channel, static_cast<int> (sample));
            const auto normalised = input / scale;
            const auto phase = 4.1f * std::tanh (normalised) + phaseBias;
            const auto excursion = 0.70f * (shape (phase, selectedWaveform) - zeroOffset);
            // Saturating the sum is what couples the shape to the source.
            // Subtracting the unexcited response isolates the added texture.
            const auto difference = scale / 1.4f
                * (std::tanh (1.4f * (normalised + excursion)) - std::tanh (1.4f * normalised));
            dcState[index] += dcCoefficient * (difference - dcState[index]);
            const auto ac = difference - dcState[index];
            lowPass1[index] += lowPassCoefficient * (ac - lowPass1[index]);
            lowPass2[index] += lowPassCoefficient * (lowPass1[index] - lowPass2[index]);
            up.setSample (channel, static_cast<int> (sample), lowPass2[index]);
        }
    }

    juce::dsp::AudioBlock<float> down (buffer);
    oversampling->processSamplesDown (down);
    for (int sample = 0; sample < samples; ++sample)
    {
        const auto amount = amountSmooth.getNextValue();
        for (int channel = 0; channel < channels; ++channel)
        {
            const auto reference = delayedDry.getSample (channel, sample);
            const auto residual = buffer.getSample (channel, sample);
            buffer.setSample (channel, sample,
                amount == 0.0f ? reference : reference + amount * residual);
        }
    }
}
}
