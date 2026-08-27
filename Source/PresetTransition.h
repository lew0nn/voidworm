#pragma once

#include "DSP/VoidEngine.h"

namespace voidworm
{
struct PresetSnapshot
{
    Parameters parameters;
};

// One message-thread producer and one audio-thread consumer. Slots are never
// overwritten until the consumer releases them, so snapshots remain coherent
// without locks, allocation, or retry loops on the audio thread.
class PresetSnapshotMailbox
{
public:
    bool push (const PresetSnapshot& snapshot) noexcept
    {
        const auto write = writeIndex.load (std::memory_order_relaxed);
        const auto next = increment (write);
        if (next == readIndex.load (std::memory_order_acquire))
            return false;
        snapshots[write] = snapshot;
        writeIndex.store (next, std::memory_order_release);
        return true;
    }

    bool pop (PresetSnapshot& snapshot) noexcept
    {
        const auto read = readIndex.load (std::memory_order_relaxed);
        if (read == writeIndex.load (std::memory_order_acquire))
            return false;
        snapshot = snapshots[read];
        readIndex.store (increment (read), std::memory_order_release);
        return true;
    }

private:
    static constexpr size_t capacity = 64;
    static constexpr size_t increment (size_t index) noexcept { return (index + 1) % capacity; }

    std::array<PresetSnapshot, capacity> snapshots {};
    std::atomic<size_t> writeIndex { 0 };
    std::atomic<size_t> readIndex { 0 };
};
}
