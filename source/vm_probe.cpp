#include "vm_probe.hpp"

#include <switch.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <malloc.h>

namespace mkw::vm_probe {
namespace {

constexpr std::uint64_t kGuestSpaceSize = 0x1'0000'0000ull; // 4 GiB
constexpr std::uint64_t kUpstreamAarch64GuestBase = 0x10'0000'0000ull; // 64 GiB
constexpr std::size_t kAliasProbeSize = 0x10000;
constexpr std::size_t kGuardSize = 0x1000;
constexpr std::uint32_t kNotAttempted = 0xFFFF'FFFFu;

bool range_contains(std::uint64_t region_base,
                    std::uint64_t region_size,
                    std::uint64_t wanted_base,
                    std::uint64_t wanted_size) {
    if (wanted_base < region_base || wanted_size > region_size) {
        return false;
    }
    const std::uint64_t offset = wanted_base - region_base;
    return offset <= region_size - wanted_size;
}

void emit(FILE* out, const ProbeResult& r) {
    std::fprintf(out, "WiiCompiled-Switch VM probe\n");
    std::fprintf(out, "===========================\n");
    std::fprintf(out, "guest-space target : 0x%llx bytes (4 GiB)\n",
                 static_cast<unsigned long long>(kGuestSpaceSize));
    std::fprintf(out, "upstream AArch64 base: 0x%llx\n\n",
                 static_cast<unsigned long long>(kUpstreamAarch64GuestBase));

    std::fprintf(out, "ASLR info          : %s\n", r.aslr_info_ok ? "OK" : "FAILED");
    if (r.aslr_info_ok) {
        std::fprintf(out, "  base             : 0x%016llx\n",
                     static_cast<unsigned long long>(r.aslr_base));
        std::fprintf(out, "  size             : 0x%016llx\n",
                     static_cast<unsigned long long>(r.aslr_size));
    }

    std::fprintf(out, "fixed-base query   : %s\n", r.fixed_query_ok ? "OK" : "FAILED");
    if (r.fixed_query_ok) {
        std::fprintf(out, "  region base      : 0x%016llx\n",
                     static_cast<unsigned long long>(r.fixed_region_base));
        std::fprintf(out, "  region size      : 0x%016llx\n",
                     static_cast<unsigned long long>(r.fixed_region_size));
        std::fprintf(out, "  full 4 GiB free  : %s\n", r.fixed_4g_range_unmapped ? "YES" : "NO");
    }

    std::fprintf(out, "random 4 GiB VA    : %s",
                 r.random_4g_candidate_found ? "FOUND" : "NOT FOUND");
    if (r.random_4g_candidate_found) {
        std::fprintf(out, " at 0x%016llx",
                     static_cast<unsigned long long>(r.random_4g_base));
    }
    std::fprintf(out, "\n\n");

    std::fprintf(out, "alias source alloc : %s\n", r.alias_source_allocated ? "OK" : "FAILED");
    std::fprintf(out, "alias destination  : %s\n", r.alias_destination_found ? "FOUND" : "NOT FOUND");
    std::fprintf(out, "svcMapMemory       : 0x%08x\n", r.alias_map_result);
    std::fprintf(out, "alias initial read : %s\n", r.alias_initial_data_visible ? "OK" : "FAILED/NOT RUN");
    std::fprintf(out, "permission -> R    : 0x%08x\n", r.permission_read_result);
    std::fprintf(out, "permission -> RW   : 0x%08x\n", r.permission_rw_result);
    std::fprintf(out, "svcUnmapMemory     : 0x%08x\n", r.alias_unmap_result);
    std::fprintf(out, "alias writeback    : %s\n", r.alias_writeback_verified ? "OK" : "FAILED/NOT RUN");
}

} // namespace

ProbeResult run() {
    ProbeResult r{};
    r.alias_map_result = kNotAttempted;
    r.permission_read_result = kNotAttempted;
    r.permission_rw_result = kNotAttempted;
    r.alias_unmap_result = kNotAttempted;

    u64 aslr_base = 0;
    u64 aslr_size = 0;
    const Result aslr_base_rc = svcGetInfo(&aslr_base, InfoType_AslrRegionAddress,
                                           CUR_PROCESS_HANDLE, 0);
    const Result aslr_size_rc = svcGetInfo(&aslr_size, InfoType_AslrRegionSize,
                                           CUR_PROCESS_HANDLE, 0);
    if (R_SUCCEEDED(aslr_base_rc) && R_SUCCEEDED(aslr_size_rc)) {
        r.aslr_info_ok = true;
        r.aslr_base = aslr_base;
        r.aslr_size = aslr_size;
    }

    MemoryInfo fixed_info{};
    u32 fixed_page_info = 0;
    const Result fixed_rc = svcQueryMemory(&fixed_info, &fixed_page_info,
                                           kUpstreamAarch64GuestBase);
    if (R_SUCCEEDED(fixed_rc)) {
        r.fixed_query_ok = true;
        r.fixed_region_base = fixed_info.addr;
        r.fixed_region_size = fixed_info.size;
        r.fixed_4g_range_unmapped =
            fixed_info.type == MemType_Unmapped &&
            range_contains(fixed_info.addr, fixed_info.size,
                           kUpstreamAarch64GuestBase, kGuestSpaceSize);
    }

    // Ask libnx's address-space manager whether any 4 GiB slice is currently
    // available. This reserves no physical memory and is safe to release
    // immediately after the probe.
    virtmemLock();
    void* candidate = virtmemFindAslr(static_cast<std::size_t>(kGuestSpaceSize), kGuardSize);
    VirtmemReservation* reservation = nullptr;
    if (candidate) {
        reservation = virtmemAddReservation(candidate, static_cast<std::size_t>(kGuestSpaceSize));
        if (reservation) {
            r.random_4g_candidate_found = true;
            r.random_4g_base = reinterpret_cast<std::uintptr_t>(candidate);
            virtmemRemoveReservation(reservation);
        }
    }
    virtmemUnlock();

    // Small dual-alias test. We intentionally test only 64 KiB of backing;
    // the 4 GiB check above is virtual-address-space-only and allocates no
    // giant physical buffer.
    void* source = memalign(0x1000, kAliasProbeSize);
    if (!source) {
        return r;
    }
    r.alias_source_allocated = true;
    std::memset(source, 0x5A, kAliasProbeSize);

    void* alias = nullptr;
    virtmemLock();
    alias = virtmemFindAslr(kAliasProbeSize, kGuardSize);
    if (alias) {
        r.alias_destination_found = true;
        const Result map_rc = svcMapMemory(alias, source, kAliasProbeSize);
        r.alias_map_result = map_rc;
    }
    virtmemUnlock();

    if (!alias || R_FAILED(static_cast<Result>(r.alias_map_result))) {
        std::free(source);
        return r;
    }

    const auto* alias_bytes = static_cast<const volatile std::uint8_t*>(alias);
    r.alias_initial_data_visible =
        alias_bytes[0] == 0x5A && alias_bytes[kAliasProbeSize - 1] == 0x5A;

    const Result read_rc = svcSetMemoryPermission(alias, kAliasProbeSize, Perm_R);
    r.permission_read_result = read_rc;

    bool alias_is_writable = R_FAILED(read_rc);
    if (R_SUCCEEDED(read_rc)) {
        const Result rw_rc = svcSetMemoryPermission(alias, kAliasProbeSize, Perm_Rw);
        r.permission_rw_result = rw_rc;
        alias_is_writable = R_SUCCEEDED(rw_rc);
    }

    if (alias_is_writable) {
        auto* writable_alias = static_cast<volatile std::uint8_t*>(alias);
        writable_alias[0] = 0xA5;
        writable_alias[kAliasProbeSize - 1] = 0x3C;
    }

    const Result unmap_rc = svcUnmapMemory(alias, source, kAliasProbeSize);
    r.alias_unmap_result = unmap_rc;
    if (R_SUCCEEDED(unmap_rc) && alias_is_writable) {
        const auto* source_bytes = static_cast<const std::uint8_t*>(source);
        r.alias_writeback_verified =
            source_bytes[0] == 0xA5 && source_bytes[kAliasProbeSize - 1] == 0x3C;
    }

    std::free(source);
    return r;
}

void print(const ProbeResult& result) {
    std::printf("\n");
    emit(stdout, result);
    std::printf("\nReport path: sdmc:/switch/WiiCompiled-Switch/vm-probe.txt\n");
    consoleUpdate(nullptr);
}

bool write_report(const ProbeResult& result) {
    FILE* out = std::fopen("sdmc:/switch/WiiCompiled-Switch/vm-probe.txt", "w");
    if (!out) {
        return false;
    }
    emit(out, result);
    std::fclose(out);
    return true;
}

} // namespace mkw::vm_probe
