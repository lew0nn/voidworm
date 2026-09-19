#pragma once

#include <JuceHeader.h>
#include "DSP/VoidEngine.h"
#include "PresetTransition.h"

class VoidwormAudioProcessor final : public juce::AudioProcessor
{
public:
    VoidwormAudioProcessor();
    ~VoidwormAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.25; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState parameters;
    float getVisualLevel() const noexcept
    {
        return juce::jmax (getOutputVisualLevel (0), getOutputVisualLevel (1));
    }
    float getInputVisualLevel() const noexcept
    {
        return juce::jmax (getInputVisualLevel (0), getInputVisualLevel (1));
    }
    float getInputVisualLevel (int channel) const noexcept
    {
        return inputMeterPeak[static_cast<size_t> (juce::jlimit (0, 1, channel))].load (std::memory_order_relaxed);
    }
    float getOutputVisualLevel (int channel) const noexcept
    {
        return outputMeterPeak[static_cast<size_t> (juce::jlimit (0, 1, channel))].load (std::memory_order_relaxed);
    }
    float getInputMeterRms (int channel) const noexcept
    {
        return inputMeterRms[static_cast<size_t> (juce::jlimit (0, 1, channel))].load (std::memory_order_relaxed);
    }
    float getOutputMeterRms (int channel) const noexcept
    {
        return outputMeterRms[static_cast<size_t> (juce::jlimit (0, 1, channel))].load (std::memory_order_relaxed);
    }
    uint32_t getMeterFaultCount() const noexcept { return meterFaultCount.load (std::memory_order_relaxed); }
    float getTearActivity() const noexcept { return engine.getTearActivity(); }
    bool isInputGateMuting() const noexcept { return engine.isInputGateMuting(); }
    float getWeldGainReductionDb() const noexcept { return engine.getWeldGainReductionDb(); }
    float getLimiterGainReductionDb() const noexcept { return engine.getLimiterGainReductionDb(); }
    void setReactorSoloTarget (int target) noexcept
    {
        transientReactorSolo.store (juce::jlimit (0, 4, target), std::memory_order_relaxed);
    }
    int getReactorSoloTarget() const noexcept
    {
        return transientReactorSolo.load (std::memory_order_relaxed);
    }
    voidworm::Parameters captureParameterSnapshot() noexcept;
    void beginPresetChange() noexcept
    {
        presetWriteInProgress.store (true, std::memory_order_release);
    }
    bool commitPresetChange (const voidworm::PresetSnapshot& snapshot) noexcept;

private:
    // Resolved once so the audio thread never hashes a parameter ID string.
    struct EqHandles
    {
        std::atomic<float>* hp = nullptr;
        std::atomic<float>* focusFrequency = nullptr;
        std::atomic<float>* focusGain = nullptr;
        std::atomic<float>* lp = nullptr;
        std::atomic<float>* focus2Frequency = nullptr;
        std::atomic<float>* focus2Gain = nullptr;
    };
    struct ParameterHandles
    {
        std::atomic<float>* breach = nullptr;
        std::atomic<float>* tear = nullptr;
        std::atomic<float>* rot = nullptr;
        std::atomic<float>* drive = nullptr;
        std::atomic<float>* overload = nullptr;
        std::atomic<float>* mix = nullptr;
        std::atomic<float>* range = nullptr;
        std::atomic<float>* low = nullptr;
        std::atomic<float>* mid = nullptr;
        std::atomic<float>* high = nullptr;
        std::atomic<float>* output = nullptr;
        std::atomic<float>* weld = nullptr;
        std::atomic<float>* limiterEnabled = nullptr;
        std::atomic<float>* limiterThreshold = nullptr;
        std::atomic<float>* limiterCeiling = nullptr;
        std::atomic<float>* gateEnabled = nullptr;
        std::atomic<float>* gateThreshold = nullptr;
        std::atomic<float>* surge = nullptr;
        std::atomic<float>* oversample = nullptr;
        std::atomic<float>* hqMode = nullptr;
        std::array<std::atomic<float>*, 4> reactorEnabled {};
        std::array<std::atomic<float>*, 4> reactorAmounts {};
        std::array<std::atomic<float>*, 8> character {};
        std::array<EqHandles, 4> eq {};
    };
    void cacheParameterHandles();
    ParameterHandles handles;
    void resetMeters() noexcept;
    void publishMeterBlock (const juce::AudioBuffer<float>&, bool input) noexcept;
    voidworm::VoidEngine engine;
    voidworm::PresetSnapshotMailbox presetMailbox;
    voidworm::Parameters activeParameters;
    bool activeParametersInitialised = false;
    std::atomic<bool> presetWriteInProgress { false };
    std::atomic<bool> presetMailboxOverflow { false };
    std::atomic<int> transientReactorSolo { 0 };
    std::array<std::atomic<float>, 2> inputMeterPeak { std::atomic<float> { 0.0f }, std::atomic<float> { 0.0f } };
    std::array<std::atomic<float>, 2> inputMeterRms { std::atomic<float> { 0.0f }, std::atomic<float> { 0.0f } };
    std::array<std::atomic<float>, 2> outputMeterPeak { std::atomic<float> { 0.0f }, std::atomic<float> { 0.0f } };
    std::array<std::atomic<float>, 2> outputMeterRms { std::atomic<float> { 0.0f }, std::atomic<float> { 0.0f } };
    std::atomic<uint32_t> meterFaultCount { 0 };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoidwormAudioProcessor)
};
