#include "VoidEngine.h"
#include "PerformanceProfile.h"

namespace voidworm
{
namespace
{
float finiteClamped (float value, float low, float high, float fallback) noexcept
{
    return std::isfinite (value) ? juce::jlimit (low, high, value) : fallback;
}
}

void VoidEngine::prepare (double newSampleRate, int maximumBlockSize, int channels)
{
    jassert (newSampleRate > 0.0 && maximumBlockSize > 0);
    sampleRate = juce::jmax (1.0, newSampleRate);
    channelCount = juce::jlimit (1, 2, channels);
    outputTone.prepare (sampleRate, channelCount);
    inputNoiseGate.prepare (sampleRate);
    sourceAnalyzer.prepare (sampleRate);
    oversampling.prepare (channelCount, maximumBlockSize);
    reactorRack.prepare (sampleRate, maximumBlockSize, channelCount);
    tearProcessor.prepare (sampleRate, channelCount);
    weldProcessor.prepare (sampleRate);
    finalLimiter.prepare (sampleRate, maximumBlockSize, channelCount);
    dryWetMixer = std::make_unique<juce::dsp::DryWetMixer<float>> (oversampling.getFixedLatencySamples());
    dryWetMixer->setMixingRule (juce::dsp::DryWetMixingRule::sin3dB);
    dryWetMixer->setWetLatency (static_cast<float> (oversampling.getFixedLatencySamples()));
    dryWetMixer->prepare ({ sampleRate, static_cast<juce::uint32> (maximumBlockSize),
                            static_cast<juce::uint32> (channelCount) });
    for (auto* value : { &breachSmooth, &tearSmooth, &rotSmooth, &driveSmooth, &overloadSmooth, &mixSmooth, &rangeSmooth,
                         &lowSmooth, &midSmooth, &highSmooth, &outputSmooth, &weldSmooth,
                         &limiterThresholdSmooth, &limiterCeilingSmooth })
        value->reset (sampleRate, 0.035);
    gateContainmentSmooth.reset (sampleRate, 0.008);
    surgeAttackCoefficient = std::exp (-1.0f / (0.200f * static_cast<float> (sampleRate)));
    surgeReleaseCoefficient = std::exp (-1.0f / (0.450f * static_cast<float> (sampleRate)));
    oversamplingTransitionSamples = juce::jmax (1, static_cast<int> (0.005 * sampleRate));
    presetFadeOutSamples = juce::jmax (1, static_cast<int> (0.008 * sampleRate));
    presetFadeInSamples = juce::jmax (1, static_cast<int> (0.015 * sampleRate));
    reset();
}

void VoidEngine::reset() noexcept
{
    outputTone.reset();
    inputNoiseGate.reset();
    sourceAnalyzer.reset();
    oversampling.reset();
    reactorRack.reset();
    tearProcessor.reset();
    weldProcessor.reset();
    finalLimiter.reset();
    if (dryWetMixer != nullptr)
        dryWetMixer->reset();
    tearActivity.store (0.0f);
    weldGainReductionDb.store (0.0f, std::memory_order_relaxed);
    limiterGainReductionDb.store (0.0f, std::memory_order_relaxed);
    for (auto& activity : reactorActivity) activity.store (0.0f);
    dspFaultCount.store (0, std::memory_order_relaxed);
    preEqFaultCount.store (0, std::memory_order_relaxed);
    feedbackStateFaultCount.store (0, std::memory_order_relaxed);
    filterStateFaultCount.store (0, std::memory_order_relaxed);
    nonFiniteRepairCount.store (0, std::memory_order_relaxed);
    weldStateFaultCount.store (0, std::memory_order_relaxed);
    limiterStateFaultCount.store (0, std::memory_order_relaxed);
    surgeState = surgeTarget = 0.0f;
    oversamplingTransitionStart = {};
    lastWetSamples = {};
    oversamplingTransitionRemaining = 0;
    pendingPreset = {};
    presetTransitionState = PresetTransitionState::normal;
    presetTransitionGain = 1.0f;
    presetTransitionSamplesRemaining = 0;
    presetCommitCount = 0;
    targetsInitialised = false;
    gateContainmentSmooth.setCurrentAndTargetValue (1.0f);
    gateWasFullyClosed = false;
    gateStateQuenched = false;
}

void VoidEngine::resetForPresetChange() noexcept
{
    reactorRack.resetForPresetChange();
    tearProcessor.resetForPresetChange();
    weldProcessor.reset();
    oversamplingTransitionStart = {};
    lastWetSamples = {};
    oversamplingTransitionRemaining = 0;
}

void VoidEngine::quenchClosedGateState() noexcept
{
    sourceAnalyzer.reset();
    reactorRack.resetForPresetChange();
    tearProcessor.quench();
    weldProcessor.reset();
    outputTone.reset();
    oversamplingTransitionStart = {};
    lastWetSamples = {};
    oversamplingTransitionRemaining = 0;
    tearActivity.store (0.0f, std::memory_order_relaxed);
    weldGainReductionDb.store (0.0f, std::memory_order_relaxed);
    for (auto& activity : reactorActivity)
        activity.store (0.0f, std::memory_order_relaxed);
}

void VoidEngine::setTargetsImmediately (const Parameters& parameters) noexcept
{
    targetsInitialised = false;
    setTargets (parameters);
    surgeState = surgeTarget;
}

void VoidEngine::beginPresetTransition (const Parameters& parameters) noexcept
{
    pendingPreset = parameters;
    if (presetTransitionState == PresetTransitionState::fadeOut
        || presetTransitionState == PresetTransitionState::commitPending)
        return;

    presetTransitionState = PresetTransitionState::fadeOut;
    presetTransitionSamplesRemaining = juce::jmax (1,
        juce::roundToInt (presetFadeOutSamples
            * juce::jlimit (0.0f, 1.0f, presetTransitionGain)));
}

float VoidEngine::nextPresetTransitionGain() noexcept
{
    if (presetTransitionState == PresetTransitionState::fadeOut)
    {
        if (--presetTransitionSamplesRemaining <= 0)
        {
            presetTransitionGain = 0.0f;
            presetTransitionState = PresetTransitionState::commitPending;
        }
        else
            presetTransitionGain = static_cast<float> (presetTransitionSamplesRemaining)
                                 / static_cast<float> (presetFadeOutSamples);
    }
    else if (presetTransitionState == PresetTransitionState::fadeIn)
    {
        if (--presetTransitionSamplesRemaining <= 0)
        {
            presetTransitionGain = 1.0f;
            presetTransitionState = PresetTransitionState::normal;
        }
        else
            presetTransitionGain = 1.0f - static_cast<float> (presetTransitionSamplesRemaining)
                                        / static_cast<float> (presetFadeInSamples);
    }
    return juce::jlimit (0.0f, 1.0f, presetTransitionGain);
}

void VoidEngine::setTargets (const Parameters& p) noexcept
{
    const auto breach = finiteClamped (p.breach, 0.0f, 1.0f, 0.35f);
    const auto tear = finiteClamped (p.tear, 0.0f, 1.0f, 0.04f);
    const auto rot = finiteClamped (p.rot, 0.0f, 1.0f, 0.42f);
    const auto drive = juce::Decibels::decibelsToGain (finiteClamped (p.driveDb, 0.0f, 18.0f, 4.0f));
    const auto overload = finiteClamped (p.overload, 0.0f, 1.0f, 0.35f);
    const auto mix = finiteClamped (p.mix, 0.0f, 1.0f, 0.72f);
    const auto range = finiteClamped (p.range, 0.0f, 1.0f, 0.92f);
    const auto low = finiteClamped (p.lowDb, -12.0f, 12.0f, 0.0f);
    const auto mid = finiteClamped (p.midDb, -12.0f, 12.0f, 0.0f);
    const auto high = finiteClamped (p.highDb, -12.0f, 12.0f, 0.0f);
    const auto output = juce::Decibels::decibelsToGain (finiteClamped (p.outputDb, -24.0f, 6.0f, -4.0f));
    const auto weld = finiteClamped (p.weld, 0.0f, 1.0f, 0.30f);
    const auto limiterThreshold = finiteClamped (p.limiterThresholdDb, -18.0f, 0.0f, -3.0f);
    const auto limiterCeiling = finiteClamped (p.limiterCeilingDb, -6.0f, 0.0f, -0.8f);
    const auto gateThreshold = finiteClamped (p.gateThresholdDb, -80.0f, -20.0f, -50.0f);
    const auto surge = p.surge ? 1.0f : 0.0f;
    if (! targetsInitialised)
    {
        breachSmooth.setCurrentAndTargetValue (breach);
        tearSmooth.setCurrentAndTargetValue (tear);
        rotSmooth.setCurrentAndTargetValue (rot);
        driveSmooth.setCurrentAndTargetValue (drive);
        overloadSmooth.setCurrentAndTargetValue (overload);
        mixSmooth.setCurrentAndTargetValue (mix);
        rangeSmooth.setCurrentAndTargetValue (range);
        lowSmooth.setCurrentAndTargetValue (low);
        midSmooth.setCurrentAndTargetValue (mid);
        highSmooth.setCurrentAndTargetValue (high);
        outputSmooth.setCurrentAndTargetValue (output);
        weldSmooth.setCurrentAndTargetValue (weld);
        limiterThresholdSmooth.setCurrentAndTargetValue (limiterThreshold);
        limiterCeilingSmooth.setCurrentAndTargetValue (limiterCeiling);
        surgeState = 0.0f;
        surgeTarget = surge;
        targetsInitialised = true;
    }
    else
    {
        breachSmooth.setTargetValue (breach);
        tearSmooth.setTargetValue (tear);
        rotSmooth.setTargetValue (rot);
        driveSmooth.setTargetValue (drive);
        overloadSmooth.setTargetValue (overload);
        mixSmooth.setTargetValue (mix);
        rangeSmooth.setTargetValue (range);
        lowSmooth.setTargetValue (low);
        midSmooth.setTargetValue (mid);
        highSmooth.setTargetValue (high);
        outputSmooth.setTargetValue (output);
        weldSmooth.setTargetValue (weld);
        limiterThresholdSmooth.setTargetValue (limiterThreshold);
        limiterCeilingSmooth.setTargetValue (limiterCeiling);
        surgeTarget = surge;
    }
    oversampleFactor = p.oversampleFactor >= 8 ? 8 : p.oversampleFactor >= 4 ? 4 : p.oversampleFactor >= 2 ? 2 : 1;
    hqMode = p.hqMode;
    limiterEnabled = p.limiterEnabled;
    inputNoiseGate.setParameters (p.gateEnabled, gateThreshold);
    reactorEnabled = p.reactorEnabled;
    for (size_t index = 0; index < reactorAmounts.size(); ++index)
        reactorAmounts[index] = std::isfinite (p.reactorAmounts[index])
            ? juce::jlimit (0.0f, 1.0f, p.reactorAmounts[index]) : 1.0f;
    reactorCharacter = character::sanitise (p.reactorCharacter);
    reactorSolo = juce::jlimit (0, 4, p.reactorSolo);
    massEq = p.massEq;
    furnaceEq = p.furnaceEq;
    arcEq = p.arcEq;
    feedbackEq = p.feedbackEq;
}

void VoidEngine::recordFaults (const DspFaultCounters& counters) noexcept
{
    preEqFaultCount.fetch_add (counters.preEqFaultCount, std::memory_order_relaxed);
    feedbackStateFaultCount.fetch_add (counters.feedbackStateFaultCount, std::memory_order_relaxed);
    filterStateFaultCount.fetch_add (counters.filterStateFaultCount, std::memory_order_relaxed);
    nonFiniteRepairCount.fetch_add (counters.nonFiniteRepairCount, std::memory_order_relaxed);
    weldStateFaultCount.fetch_add (counters.weldStateFaultCount, std::memory_order_relaxed);
    limiterStateFaultCount.fetch_add (counters.limiterStateFaultCount, std::memory_order_relaxed);
    dspFaultCount.fetch_add (counters.total(), std::memory_order_relaxed);
}

void VoidEngine::recordNonFiniteRepairs (uint32_t count) noexcept
{
    DspFaultCounters counters;
    counters.nonFiniteRepairCount = count;
    recordFaults (counters);
}

DspFaultCounters VoidEngine::getDspFaultCounters() const noexcept
{
    return { preEqFaultCount.load (std::memory_order_relaxed),
             feedbackStateFaultCount.load (std::memory_order_relaxed),
             filterStateFaultCount.load (std::memory_order_relaxed),
             nonFiniteRepairCount.load (std::memory_order_relaxed),
             weldStateFaultCount.load (std::memory_order_relaxed),
             limiterStateFaultCount.load (std::memory_order_relaxed) };
}

uint32_t VoidEngine::repairNonFiniteBuffer (juce::AudioBuffer<float>& buffer) noexcept
{
    uint32_t repaired = 0;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            if (! std::isfinite (buffer.getSample (channel, sample)))
            {
                buffer.setSample (channel, sample, 0.0f);
                ++repaired;
            }
    if (repaired)
        recordNonFiniteRepairs (repaired);
    return repaired;
}

ReactorActivity VoidEngine::getReactorActivity() const noexcept
{
    return { reactorActivity[0].load(), reactorActivity[1].load(),
             reactorActivity[2].load(), reactorActivity[3].load() };
}

float VoidEngine::emergencyLimit (float sample) noexcept
{
    if (! std::isfinite (sample))
    {
        recordNonFiniteRepairs();
        return 0.0f;
    }
    const auto magnitude = std::abs (sample);
    if (magnitude <= 1.0f)
        return sample;
    return std::copysign (1.0f + 0.05f * std::tanh ((magnitude - 1.0f) * 5.0f), sample);
}

void VoidEngine::process (juce::AudioBuffer<float>& buffer) noexcept
{
    juce::ScopedNoDenormals noDenormals;
#if VOIDWORM_ENABLE_DIAGNOSTICS
    diagnostics = {};
    observeBuffer (diagnostics.input, buffer);
    reactorRack.beginDiagnosticsBlock();
    outputTone.beginDiagnosticsBlock();
#endif
    uint32_t inputRepairs = 0;
    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::inputRepair);
        inputRepairs = repairNonFiniteBuffer (buffer);
    }
#if VOIDWORM_ENABLE_DIAGNOSTICS
    diagnostics.input.repairedCount += inputRepairs;
#else
    juce::ignoreUnused (inputRepairs);
#endif

    // Bound analyzer look-ahead to a short causal chunk. Each chunk is analyzed
    // after DRIVE and then its reactors consume that current source description.
    // AudioBuffer views do not allocate or copy sample storage.
    for (int offset = 0; offset < buffer.getNumSamples(); offset += sourceAnalysisChunkSize)
    {
        const auto count = juce::jmin (sourceAnalysisChunkSize, buffer.getNumSamples() - offset);
        juce::AudioBuffer<float> chunk (buffer.getArrayOfWritePointers(), buffer.getNumChannels(),
                                        offset, count);
        processChunk (chunk);
    }
#if VOIDWORM_ENABLE_DIAGNOSTICS
    diagnostics.reactor = reactorRack.getDiagnostics();
    diagnostics.outputFilterStateMaximum = outputTone.getMaximumStateMagnitude();
    diagnostics.faults = getDspFaultCounters();
#endif
}

void VoidEngine::processChunk (juce::AudioBuffer<float>& buffer) noexcept
{
    const auto channels = juce::jmin (channelCount, buffer.getNumChannels());
    const auto samples = buffer.getNumSamples();
    if (channels <= 0 || samples <= 0 || dryWetMixer == nullptr)
        return;
    if (presetTransitionState == PresetTransitionState::commitPending)
    {
        resetForPresetChange();
        setTargetsImmediately (pendingPreset);
        outputTone.resetForPresetChange (pendingPreset.range, pendingPreset.lowDb,
                                         pendingPreset.midDb, pendingPreset.highDb);
        presetTransitionGain = 0.0f;
        presetTransitionSamplesRemaining = presetFadeInSamples;
        presetTransitionState = PresetTransitionState::fadeIn;
        ++presetCommitCount;
    }

    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::gate);
        inputNoiseGate.process (buffer);
    }
    const auto gateFullyClosed = inputNoiseGate.isFullyClosed();
    if (gateFullyClosed != gateWasFullyClosed)
    {
        gateContainmentSmooth.setTargetValue (gateFullyClosed ? 0.0f : 1.0f);
        if (! gateFullyClosed)
            gateStateQuenched = false;
        gateWasFullyClosed = gateFullyClosed;
    }

    const juce::dsp::AudioBlock<const float> dryBlock (buffer);
    dryWetMixer->pushDrySamples (dryBlock);

    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::driveAndAnalyzer);
        for (int sampleIndex = 0; sampleIndex < samples; ++sampleIndex)
        {
            const auto drive = driveSmooth.getNextValue();
            const auto inputLeft = buffer.getSample (0, sampleIndex);
            const auto inputRight = channels > 1 ? buffer.getSample (1, sampleIndex) : inputLeft;
            const auto drivenLeft = std::isfinite (inputLeft) ? inputLeft * drive : 0.0f;
            const auto drivenRight = std::isfinite (inputRight) ? inputRight * drive : 0.0f;
#if VOIDWORM_ENABLE_DIAGNOSTICS
            diagnostics.afterDrive.observe (drivenLeft, 0);
            if (channels > 1)
                diagnostics.afterDrive.observe (drivenRight, 1);
#endif
            if (! std::isfinite (inputLeft) || ! std::isfinite (inputRight))
                recordNonFiniteRepairs();
            sourceAnalyzer.processSampleState (drivenLeft, drivenRight);
            buffer.setSample (0, sampleIndex, drivenLeft);
            if (channels > 1)
                buffer.setSample (1, sampleIndex, drivenRight);
        }
        sourceAnalyzer.finaliseFeatures();
    }

    const auto currentBreach = breachSmooth.skip (samples);
    const auto currentTear = tearSmooth.skip (samples);
    const auto currentRot = rotSmooth.skip (samples);
    const auto currentOverload = overloadSmooth.skip (samples);
    const auto currentWeld = weldSmooth.skip (samples);
    const auto surgeCoefficient = surgeTarget > surgeState ? surgeAttackCoefficient : surgeReleaseCoefficient;
    surgeState = surgeTarget + std::pow (surgeCoefficient, static_cast<float> (samples))
                             * (surgeState - surgeTarget);
    const auto currentSurge = surgeState;

    const auto oversamplingChanged = oversampling.select (oversampleFactor, hqMode);
    const auto& constWetBuffer = buffer;
    const juce::dsp::AudioBlock<const float> wetInputBlock (constWetBuffer);
    juce::dsp::AudioBlock<float> oversampledBlock;
    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::oversamplingUp);
        oversampledBlock = oversampling.processSamplesUp (wetInputBlock);
    }
    const auto& features = sourceAnalyzer.getFeatures();
    const auto activity = reactorRack.process (oversampledBlock, oversampling.getSelectedFactor(), features,
                                               currentRot, currentOverload, currentBreach, currentSurge, currentWeld,
                                               reactorEnabled, reactorAmounts, reactorCharacter, reactorSolo,
                                               massEq, furnaceEq, arcEq, feedbackEq);
    recordFaults (reactorRack.getAndClearFaultCounters());
    reactorActivity[0].store (activity.mass);
    reactorActivity[1].store (activity.furnace);
    reactorActivity[2].store (activity.arc);
    reactorActivity[3].store (activity.feedback);
    juce::dsp::AudioBlock<float> wetOutputBlock (buffer);
    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::oversamplingDown);
        oversampling.processSamplesDown (wetOutputBlock);
    }
    const auto rackRepairs = repairNonFiniteBuffer (buffer);
#if VOIDWORM_ENABLE_DIAGNOSTICS
    diagnostics.reactor.bus.repairedCount += rackRepairs;
#endif
    if (rackRepairs != 0)
    {
        oversampling.reset();
        reactorRack.reset();
    }
    if (oversamplingChanged)
    {
        oversamplingTransitionStart = lastWetSamples;
        oversamplingTransitionRemaining = oversamplingTransitionSamples;
    }
    for (int sampleIndex = 0; sampleIndex < samples; ++sampleIndex)
    {
        const auto transition = oversamplingTransitionRemaining > 0
            ? 1.0f - static_cast<float> (oversamplingTransitionRemaining)
                     / static_cast<float> (oversamplingTransitionSamples)
            : 1.0f;
        for (int channel = 0; channel < channels; ++channel)
        {
            const auto next = buffer.getSample (channel, sampleIndex);
            const auto emitted = juce::jmap (transition,
                oversamplingTransitionStart[static_cast<size_t> (channel)], next);
            buffer.setSample (channel, sampleIndex, emitted);
            lastWetSamples[static_cast<size_t> (channel)] = emitted;
        }
        if (oversamplingTransitionRemaining > 0)
            --oversamplingTransitionRemaining;
    }

    float tearEventActivity = 0.0f;
    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::tear);
        tearEventActivity = tearProcessor.process (buffer, currentTear, currentOverload,
                                                   currentSurge, features);
    }
#if VOIDWORM_ENABLE_DIAGNOSTICS
    observeBuffer (diagnostics.afterTear, buffer);
#endif
    const auto tearRepairs = repairNonFiniteBuffer (buffer);
#if VOIDWORM_ENABLE_DIAGNOSTICS
    diagnostics.afterTear.repairedCount += tearRepairs;
#endif
    if (tearRepairs != 0)
        tearProcessor.reset();
    float weldReduction = 0.0f;
    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::weldDynamics);
        weldReduction = weldProcessor.process (buffer, currentWeld);
    }
    weldGainReductionDb.store (std::isfinite (weldReduction) ? weldReduction : 0.0f,
                               std::memory_order_relaxed);
#if VOIDWORM_ENABLE_DIAGNOSTICS
    observeBuffer (diagnostics.afterWeld, buffer);
#endif
    if (const auto weldFaults = weldProcessor.getAndClearStateFaultCount(); weldFaults != 0)
    {
        DspFaultCounters counters;
        counters.weldStateFaultCount = weldFaults;
        recordFaults (counters);
    }
    const auto weldRepairs = repairNonFiniteBuffer (buffer);
    if (weldRepairs != 0)
        weldProcessor.reset();
    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::fixedLatencyAndMix);
        oversampling.applyFixedLatency (buffer);
        dryWetMixer->setWetMixProportion (mixSmooth.skip (samples));
        dryWetMixer->mixWetSamples (wetOutputBlock);
    }
#if VOIDWORM_ENABLE_DIAGNOSTICS
    observeBuffer (diagnostics.afterMix, buffer);
#endif
    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::masterEqAndOutput);
        outputTone.update (rangeSmooth.skip (samples), lowSmooth.skip (samples),
                           midSmooth.skip (samples), highSmooth.skip (samples));
        for (int sampleIndex = 0; sampleIndex < samples; ++sampleIndex)
        {
            const auto gain = outputSmooth.getNextValue();
            const auto transitionGain = nextPresetTransitionGain();
            for (int channel = 0; channel < channels; ++channel)
            {
                const auto equalised = outputTone.processSample (channel, buffer.getSample (channel, sampleIndex));
#if VOIDWORM_ENABLE_DIAGNOSTICS
                diagnostics.masterEq.observe (equalised, channel);
#endif
                const auto scaled = equalised * gain * transitionGain;
#if VOIDWORM_ENABLE_DIAGNOSTICS
                diagnostics.afterOutputGain.observe (scaled, channel);
#endif
                buffer.setSample (channel, sampleIndex, scaled);
            }
        }
    }
    float limiterReduction = 0.0f;
    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::limiter);
        limiterReduction = finalLimiter.process (buffer, limiterEnabled,
                                                 limiterThresholdSmooth, limiterCeilingSmooth);
    }
    limiterGainReductionDb.store (std::isfinite (limiterReduction) ? limiterReduction : 0.0f,
                                  std::memory_order_relaxed);
#if VOIDWORM_ENABLE_DIAGNOSTICS
    observeBuffer (diagnostics.afterLimiter, buffer);
#endif
    if (const auto limiterFaults = finalLimiter.getAndClearStateFaultCount(); limiterFaults != 0)
    {
        DspFaultCounters counters;
        counters.limiterStateFaultCount = limiterFaults;
        recordFaults (counters);
    }
    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::containment);
        for (int sampleIndex = 0; sampleIndex < samples; ++sampleIndex)
        {
            const auto containmentGain = gateContainmentSmooth.getNextValue();
            for (int channel = 0; channel < channels; ++channel)
            {
                const auto limited = buffer.getSample (channel, sampleIndex);
                const auto safeLimited = emergencyLimit (limited);
                const auto output = safeLimited * containmentGain;
#if VOIDWORM_ENABLE_DIAGNOSTICS
                diagnostics.finalOutput.observe (output, channel);
                if (std::isfinite (limited) && safeLimited != limited)
                    ++diagnostics.finalOutput.clippedCount;
#endif
                buffer.setSample (channel, sampleIndex, output);
            }
            if (gateFullyClosed && ! gateStateQuenched
                && ! gateContainmentSmooth.isSmoothing() && containmentGain <= 0.0f)
            {
                quenchClosedGateState();
                gateStateQuenched = true;
            }
        }
    }
    const auto toneFaults = outputTone.getAndClearDspFaultCount();
    if (toneFaults != 0)
    {
        DspFaultCounters counters;
        counters.filterStateFaultCount = toneFaults;
        recordFaults (counters);
    }

    tearActivity.store (0.88f * tearActivity.load() + 0.12f * tearEventActivity);
}
}
