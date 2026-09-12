#pragma once

#include <cstdint>

struct CpuContext;

namespace mkw::switch_nand_runtime {

std::int32_t OpenSync(std::uint32_t pathPtr,
                      std::uint32_t fileInfoPtr,
                      std::uint32_t mode) noexcept;

void QueueCallback(std::uint32_t callbackPtr,
                   std::int32_t result,
                   std::uint32_t commandBlockPtr) noexcept;

void PumpCallbacks(CpuContext* cpu) noexcept;

} // namespace mkw::switch_nand_runtime

// Called from the translated dispatch seam. Keeping the pump outside the
// diagnostics hook makes deferred guest callbacks part of runtime semantics,
// not a side effect of fast-track logging.
extern "C" void mkw_switch_pump_deferred_guest_callbacks(CpuContext* cpu) noexcept;
