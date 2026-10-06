#include "switch_gx_hle_traits.hpp"

// Retain the production boundary without invoking a renderer during startup.
extern "C" void synthetic_gx_pix_mode_sync_hle_probe(CpuContext* cpu) noexcept {
    KnownNativeCpuCall<0x8016EB70u>::Invoke(cpu);
}
