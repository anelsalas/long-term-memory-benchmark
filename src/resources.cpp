#include "long_term_memory_benchmark/resources.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#endif

namespace ltm {
ProcessResources sample_process_resources() {
    ProcessResources sample;
#if defined(_WIN32)
    FILETIME creation{}, exit{}, kernel{}, user{};
    if (GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user)) {
        const auto ticks = [](FILETIME value) {
            return (static_cast<std::uint64_t>(value.dwHighDateTime) << 32) | value.dwLowDateTime;
        };
        sample.cpu_total_ms = (static_cast<double>(ticks(kernel)) + static_cast<double>(ticks(user))) / 10000.0;
    }
    PROCESS_MEMORY_COUNTERS_EX memory{};
    memory.cb = sizeof(memory);
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory))) {
        sample.resident_bytes = memory.WorkingSetSize;
        sample.private_committed_bytes = memory.PrivateUsage;
        sample.process_lifetime_peak_resident_bytes = memory.PeakWorkingSetSize;
    }
#endif
    return sample;
}
ResourceMeasurement resource_interval(std::string stage, ProcessResources before,
    ProcessResources after, double wall_ms) {
    if (!std::isfinite(wall_ms) || wall_ms < 0)
        throw std::invalid_argument("Resource interval wall time must be finite and nonnegative");
    ResourceMeasurement interval{std::move(stage), wall_ms, before, after, std::nullopt};
    if (before.cpu_total_ms && after.cpu_total_ms && std::isfinite(*before.cpu_total_ms) &&
        std::isfinite(*after.cpu_total_ms) && *before.cpu_total_ms >= 0 && *after.cpu_total_ms >= *before.cpu_total_ms) {
        const auto delta = *after.cpu_total_ms - *before.cpu_total_ms;
        if (std::isfinite(delta)) interval.cpu_delta_ms = delta;
    }
    return interval;
}
} // namespace ltm
