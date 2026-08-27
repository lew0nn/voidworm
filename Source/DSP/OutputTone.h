#pragma once

#include <JuceHeader.h>

namespace voidworm
{
class OutputTone
{
public:
    void prepare (double sampleRate, int channels) noexcept;
    void reset() noexcept;
    void resetForPresetChange (float range, float lowDb, float midDb, float highDb) noexcept;
    void update (float range, float lowDb, float midDb, float highDb) noexcept;
    float processSample (int channel, float input) noexcept;
    uint32_t getAndClearDspFaultCount() noexcept;
#if VOIDWORM_ENABLE_DIAGNOSTICS
    void beginDiagnosticsBlock() noexcept { maximumStateMagnitude = 0.0f; }
    float getMaximumStateMagnitude() const noexcept { return maximumStateMagnitude; }
#endif

    static float getResponseMagnitude (double sampleRate, float frequency, float range,
                                       float lowDb, float midDb, float highDb) noexcept;
    static float getHighPassFrequency (float range) noexcept;
    static float getLowPassFrequency (float range) noexcept;

    struct Coefficients
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
    };

private:

    struct BiquadState
    {
        float z1 = 0.0f, z2 = 0.0f;
        void reset() noexcept { z1 = z2 = 0.0f; }
        float process (float input, const Coefficients& coefficients, bool& fault) noexcept;
    };

    struct ChannelState
    {
        std::array<BiquadState, 5> filters;
    };

    static Coefficients makeHighPass (double sampleRate, float frequency, float q) noexcept;
    static Coefficients makeLowPass (double sampleRate, float frequency, float q) noexcept;
    static Coefficients makePeak (double sampleRate, float frequency, float q, float gainDb) noexcept;
    static Coefficients makeLowShelf (double sampleRate, float frequency, float gainDb) noexcept;
    static Coefficients makeHighShelf (double sampleRate, float frequency, float gainDb) noexcept;
    static float coefficientMagnitude (const Coefficients&, double sampleRate, float frequency) noexcept;
    static float logarithmicCurve (float position, float start, float middle, float end) noexcept;
    void updateCoefficients() noexcept;
    void beginCoefficientTransition() noexcept;
    float processBankSample (int bank, int channel, float input) noexcept;

    std::array<std::array<ChannelState, 2>, 2> states {};
    std::array<std::array<Coefficients, 5>, 2> bankCoefficients {};
    std::array<Coefficients, 5> targetCoefficients {};
    double sampleRate = 44100.0;
    int channelCount = 2;
    int activeBank = 0;
    int transitionBank = 1;
    int transitionSamples = 1;
    int transitionSamplesRemaining = 0;
    bool pendingTargetChange = false;
    bool coefficientsInitialised = false;
    float currentRange = -1.0f;
    float currentLowDb = 100.0f;
    float currentMidDb = 100.0f;
    float currentHighDb = 100.0f;
    uint32_t dspFaultCount = 0;
#if VOIDWORM_ENABLE_DIAGNOSTICS
    float maximumStateMagnitude = 0.0f;
#endif
};
}
