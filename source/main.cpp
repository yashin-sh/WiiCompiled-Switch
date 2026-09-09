#include "memory_init_probe.hpp"
#include "memory_switch_slice.hpp"
#include "switch_platform.hpp"

#include <switch.h>
#include <cstdio>

namespace {

constexpr const char* kReportPath = "sdmc:/switch/WiiCompiled-Switch/vm-probe.txt";
constexpr const char* kBuildStamp = "memory-init-isolation-v2";

void write_marker(const char* marker, bool truncate = false) {
    std::FILE* file = std::fopen(kReportPath, truncate ? "w" : "a");
    if (!file) {
        return;
    }
    std::fprintf(file, "%s\n", marker);
    std::fclose(file);
    std::printf("%s\n", marker);
    consoleUpdate(nullptr);
}

void run_preflight(const char* name, const Memory::Config& config) {
    char marker[128]{};
    std::snprintf(marker, sizeof(marker), "PRECHECK %s START", name);
    write_marker(marker);
    Memory::Init(config);
    std::snprintf(marker, sizeof(marker), "PRECHECK %s INIT OK", name);
    write_marker(marker);
    Memory::Reset();
    std::snprintf(marker, sizeof(marker), "PRECHECK %s RESET OK", name);
    write_marker(marker);
}

Memory::Config mem1_config() {
    Memory::Config config;
    config.regions.push_back({"MEM1_PHYS", Memory::kMem1PhysicalBase, Memory::kMem1Size});
    config.regions.push_back({"MEM1_CACHED", Memory::kMem1CachedBase, Memory::kMem1Size});
    config.regions.push_back({"MEM1_UNCACHED", Memory::kMem1UncachedBase, Memory::kMem1Size});
    return config;
}

Memory::Config mem2_config() {
    Memory::Config config;
    config.regions.push_back({"MEM2_PHYS", Memory::kMem2PhysicalBase, Memory::kMem2Size});
    config.regions.push_back({"MEM2_CACHED", Memory::kMem2CachedBase, Memory::kMem2Size});
    config.regions.push_back({"MEM2_UNCACHED", Memory::kMem2UncachedBase, Memory::kMem2Size});
    return config;
}

Memory::Config locked_cache_config() {
    Memory::Config config;
    config.regions.push_back({"LOCKED_CACHE", Memory::kLockedCacheBase, Memory::kLockedCacheSize});
    return config;
}

} // namespace

int main(int, char**) {
    auto info = mkw::switch_platform::initialize();
    mkw::switch_platform::present_bootstrap_screen(info);

    write_marker("WiiCompiled-Switch Memory isolation diagnostic", true);
    write_marker("Build stamp: memory-init-isolation-v2");
    write_marker("Run in hbmenu application/full-memory mode");

    run_preflight("MEM1_24MiB", mem1_config());
    run_preflight("MEM2_128MiB", mem2_config());
    run_preflight("LOCKED_CACHE", locked_cache_config());

    write_marker("FULL Memory::Init START");
    const auto memory_init_result = mkw::memory_init_probe::run();
    write_marker("FULL Memory::Init RETURNED");
    mkw::memory_init_probe::print(memory_init_result);
    if (!mkw::memory_init_probe::append_report(memory_init_result)) {
        write_marker("FULL report append FAILED");
    }
    write_marker("ALL MEMORY DIAGNOSTICS COMPLETE");

    std::printf("Press + to exit.\n");
    consoleUpdate(nullptr);
    while (!mkw::switch_platform::should_exit()) {
        svcSleepThread(16'000'000);
    }

    mkw::switch_platform::shutdown();
    return 0;
}
