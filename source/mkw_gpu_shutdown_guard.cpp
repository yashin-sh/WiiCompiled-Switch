#include "mkw_gpu_shutdown_guard.hpp"
#include <atomic>

namespace {
std::atomic<bool> shutdown_failed{false};
}

extern "C" bool mkw_switch_gpu_shutdown_failed() noexcept {
    return shutdown_failed.load(std::memory_order_acquire);
}

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
// Keep the pinned driver implementation and its ownership decisions intact.
// These exact opaque-pointer signatures match its two checked bool APIs;
// final linked disassembly verifies Device destruction reaches the wrappers.
struct nvk_device;
struct nvk_queue;
struct nvk_upload_queue;

extern "C" bool __real_nvk_queue_try_destroy(nvk_device*, nvk_queue*);
extern "C" bool __real_nvk_upload_queue_try_finish(nvk_device*, nvk_upload_queue*);

extern "C" bool __wrap_nvk_queue_try_destroy(nvk_device* device, nvk_queue* queue) {
    const bool result = __real_nvk_queue_try_destroy(device, queue);
    if (!result)
        shutdown_failed.store(true, std::memory_order_release);
    return result;
}

extern "C" bool __wrap_nvk_upload_queue_try_finish(nvk_device* device, nvk_upload_queue* queue) {
    const bool result = __real_nvk_upload_queue_try_finish(device, queue);
    if (!result)
        shutdown_failed.store(true, std::memory_order_release);
    return result;
}
#endif
