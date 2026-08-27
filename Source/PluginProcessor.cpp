#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterLayout.h"

VoidwormAudioProcessor::VoidwormAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "VOIDWORM_STATE", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout VoidwormAudioProcessor::createParameterLayout()
{
    return createVoidwormParameterLayout();
}

void VoidwormAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    resetMeters();
    engine.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    activeParameters = captureParameterSnapshot();
    activeParametersInitialised = true;
    engine.setTargets (activeParameters);
    setLatencySamples (engine.getLatencySamples());
}

void VoidwormAudioProcessor::releaseResources() {}

void VoidwormAudioProcessor::resetMeters() noexcept
{
    for (auto& value : inputMeterPeak) value.store (0.0f, std::memory_order_relaxed);
    for (auto& value : inputMeterRms) value.store (0.0f, std::memory_order_relaxed);
    for (auto& value : outputMeterPeak) value.store (0.0f, std::memory_order_relaxed);
    for (auto& value : outputMeterRms) value.store (0.0f, std::memory_order_relaxed);
    meterFaultCount.store (0, std::memory_order_relaxed);
}

void VoidwormAudioProcessor::publishMeterBlock (const juce::AudioBuffer<float>& buffer, bool input) noexcept
{
    auto& peaks = input ? inputMeterPeak : outputMeterPeak;
    auto& rmsValues = input ? inputMeterRms : outputMeterRms;
    const auto channels = juce::jmax (1, buffer.getNumChannels());
    const auto samples = buffer.getNumSamples();
    for (int displayChannel = 0; displayChannel < 2; ++displayChannel)
    {
        const auto sourceChannel = juce::jmin (displayChannel, channels - 1);
        auto peak = 0.0f;
        double sumSquares = 0.0;
        uint32_t finiteSamples = 0;
        if (sourceChannel < buffer.getNumChannels())
            for (int sample = 0; sample < samples; ++sample)
            {
                const auto value = buffer.getSample (sourceChannel, sample);
                if (! std::isfinite (value))
                {
                    meterFaultCount.fetch_add (1, std::memory_order_relaxed);
                    continue;
                }
                peak = juce::jmax (peak, std::abs (value));
                sumSquares += static_cast<double> (value) * static_cast<double> (value);
                ++finiteSamples;
            }
        const auto rms = finiteSamples == 0 ? 0.0f
            : static_cast<float> (std::sqrt (sumSquares / static_cast<double> (finiteSamples)));
        peaks[static_cast<size_t> (displayChannel)].store (peak, std::memory_order_relaxed);
        rmsValues[static_cast<size_t> (displayChannel)].store (rms, std::memory_order_relaxed);
    }
}

bool VoidwormAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    return (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo())
        && output == layouts.getMainInputChannelSet();
}

voidworm::Parameters VoidwormAudioProcessor::captureParameterSnapshot() noexcept
{
    voidworm::Parameters p;
    p.breach = parameters.getRawParameterValue ("breach")->load();
    p.tear = parameters.getRawParameterValue ("tear")->load();
    p.rot = parameters.getRawParameterValue ("rot")->load();
    p.driveDb = parameters.getRawParameterValue ("drive")->load();
    p.overload = parameters.getRawParameterValue ("overload")->load();
    p.mix = parameters.getRawParameterValue ("mix")->load();
    p.range = parameters.getRawParameterValue ("range")->load();
    p.lowDb = parameters.getRawParameterValue ("low")->load();
    p.midDb = parameters.getRawParameterValue ("mid")->load();
    p.highDb = parameters.getRawParameterValue ("high")->load();
    p.outputDb = parameters.getRawParameterValue ("output")->load();
    p.weld = parameters.getRawParameterValue ("weld")->load();
    p.limiterEnabled = parameters.getRawParameterValue ("limiterEnabled")->load() >= 0.5f;
    p.limiterThresholdDb = parameters.getRawParameterValue ("limiterThreshold")->load();
    p.limiterCeilingDb = parameters.getRawParameterValue ("limiterCeiling")->load();
    p.gateEnabled = parameters.getRawParameterValue ("gateEnabled")->load() >= 0.5f;
    p.gateThresholdDb = parameters.getRawParameterValue ("gateThreshold")->load();
    p.surge = parameters.getRawParameterValue ("surge")->load() >= 0.5f;
    constexpr std::array<int, 4> oversamplingFactors { 1, 2, 4, 8 };
    const auto oversamplingIndex = juce::jlimit (0, 3,
        juce::roundToInt (parameters.getRawParameterValue ("oversample")->load()));
    p.oversampleFactor = oversamplingFactors[static_cast<size_t> (oversamplingIndex)];
    p.hqMode = parameters.getRawParameterValue ("hqMode")->load() >= 0.5f;
    p.reactorEnabled = {
        parameters.getRawParameterValue ("massEnabled")->load() >= 0.5f,
        parameters.getRawParameterValue ("furnaceEnabled")->load() >= 0.5f,
        parameters.getRawParameterValue ("arcEnabled")->load() >= 0.5f,
        parameters.getRawParameterValue ("feedbackEnabled")->load() >= 0.5f
    };
    p.reactorAmounts = {
        parameters.getRawParameterValue ("massAmount")->load(),
        parameters.getRawParameterValue ("furnaceAmount")->load(),
        parameters.getRawParameterValue ("arcAmount")->load(),
        parameters.getRawParameterValue ("feedbackAmount")->load()
    };
    p.reactorCharacter = {
        parameters.getRawParameterValue ("massSaturation")->load(),
        parameters.getRawParameterValue ("massHarmonics")->load(),
        parameters.getRawParameterValue ("furnaceStarve")->load(),
        parameters.getRawParameterValue ("furnaceFold")->load(),
        parameters.getRawParameterValue ("arcXmod")->load(),
        parameters.getRawParameterValue ("arcFold")->load(),
        parameters.getRawParameterValue ("feedbackReturn")->load(),
        parameters.getRawParameterValue ("feedbackDamp")->load()
    };
    p.reactorSolo = getReactorSoloTarget();
    const auto readEq = [this] (const char* hp, const char* focus, const char* gain, const char* lp,
                                const char* focus2, const char* gain2)
    {
        return voidworm::ReactorEqSettings {
            parameters.getRawParameterValue (hp)->load(),
            parameters.getRawParameterValue (focus)->load(),
            parameters.getRawParameterValue (gain)->load(),
            parameters.getRawParameterValue (lp)->load(),
            parameters.getRawParameterValue (focus2)->load(),
            parameters.getRawParameterValue (gain2)->load()
        };
    };
    p.massEq = readEq ("massHp", "massFocusFreq", "massFocusGain", "massLp", "massFocus2Freq", "massFocus2Gain");
    p.furnaceEq = readEq ("furnaceHp", "furnaceFocusFreq", "furnaceFocusGain", "furnaceLp", "furnaceFocus2Freq", "furnaceFocus2Gain");
    p.arcEq = readEq ("arcHp", "arcFocusFreq", "arcFocusGain", "arcLp", "arcFocus2Freq", "arcFocus2Gain");
    p.feedbackEq = readEq ("feedbackHp", "feedbackFocusFreq", "feedbackFocusGain", "feedbackLp", "feedbackFocus2Freq", "feedbackFocus2Gain");
    return p;
}

bool VoidwormAudioProcessor::commitPresetChange (const voidworm::PresetSnapshot& snapshot) noexcept
{
    const auto queued = presetMailbox.push (snapshot);
    if (! queued)
        presetMailboxOverflow.store (true, std::memory_order_release);
    presetWriteInProgress.store (false, std::memory_order_release);
    return queued;
}

void VoidwormAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());
    publishMeterBlock (buffer, true);

    voidworm::PresetSnapshot requested;
    auto hasPresetRequest = false;
    while (presetMailbox.pop (requested))
        hasPresetRequest = true;

    const auto presetWritesActive = presetWriteInProgress.load (std::memory_order_acquire);
    if (! presetWritesActive && presetMailboxOverflow.exchange (false, std::memory_order_acq_rel))
    {
        requested.parameters = captureParameterSnapshot();
        hasPresetRequest = true;
    }

    if (hasPresetRequest)
    {
        activeParameters = requested.parameters;
        activeParameters.reactorSolo = getReactorSoloTarget();
        activeParametersInitialised = true;
        engine.beginPresetTransition (activeParameters);
    }
    else if (! presetWritesActive && ! engine.isPresetTransitionActive())
    {
        activeParameters = captureParameterSnapshot();
        activeParametersInitialised = true;
        engine.setTargets (activeParameters);
    }
    else if (! activeParametersInitialised)
    {
        activeParameters = {};
        activeParametersInitialised = true;
        engine.setTargets (activeParameters);
    }

    if (! hasPresetRequest && activeParametersInitialised)
    {
        const auto solo = getReactorSoloTarget();
        if (solo != activeParameters.reactorSolo)
        {
            activeParameters.reactorSolo = solo;
            if (! engine.isPresetTransitionActive())
                engine.setTargets (activeParameters);
        }
    }

    engine.process (buffer);
    publishMeterBlock (buffer, false);
}

juce::AudioProcessorEditor* VoidwormAudioProcessor::createEditor()
{
    return new VoidwormAudioProcessorEditor (*this);
}

void VoidwormAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void VoidwormAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    setReactorSoloTarget (0);
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
        {
            beginPresetChange();
            auto restoredState = juce::ValueTree::fromXml (*xml);
            restoredState.removeProperty ("reactorSolo", nullptr);
            parameters.replaceState (restoredState);
            commitPresetChange ({ captureParameterSnapshot() });
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoidwormAudioProcessor();
}
