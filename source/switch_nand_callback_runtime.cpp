#include "switch_nand_runtime.hpp"

#include "abi_bridge.h"

namespace mkw::switch_nand_runtime {

void QueueCallback(std::uint32_t, std::int32_t, std::uint32_t) noexcept {}
void PumpCallbacks(CpuContext*) noexcept {}

} // namespace mkw::switch_nand_runtime

extern "C" void mkw_switch_pump_deferred_guest_callbacks(CpuContext* cpu) noexcept {
    mkw::switch_nand_runtime::PumpCallbacks(cpu);
}
