#include "abi_bridge.h"
#include "switch_sc_hle_traits.hpp"

#include <cstdint>

static_assert(KnownNativeCpuCall<0x801B23A0u>::kAvailable);

// Nintendo-data-free compile/link coverage for PAL SCGetProductArea. The real
// local product provides the SDK region table at 0x8029CEB0; public CI embeds no
// Nintendo table and therefore only validates the native/HLE dispatch seam.
extern "C" __attribute__((used)) void synthetic_sc_get_product_area_hle_probe(
    CpuContext* cpu) {
    if (!cpu) {
        return;
    }

    const std::uint32_t savedR3 = cpu->gpr[3];
    InvokeDirectCpu<0x801B23A0u>(cpu);
    cpu->gpr[3] = savedR3;
}
