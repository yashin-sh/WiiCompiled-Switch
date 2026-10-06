#include "switch_gx_hle_traits.hpp"

// Link both boundaries without executing renderer operations during startup.
extern "C" void synthetic_gx_viewport_state_hle_probe(CpuContext* cpu) noexcept {
    KnownNativeCpuCall<0x801733E0u>::Invoke(cpu);
    KnownNativeCpuCall<0x80173400u>::Invoke(cpu);
}
