#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include "../Source/DSP/PerformanceProfile.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

namespace
{
constexpr double auditSampleRate = 44100.0;
constexpr int blockSize = 128;
constexpr int warmupBlocks = 320;
constexpr int measuredBlocks = 1600;

struct Case
{
    std::string name;
    int factor = 4;
    bool hq = true;
    std::array<bool, 4> reactors { true, true, true, true };
    bool silence = false;
    bool editor = false;
};

struct Result
{
    double averageUs = 0.0;
    double p99Us = 0.0;
    double worstUs = 0.0;
    double realtimePercent = 0.0;
    double checksum = 0.0;
    uint64_t outputHash = 1469598103934665603ull;
    voidworm::performance::Snapshot stages;
};

void setParameter (VoidwormAudioProcessor& processor, const char* id, float value)
{
    if (auto* parameter = processor.parameters.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

void configure (VoidwormAudioProcessor& processor, const Case& testCase)
{
    const auto factorIndex = testCase.factor >= 8 ? 3.0f : testCase.factor >= 4 ? 2.0f
                           : testCase.factor >= 2 ? 1.0f : 0.0f;
    setParameter (processor, "oversample", factorIndex);
    setParameter (processor, "hqMode", testCase.hq ? 1.0f : 0.0f);
    setParameter (processor, "gateEnabled", 0.0f);
    constexpr std::array<const char*, 4> enabledIds {
        "massEnabled", "furnaceEnabled", "arcEnabled", "feedbackEnabled"
    };
    for (size_t index = 0; index < enabledIds.size(); ++index)
        setParameter (processor, enabledIds[index], testCase.reactors[index] ? 1.0f : 0.0f);
}

void fillInput (juce::AudioBuffer<float>& buffer, uint64_t blockIndex, bool silence)
{
    if (silence)
    {
        buffer.clear();
        return;
    }

    uint32_t noiseState = 0x9e3779b9u ^ static_cast<uint32_t> (blockIndex * 2654435761u);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto absoluteSample = static_cast<double> (blockIndex * blockSize + static_cast<uint64_t> (sample));
        noiseState ^= noiseState << 13;
        noiseState ^= noiseState >> 17;
        noiseState ^= noiseState << 5;
        const auto noise = (static_cast<float> (noiseState & 0xffffu) / 32767.5f - 1.0f) * 0.035f;
        const auto transient = (static_cast<uint64_t> (absoluteSample) % 4096u) < 12u
            ? 0.72f * std::exp (-static_cast<float> (static_cast<uint64_t> (absoluteSample) % 4096u) * 0.34f)
            : 0.0f;
        const auto bass = 0.24f * std::sin (juce::MathConstants<double>::twoPi * 73.0 * absoluteSample / auditSampleRate);
        const auto mid = 0.13f * std::sin (juce::MathConstants<double>::twoPi * 713.0 * absoluteSample / auditSampleRate);
        const auto high = 0.06f * std::sin (juce::MathConstants<double>::twoPi * 4271.0 * absoluteSample / auditSampleRate);
        const auto left = static_cast<float> (bass + mid + high) + noise + transient;
        const auto right = static_cast<float> (0.96 * bass - 0.72 * mid + 0.84 * high) - noise + 0.91f * transient;
        buffer.setSample (0, sample, left);
        buffer.setSample (1, sample, right);
    }
}

Result runCase (const Case& testCase)
{
    VoidwormAudioProcessor processor;
    configure (processor, testCase);
    processor.prepareToPlay (auditSampleRate, blockSize);
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    if (testCase.editor)
        editor.reset (processor.createEditor());

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;
    for (int block = 0; block < warmupBlocks; ++block)
    {
        fillInput (buffer, static_cast<uint64_t> (block), testCase.silence);
        processor.processBlock (buffer, midi);
        if (editor != nullptr)
            juce::Timer::callPendingTimersSynchronously();
    }

    voidworm::performance::reset();
    std::vector<double> elapsed;
    elapsed.reserve (measuredBlocks);
    double checksum = 0.0;
    uint64_t outputHash = 1469598103934665603ull;
    using Clock = std::chrono::steady_clock;
    for (int block = 0; block < measuredBlocks; ++block)
    {
        const auto blockIndex = static_cast<uint64_t> (warmupBlocks + block);
        fillInput (buffer, blockIndex, testCase.silence);
        const auto start = Clock::now();
        processor.processBlock (buffer, midi);
        const auto stop = Clock::now();
        elapsed.push_back (std::chrono::duration<double, std::micro> (stop - start).count());
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                const auto value = buffer.getSample (channel, sample);
                uint32_t bits = 0;
                static_assert (sizeof (bits) == sizeof (value));
                std::memcpy (&bits, &value, sizeof (bits));
                outputHash ^= bits;
                outputHash *= 1099511628211ull;
                if (sample % 17 == 0)
                    checksum += static_cast<double> (value) * static_cast<double> (1 + channel + sample);
            }
        if (editor != nullptr)
            juce::Timer::callPendingTimersSynchronously();
    }

    std::sort (elapsed.begin(), elapsed.end());
    const auto average = std::accumulate (elapsed.begin(), elapsed.end(), 0.0)
                       / static_cast<double> (elapsed.size());
    const auto p99Index = static_cast<size_t> (0.99 * static_cast<double> (elapsed.size() - 1));
    const auto blockDurationUs = 1.0e6 * static_cast<double> (blockSize) / auditSampleRate;
    return { average, elapsed[p99Index], elapsed.back(), 100.0 * average / blockDurationUs,
             checksum, outputHash, voidworm::performance::snapshot() };
}

void printResult (const Case& testCase, const Result& result, bool printStages)
{
    std::cout << std::left << std::setw (25) << testCase.name
              << " avg_us=" << std::right << std::fixed << std::setprecision (2) << result.averageUs
              << " p99_us=" << result.p99Us
              << " worst_us=" << result.worstUs
              << " realtime_cpu=" << result.realtimePercent << "%"
              << " checksum=" << std::setprecision (9) << result.checksum
              << " hash=0x" << std::hex << result.outputHash << std::dec << '\n';

    if (! printStages)
        return;

    struct RankedStage { size_t index; uint64_t nanoseconds; };
    std::vector<RankedStage> ranked;
    uint64_t accounted = 0;
    for (size_t index = 0; index < result.stages.nanoseconds.size(); ++index)
    {
        if (result.stages.calls[index] != 0)
        {
            ranked.push_back ({ index, result.stages.nanoseconds[index] });
            accounted += result.stages.nanoseconds[index];
        }
    }
    std::sort (ranked.begin(), ranked.end(), [] (const auto& left, const auto& right)
    {
        return left.nanoseconds > right.nanoseconds;
    });
    std::cout << "  stage profile (nested stages are reported independently):\n";
    for (const auto& item : ranked)
    {
        const auto calls = result.stages.calls[item.index];
        const auto averageCallUs = static_cast<double> (item.nanoseconds) / static_cast<double> (calls) / 1000.0;
        const auto measuredTotalNs = result.averageUs * 1000.0 * static_cast<double> (measuredBlocks);
        const auto percentOfProcess = measuredTotalNs > 0.0
            ? 100.0 * static_cast<double> (item.nanoseconds) / measuredTotalNs : 0.0;
        std::cout << "    " << std::left << std::setw (30) << voidworm::performance::stageNames[item.index]
                  << " total_ms=" << std::right << std::setw (10) << std::setprecision (3)
                  << static_cast<double> (item.nanoseconds) / 1.0e6
                  << " avg_call_us=" << std::setw (9) << averageCallUs
                  << " process_share=" << std::setw (7) << percentOfProcess << "%\n";
    }
    juce::ignoreUnused (accounted);
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    std::cout << "VOIDWORM CPU audit: 44.1 kHz, stereo, 128 samples, "
              << warmupBlocks << " warmup blocks, " << measuredBlocks << " measured blocks\n";
    std::cout << "stage_timers=" << (VOIDWORM_ENABLE_PERF_PROFILING ? "ON" : "OFF") << "\n\n";

    std::vector<Case> cases;
    for (const auto hq : { false, true })
        for (const auto factor : { 1, 2, 4, 8 })
            cases.push_back ({ std::to_string (factor) + "x HQ " + (hq ? "ON" : "OFF"), factor, hq });

    cases.push_back ({ "4x MASS only", 4, true, { true, false, false, false } });
    cases.push_back ({ "4x FURNACE only", 4, true, { false, true, false, false } });
    cases.push_back ({ "4x ARC only", 4, true, { false, false, true, false } });
    cases.push_back ({ "4x FEEDBACK only", 4, true, { false, false, false, true } });
    cases.push_back ({ "4x all reactors", 4, true, { true, true, true, true } });
    cases.push_back ({ "4x silence/tail", 4, true, { true, true, true, true }, true });
    cases.push_back ({ "4x editor closed", 4, true, { true, true, true, true }, false, false });
    cases.push_back ({ "4x editor open", 4, true, { true, true, true, true }, false, true });

    for (const auto& testCase : cases)
    {
        const auto result = runCase (testCase);
        const auto printStages = testCase.name == "4x all reactors";
        printResult (testCase, result, printStages);
    }
    return 0;
}
