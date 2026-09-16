#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"

#include <cstdint>
#include <cstdlib>

namespace {

constexpr std::uint32_t kOSLoadContextAddress = 0x801A1F58u;
constexpr std::uint32_t kCrOffset = 0x80u;
constexpr std::uint32_t kLrOffset = 0x84u;
constexpr std::uint32_t kCtrOffset = 0x88u;
constexpr std::uint32_t kXerOffset = 0x8Cu;
constexpr std::uint32_t kSrr0Offset = 0x198u;
constexpr std::uint32_t kSrr1Offset = 0x19Cu;
constexpr std::uint32_t kModeFlagsOffset = 0x1A2u;
constexpr std::uint32_t kGqr1Offset = 0x1A8u;
constexpr std::uint16_t kExceptionContextBit = 0x0002u;

[[noreturn]] void AbortLoadContextBoundary(
    const char* kind,
    std::uint32_t target,
    CpuContext* cpu) noexcept {
    mkw_switch_report_unsupported_translated_dispatch(kind, target, cpu);
    std::abort();
}

} // namespace

extern "C" [[noreturn]] void mkw_switch_hle_os_load_context(CpuContext* ctx) noexcept {
    CpuContext* cpu = ctx ? ctx : &GetPersistentCpuContext();
    const std::uint32_t guestContextAddr = cpu->gpr[3];

    if (guestContextAddr == 0u) {
        AbortLoadContextBoundary("OSLOADCONTEXT_NULL_CONTEXT", kOSLoadContextAddress, cpu);
    }

    try {
        // Pinned WiiCompiled validates the rfi target before mutating any other
        // CPU state, then restores the guest OSContext register file verbatim.
        const std::uint32_t srr0 = Memory::Read32(guestContextAddr + kSrr0Offset);
        if (srr0 == 0u) {
            AbortLoadContextBoundary("OSLOADCONTEXT_NULL_SRR0", kOSLoadContextAddress, cpu);
        }

        for (std::uint32_t i = 0u; i < 32u; ++i) {
            cpu->gpr[i] = Memory::Read32(guestContextAddr + i * 4u);
        }

        cpu->cr = Memory::Read32(guestContextAddr + kCrOffset);
        cpu->lr = Memory::Read32(guestContextAddr + kLrOffset);
        cpu->ctr = Memory::Read32(guestContextAddr + kCtrOffset);
        cpu->xer = Memory::Read32(guestContextAddr + kXerOffset);

        // GQR0 is not saved/restored by the pinned HLE and is forced to zero.
        cpu->gqr[0] = 0u;
        for (std::uint32_t i = 1u; i < 8u; ++i) {
            cpu->gqr[i] = Memory::Read32(
                guestContextAddr + kGqr1Offset + (i - 1u) * 4u);
        }

        std::uint16_t modeFlags = Memory::Read16(guestContextAddr + kModeFlagsOffset);
        if ((modeFlags & kExceptionContextBit) != 0u) {
            modeFlags = static_cast<std::uint16_t>(modeFlags & ~kExceptionContextBit);
            Memory::Write16(guestContextAddr + kModeFlagsOffset, modeFlags);
        }

        cpu->srr0 = srr0;
        cpu->srr1 = Memory::Read32(guestContextAddr + kSrr1Offset);
        cpu->pc = srr0;

        // Equivalent to the pin's rfi replacement: dispatch the restored SRR0
        // through the generated dynamic table. A missing target remains a
        // durable INDIRECT_JUMP_MISS blocker instead of fabricating execution.
        InvokeIndirectJump(srr0, cpu);

        // A restored thread entry is not expected to return to the scheduler's
        // previous host call stack. Keep that impossible pin path explicit and
        // durable if it ever occurs on hardware.
        AbortLoadContextBoundary("OSLOADCONTEXT_TARGET_RETURNED", srr0, cpu);
    } catch (...) {
        AbortLoadContextBoundary("OSLOADCONTEXT_GUEST_MEMORY", kOSLoadContextAddress, cpu);
    }
}

#endif
