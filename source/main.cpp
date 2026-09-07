#include "switch_platform.hpp"

#include <switch.h>

int main(int, char**) {
    auto info = mkw::switch_platform::initialize();
    mkw::switch_platform::present_bootstrap_screen(info);

    while (!mkw::switch_platform::should_exit()) {
        svcSleepThread(16'000'000); // ~16 ms; bootstrap only, not final frame pacing.
    }

    mkw::switch_platform::shutdown();
    return 0;
}
