#include "guest_flat_api_probe.hpp"
#include "guest_flat_memory.h"

#include <switch.h>

#include <cstdio>
#include <vector>

namespace mkw::guest_flat_api_probe {
namespace {

constexpr const char* kReportPath = "sdmc:/switch/WiiCompiled-Switch/vm-probe.txt";

void emit(FILE* out, const Result& r) {
    std::fprintf(out, "\nWiiCompiled upstream GuestFlat API smoke test\n");
    std::fprintf(out, "===========================================\n");
    std::fprintf(out, "initialized          : %s\n", r.initialized ? "OK" : "FAILED");
    std::fprintf(out, "runtime guest base   : %s\n", r.runtime_base_available ? "OK" : "FAILED");
    std::fprintf(out, "checked-access policy: %s\n", r.checked_access_policy ? "OK" : "FAILED");
    std::fprintf(out, "HostPointer          : %s\n", r.host_pointer_available ? "OK" : "FAILED");
    std::fprintf(out, "MEM1 aliases         : %s\n", r.mem1_alias_coherent ? "OK" : "FAILED");
    std::fprintf(out, "MEM2 aliases         : %s\n", r.mem2_alias_coherent ? "OK" : "FAILED");
    std::fprintf(out, "Owned mapping        : %s\n", r.owned_visible ? "OK" : "FAILED");
    std::fprintf(out, "teardown             : %s\n", r.teardown_ok ? "OK" : "FAILED");
    std::fprintf(out, "GuestFlat API smoke  : %s\n", r.passed() ? "PASS" : "FAIL");
}

} // namespace

Result run() {
    Result r{};

    const std::vector<GuestFlat::RegionRequest> regions = {
        {0x00000000u, 0x4000u, GuestFlat::Backing::Mem1},
        {0x80000000u, 0x4000u, GuestFlat::Backing::Mem1},
        {0xC0000000u, 0x4000u, GuestFlat::Backing::Mem1},
        {0x10000000u, 0x4000u, GuestFlat::Backing::Mem2},
        {0x90000000u, 0x4000u, GuestFlat::Backing::Mem2},
        {0xD0000000u, 0x4000u, GuestFlat::Backing::Mem2},
        {0x01800000u, 0x4000u, GuestFlat::Backing::Owned},
    };

    GuestFlat::Initialize(regions);
    r.initialized = GuestFlat::IsActive();
    r.runtime_base_available = GuestFlat::Base() != nullptr && MKW_FLAT_GUEST_BASE == GuestFlat::Base();
    r.checked_access_policy = GuestFlat::RequiresCheckedAccess();

    auto* mem1_phys = GuestFlat::HostPointer(0x00000000u);
    auto* mem1_cached = GuestFlat::HostPointer(0x80000000u);
    auto* mem1_uncached = GuestFlat::HostPointer(0xC0000000u);
    auto* mem2_phys = GuestFlat::HostPointer(0x10000000u);
    auto* mem2_cached = GuestFlat::HostPointer(0x90000000u);
    auto* mem2_uncached = GuestFlat::HostPointer(0xD0000000u);
    auto* owned = GuestFlat::HostPointer(0x01800000u);

    r.host_pointer_available =
        mem1_phys && mem1_cached && mem1_uncached &&
        mem2_phys && mem2_cached && mem2_uncached && owned;

    if (mem1_phys && mem1_cached && mem1_uncached) {
        mem1_phys[5] = 0x51;
        const bool phys_to_aliases = mem1_cached[5] == 0x51 && mem1_uncached[5] == 0x51;
        mem1_uncached[6] = 0xA6;
        const bool alias_to_phys = mem1_phys[6] == 0xA6;
        r.mem1_alias_coherent =
            mem1_phys == mem1_cached &&
            mem1_phys == mem1_uncached &&
            phys_to_aliases && alias_to_phys;
    }

    if (mem2_phys && mem2_cached && mem2_uncached) {
        mem2_phys[9] = 0x92;
        const bool phys_to_aliases = mem2_cached[9] == 0x92 && mem2_uncached[9] == 0x92;
        mem2_uncached[10] = 0x2A;
        const bool alias_to_phys = mem2_phys[10] == 0x2A;
        r.mem2_alias_coherent =
            mem2_phys == mem2_cached &&
            mem2_phys == mem2_uncached &&
            phys_to_aliases && alias_to_phys;
    }

    if (owned) {
        owned[3] = 0x18;
        auto* checked = GuestFlat::HostPointer(0x01800003u);
        r.owned_visible = checked != nullptr && *checked == 0x18;
    }

    GuestFlat::Shutdown();
    r.teardown_ok = !GuestFlat::IsActive() && GuestFlat::Base() == nullptr;
    return r;
}

void print(const Result& result) {
    emit(stdout, result);
    consoleUpdate(nullptr);
}

bool append_report(const Result& result) {
    FILE* out = std::fopen(kReportPath, "a");
    if (!out) {
        return false;
    }
    emit(out, result);
    std::fclose(out);
    return true;
}

} // namespace mkw::guest_flat_api_probe
