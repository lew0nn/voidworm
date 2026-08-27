#include "OutputTone.h"
#include <complex>

namespace voidworm
{
namespace
{
constexpr float butterworthQ = 0.70710678f;

bool coefficientsAreFinite (const OutputTone::Coefficients& coefficients) noexcept
{
    return std::isfinite (coefficients.b0) && std::isfinite (coefficients.b1)
        && std::isfinite (coefficients.b2) && std::isfinite (coefficients.a1)
        && std::isfinite (coefficients.a2);
}

OutputTone::Coefficients normalise (float b0, float b1, float b2,
                                    float a0, float a1, float a2) noexcept
{
    if (! std::isfinite (b0) || ! std::isfinite (b1) || ! std::isfinite (b2)
        || ! std::isfinite (a0) || ! std::isfinite (a1) || ! std::isfinite (a2)
        || std::abs (a0) < 1.0e-8f)
        return {};
    const auto inverse = 1.0f / a0;
    const OutputTone::Coefficients result {
        b0 * inverse, b1 * inverse, b2 * inverse, a1 * inverse, a2 * inverse
    };
    return coefficientsAreFinite (result) ? result : OutputTone::Coefficients {};
}
}

float OutputTone::BiquadState::process (float input, const Coefficients& coefficients, bool& fault) noexcept
{
    const auto safeInput = std::isfinite (input) ? input : 0.0f;
    // The maximum configured EQ response cannot produce internal state near this
    // bound from the rack's bounded signal. Crossing it indicates corrupted or
    // coefficient-incompatible state, not ordinary program material.
    constexpr auto runawayLimit = 128.0f;
    if (! std::isfinite (input) || ! std::isfinite (z1) || ! std::isfinite (z2)
        || std::abs (z1) > runawayLimit || std::abs (z2) > runawayLimit)
    {
        reset();
        fault = true;
        return safeInput;
    }
    const auto output = coefficients.b0 * safeInput + z1;
    const auto nextZ1 = coefficients.b1 * safeInput - coefficients.a1 * output + z2;
    const auto nextZ2 = coefficients.b2 * safeInput - coefficients.a2 * output;
    if (! std::isfinite (output) || ! std::isfinite (nextZ1) || ! std::isfinite (nextZ2)
        || std::abs (output) > runawayLimit || std::abs (nextZ1) > runawayLimit
        || std::abs (nextZ2) > runawayLimit)
    {
        reset();
        fault = true;
        return safeInput;
    }
    z1 = nextZ1;
    z2 = nextZ2;
    return output;
}

float OutputTone::logarithmicCurve (float position, float start, float middle, float end) noexcept
{
    const auto t = juce::jlimit (0.0f, 1.0f, position);
    const auto l0 = std::log (start);
    const auto lm = std::log (middle);
    const auto l1 = std::log (end);
    const auto value = l0 * (2.0f * (t - 0.5f) * (t - 1.0f))
                     + lm * (-4.0f * t * (t - 1.0f))
                     + l1 * (2.0f * t * (t - 0.5f));
    return std::exp (value);
}

float OutputTone::getHighPassFrequency (float range) noexcept
{
    return logarithmicCurve (range, 150.0f, 52.0f, 22.0f);
}

float OutputTone::getLowPassFrequency (float range) noexcept
{
    return logarithmicCurve (range, 6000.0f, 13000.0f, 19500.0f);
}

OutputTone::Coefficients OutputTone::makeHighPass (double rate, float frequency, float q) noexcept
{
    const auto omega = juce::MathConstants<float>::twoPi * frequency / static_cast<float> (rate);
    const auto cosine = std::cos (omega);
    const auto alpha = std::sin (omega) / (2.0f * q);
    return normalise ((1.0f + cosine) * 0.5f, -(1.0f + cosine), (1.0f + cosine) * 0.5f,
                      1.0f + alpha, -2.0f * cosine, 1.0f - alpha);
}

OutputTone::Coefficients OutputTone::makeLowPass (double rate, float frequency, float q) noexcept
{
    const auto omega = juce::MathConstants<float>::twoPi * frequency / static_cast<float> (rate);
    const auto cosine = std::cos (omega);
    const auto alpha = std::sin (omega) / (2.0f * q);
    return normalise ((1.0f - cosine) * 0.5f, 1.0f - cosine, (1.0f - cosine) * 0.5f,
                      1.0f + alpha, -2.0f * cosine, 1.0f - alpha);
}

OutputTone::Coefficients OutputTone::makePeak (double rate, float frequency, float q, float gainDb) noexcept
{
    const auto amplitude = std::pow (10.0f, gainDb / 40.0f);
    const auto omega = juce::MathConstants<float>::twoPi * frequency / static_cast<float> (rate);
    const auto cosine = std::cos (omega);
    const auto alpha = std::sin (omega) / (2.0f * q);
    return normalise (1.0f + alpha * amplitude, -2.0f * cosine, 1.0f - alpha * amplitude,
                      1.0f + alpha / amplitude, -2.0f * cosine, 1.0f - alpha / amplitude);
}

OutputTone::Coefficients OutputTone::makeLowShelf (double rate, float frequency, float gainDb) noexcept
{
    const auto amplitude = std::pow (10.0f, gainDb / 40.0f);
    const auto omega = juce::MathConstants<float>::twoPi * frequency / static_cast<float> (rate);
    const auto cosine = std::cos (omega);
    const auto sine = std::sin (omega);
    const auto alpha = sine * std::sqrt (2.0f) * 0.5f;
    const auto twoRootAAlpha = 2.0f * std::sqrt (amplitude) * alpha;
    return normalise (amplitude * ((amplitude + 1.0f) - (amplitude - 1.0f) * cosine + twoRootAAlpha),
                      2.0f * amplitude * ((amplitude - 1.0f) - (amplitude + 1.0f) * cosine),
                      amplitude * ((amplitude + 1.0f) - (amplitude - 1.0f) * cosine - twoRootAAlpha),
                      (amplitude + 1.0f) + (amplitude - 1.0f) * cosine + twoRootAAlpha,
                      -2.0f * ((amplitude - 1.0f) + (amplitude + 1.0f) * cosine),
                      (amplitude + 1.0f) + (amplitude - 1.0f) * cosine - twoRootAAlpha);
}

OutputTone::Coefficients OutputTone::makeHighShelf (double rate, float frequency, float gainDb) noexcept
{
    const auto amplitude = std::pow (10.0f, gainDb / 40.0f);
    const auto omega = juce::MathConstants<float>::twoPi * frequency / static_cast<float> (rate);
    const auto cosine = std::cos (omega);
    const auto sine = std::sin (omega);
    const auto alpha = sine * std::sqrt (2.0f) * 0.5f;
    const auto twoRootAAlpha = 2.0f * std::sqrt (amplitude) * alpha;
    return normalise (amplitude * ((amplitude + 1.0f) + (amplitude - 1.0f) * cosine + twoRootAAlpha),
                      -2.0f * amplitude * ((amplitude - 1.0f) + (amplitude + 1.0f) * cosine),
                      amplitude * ((amplitude + 1.0f) + (amplitude - 1.0f) * cosine - twoRootAAlpha),
                      (amplitude + 1.0f) - (amplitude - 1.0f) * cosine + twoRootAAlpha,
                      2.0f * ((amplitude - 1.0f) - (amplitude + 1.0f) * cosine),
                      (amplitude + 1.0f) - (amplitude - 1.0f) * cosine - twoRootAAlpha);
}

void OutputTone::prepare (double newSampleRate, int channels) noexcept
{
    sampleRate = std::isfinite (newSampleRate) ? juce::jmax (1.0, newSampleRate) : 44100.0;
    channelCount = juce::jlimit (1, 2, channels);
    transitionSamples = juce::jmax (1, juce::roundToInt (0.015 * sampleRate));
    coefficientsInitialised = false;
    currentRange = -1.0f;
    update (0.92f, 0.0f, 0.0f, 0.0f);
    reset();
}

void OutputTone::reset() noexcept
{
    states = {};
    activeBank = 0;
    transitionBank = 1;
    transitionSamplesRemaining = 0;
    pendingTargetChange = false;
    dspFaultCount = 0;
#if VOIDWORM_ENABLE_DIAGNOSTICS
    maximumStateMagnitude = 0.0f;
#endif
}

void OutputTone::resetForPresetChange (float range, float lowDb, float midDb, float highDb) noexcept
{
    states = {};
    update (range, lowDb, midDb, highDb);
    bankCoefficients[0] = targetCoefficients;
    bankCoefficients[1] = targetCoefficients;
    activeBank = 0;
    transitionBank = 1;
    transitionSamplesRemaining = 0;
    pendingTargetChange = false;
    coefficientsInitialised = true;
    dspFaultCount = 0;
}

uint32_t OutputTone::getAndClearDspFaultCount() noexcept
{
    const auto result = dspFaultCount;
    dspFaultCount = 0;
    return result;
}

void OutputTone::update (float range, float lowDb, float midDb, float highDb) noexcept
{
    range = std::isfinite (range) ? range : currentRange >= 0.0f ? currentRange : 0.92f;
    lowDb = std::isfinite (lowDb) ? lowDb : currentLowDb <= 12.0f ? currentLowDb : 0.0f;
    midDb = std::isfinite (midDb) ? midDb : currentMidDb <= 12.0f ? currentMidDb : 0.0f;
    highDb = std::isfinite (highDb) ? highDb : currentHighDb <= 12.0f ? currentHighDb : 0.0f;
    range = juce::jlimit (0.0f, 1.0f, range);
    lowDb = juce::jlimit (-12.0f, 12.0f, lowDb);
    midDb = juce::jlimit (-12.0f, 12.0f, midDb);
    highDb = juce::jlimit (-12.0f, 12.0f, highDb);
    if (std::abs (range - currentRange) < 0.0001f && std::abs (lowDb - currentLowDb) < 0.001f
        && std::abs (midDb - currentMidDb) < 0.001f && std::abs (highDb - currentHighDb) < 0.001f)
        return;
    currentRange = range;
    currentLowDb = lowDb;
    currentMidDb = midDb;
    currentHighDb = highDb;
    updateCoefficients();
}

void OutputTone::updateCoefficients() noexcept
{
    const auto nyquistSafe = static_cast<float> (sampleRate * 0.45);
    targetCoefficients[0] = makeHighPass (sampleRate, juce::jmin (nyquistSafe, getHighPassFrequency (currentRange)), butterworthQ);
    targetCoefficients[1] = makeLowShelf (sampleRate, juce::jmin (nyquistSafe, 125.0f), currentLowDb);
    targetCoefficients[2] = makePeak (sampleRate, juce::jmin (nyquistSafe, 1100.0f), 0.68f, currentMidDb);
    targetCoefficients[3] = makeHighShelf (sampleRate, juce::jmin (nyquistSafe, 6000.0f), currentHighDb);
    targetCoefficients[4] = makeLowPass (sampleRate, juce::jmin (nyquistSafe, getLowPassFrequency (currentRange)), butterworthQ);
    for (auto& coefficient : targetCoefficients)
        if (! coefficientsAreFinite (coefficient))
        {
            coefficient = {};
            ++dspFaultCount;
        }
    if (! coefficientsInitialised)
    {
        bankCoefficients[0] = targetCoefficients;
        bankCoefficients[1] = targetCoefficients;
        coefficientsInitialised = true;
        return;
    }

    if (transitionSamplesRemaining > 0)
        pendingTargetChange = true;
    else
        beginCoefficientTransition();
}

void OutputTone::beginCoefficientTransition() noexcept
{
    transitionBank = 1 - activeBank;
    bankCoefficients[static_cast<size_t> (transitionBank)] = targetCoefficients;
    states[static_cast<size_t> (transitionBank)] = {};
    transitionSamplesRemaining = transitionSamples;
    pendingTargetChange = false;
}

float OutputTone::processBankSample (int bank, int channel, float input) noexcept
{
    const auto safeBank = static_cast<size_t> (juce::jlimit (0, 1, bank));
    auto& state = states[safeBank][static_cast<size_t> (juce::jlimit (0, 1, channel))];
    const auto& coefficients = bankCoefficients[safeBank];
    auto output = input;
    for (size_t stage = 0; stage < coefficients.size(); ++stage)
    {
        auto fault = false;
        output = state.filters[stage].process (output, coefficients[stage], fault);
        if (fault)
            ++dspFaultCount;
#if VOIDWORM_ENABLE_DIAGNOSTICS
        maximumStateMagnitude = juce::jmax (maximumStateMagnitude,
            juce::jmax (std::abs (state.filters[stage].z1), std::abs (state.filters[stage].z2)));
#endif
    }
    return output;
}

float OutputTone::processSample (int channel, float input) noexcept
{
    const auto currentOutput = processBankSample (activeBank, channel, input);
    if (transitionSamplesRemaining <= 0)
        return currentOutput;

    const auto nextOutput = processBankSample (transitionBank, channel, input);
    const auto position = 1.0f - static_cast<float> (transitionSamplesRemaining)
                                 / static_cast<float> (transitionSamples);
    const auto output = juce::jmap (position, currentOutput, nextOutput);
    if (channel >= channelCount - 1 && --transitionSamplesRemaining <= 0)
    {
        activeBank = transitionBank;
        if (pendingTargetChange)
            beginCoefficientTransition();
    }
    return output;
}

float OutputTone::coefficientMagnitude (const Coefficients& coefficients, double rate, float frequency) noexcept
{
    const auto omega = juce::MathConstants<double>::twoPi * static_cast<double> (frequency) / rate;
    const std::complex<double> z1 = std::polar (1.0, -omega);
    const auto z2 = z1 * z1;
    const auto numerator = static_cast<double> (coefficients.b0)
                         + static_cast<double> (coefficients.b1) * z1
                         + static_cast<double> (coefficients.b2) * z2;
    const auto denominator = 1.0 + static_cast<double> (coefficients.a1) * z1
                            + static_cast<double> (coefficients.a2) * z2;
    return static_cast<float> (std::abs (numerator / denominator));
}

float OutputTone::getResponseMagnitude (double rate, float frequency, float range,
                                        float lowDb, float midDb, float highDb) noexcept
{
    const auto safeRate = juce::jmax (1.0, rate);
    const auto nyquistSafe = static_cast<float> (safeRate * 0.45);
    const std::array<Coefficients, 5> response {
        makeHighPass (safeRate, juce::jmin (nyquistSafe, getHighPassFrequency (range)), butterworthQ),
        makeLowShelf (safeRate, juce::jmin (nyquistSafe, 125.0f), lowDb),
        makePeak (safeRate, juce::jmin (nyquistSafe, 1100.0f), 0.68f, midDb),
        makeHighShelf (safeRate, juce::jmin (nyquistSafe, 6000.0f), highDb),
        makeLowPass (safeRate, juce::jmin (nyquistSafe, getLowPassFrequency (range)), butterworthQ)
    };
    auto magnitude = 1.0f;
    for (const auto& stage : response)
        magnitude *= coefficientMagnitude (stage, safeRate, frequency);
    return std::isfinite (magnitude) ? magnitude : 0.0f;
}
}
