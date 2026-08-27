#pragma once

#include <JuceHeader.h>

namespace voidworm
{
struct ReactorEqSettings
{
    float hp = 20.0f;
    float focusFrequency = 1000.0f;
    float focusGainDb = 0.0f;
    float lp = 20000.0f;
    float focus2Frequency = 2200.0f;
    float focus2GainDb = 0.0f;
};

inline ReactorEqSettings massEqDefaults() noexcept     { return { 20.0f, 235.0f, 0.0f, 20000.0f, 700.0f, 0.0f }; }
inline ReactorEqSettings furnaceEqDefaults() noexcept  { return { 82.0f, 720.0f, 0.0f, 20000.0f, 2200.0f, 0.0f }; }
inline ReactorEqSettings arcEqDefaults() noexcept      { return { 20.0f, 2850.0f, 0.0f, 20000.0f, 6500.0f, 0.0f }; }
inline ReactorEqSettings feedbackEqDefaults() noexcept { return { 20.0f, 4200.0f, 0.0f, 20000.0f, 2000.0f, 0.0f }; }

class ReactorPreEq
{
public:
    void prepare (double newSampleRate) noexcept;
    void reset() noexcept;
    void process (juce::dsp::AudioBlock<float>& block, ReactorEqSettings settings) noexcept;
    uint32_t getAndClearDspFaultCount() noexcept;

    static ReactorEqSettings sanitise (double sampleRate, ReactorEqSettings settings) noexcept;
    static float getResponseMagnitude (double sampleRate, float frequency,
                                       ReactorEqSettings settings) noexcept;

private:
    struct Coefficients
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
    };

    struct FilterState
    {
        double z1 = 0.0;
        double z2 = 0.0;
        void reset() noexcept { z1 = z2 = 0.0; }
        float process (float input, const Coefficients& c, bool& fault) noexcept;
    };

    static Coefficients makeHighPass (double sampleRate, float frequency) noexcept;
    static Coefficients makePeak (double sampleRate, float frequency, float gainDb) noexcept;
    static Coefficients makeLowPass (double sampleRate, float frequency) noexcept;
    static Coefficients normalise (double b0, double b1, double b2,
                                   double a0, double a1, double a2) noexcept;
    static float responseMagnitude (const Coefficients&, double sampleRate, float frequency) noexcept;
    static void approach (Coefficients& current, const Coefficients& target, float amount) noexcept;
    static bool coefficientsAreFinite (const Coefficients&) noexcept;
    static bool coefficientsAreStable (const Coefficients&) noexcept;

    std::array<Coefficients, 4> currentCoefficients {};
    std::array<Coefficients, 4> targetCoefficients {};
    std::array<std::array<FilterState, 4>, 2> states {};
    ReactorEqSettings cachedSettings {};
    double sampleRate = 44100.0;
    float smoothingCoefficient = 1.0f;
    bool coefficientsInitialised = false;
    bool targetsInitialised = false;
    uint32_t dspFaultCount = 0;
};
}
