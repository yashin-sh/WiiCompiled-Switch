#if defined(__SWITCH__)

#include <atomic>

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
}

#endif
