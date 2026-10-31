#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace ltm {

struct ProcessResources {
    std::optional<double> cpu_total_ms;
    std::optional<std::uint64_t> resident_bytes;
    std::optional<std::uint64_t> private_committed_bytes;
    std::optional<std::uint64_t> process_lifetime_peak_resident_bytes;
};

struct ResourceMeasurement {
    std::string stage;
    double wall_ms;
    ProcessResources before;
    ProcessResources after;
    std::optional<double> cpu_delta_ms;
};

ProcessResources sample_process_resources();
ResourceMeasurement resource_interval(std::string stage, ProcessResources before,
    ProcessResources after, double wall_ms);

} // namespace ltm
