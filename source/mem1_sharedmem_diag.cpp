#include "mem1_sharedmem_diag.hpp"
#include "heap_runtime_diag.hpp"

#include <switch.h>

#include <cstdint>
#include <cstdio>
#include <cstring>

namespace mkw::mem1_sharedmem_diag {
namespace {

constexpr const char* kReportPath = "sdmc:/switch/WiiCompiled-Switch/vm-probe.txt";
constexpr std::size_t kMem1Size = 24u * 1024u * 1024u;
constexpr std::size_t kGuestSpaceSize = 0x1'0000'0000ull;
constexpr std::size_t kGuardSize = 0x1000;

void marker(const char* text) {
    if (std::FILE* file = std::fopen(kReportPath, "a")) {
        std::fprintf(file, "%s\n", text);
        std::fclose(file);
    }
    std::printf("%s\n", text);
    consoleUpdate(nullptr);
}

void marker_rc(const char* label, Result rc) {
    char line[128]{};
    std::snprintf(line, sizeof(line), "%s: 0x%08x", label,
                  static_cast<unsigned int>(rc));
    marker(line);
}

void marker_ptr(const char* label, const void* ptr) {
    char line[128]{};
    std::snprintf(line, sizeof(line), "%s: 0x%016llx", label,
                  static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(ptr)));
    marker(line);
}

void marker_u64(const char* label, std::uint64_t value) {
    char line[128]{};
    std::snprintf(line, sizeof(line), "%s: 0x%016llx (%llu MiB)", label,
                  static_cast<unsigned long long>(value),
                  static_cast<unsigned long long>(value / (1024ull * 1024ull)));
    marker(line);
}

void emit_heap_state() {
    const auto& heap = mkw::heap_runtime_diag::state();
    marker(heap.env_override ? "HEAP env override: YES" : "HEAP env override: NO");
    marker_ptr("HEAP env addr", reinterpret_cast<const void*>(heap.env_addr));
    marker_u64("HEAP env size", heap.env_size);
    marker_rc("HEAP svcSetHeapSize(512MiB) rc", static_cast<Result>(heap.set_heap_rc));
    marker_ptr("HEAP selected addr", reinterpret_cast<const void*>(heap.selected_heap_addr));
    marker_u64("HEAP selected size", heap.selected_heap_size);
    marker_u64("MEM total before", heap.total_before);
    marker_u64("MEM used before", heap.used_before);
    marker_u64("MEM total after", heap.total_after);
    marker_u64("MEM used after", heap.used_after);
}

} // namespace

void run() {
    marker("MEM1 RAW DIAG START");
    emit_heap_state();

    Handle handle = INVALID_HANDLE;
    marker("MEM1 RAW create SharedMemory begin");
    const Result create_rc = svcCreateSharedMemory(&handle, kMem1Size, Perm_Rw, Perm_Rw);
    marker_rc("MEM1 RAW create SharedMemory rc", create_rc);
    if (R_FAILED(create_rc)) {
        marker("MEM1 RAW STOP after create failure");
        return;
    }

    marker("MEM1 RAW find host VA begin");
    void* host = nullptr;
    virtmemLock();
    host = virtmemFindAslr(kMem1Size, kGuardSize);
    virtmemUnlock();
    marker_ptr("MEM1 RAW host candidate", host);
    if (host == nullptr) {
        marker("MEM1 RAW STOP no host VA");
        svcCloseHandle(handle);
        return;
    }

    marker("MEM1 RAW map host begin");
    const Result host_map_rc = svcMapSharedMemory(handle, host, kMem1Size, Perm_Rw);
    marker_rc("MEM1 RAW map host rc", host_map_rc);
    if (R_FAILED(host_map_rc)) {
        svcCloseHandle(handle);
        marker("MEM1 RAW STOP host map failure");
        return;
    }

    auto* bytes = static_cast<std::uint8_t*>(host);
    marker("MEM1 RAW host touch first/last begin");
    bytes[0] = 0x5A;
    bytes[kMem1Size - 1] = 0xA5;
    marker("MEM1 RAW host touch first/last OK");

    marker("MEM1 RAW memset 24MiB begin");
    std::memset(host, 0, kMem1Size);
    marker("MEM1 RAW memset 24MiB OK");

    marker("MEM1 RAW find 4GiB guest VA begin");
    void* guest_base = nullptr;
    VirtmemReservation* reservation = nullptr;
    virtmemLock();
    guest_base = virtmemFindAslr(kGuestSpaceSize, kGuardSize);
    if (guest_base != nullptr) {
        reservation = virtmemAddReservation(guest_base, kGuestSpaceSize);
    }
    virtmemUnlock();
    marker_ptr("MEM1 RAW guest candidate", guest_base);
    marker_ptr("MEM1 RAW reservation", reservation);
    if (guest_base == nullptr || reservation == nullptr) {
        marker("MEM1 RAW STOP guest reservation failure");
        svcUnmapSharedMemory(handle, host, kMem1Size);
        svcCloseHandle(handle);
        return;
    }

    auto map_alias = [&](std::uintptr_t offset, const char* label) -> bool {
        auto* target = static_cast<std::uint8_t*>(guest_base) + offset;
        char begin[128]{};
        std::snprintf(begin, sizeof(begin), "%s begin @ 0x%016llx", label,
                      static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(target)));
        marker(begin);
        const Result rc = svcMapSharedMemory(handle, target, kMem1Size, Perm_Rw);
        marker_rc(label, rc);
        return R_SUCCEEDED(rc);
    };

    const bool phys_ok = map_alias(0x00000000ull, "MEM1 RAW guest phys map rc");
    if (phys_ok) {
        marker("MEM1 RAW guest phys touch begin");
        auto* phys = static_cast<std::uint8_t*>(guest_base);
        phys[0x100] = 0x11;
        marker("MEM1 RAW guest phys touch OK");
    }

    const bool cached_ok = phys_ok && map_alias(0x80000000ull, "MEM1 RAW guest cached map rc");
    if (cached_ok) {
        marker("MEM1 RAW guest cached coherence begin");
        auto* cached = static_cast<std::uint8_t*>(guest_base) + 0x80000000ull;
        cached[0x101] = 0x22;
        if (bytes[0x100] == 0x11 && bytes[0x101] == 0x22) {
            marker("MEM1 RAW guest cached coherence OK");
        } else {
            marker("MEM1 RAW guest cached coherence FAILED");
        }
    }

    const bool uncached_ok = cached_ok && map_alias(0xC0000000ull, "MEM1 RAW guest uncached map rc");
    if (uncached_ok) {
        marker("MEM1 RAW guest uncached coherence begin");
        auto* uncached = static_cast<std::uint8_t*>(guest_base) + 0xC0000000ull;
        uncached[0x102] = 0x33;
        if (bytes[0x102] == 0x33) {
            marker("MEM1 RAW guest uncached coherence OK");
        } else {
            marker("MEM1 RAW guest uncached coherence FAILED");
        }
    }

    marker("MEM1 RAW cleanup begin");
    if (uncached_ok) {
        svcUnmapSharedMemory(handle,
            static_cast<std::uint8_t*>(guest_base) + 0xC0000000ull, kMem1Size);
    }
    if (cached_ok) {
        svcUnmapSharedMemory(handle,
            static_cast<std::uint8_t*>(guest_base) + 0x80000000ull, kMem1Size);
    }
    if (phys_ok) {
        svcUnmapSharedMemory(handle, guest_base, kMem1Size);
    }
    virtmemLock();
    virtmemRemoveReservation(reservation);
    virtmemUnlock();
    svcUnmapSharedMemory(handle, host, kMem1Size);
    svcCloseHandle(handle);
    marker("MEM1 RAW cleanup OK");
    marker("MEM1 RAW DIAG COMPLETE");
}

} // namespace mkw::mem1_sharedmem_diag
