#include "long_term_memory_benchmark/resources.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <fstream>
#include <sys/resource.h>
#include <unistd.h>
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
#elif defined(__linux__)
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        sample.cpu_total_ms = static_cast<double>(usage.ru_utime.tv_sec) * 1000.0 +
            static_cast<double>(usage.ru_utime.tv_usec) / 1000.0 +
            static_cast<double>(usage.ru_stime.tv_sec) * 1000.0 +
            static_cast<double>(usage.ru_stime.tv_usec) / 1000.0;
        if (usage.ru_maxrss >= 0 && static_cast<std::uint64_t>(usage.ru_maxrss) <=
            std::numeric_limits<std::uint64_t>::max() / 1024)
            sample.process_lifetime_peak_resident_bytes = static_cast<std::uint64_t>(usage.ru_maxrss) * 1024;
    }
    std::ifstream statm("/proc/self/statm");
    std::uint64_t total_pages = 0, resident_pages = 0;
    const long page_size = sysconf(_SC_PAGESIZE);
    if ((statm >> total_pages >> resident_pages) && page_size > 0 && resident_pages <=
        std::numeric_limits<std::uint64_t>::max() / static_cast<std::uint64_t>(page_size))
        sample.resident_bytes = resident_pages * static_cast<std::uint64_t>(page_size);
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
