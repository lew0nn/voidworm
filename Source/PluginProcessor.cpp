#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterLayout.h"

VoidwormAudioProcessor::VoidwormAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "VOIDWORM_STATE", createParameterLayout())
{
    cacheParameterHandles();
}

void VoidwormAudioProcessor::cacheParameterHandles()
{
    const auto resolve = [this] (const char* id)
    {
        auto* value = parameters.getRawParameterValue (id);
        // A missing ID is a layout mistake, not a runtime condition. Fail loudly
        // in debug rather than handing the audio thread a null to dereference.
        jassert (value != nullptr);
        return value;
    };
    handles.breach = resolve ("breach");
    handles.tear = resolve ("tear");
    handles.rot = resolve ("rot");
    handles.drive = resolve ("drive");
    handles.overload = resolve ("overload");
    handles.mix = resolve ("mix");
    handles.range = resolve ("range");
    handles.low = resolve ("low");
    handles.mid = resolve ("mid");
    handles.high = resolve ("high");
    handles.output = resolve ("output");
    handles.weld = resolve ("weld");
    handles.limiterEnabled = resolve ("limiterEnabled");
    handles.limiterThreshold = resolve ("limiterThreshold");
    handles.limiterCeiling = resolve ("limiterCeiling");
    handles.gateEnabled = resolve ("gateEnabled");
    handles.gateThreshold = resolve ("gateThreshold");
    handles.surge = resolve ("surge");
    handles.oversample = resolve ("oversample");
    handles.hqMode = resolve ("hqMode");
    constexpr std::array<const char*, 4> enabledIds {
        "massEnabled", "furnaceEnabled", "arcEnabled", "feedbackEnabled" };
    constexpr std::array<const char*, 4> amountIds {
        "massAmount", "furnaceAmount", "arcAmount", "feedbackAmount" };
    constexpr std::array<const char*, 8> characterIds {
        "massSaturation", "massHarmonics", "furnaceStarve", "furnaceFold",
        "arcXmod", "arcFold", "feedbackReturn", "feedbackDamp" };
    for (size_t index = 0; index < enabledIds.size(); ++index)
    {
        handles.reactorEnabled[index] = resolve (enabledIds[index]);
        handles.reactorAmounts[index] = resolve (amountIds[index]);
    }
    for (size_t index = 0; index < characterIds.size(); ++index)
        handles.character[index] = resolve (characterIds[index]);
    constexpr std::array<const char*, 4> eqPrefixes { "mass", "furnace", "arc", "feedback" };
    for (size_t index = 0; index < eqPrefixes.size(); ++index)
    {
        const juce::String prefix (eqPrefixes[index]);
        handles.eq[index] = { resolve ((prefix + "Hp").toRawUTF8()),
                              resolve ((prefix + "FocusFreq").toRawUTF8()),
                              resolve ((prefix + "FocusGain").toRawUTF8()),
                              resolve ((prefix + "Lp").toRawUTF8()),
                              resolve ((prefix + "Focus2Freq").toRawUTF8()),
                              resolve ((prefix + "Focus2Gain").toRawUTF8()) };
    }
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
    p.breach = handles.breach->load();
    p.tear = handles.tear->load();
    p.rot = handles.rot->load();
    p.driveDb = handles.drive->load();
    p.overload = handles.overload->load();
    p.mix = handles.mix->load();
    p.range = handles.range->load();
    p.lowDb = handles.low->load();
    p.midDb = handles.mid->load();
    p.highDb = handles.high->load();
    p.outputDb = handles.output->load();
    p.weld = handles.weld->load();
    p.limiterEnabled = handles.limiterEnabled->load() >= 0.5f;
    p.limiterThresholdDb = handles.limiterThreshold->load();
    p.limiterCeilingDb = handles.limiterCeiling->load();
    p.gateEnabled = handles.gateEnabled->load() >= 0.5f;
    p.gateThresholdDb = handles.gateThreshold->load();
    p.surge = handles.surge->load() >= 0.5f;
    constexpr std::array<int, 4> oversamplingFactors { 1, 2, 4, 8 };
    const auto oversamplingIndex = juce::jlimit (0, 3, juce::roundToInt (handles.oversample->load()));
    p.oversampleFactor = oversamplingFactors[static_cast<size_t> (oversamplingIndex)];
    p.hqMode = handles.hqMode->load() >= 0.5f;
    for (size_t index = 0; index < p.reactorEnabled.size(); ++index)
    {
        p.reactorEnabled[index] = handles.reactorEnabled[index]->load() >= 0.5f;
        p.reactorAmounts[index] = handles.reactorAmounts[index]->load();
    }
    p.reactorCharacter = {
        handles.character[0]->load(), handles.character[1]->load(),
        handles.character[2]->load(), handles.character[3]->load(),
        handles.character[4]->load(), handles.character[5]->load(),
        handles.character[6]->load(), handles.character[7]->load()
    };
    p.reactorSolo = getReactorSoloTarget();
    const auto readEq = [] (const EqHandles& eq)
    {
        return voidworm::ReactorEqSettings {
            eq.hp->load(), eq.focusFrequency->load(), eq.focusGain->load(),
            eq.lp->load(), eq.focus2Frequency->load(), eq.focus2Gain->load()
        };
    };
    p.massEq = readEq (handles.eq[0]);
    p.furnaceEq = readEq (handles.eq[1]);
    p.arcEq = readEq (handles.eq[2]);
    p.feedbackEq = readEq (handles.eq[3]);
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
