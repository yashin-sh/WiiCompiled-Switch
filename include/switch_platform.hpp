#pragma once

#include <cstdint>

namespace mkw::switch_platform {

struct BootstrapInfo {
    bool sd_mounted;
    std::uint64_t ticks;
};

BootstrapInfo initialize();
void shutdown();
bool should_exit();
void present_bootstrap_screen(const BootstrapInfo& info);

} // namespace mkw::switch_platform
