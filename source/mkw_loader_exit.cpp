#if defined(__SWITCH__)

#include <atomic>
#include <switch.h>
#include "mkw_gpu_shutdown_guard.hpp"

extern "C" void __libc_fini_array();

namespace {
std::atomic<bool> finalized{false};
}

// libnx calls this owner-thread hook after newlib's exit procedures, before
// shutting down services and returning to hbloader. The pinned devkit runtime
// runs init_array at startup but does not register fini_array with exit().
// Dispatch it here so static GPU references release their devices and Dawn's
// worker-pool destructors join before the loader reuses the process.
extern "C" void userAppExit() noexcept {
    if (!finalized.exchange(true, std::memory_order_acq_rel))
        __libc_fini_array();
    // Static finalizers can also reach checked native teardown. A void Vulkan
    // destruction API must not hide the driver's quarantined ownership graph.
    while (mkw_switch_gpu_shutdown_failed())
        svcSleepThread(50'000'000);
}

#endif
