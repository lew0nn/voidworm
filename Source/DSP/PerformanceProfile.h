#pragma once

#include <array>
#include <chrono>
#include <cstdint>

#ifndef VOIDWORM_ENABLE_PERF_PROFILING
#define VOIDWORM_ENABLE_PERF_PROFILING 0
#endif

namespace voidworm::performance
{
enum class Stage : size_t
{
    inputRepair,
    gate,
    driveAndAnalyzer,
    oversamplingUp,
    pathCopies,
    mass,
    furnace,
    arc,
    breach,
    feedback,
    recombination,
    busDynamics,
    rot,
    weldSaturation,
    pathValidation,
    oversamplingDown,
    tear,
    weldDynamics,
    fixedLatencyAndMix,
    masterEqAndOutput,
    limiter,
    containment,
    count
};

inline constexpr std::array<const char*, static_cast<size_t> (Stage::count)> stageNames {
    "input repair", "gate", "DRIVE + SourceAnalyzer", "oversampling up",
    "reactor path copies", "MASS", "FURNACE", "ARC", "BREACH", "FEEDBACK",
    "reactor recombination", "reactor bus compressor", "ROT", "oversampled WELD saturation",
    "reactor validation", "oversampling down", "TEAR", "WELD compressor",
    "fixed latency + dry/wet", "MASTER EQ + output", "final limiter", "gate containment"
};

struct Snapshot
{
    std::array<uint64_t, static_cast<size_t> (Stage::count)> nanoseconds {};
    std::array<uint64_t, static_cast<size_t> (Stage::count)> calls {};
};

#if VOIDWORM_ENABLE_PERF_PROFILING
inline Snapshot totals;

inline void reset() noexcept
{
    totals = {};
}

inline Snapshot snapshot() noexcept
{
    return totals;
}

class Scope
{
public:
    explicit Scope (Stage stageToMeasure) noexcept
        : stage (stageToMeasure), start (Clock::now())
    {
    }

    ~Scope() noexcept
    {
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds> (Clock::now() - start).count();
        const auto index = static_cast<size_t> (stage);
        totals.nanoseconds[index] += static_cast<uint64_t> (elapsed);
        ++totals.calls[index];
    }

private:
    using Clock = std::chrono::steady_clock;
    Stage stage;
    Clock::time_point start;
};
#else
inline void reset() noexcept {}
inline Snapshot snapshot() noexcept { return {}; }
class Scope
{
public:
    explicit Scope (Stage) noexcept {}
};
#endif
}

#define VOIDWORM_DETAIL_JOIN_INNER(a, b) a##b
#define VOIDWORM_DETAIL_JOIN(a, b) VOIDWORM_DETAIL_JOIN_INNER(a, b)
#define VOIDWORM_PROFILE_SCOPE(stage) \
    ::voidworm::performance::Scope VOIDWORM_DETAIL_JOIN(voidwormProfileScope_, __LINE__) (stage)
