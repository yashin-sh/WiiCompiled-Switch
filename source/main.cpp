#include "switch_platform.hpp"
#include "vm_probe.hpp"

#include <switch.h>
#include <cstdio>

int main(int, char**) {
    auto info = mkw::switch_platform::initialize();
    mkw::switch_platform::present_bootstrap_screen(info);

    const auto vm_result = mkw::vm_probe::run();
    mkw::vm_probe::print(vm_result);
    if (!mkw::vm_probe::write_report(vm_result)) {
        std::printf("WARNING: could not write vm-probe.txt to the app folder.\n");
        consoleUpdate(nullptr);
    }

    while (!mkw::switch_platform::should_exit()) {
        svcSleepThread(16'000'000); // ~16 ms; bootstrap only, not final frame pacing.
    }

    mkw::switch_platform::shutdown();
    return 0;
}
