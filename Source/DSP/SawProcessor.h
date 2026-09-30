#pragma once

#include <JuceHeader.h>

namespace voidworm
{
// A smooth periodic transfer driven by the audio itself. There is no
// oscillator, note detection or free-running signal: the input's own envelope
// sets the operating point, so the stage stays silent when the source is.
class SawProcessor
{
public:
    enum class Waveform { saw = 0, square, triangle };

    void prepare (double sampleRate, int maximumBlockSize, int channels);
    void reset() noexcept;
    void setParameters (bool enabled, float amount, Waveform waveform, bool immediate = false) noexcept;
    void process (juce::AudioBuffer<float>& buffer) noexcept;

    // Reported whether or not the stage is enabled, so switching SAW never
    // moves the plugin's latency and never forces the host to re-sync.
    int getLatencySamples() const noexcept { return latencySamples; }

private:
    static float shape (float phase, Waveform waveform) noexcept;
    float lookupShape (float phase) const noexcept;

    // Each shape over one period of phase, sampled finely enough that a cubic
    // through the table is within 1.3e-7 of the partial sum (see prepare).
    static constexpr int shapeTableSize = 2048;
    std::array<std::vector<float>, 3> shapeTables;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::AudioBuffer<float> delayedDry;
    std::array<std::vector<float>, 2> dryDelay;
    std::array<float, 2> dcState {}, lowPass1 {}, lowPass2 {};
    juce::SmoothedValue<float> amountSmooth;
    Waveform selectedWaveform = Waveform::saw;
    float envelope = 0.0f;
    float attack = 0.0f, release = 0.0f, dcCoefficient = 0.0f, lowPassCoefficient = 0.0f;
    float zeroOffset = 0.0f;
    float targetAmount = 0.0f;
    bool residualActive = false;
    int latencySamples = 0, delayPosition = 0, preparedChannels = 2;
};
}
