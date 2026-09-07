#pragma once

#include <cstdint>

namespace mkw::context_probe {

struct ProbeResult {
    bool stack_allocated = false;
    bool context_initialized = false;
    bool first_handoff_ok = false;
    bool continuation_ok = false;
    bool register_preservation_ok = false;
    bool stress_ok = false;
    std::uint32_t completed_switches = 0;
    std::uint64_t elapsed_ticks = 0;
};

ProbeResult run();
void print(const ProbeResult& result);
bool append_report(const ProbeResult& result);

} // namespace mkw::context_probe
