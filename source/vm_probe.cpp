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
constexpr std::uintptr_t kDynamicGuestProbeOffset = 0x0200'0000ull; // 32 MiB
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

bool point_in_range(std::uint64_t region_base,
                    std::uint64_t region_size,
                    std::uint64_t point) {
    return region_size != 0 && point >= region_base && point < region_base + region_size;
}

bool get_region_info(InfoType address_type,
                     InfoType size_type,
                     std::uint64_t& out_base,
                     std::uint64_t& out_size) {
    u64 base = 0;
    u64 size = 0;
    const Result base_rc = svcGetInfo(&base, address_type, CUR_PROCESS_HANDLE, 0);
    const Result size_rc = svcGetInfo(&size, size_type, CUR_PROCESS_HANDLE, 0);
    if (R_FAILED(base_rc) || R_FAILED(size_rc)) {
        return false;
    }
    out_base = base;
    out_size = size;
    return true;
}

void emit_region(FILE* out,
                 const char* name,
                 bool ok,
                 std::uint64_t base,
                 std::uint64_t size) {
    std::fprintf(out, "%s: %s\n", name, ok ? "OK" : "FAILED");
    if (ok) {
        std::fprintf(out, "  base             : 0x%016llx\n",
                     static_cast<unsigned long long>(base));
        std::fprintf(out, "  size             : 0x%016llx\n",
                     static_cast<unsigned long long>(size));
    }
}

void emit(FILE* out, const ProbeResult& r) {
    std::fprintf(out, "WiiCompiled-Switch VM probe v3\n");
    std::fprintf(out, "==============================\n");
    std::fprintf(out, "guest-space target : 0x%llx bytes (4 GiB)\n",
                 static_cast<unsigned long long>(kGuestSpaceSize));
    std::fprintf(out, "upstream AArch64 base: 0x%llx\n\n",
                 static_cast<unsigned long long>(kUpstreamAarch64GuestBase));

    emit_region(out, "ASLR info          ", r.aslr_info_ok, r.aslr_base, r.aslr_size);
    emit_region(out, "Alias region       ", r.alias_info_ok, r.alias_base, r.alias_size);
    emit_region(out, "Heap region        ", r.heap_info_ok, r.heap_base, r.heap_size);
    emit_region(out, "Stack region       ", r.stack_info_ok, r.stack_base, r.stack_size);
    std::fprintf(out, "\n");

    std::fprintf(out, "upstream fixed-base diagnostics\n");
    std::fprintf(out, "  query             : %s\n", r.fixed_query_ok ? "OK" : "FAILED");
    if (r.fixed_query_ok) {
        std::fprintf(out, "  free region base  : 0x%016llx\n",
                     static_cast<unsigned long long>(r.fixed_region_base));
        std::fprintf(out, "  free region size  : 0x%016llx\n",
                     static_cast<unsigned long long>(r.fixed_region_size));
        std::fprintf(out, "  full 4 GiB free   : %s\n", r.fixed_4g_range_unmapped ? "YES" : "NO");
    }
    std::fprintf(out, "  base in Alias     : %s\n", r.fixed_base_in_alias ? "YES" : "NO");
    std::fprintf(out, "  base in Heap      : %s\n", r.fixed_base_in_heap ? "YES" : "NO");
    std::fprintf(out, "  base in Stack     : %s\n\n", r.fixed_base_in_stack ? "YES" : "NO");

    std::fprintf(out, "runtime-selected 4 GiB guest window\n");
    std::fprintf(out, "  candidate         : %s",
                 r.dynamic_4g_candidate_found ? "FOUND" : "NOT FOUND");
    if (r.dynamic_4g_candidate_found) {
        std::fprintf(out, " at 0x%016llx",
                     static_cast<unsigned long long>(r.dynamic_4g_base));
    }
    std::fprintf(out, "\n");
    std::fprintf(out, "  reservation       : %s\n\n", r.dynamic_4g_reserved ? "OK" : "FAILED/NOT RUN");

    std::fprintf(out, "Path A: svcMapMemory heap -> stack region\n");
    std::fprintf(out, "  source alloc      : %s\n", r.stack_alias_source_allocated ? "OK" : "FAILED");
    std::fprintf(out, "  destination       : %s\n", r.stack_alias_destination_found ? "FOUND" : "NOT FOUND");
    std::fprintf(out, "  map result        : 0x%08x\n", r.stack_alias_map_result);
    std::fprintf(out, "  initial read      : %s\n", r.stack_alias_initial_data_visible ? "OK" : "FAILED/NOT RUN");
    std::fprintf(out, "  unmap result      : 0x%08x\n", r.stack_alias_unmap_result);
    std::fprintf(out, "  writeback         : %s\n\n", r.stack_alias_writeback_verified ? "OK" : "FAILED/NOT RUN");

    std::fprintf(out, "Path B: dynamic SharedMemory dual mapping\n");
    std::fprintf(out, "  create            : 0x%08x\n", r.shmem_create_result);
    std::fprintf(out, "  guest map         : 0x%08x", r.shmem_guest_map_result);
    if (r.shmem_guest_base != 0) {
        std::fprintf(out, " at 0x%016llx",
                     static_cast<unsigned long long>(r.shmem_guest_base));
    }
    std::fprintf(out, "\n");
    std::fprintf(out, "  host map          : 0x%08x", r.shmem_host_map_result);
    if (r.shmem_host_base != 0) {
        std::fprintf(out, " at 0x%016llx",
                     static_cast<unsigned long long>(r.shmem_host_base));
    }
    std::fprintf(out, "\n");
    std::fprintf(out, "  guest -> host     : %s\n", r.shmem_guest_to_host_visible ? "OK" : "FAILED/NOT RUN");
    std::fprintf(out, "  host -> guest     : %s\n", r.shmem_host_to_guest_visible ? "OK" : "FAILED/NOT RUN");
    std::fprintf(out, "  guest query       : %s", r.shmem_guest_query_ok ? "OK" : "FAILED/NOT RUN");
    if (r.shmem_guest_query_ok) {
        std::fprintf(out, " type=0x%02x perm=0x%x", r.shmem_guest_type, r.shmem_guest_perm);
    }
    std::fprintf(out, "\n");
    std::fprintf(out, "  host query        : %s", r.shmem_host_query_ok ? "OK" : "FAILED/NOT RUN");
    if (r.shmem_host_query_ok) {
        std::fprintf(out, " type=0x%02x perm=0x%x", r.shmem_host_type, r.shmem_host_perm);
    }
    std::fprintf(out, "\n");
    std::fprintf(out, "  permission -> R   : 0x%08x (failure is expected for SharedMemory)\n",
                 r.shmem_permission_read_result);
    std::fprintf(out, "  guest unmap       : 0x%08x\n", r.shmem_guest_unmap_result);
    std::fprintf(out, "  host unmap        : 0x%08x\n", r.shmem_host_unmap_result);
    std::fprintf(out, "  close             : 0x%08x\n", r.shmem_close_result);
}

} // namespace

ProbeResult run() {
    ProbeResult r{};
    r.stack_alias_map_result = kNotAttempted;
    r.stack_alias_unmap_result = kNotAttempted;
    r.shmem_create_result = kNotAttempted;
    r.shmem_guest_map_result = kNotAttempted;
    r.shmem_host_map_result = kNotAttempted;
    r.shmem_permission_read_result = kNotAttempted;
    r.shmem_guest_unmap_result = kNotAttempted;
    r.shmem_host_unmap_result = kNotAttempted;
    r.shmem_close_result = kNotAttempted;

    r.aslr_info_ok = get_region_info(InfoType_AslrRegionAddress,
                                     InfoType_AslrRegionSize,
                                     r.aslr_base,
                                     r.aslr_size);
    r.alias_info_ok = get_region_info(InfoType_AliasRegionAddress,
                                      InfoType_AliasRegionSize,
                                      r.alias_base,
                                      r.alias_size);
    r.heap_info_ok = get_region_info(InfoType_HeapRegionAddress,
                                     InfoType_HeapRegionSize,
                                     r.heap_base,
                                     r.heap_size);
    r.stack_info_ok = get_region_info(InfoType_StackRegionAddress,
                                      InfoType_StackRegionSize,
                                      r.stack_base,
                                      r.stack_size);

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

    if (r.alias_info_ok) {
        r.fixed_base_in_alias = point_in_range(r.alias_base, r.alias_size,
                                               kUpstreamAarch64GuestBase);
    }
    if (r.heap_info_ok) {
        r.fixed_base_in_heap = point_in_range(r.heap_base, r.heap_size,
                                              kUpstreamAarch64GuestBase);
    }
    if (r.stack_info_ok) {
        r.fixed_base_in_stack = point_in_range(r.stack_base, r.stack_size,
                                               kUpstreamAarch64GuestBase);
    }

    // Reserve a complete 4 GiB virtual window for guest addresses. This is a
    // libnx bookkeeping reservation only: it allocates no 4 GiB physical RAM.
    void* dynamic_guest_window = nullptr;
    VirtmemReservation* guest_reservation = nullptr;
    virtmemLock();
    dynamic_guest_window = virtmemFindAslr(static_cast<std::size_t>(kGuestSpaceSize), kGuardSize);
    if (dynamic_guest_window) {
        r.dynamic_4g_candidate_found = true;
        r.dynamic_4g_base = reinterpret_cast<std::uintptr_t>(dynamic_guest_window);
        guest_reservation = virtmemAddReservation(dynamic_guest_window,
                                                   static_cast<std::size_t>(kGuestSpaceSize));
        r.dynamic_4g_reserved = guest_reservation != nullptr;
    }
    virtmemUnlock();

    // Path A: retain the known-good Horizon remap primitive as a regression
    // check while the SharedMemory strategy evolves.
    void* source = memalign(0x1000, kAliasProbeSize);
    if (source) {
        r.stack_alias_source_allocated = true;
        std::memset(source, 0x5A, kAliasProbeSize);

        void* alias = nullptr;
        virtmemLock();
        alias = virtmemFindStack(kAliasProbeSize, kGuardSize);
        if (alias) {
            r.stack_alias_destination_found = true;
            r.stack_alias_map_result = svcMapMemory(alias, source, kAliasProbeSize);
        }
        virtmemUnlock();

        if (alias && R_SUCCEEDED(static_cast<Result>(r.stack_alias_map_result))) {
            const auto* alias_bytes = static_cast<const volatile std::uint8_t*>(alias);
            r.stack_alias_initial_data_visible =
                alias_bytes[0] == 0x5A && alias_bytes[kAliasProbeSize - 1] == 0x5A;

            auto* writable_alias = static_cast<volatile std::uint8_t*>(alias);
            writable_alias[0] = 0xA5;
            writable_alias[kAliasProbeSize - 1] = 0x3C;

            r.stack_alias_unmap_result = svcUnmapMemory(alias, source, kAliasProbeSize);
            if (R_SUCCEEDED(static_cast<Result>(r.stack_alias_unmap_result))) {
                const auto* source_bytes = static_cast<const std::uint8_t*>(source);
                r.stack_alias_writeback_verified =
                    source_bytes[0] == 0xA5 && source_bytes[kAliasProbeSize - 1] == 0x3C;
            }
        }

        std::free(source);
    }

    // Path B: the actual proposed WiiCompiled/Horizon layout. Keep the full
    // guest window reserved in libnx, map only a tiny physical SharedMemory
    // object inside it, and map the same object again at an independent host VA.
    Handle shmem = INVALID_HANDLE;
    void* guest = nullptr;
    void* host = nullptr;

    if (r.dynamic_4g_reserved) {
        const std::uintptr_t guest_addr =
            reinterpret_cast<std::uintptr_t>(dynamic_guest_window) + kDynamicGuestProbeOffset;
        guest = reinterpret_cast<void*>(guest_addr);
        r.shmem_guest_base = guest_addr;

        const Result create_rc =
            svcCreateSharedMemory(&shmem, kAliasProbeSize, Perm_Rw, Perm_Rw);
        r.shmem_create_result = create_rc;

        if (R_SUCCEEDED(create_rc)) {
            r.shmem_guest_map_result =
                svcMapSharedMemory(shmem, guest, kAliasProbeSize, Perm_Rw);

            if (R_SUCCEEDED(static_cast<Result>(r.shmem_guest_map_result))) {
                virtmemLock();
                host = virtmemFindAslr(kAliasProbeSize, kGuardSize);
                if (host) {
                    r.shmem_host_base = reinterpret_cast<std::uintptr_t>(host);
                    r.shmem_host_map_result =
                        svcMapSharedMemory(shmem, host, kAliasProbeSize, Perm_Rw);
                }
                virtmemUnlock();
            }
        }
    }

    const bool dual_mapped =
        guest != nullptr && host != nullptr &&
        R_SUCCEEDED(static_cast<Result>(r.shmem_guest_map_result)) &&
        R_SUCCEEDED(static_cast<Result>(r.shmem_host_map_result));

    if (dual_mapped) {
        auto* guest_bytes = static_cast<volatile std::uint8_t*>(guest);
        auto* host_bytes = static_cast<volatile std::uint8_t*>(host);

        guest_bytes[0] = 0xC3;
        guest_bytes[kAliasProbeSize - 1] = 0x7E;
        r.shmem_guest_to_host_visible =
            host_bytes[0] == 0xC3 && host_bytes[kAliasProbeSize - 1] == 0x7E;

        host_bytes[0] = 0x42;
        host_bytes[kAliasProbeSize - 1] = 0x24;
        r.shmem_host_to_guest_visible =
            guest_bytes[0] == 0x42 && guest_bytes[kAliasProbeSize - 1] == 0x24;

        MemoryInfo guest_info{};
        u32 guest_page_info = 0;
        if (R_SUCCEEDED(svcQueryMemory(&guest_info, &guest_page_info,
                                       reinterpret_cast<std::uint64_t>(guest)))) {
            r.shmem_guest_query_ok = true;
            r.shmem_guest_type = guest_info.type;
            r.shmem_guest_perm = guest_info.perm;
        }

        MemoryInfo host_info{};
        u32 host_page_info = 0;
        if (R_SUCCEEDED(svcQueryMemory(&host_info, &host_page_info,
                                       reinterpret_cast<std::uint64_t>(host)))) {
            r.shmem_host_query_ok = true;
            r.shmem_host_type = host_info.type;
            r.shmem_host_perm = host_info.perm;
        }

        // Mesosphere's KMemoryState_Shared lacks FlagCanReprotect, so failure
        // here is expected and confirms checked accesses are required for
        // WiiCompiled's MMIO/deferred/executable guard cases on Horizon.
        r.shmem_permission_read_result =
            svcSetMemoryPermission(guest, kAliasProbeSize, Perm_R);
    }

    if (R_SUCCEEDED(static_cast<Result>(r.shmem_guest_map_result)) && guest) {
        r.shmem_guest_unmap_result =
            svcUnmapSharedMemory(shmem, guest, kAliasProbeSize);
    }
    if (R_SUCCEEDED(static_cast<Result>(r.shmem_host_map_result)) && host) {
        r.shmem_host_unmap_result =
            svcUnmapSharedMemory(shmem, host, kAliasProbeSize);
    }
    if (shmem != INVALID_HANDLE) {
        r.shmem_close_result = svcCloseHandle(shmem);
    }

    if (guest_reservation) {
        virtmemLock();
        virtmemRemoveReservation(guest_reservation);
        virtmemUnlock();
    }

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
