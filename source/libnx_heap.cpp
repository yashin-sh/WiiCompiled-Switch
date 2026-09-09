#include "heap_runtime_diag.hpp"

#include <switch.h>

#include <cstddef>
#include <cstdint>

namespace {

constexpr std::size_t kTargetHeapSize = 0x20000000ULL; // 512 MiB.
mkw::heap_runtime_diag::State g_state{};

} // namespace

namespace mkw::heap_runtime_diag {

const State& state() noexcept {
    return g_state;
}

} // namespace mkw::heap_runtime_diag

extern "C" {
extern char* fake_heap_start;
extern char* fake_heap_end;

// hbmenu/hbloader can provide an environment heap override. In that case the
// stock libnx __libnx_initheap() ignores __nx_heap_size entirely. WiiCompiled
// needs kernel memory headroom for MEM1/MEM2 SharedMemory objects, so explicitly
// resize the process heap here and fall back to the loader-provided heap only if
// Horizon rejects the resize. The fallback keeps the diagnostic NRO alive so it
// can report the exact failure instead of aborting during libnx startup.
void __libnx_initheap(void) {
    g_state.env_override = envHasHeapOverride();
    if (g_state.env_override) {
        g_state.env_addr = reinterpret_cast<std::uintptr_t>(envGetHeapOverrideAddr());
        g_state.env_size = envGetHeapOverrideSize();
    }

    u64 total = 0;
    u64 used = 0;
    svcGetInfo(&total, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0);
    svcGetInfo(&used, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0);
    g_state.total_before = total;
    g_state.used_before = used;

    void* heap_addr = nullptr;
    const Result resize_rc = svcSetHeapSize(&heap_addr, kTargetHeapSize);
    g_state.set_heap_rc = static_cast<std::uint32_t>(resize_rc);

    if (R_SUCCEEDED(resize_rc)) {
        g_state.selected_heap_addr = reinterpret_cast<std::uintptr_t>(heap_addr);
        g_state.selected_heap_size = kTargetHeapSize;
        fake_heap_start = static_cast<char*>(heap_addr);
        fake_heap_end = fake_heap_start + kTargetHeapSize;
    } else if (g_state.env_override && g_state.env_addr != 0 && g_state.env_size != 0) {
        auto* fallback = reinterpret_cast<char*>(g_state.env_addr);
        g_state.selected_heap_addr = g_state.env_addr;
        g_state.selected_heap_size = g_state.env_size;
        fake_heap_start = fallback;
        fake_heap_end = fallback + g_state.env_size;
    } else {
        // No usable heap exists. Keep the failure deterministic instead of
        // entering newlib with null heap bounds.
        diagAbortWithResult(resize_rc);
    }

    total = 0;
    used = 0;
    svcGetInfo(&total, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0);
    svcGetInfo(&used, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0);
    g_state.total_after = total;
    g_state.used_after = used;
}

} // extern "C"
