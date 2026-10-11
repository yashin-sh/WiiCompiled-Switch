#pragma once

// Sticky evidence from the pinned NVK checked teardown paths. A quarantined
// native device must remain in its current process instead of returning a
// reusable address space to hbloader.
extern "C" bool mkw_switch_gpu_shutdown_failed() noexcept;
