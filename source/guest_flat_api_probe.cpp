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

    auto* mem1_host = GuestFlat::HostPointer(0x80000000u);
    auto* mem2_host = GuestFlat::HostPointer(0x90000000u);
    auto* owned_host = GuestFlat::HostPointer(0x01800000u);
    r.host_pointer_available = mem1_host != nullptr && mem2_host != nullptr && owned_host != nullptr;

    auto* guest = GuestFlat::Base();
    if (guest && mem1_host) {
        mem1_host[5] = 0x51;
        const bool cached = guest[0x80000005u] == 0x51;
        const bool uncached = guest[0xC0000005u] == 0x51;
        guest[0x00000006u] = 0xA6;
        const bool physical_to_host = mem1_host[6] == 0xA6;
        r.mem1_alias_coherent = cached && uncached && physical_to_host;
    }

    if (guest && mem2_host) {
        mem2_host[9] = 0x92;
        const bool cached = guest[0x90000009u] == 0x92;
        const bool uncached = guest[0xD0000009u] == 0x92;
        guest[0x1000000Au] = 0x2A;
        const bool physical_to_host = mem2_host[10] == 0x2A;
        r.mem2_alias_coherent = cached && uncached && physical_to_host;
    }

    if (guest && owned_host) {
        owned_host[3] = 0x18;
        r.owned_visible = guest[0x01800003u] == 0x18;
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
