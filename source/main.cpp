#include "mem1_sharedmem_diag.hpp"
#include "switch_platform.hpp"

#include <switch.h>
#include <cstdio>

namespace {

constexpr const char* kReportPath = "sdmc:/switch/WiiCompiled-Switch/vm-probe.txt";

void write_marker(const char* marker, bool truncate = false) {
    std::FILE* file = std::fopen(kReportPath, truncate ? "w" : "a");
    if (file) {
        std::fprintf(file, "%s\n", marker);
        std::fclose(file);
    }
    std::printf("%s\n", marker);
    consoleUpdate(nullptr);
}

} // namespace

int main(int, char**) {
    auto info = mkw::switch_platform::initialize();
    mkw::switch_platform::present_bootstrap_screen(info);

    write_marker("WiiCompiled-Switch MEM1 raw SharedMemory diagnostic", true);
    write_marker("Build stamp: mem1-raw-shmem-v3");
    write_marker("Run in hbmenu application/full-memory mode");

    mkw::mem1_sharedmem_diag::run();

    write_marker("MEM1 RAW diagnostic returned to main");
    std::printf("Press + to exit.\n");
    consoleUpdate(nullptr);

    while (!mkw::switch_platform::should_exit()) {
        svcSleepThread(16'000'000);
    }

    mkw::switch_platform::shutdown();
    return 0;
}
