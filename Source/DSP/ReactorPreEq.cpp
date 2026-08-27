#include "ReactorPreEq.h"

namespace voidworm
{
namespace
{
constexpr float broadQ = 0.82f;
constexpr float butterworthQ = 0.70710678f;

float finiteOr (float value, float fallback) noexcept
{
    return std::isfinite (value) ? value : fallback;
}
}

bool ReactorPreEq::coefficientsAreFinite (const Coefficients& c) noexcept
{
    return std::isfinite (c.b0) && std::isfinite (c.b1) && std::isfinite (c.b2)
        && std::isfinite (c.a1) && std::isfinite (c.a2);
}

bool ReactorPreEq::coefficientsAreStable (const Coefficients& c) noexcept
{
    if (! coefficientsAreFinite (c))
        return false;

    // Jury stability conditions for 1 + a1 z^-1 + a2 z^-2. These linear
    // inequalities also remain valid while coefficients are interpolated.
    constexpr auto margin = 1.0e-12;
    return 1.0 + c.a1 + c.a2 > margin
        && 1.0 - c.a1 + c.a2 > margin
        && 1.0 - c.a2 > margin;
}

float ReactorPreEq::FilterState::process (float input, const Coefficients& c, bool& fault) noexcept
{
    const auto safeInput = std::isfinite (input) ? static_cast<double> (input) : 0.0;
    constexpr auto runawayLimit = 10000.0;
    if (! std::isfinite (input) || ! std::isfinite (z1) || ! std::isfinite (z2)
        || std::abs (z1) > runawayLimit || std::abs (z2) > runawayLimit)
    {
        reset();
        fault = true;
        return static_cast<float> (safeInput);
    }

    const auto output = c.b0 * safeInput + z1;
    const auto nextZ1 = c.b1 * safeInput - c.a1 * output + z2;
    const auto nextZ2 = c.b2 * safeInput - c.a2 * output;
    if (! std::isfinite (output) || ! std::isfinite (nextZ1) || ! std::isfinite (nextZ2)
        || std::abs (output) > runawayLimit || std::abs (nextZ1) > runawayLimit
        || std::abs (nextZ2) > runawayLimit)
    {
        reset();
        fault = true;
        return static_cast<float> (safeInput);
    }

    z1 = nextZ1;
    z2 = nextZ2;
    return static_cast<float> (output);
}

void ReactorPreEq::prepare (double newSampleRate) noexcept
{
    sampleRate = std::isfinite (newSampleRate) ? juce::jmax (1.0, newSampleRate) : 44100.0;
    smoothingCoefficient = 1.0f - std::exp (-1.0f / (0.020f * static_cast<float> (sampleRate)));
    reset();
}

void ReactorPreEq::reset() noexcept
{
    states = {};
    currentCoefficients = {};
    targetCoefficients = {};
    coefficientsInitialised = false;
    targetsInitialised = false;
    dspFaultCount = 0;
}

uint32_t ReactorPreEq::getAndClearDspFaultCount() noexcept
{
    const auto result = dspFaultCount;
    dspFaultCount = 0;
    return result;
}

ReactorPreEq::Coefficients ReactorPreEq::normalise (double b0, double b1, double b2,
                                                    double a0, double a1, double a2) noexcept
{
    if (! std::isfinite (b0) || ! std::isfinite (b1) || ! std::isfinite (b2)
        || ! std::isfinite (a0) || ! std::isfinite (a1) || ! std::isfinite (a2)
        || std::abs (a0) < 1.0e-14)
        return {};

    const auto inverseA0 = 1.0 / a0;
    const Coefficients result { b0 * inverseA0, b1 * inverseA0, b2 * inverseA0,
                                a1 * inverseA0, a2 * inverseA0 };
    return coefficientsAreStable (result) ? result : Coefficients {};
}

ReactorEqSettings ReactorPreEq::sanitise (double rate, ReactorEqSettings settings) noexcept
{
    const auto safeRate = std::isfinite (rate) ? juce::jmax (1.0, rate) : 44100.0;
    settings.hp = finiteOr (settings.hp, 20.0f);
    settings.focusFrequency = finiteOr (settings.focusFrequency, 1000.0f);
    settings.focusGainDb = finiteOr (settings.focusGainDb, 0.0f);
    settings.lp = finiteOr (settings.lp, 20000.0f);
    settings.focus2Frequency = finiteOr (settings.focus2Frequency, 2200.0f);
    settings.focus2GainDb = finiteOr (settings.focus2GainDb, 0.0f);
    const auto nyquistSafe = static_cast<float> (safeRate) * 0.45f;
    settings.lp = juce::jlimit (200.0f, juce::jmin (20000.0f, nyquistSafe), settings.lp);
    settings.hp = juce::jlimit (20.0f, juce::jmin (5000.0f, nyquistSafe), settings.hp);
    settings.focusFrequency = juce::jlimit (30.0f, juce::jmin (16000.0f, nyquistSafe),
                                             settings.focusFrequency);
    settings.focusGainDb = juce::jlimit (-12.0f, 12.0f, settings.focusGainDb);
    settings.focus2Frequency = juce::jlimit (30.0f, juce::jmin (16000.0f, nyquistSafe),
                                              settings.focus2Frequency);
    settings.focus2GainDb = juce::jlimit (-12.0f, 12.0f, settings.focus2GainDb);
    return settings;
}

ReactorPreEq::Coefficients ReactorPreEq::makeHighPass (double rate, float frequency) noexcept
{
    const auto omega = juce::MathConstants<double>::twoPi * static_cast<double> (frequency) / rate;
    const auto cosine = std::cos (omega);
    const auto alpha = std::sin (omega) / (2.0 * static_cast<double> (butterworthQ));
    return normalise ((1.0 + cosine) * 0.5, -(1.0 + cosine), (1.0 + cosine) * 0.5,
                      1.0 + alpha, -2.0 * cosine, 1.0 - alpha);
}

ReactorPreEq::Coefficients ReactorPreEq::makePeak (double rate, float frequency, float gainDb) noexcept
{
    const auto amplitude = std::pow (10.0, static_cast<double> (gainDb) / 40.0);
    const auto omega = juce::MathConstants<double>::twoPi * static_cast<double> (frequency) / rate;
    const auto cosine = std::cos (omega);
    const auto alpha = std::sin (omega) / (2.0 * static_cast<double> (broadQ));
    return normalise (1.0 + alpha * amplitude, -2.0 * cosine, 1.0 - alpha * amplitude,
                      1.0 + alpha / amplitude, -2.0 * cosine, 1.0 - alpha / amplitude);
}

ReactorPreEq::Coefficients ReactorPreEq::makeLowPass (double rate, float frequency) noexcept
{
    const auto omega = juce::MathConstants<double>::twoPi * static_cast<double> (frequency) / rate;
    const auto cosine = std::cos (omega);
    const auto alpha = std::sin (omega) / (2.0 * static_cast<double> (butterworthQ));
    return normalise ((1.0 - cosine) * 0.5, 1.0 - cosine, (1.0 - cosine) * 0.5,
                      1.0 + alpha, -2.0 * cosine, 1.0 - alpha);
}

void ReactorPreEq::approach (Coefficients& current, const Coefficients& target, float amount) noexcept
{
    if (! std::isfinite (amount))
    {
        current = target;
        return;
    }
    current.b0 += amount * (target.b0 - current.b0);
    current.b1 += amount * (target.b1 - current.b1);
    current.b2 += amount * (target.b2 - current.b2);
    current.a1 += amount * (target.a1 - current.a1);
    current.a2 += amount * (target.a2 - current.a2);
}

void ReactorPreEq::process (juce::dsp::AudioBlock<float>& block, ReactorEqSettings settings) noexcept
{
    const auto invalidSettings = ! std::isfinite (settings.hp)
        || ! std::isfinite (settings.focusFrequency)
        || ! std::isfinite (settings.focusGainDb)
        || ! std::isfinite (settings.lp)
        || ! std::isfinite (settings.focus2Frequency)
        || ! std::isfinite (settings.focus2GainDb);
    if (invalidSettings)
        ++dspFaultCount;

    settings = sanitise (sampleRate, settings);
    const auto settingsChanged = ! targetsInitialised
        || settings.hp != cachedSettings.hp
        || settings.focusFrequency != cachedSettings.focusFrequency
        || settings.focusGainDb != cachedSettings.focusGainDb
        || settings.lp != cachedSettings.lp
        || settings.focus2Frequency != cachedSettings.focus2Frequency
        || settings.focus2GainDb != cachedSettings.focus2GainDb;
    if (settingsChanged)
    {
        cachedSettings = settings;
        targetCoefficients = {
            makeHighPass (sampleRate, settings.hp),
            makePeak (sampleRate, settings.focusFrequency, settings.focusGainDb),
            makePeak (sampleRate, settings.focus2Frequency, settings.focus2GainDb),
            makeLowPass (sampleRate, settings.lp)
        };
        for (auto& target : targetCoefficients)
        {
            if (! coefficientsAreStable (target))
            {
                target = {};
                ++dspFaultCount;
            }
        }
        targetsInitialised = true;
    }

    if (! coefficientsInitialised)
    {
        currentCoefficients = targetCoefficients;
        coefficientsInitialised = true;
    }

    for (size_t stage = 0; stage < currentCoefficients.size(); ++stage)
    {
        if (! coefficientsAreStable (currentCoefficients[stage]))
        {
            currentCoefficients[stage] = targetCoefficients[stage];
            ++dspFaultCount;
        }
    }

    const auto channels = juce::jmin (block.getNumChannels(), states.size());
    for (size_t sample = 0; sample < block.getNumSamples(); ++sample)
    {
        for (size_t stage = 0; stage < currentCoefficients.size(); ++stage)
            approach (currentCoefficients[stage], targetCoefficients[stage], smoothingCoefficient);

        for (size_t channel = 0; channel < channels; ++channel)
        {
            auto value = block.getSample (static_cast<int> (channel), static_cast<int> (sample));
            for (size_t stage = 0; stage < currentCoefficients.size(); ++stage)
            {
                auto fault = false;
                value = states[channel][stage].process (value, currentCoefficients[stage], fault);
                if (fault)
                    ++dspFaultCount;
            }
            block.setSample (static_cast<int> (channel), static_cast<int> (sample), value);
        }
    }
}

float ReactorPreEq::responseMagnitude (const Coefficients& c, double rate, float frequency) noexcept
{
    if (! coefficientsAreStable (c) || ! std::isfinite (rate) || rate <= 0.0
        || ! std::isfinite (frequency))
        return 1.0f;
    const auto omega = juce::MathConstants<double>::twoPi * static_cast<double> (frequency) / rate;
    const std::complex<double> z1 = std::polar (1.0, -omega);
    const auto z2 = z1 * z1;
    const auto numerator = c.b0 + c.b1 * z1 + c.b2 * z2;
    const auto denominator = 1.0 + c.a1 * z1 + c.a2 * z2;
    const auto denominatorMagnitude = std::abs (denominator);
    if (! std::isfinite (denominatorMagnitude) || denominatorMagnitude < 1.0e-12)
        return 1.0f;
    const auto magnitude = static_cast<float> (std::abs (numerator / denominator));
    return std::isfinite (magnitude) ? magnitude : 1.0f;
}

float ReactorPreEq::getResponseMagnitude (double rate, float frequency,
                                          ReactorEqSettings settings) noexcept
{
    const auto safeRate = std::isfinite (rate) ? juce::jmax (1.0, rate) : 44100.0;
    settings = sanitise (safeRate, settings);
    return responseMagnitude (makeHighPass (safeRate, settings.hp), safeRate, frequency)
         * responseMagnitude (makePeak (safeRate, settings.focusFrequency, settings.focusGainDb), safeRate, frequency)
         * responseMagnitude (makePeak (safeRate, settings.focus2Frequency, settings.focus2GainDb), safeRate, frequency)
         * responseMagnitude (makeLowPass (safeRate, settings.lp), safeRate, frequency);
}
}
