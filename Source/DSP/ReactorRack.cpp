#include "ReactorRack.h"
#include "PerformanceProfile.h"

namespace voidworm
{
namespace
{
uint32_t repairNonFinitePath (juce::dsp::AudioBlock<float>& path,
                              const juce::dsp::AudioBlock<float>& source) noexcept
{
    uint32_t repaired = 0;
    for (size_t channel = 0; channel < path.getNumChannels(); ++channel)
    {
        auto* destination = path.getChannelPointer (channel);
        const auto* fallback = source.getChannelPointer (channel);
        for (size_t sample = 0; sample < path.getNumSamples(); ++sample)
        {
            if (! std::isfinite (destination[sample]))
            {
                destination[sample] = std::isfinite (fallback[sample]) ? fallback[sample] : 0.0f;
                ++repaired;
            }
        }
    }
    return repaired;
}

}

int ReactorRack::factorToIndex (int factor) noexcept
{
    if (factor >= 8) return 3;
    if (factor >= 4) return 2;
    if (factor >= 2) return 1;
    return 0;
}

float ReactorRack::smoothstep (float low, float high, float value) noexcept
{
    const auto position = juce::jlimit (0.0f, 1.0f, (value - low) / juce::jmax (0.0001f, high - low));
    return position * position * (3.0f - 2.0f * position);
}

void ReactorRack::prepare (double hostSampleRate, int maximumBlockSize, int channels)
{
    channelCount = juce::jlimit (1, 2, channels);
    for (int index = 0; index < 4; ++index)
    {
        const auto factor = 1 << index;
        const auto rate = hostSampleRate * static_cast<double> (factor);
        massChains[static_cast<size_t> (index)].prepare (rate);
        furnaceChains[static_cast<size_t> (index)].prepare (rate);
        arcChains[static_cast<size_t> (index)].prepare (rate);
        feedbackChains[static_cast<size_t> (index)].prepare (rate);
        processingRates[static_cast<size_t> (index)] = rate;
        routingSmoothingCoefficients[static_cast<size_t> (index)] =
            1.0f - std::exp (-1.0f / (0.020f * static_cast<float> (rate)));
        busDynamics[static_cast<size_t> (index)].prepare (rate);
        massBuffers[static_cast<size_t> (index)].setSize (channelCount, maximumBlockSize * factor,
                                                          false, true, true);
        furnaceBuffers[static_cast<size_t> (index)].setSize (channelCount, maximumBlockSize * factor,
                                                             false, true, true);
        arcBuffers[static_cast<size_t> (index)].setSize (channelCount, maximumBlockSize * factor,
                                                         false, true, true);
        feedbackBuffers[static_cast<size_t> (index)].setSize (channelCount, maximumBlockSize * factor,
                                                              false, true, true);
    }
    reset();
}

void ReactorRack::reset() noexcept
{
    resetForPresetChange();
    for (auto& buffer : massBuffers) buffer.clear();
    for (auto& buffer : furnaceBuffers) buffer.clear();
    for (auto& buffer : arcBuffers) buffer.clear();
    for (auto& buffer : feedbackBuffers) buffer.clear();
}

void ReactorRack::resetForPresetChange() noexcept
{
    for (auto& chain : massChains) chain.reset();
    for (auto& chain : furnaceChains) chain.reset();
    for (auto& chain : arcChains) chain.reset();
    for (auto& chain : feedbackChains) chain.reset();
    for (auto& dynamics : busDynamics) dynamics.reset();
    for (auto& stage : busNonlinearStages) stage.reset();
    for (auto& weights : weightStates) weights = {};
    for (auto& routing : routingStates) routing = {};
    pathProcessingActive = {};
    busProcessingActive = {};
    faultCounters = {};
#if VOIDWORM_ENABLE_DIAGNOSTICS
    diagnostics = {};
#endif
}

DspFaultCounters ReactorRack::getAndClearFaultCounters() noexcept
{
    const auto result = faultCounters;
    faultCounters = {};
    return result;
}

ReactorActivity ReactorRack::process (juce::dsp::AudioBlock<float>& block, int oversamplingFactor,
                                      const SourceFeatures& features, float rot, float overload,
                                      float breach, float surge, float weld, const std::array<bool, 4>& enabled,
                                      const std::array<float, 4>& amounts,
                                      const ReactorCharacterSettings& characterSettings, int soloTarget,
                                      const ReactorEqSettings& massEq,
                                      const ReactorEqSettings& furnaceEq, const ReactorEqSettings& arcEq,
                                      const ReactorEqSettings& feedbackEq) noexcept
{
    const auto index = factorToIndex (oversamplingFactor);
    const auto stateIndex = static_cast<size_t> (index);
    auto& routing = routingStates[stateIndex];
    const auto safeSoloTarget = juce::jlimit (0, 4, soloTarget);
    std::array<float, 4> routingTargets {};
    std::array<bool, 4> routedPaths {};
    constexpr auto silentRoutingGain = 1.0e-6f;
    for (size_t path = 0; path < routingTargets.size(); ++path)
    {
        const auto amount = std::isfinite (amounts[path])
            ? juce::jlimit (0.0f, 1.0f, amounts[path]) : 1.0f;
        const auto audible = safeSoloTarget != 0
            ? safeSoloTarget == static_cast<int> (path) + 1 : enabled[path];
        routingTargets[path] = audible ? amount : 0.0f;
        if (routingTargets[path] == 0.0f && std::abs (routing.gains[path]) < silentRoutingGain)
            routing.gains[path] = 0.0f;
        routedPaths[path] = routingTargets[path] > 0.0f || routing.gains[path] != 0.0f;
    }

    const auto breachActive = std::abs (breach) > 0.000001f;
    const auto processArc = routedPaths[2];
    const auto processFurnace = routedPaths[1] || (processArc && breachActive);
    const auto processMass = routedPaths[0] || ((processFurnace || processArc) && breachActive);
    const auto processFeedback = routedPaths[3];
    const std::array<bool, 4> processPaths {
        processMass, processFurnace, processArc, processFeedback
    };
    auto& activePaths = pathProcessingActive[stateIndex];
    if (! processMass && activePaths[0]) massChains[stateIndex].quench();
    if (! processFurnace && activePaths[1]) furnaceChains[stateIndex].quench();
    if (! processArc && activePaths[2]) arcChains[stateIndex].quench();
    if (! processFeedback && activePaths[3]) feedbackChains[stateIndex].quench();
    activePaths = processPaths;

    auto& massBuffer = massBuffers[static_cast<size_t> (index)];
    auto& furnaceBuffer = furnaceBuffers[static_cast<size_t> (index)];
    auto& arcBuffer = arcBuffers[static_cast<size_t> (index)];
    auto& feedbackBuffer = feedbackBuffers[static_cast<size_t> (index)];
    jassert (static_cast<int> (block.getNumSamples()) <= massBuffer.getNumSamples());
    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::pathCopies);
        for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
        {
            const auto* source = block.getChannelPointer (channel);
            const auto count = static_cast<int> (block.getNumSamples());
            if (processMass) massBuffer.copyFrom (static_cast<int> (channel), 0, source, count);
            if (processFurnace) furnaceBuffer.copyFrom (static_cast<int> (channel), 0, source, count);
            if (processArc) arcBuffer.copyFrom (static_cast<int> (channel), 0, source, count);
            if (processFeedback) feedbackBuffer.copyFrom (static_cast<int> (channel), 0, source, count);
        }
    }

    juce::dsp::AudioBlock<float> massBlock (massBuffer);
    juce::dsp::AudioBlock<float> furnaceBlock (furnaceBuffer);
    juce::dsp::AudioBlock<float> arcBlock (arcBuffer);
    juce::dsp::AudioBlock<float> feedbackBlock (feedbackBuffer);
    massBlock = massBlock.getSubBlock (0, block.getNumSamples());
    furnaceBlock = furnaceBlock.getSubBlock (0, block.getNumSamples());
    arcBlock = arcBlock.getSubBlock (0, block.getNumSamples());
    feedbackBlock = feedbackBlock.getSubBlock (0, block.getNumSamples());
    const auto character = character::sanitise (characterSettings);
    if (processMass)
    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::mass);
        massChains[static_cast<size_t> (index)].process (massBlock, massEq, features, rot, overload, surge,
                                                        character.massSaturation, character.massHarmonics);
    }
    if (processFurnace)
    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::furnace);
        furnaceChains[static_cast<size_t> (index)].process (furnaceBlock, furnaceEq, features,
                                                            rot, overload, breach, surge,
                                                            character.furnaceStarve, character.furnaceFold);
    }
    if (processArc)
    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::arc);
        arcChains[static_cast<size_t> (index)].process (arcBlock, arcEq, features,
                                                        rot, overload, breach, surge,
                                                        character.arcXmod, character.arcFold);
    }
    auto& massChain = massChains[static_cast<size_t> (index)];
    auto& furnaceChain = furnaceChains[static_cast<size_t> (index)];
    auto& arcChain = arcChains[static_cast<size_t> (index)];
    if (processMass) faultCounters.preEqFaultCount += massChain.getAndClearDspFaultCount();
    if (processFurnace) faultCounters += furnaceChain.getAndClearFaultCounters();
    if (processArc) faultCounters.preEqFaultCount += arcChain.getAndClearDspFaultCount();
    uint32_t massRepairs = 0;
    uint32_t furnaceRepairs = 0;
    uint32_t arcRepairs = 0;
    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::pathValidation);
        if (processMass) massRepairs = repairNonFinitePath (massBlock, block);
        if (processFurnace) furnaceRepairs = repairNonFinitePath (furnaceBlock, block);
        if (processArc) arcRepairs = repairNonFinitePath (arcBlock, block);
    }
    if (massRepairs != 0) { massChain.reset(); faultCounters.nonFiniteRepairCount += massRepairs; }
    if (furnaceRepairs != 0) { furnaceChain.reset(); faultCounters.nonFiniteRepairCount += furnaceRepairs; }
    if (arcRepairs != 0) { arcChain.reset(); faultCounters.nonFiniteRepairCount += arcRepairs; }
#if VOIDWORM_ENABLE_DIAGNOSTICS
    diagnostics.mass.repairedCount += massRepairs;
    diagnostics.furnace.repairedCount += furnaceRepairs;
    diagnostics.arc.repairedCount += arcRepairs;
    observeBlock (diagnostics.mass, massBlock, 2.5f);
    observeBlock (diagnostics.furnace, furnaceBlock, 2.5f);
    observeBlock (diagnostics.arc, arcBlock, 2.5f);
    diagnostics.furnaceSagMaximum = juce::jmax (diagnostics.furnaceSagMaximum,
                                                furnaceChain.getMaximumSagEnvelope());
    diagnostics.furnaceSupplyMinimum = juce::jmin (diagnostics.furnaceSupplyMinimum,
                                                   furnaceChain.getMinimumSupply());
    diagnostics.filterStateMaximum = juce::jmax (diagnostics.filterStateMaximum,
                                                 furnaceChain.getMaximumFilterStateMagnitude());
#endif

    // BREACH contaminates normally separate reactor paths with products derived
    // from their current source components. It does not apply a broadband gain.
    if ((processFurnace || processArc) && breachActive)
    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::breach);
        for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
        {
            const auto* source = block.getChannelPointer (channel);
            const auto* mass = massBlock.getChannelPointer (channel);
            auto* furnace = furnaceBlock.getChannelPointer (channel);
            auto* arc = arcBlock.getChannelPointer (channel);
            for (size_t sample = 0; sample < block.getNumSamples(); ++sample)
            {
                const auto furnaceBeforeBreach = furnace[sample];
                if (processFurnace)
                    furnace[sample] = juce::jlimit (-2.5f, 2.5f, furnaceBeforeBreach
                        + breach * 0.34f * mass[sample] * std::abs (source[sample]));
                if (processArc)
                    arc[sample] = juce::jlimit (-2.5f, 2.5f, arc[sample] + breach
                        * (1.8f * furnaceBeforeBreach * mass[sample]
                           + 0.52f * source[sample] * std::abs (furnaceBeforeBreach)));
            }
        }
    }

    const auto massTarget = juce::jlimit (0.45f, 0.98f,
        0.52f + 0.26f * features.lowLevel + 0.18f * features.lowRatio + 0.12f * overload);
    const auto furnaceMetric = 0.50f * features.rms + 0.50f * features.midLevel;
    const auto threshold = juce::jmap (overload, 0.14f, 0.040f);
    const auto furnaceActivation = smoothstep (threshold, threshold + 0.22f, furnaceMetric);
    const auto furnaceTarget = juce::jlimit (0.0f, 0.85f,
        0.24f + (0.42f + 0.13f * rot + 0.06f * surge) * furnaceActivation);
    const auto arcMetric = 0.24f * features.transient + 0.20f * features.midLevel
                         + 0.16f * features.highLevel + 0.20f * features.brightness
                         + 0.12f * features.midRatio + 0.08f * features.highRatio;
    const auto arcThreshold = juce::jmap (overload, 0.18f, 0.05f);
    const auto arcActivation = smoothstep (arcThreshold, arcThreshold + 0.22f, arcMetric);
    const auto arcTarget = juce::jlimit (0.0f, 0.65f, (0.12f + 0.46f * arcActivation)
                         * (0.72f + 0.18f * breach + 0.10f * surge));
    const auto feedbackMetric = 0.62f * features.sustain + 0.38f * features.slowEnvelope;
    const auto feedbackThreshold = juce::jmap (overload, 0.20f, 0.075f);
    const auto feedbackActivation = smoothstep (feedbackThreshold, feedbackThreshold + 0.24f, feedbackMetric);
    const auto feedbackTarget = juce::jlimit (0.0f, 0.45f,
        feedbackActivation * (0.24f + 0.12f * overload + 0.09f * surge));
    if (processFeedback)
    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::feedback);
        feedbackChains[static_cast<size_t> (index)].process (feedbackBlock, feedbackEq, features, rot, overload,
                                                             surge, feedbackActivation,
                                                             character.feedbackReturn, character.feedbackDamp);
    }
    auto& feedbackChain = feedbackChains[static_cast<size_t> (index)];
    if (processFeedback) faultCounters += feedbackChain.getAndClearFaultCounters();
    uint32_t feedbackRepairs = 0;
    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::pathValidation);
        if (processFeedback) feedbackRepairs = repairNonFinitePath (feedbackBlock, block);
    }
    if (feedbackRepairs != 0) { feedbackChain.reset(); faultCounters.nonFiniteRepairCount += feedbackRepairs; }
#if VOIDWORM_ENABLE_DIAGNOSTICS
    diagnostics.feedback.repairedCount += feedbackRepairs;
    observeBlock (diagnostics.feedback, feedbackBlock);
    diagnostics.feedbackStateMaximum = juce::jmax (diagnostics.feedbackStateMaximum,
                                                    feedbackChain.getMaximumStateMagnitude());
    diagnostics.featureMaxima.rms = juce::jmax (diagnostics.featureMaxima.rms, features.rms);
    diagnostics.featureMaxima.peak = juce::jmax (diagnostics.featureMaxima.peak, features.peak);
    diagnostics.featureMaxima.fastEnvelope = juce::jmax (diagnostics.featureMaxima.fastEnvelope, features.fastEnvelope);
    diagnostics.featureMaxima.transient = juce::jmax (diagnostics.featureMaxima.transient, features.transient);
    diagnostics.featureMaxima.sustain = juce::jmax (diagnostics.featureMaxima.sustain, features.sustain);
    diagnostics.featureMaxima.slowEnvelope = juce::jmax (diagnostics.featureMaxima.slowEnvelope, features.slowEnvelope);
    diagnostics.featureMaxima.lowLevel = juce::jmax (diagnostics.featureMaxima.lowLevel, features.lowLevel);
    diagnostics.featureMaxima.midLevel = juce::jmax (diagnostics.featureMaxima.midLevel, features.midLevel);
    diagnostics.featureMaxima.highLevel = juce::jmax (diagnostics.featureMaxima.highLevel, features.highLevel);
    diagnostics.featureMaxima.brightness = juce::jmax (diagnostics.featureMaxima.brightness, features.brightness);
    diagnostics.featureMaxima.lowRatio = juce::jmax (diagnostics.featureMaxima.lowRatio, features.lowRatio);
    diagnostics.featureMaxima.midRatio = juce::jmax (diagnostics.featureMaxima.midRatio, features.midRatio);
    diagnostics.featureMaxima.highRatio = juce::jmax (diagnostics.featureMaxima.highRatio, features.highRatio);
#endif

    auto& weights = weightStates[static_cast<size_t> (index)];
    const auto smoothing = 1.0f - std::exp (-static_cast<float> (block.getNumSamples())
        / (0.035f * static_cast<float> (processingRates[static_cast<size_t> (index)])));
    weights.mass += smoothing * (massTarget - weights.mass);
    weights.furnace += smoothing * (furnaceTarget - weights.furnace);
    weights.arc += smoothing * (arcTarget - weights.arc);
    weights.feedback += smoothing * (feedbackTarget - weights.feedback);
    const auto massWeight = weights.mass;
    const auto furnaceWeight = weights.furnace;
    const auto arcWeight = weights.arc;
    const auto feedbackWeight = weights.feedback;
    const auto routingSmoothing = routingSmoothingCoefficients[static_cast<size_t> (index)];
    constexpr auto massBusGain = 0.88f;
    constexpr auto furnaceBusGain = 0.66f;
    constexpr auto arcBusGain = 0.64f;
    constexpr auto feedbackBusGain = 0.52f;

    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::recombination);
        for (size_t sample = 0; sample < block.getNumSamples(); ++sample)
        {
            for (size_t path = 0; path < routing.gains.size(); ++path)
                routing.gains[path] += routingSmoothing
                    * (routingTargets[path] - routing.gains[path]);
            for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
            {
                auto* destination = block.getChannelPointer (channel);
                const auto* mass = massBlock.getChannelPointer (channel);
                const auto* furnace = furnaceBlock.getChannelPointer (channel);
                const auto* arc = arcBlock.getChannelPointer (channel);
                const auto* recursive = feedbackBlock.getChannelPointer (channel);
                destination[sample] = juce::jlimit (-3.0f, 3.0f,
                    massBusGain * massWeight * routing.gains[0] * mass[sample]
                     + furnaceBusGain * furnaceWeight * routing.gains[1] * furnace[sample]
                     + arcBusGain * arcWeight * routing.gains[2] * arc[sample]
                     + feedbackBusGain * feedbackWeight * routing.gains[3] * recursive[sample]);
            }
        }
    }
#if VOIDWORM_ENABLE_DIAGNOSTICS
    observeBlock (diagnostics.sum, block, 3.0f);
#endif

    const auto anyRoutedPath = std::any_of (routedPaths.begin(), routedPaths.end(), [] (bool active)
    {
        return active;
    });
    if (! anyRoutedPath)
    {
        block.clear();
        if (busProcessingActive[stateIndex])
        {
            busDynamics[stateIndex].reset();
            busNonlinearStages[stateIndex].reset();
            busProcessingActive[stateIndex] = false;
        }
        return {};
    }
    busProcessingActive[stateIndex] = true;

    auto& busCompressor = busDynamics[static_cast<size_t> (index)];
    busCompressor.setParameters (-7.0f - 9.0f * overload, 3.0f + 5.0f * overload,
                                 9.0f - 5.0f * overload, 115.0f - 55.0f * overload);
    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::busDynamics);
        for (size_t sample = 0; sample < block.getNumSamples(); ++sample)
        {
            auto detector = 0.0f;
            for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
                detector = juce::jmax (detector, std::abs (block.getSample (static_cast<int> (channel),
                                                                          static_cast<int> (sample))));
            const auto linkedGain = busCompressor.processLinkedGain (detector);
            for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
                block.setSample (static_cast<int> (channel), static_cast<int> (sample),
                    block.getSample (static_cast<int> (channel), static_cast<int> (sample)) * linkedGain);
        }
    }
    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::rot);
        busNonlinearStages[static_cast<size_t> (index)].process (block, rot, overload, surge);
    }
#if VOIDWORM_ENABLE_DIAGNOSTICS
    observeBlock (diagnostics.bus, block, 3.0f);
#endif
    // WELD's nonlinear component lives at the end of the oversampled reactor bus.
    // The odd-symmetric curve keeps zero at zero, progressively blends from the
    // existing bus to a hard industrial plateau, and is gain-compensated.
    const auto safeWeld = std::isfinite (weld) ? juce::jlimit (0.0f, 1.0f, weld) : 0.30f;
    const auto saturationMix = safeWeld * safeWeld * (3.0f - 2.0f * safeWeld);
    const auto saturationDrive = 1.0f + 11.0f * safeWeld * safeWeld;
    const auto hardness = safeWeld * safeWeld;
    const auto compensation = 1.0f / (1.0f + 0.42f * safeWeld);
    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::weldSaturation);
        for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
        {
            auto* destination = block.getChannelPointer (channel);
            for (size_t sample = 0; sample < block.getNumSamples(); ++sample)
            {
                const auto input = destination[sample];
                const auto driven = input * saturationDrive;
                const auto curved = std::tanh (driven);
                const auto clipped = juce::jlimit (-1.0f, 1.0f, driven);
                const auto saturated = (curved + hardness * (clipped - curved)) * compensation;
                destination[sample] = input + saturationMix * (saturated - input);
            }
        }
    }
#if VOIDWORM_ENABLE_DIAGNOSTICS
    observeBlock (diagnostics.weldSaturation, block, 3.0f);
#endif
    uint32_t busRepairs = 0;
    {
        VOIDWORM_PROFILE_SCOPE (performance::Stage::pathValidation);
        for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
        {
            auto* destination = block.getChannelPointer (channel);
            for (size_t sample = 0; sample < block.getNumSamples(); ++sample)
            {
                if (! std::isfinite (destination[sample]))
                {
                    destination[sample] = 0.0f;
                    ++busRepairs;
                }
            }
        }
    }
    if (busRepairs != 0)
    {
        busCompressor.reset();
        busNonlinearStages[static_cast<size_t> (index)].reset();
        faultCounters.nonFiniteRepairCount += busRepairs;
#if VOIDWORM_ENABLE_DIAGNOSTICS
        diagnostics.bus.repairedCount += busRepairs;
#endif
    }

    return { massWeight * routing.gains[0], furnaceWeight * routing.gains[1],
             arcWeight * routing.gains[2], feedbackWeight * routing.gains[3] };
}
}
