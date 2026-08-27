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
