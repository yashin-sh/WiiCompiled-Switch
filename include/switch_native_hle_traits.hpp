#pragma once

#include "abi_bridge.h"
#include "memory.h"

#include <atomic>
#include <cstdint>
#include <cstring>

// Switch-side native/HLE extensions for translated targets that the pinned
// WiiCompiled runtime deliberately excludes from recompilation. Keeping these
// traits outside abi_bridge.h lets blocker-driven HLE coverage grow without
// turning the core ABI seam into a monolithic address catalogue.

// REGISTER_NATIVE_FUNCTION_AS keeps the original guest body translated while
// making a native wrapper the runtime winner. The pinned OSInitAlarm wrapper
// simply forwards to that translated body, so preserve that exact boundary.
extern "C" void func_801A961C(CpuContext* ctx);

// Renderer-side notification seam used by WiiCompiled cache/DMA HLE. The
// current Switch fast-track implementation is an explicit sink until a real GX
// backend replaces the FIFO sink.
extern "C" void mkw_switch_gx_notify_guest_ram_dma_write(
    std::uint32_t address,
    std::uint32_t sizeBytes) noexcept;

template <>
struct KnownNativeCpuCall<0x801A961Cu> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        if (cpu) {
            func_801A961C(cpu);
        }
    }
};

// OSClearContext (PAL 0x801A2098). Match pinned WiiCompiled's guest OSContext
// bookkeeping: clear the saved-state/mode halfwords and drop the global
// exception-context pointer when it references the context being cleared.
template <>
struct KnownNativeCpuCall<0x801A2098u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        const std::uint32_t contextAddr = cpu->gpr[3];
        if (contextAddr == 0u) {
            return;
        }

        try {
            Memory::Write16(contextAddr + 0x1A0u, 0u);
            Memory::Write16(contextAddr + 0x1A2u, 0u);

            constexpr std::uint32_t kOSExceptionContextAddr = 0x800000D8u;
            if (Memory::Read32(kOSExceptionContextAddr) == contextAddr) {
                Memory::Write32(kOSExceptionContextAddr, 0u);
            }
        } catch (...) {
            // Pinned WiiCompiled treats invalid guest bookkeeping as a logged
            // memory fault and returns; the Switch fast-track has no logger here.
        }
    }
};

// Query the shared Switch interrupt state without introducing a second mirror.
// DisableInterrupts publishes the previous value; restoring it immediately
// leaves both the host atomic and the current guest-context bit unchanged.
inline bool MkwSwitchQueryInterruptsEnabled(CpuContext* cpu) noexcept {
    if (!cpu) {
        return false;
    }

    const std::uint32_t savedR3 = cpu->gpr[3];
    mkw_switch_hle_os_disable_interrupts(cpu);
    const bool enabled = cpu->gpr[3] != 0u;
    cpu->gpr[3] = enabled ? 1u : 0u;
    mkw_switch_hle_os_restore_interrupts(cpu);
    cpu->gpr[3] = savedR3;
    return enabled;
}

// OSSetCurrentContext (PAL 0x801A1E70). Preserve pinned WiiCompiled semantics:
// publish the physical/current context globals, synchronize the exception-state
// flag, and mirror the current interrupt-enabled state into OSContext mode bit 1.
template <>
struct KnownNativeCpuCall<0x801A1E70u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu || !Memory::IsInitialized()) {
            return;
        }

        constexpr std::uint32_t kOSPhysicalContextAddr = 0x800000C0u;
        constexpr std::uint32_t kOSCurrentContextAddr = 0x800000D4u;
        constexpr std::uint32_t kOSExceptionContextAddr = 0x800000D8u;
        constexpr std::uint32_t kStateFlagsOffset = 0x19Cu;
        constexpr std::uint32_t kModeFlagsOffset = 0x1A2u;
        constexpr std::uint16_t kInterruptsEnabledBit = 0x0002u;

        const std::uint32_t contextAddr = cpu->gpr[3];
        if (!Memory::Contains(kOSPhysicalContextAddr, 4u) ||
            !Memory::Contains(kOSCurrentContextAddr, 4u)) {
            return;
        }

        if (contextAddr == 0u) {
            Memory::Write32(kOSPhysicalContextAddr, 0u);
            Memory::Write32(kOSCurrentContextAddr, 0u);
            return;
        }

        Memory::Write32(kOSPhysicalContextAddr, contextAddr & 0x3FFFFFFFu);
        Memory::Write32(kOSCurrentContextAddr, contextAddr);

        const std::uint32_t stateFlagsAddr = contextAddr + kStateFlagsOffset;
        const std::uint32_t modeFlagsAddr = contextAddr + kModeFlagsOffset;
        if (!Memory::Contains(kOSExceptionContextAddr, 4u) ||
            !Memory::Contains(stateFlagsAddr, 4u) ||
            !Memory::Contains(modeFlagsAddr, 2u)) {
            return;
        }

        const std::uint32_t exceptionContext = Memory::Read32(kOSExceptionContextAddr);
        std::uint32_t stateFlags = Memory::Read32(stateFlagsAddr);
        if (exceptionContext == contextAddr) {
            stateFlags |= 0x2000u;
        } else {
            stateFlags &= ~0x2000u;
            std::atomic_thread_fence(std::memory_order_seq_cst);
        }
        Memory::Write32(stateFlagsAddr, stateFlags);

        std::uint16_t modeFlags = Memory::Read16(modeFlagsAddr);
        if (MkwSwitchQueryInterruptsEnabled(cpu)) {
            modeFlags |= kInterruptsEnabledBit;
        } else {
            modeFlags &= static_cast<std::uint16_t>(~kInterruptsEnabledBit);
        }
        Memory::Write16(modeFlagsAddr, modeFlags);
    }
};

// OS____InitMemoryProtection (PAL 0x801A7DFC). The pinned WiiCompiled runtime
// deliberately skips Broadway MMU/Hollywood MMIO setup on the host and returns
// 0. Preserve that exact guest-visible result on Horizon.
template <>
struct KnownNativeCpuCall<0x801A7DFCu> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        if (cpu) {
            cpu->gpr[3] = 0u;
        }
    }
};

// RVL__EXIImm / EXIImm (PAL 0x80167F68). Pinned WiiCompiled reports immediate
// transfers as successful. Read/RW transfers also clear the guest destination
// bytes before returning so callers never consume stale EXI data.
template <>
struct KnownNativeCpuCall<0x80167F68u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        const std::uint32_t buffer = cpu->gpr[4];
        const std::uint32_t length = cpu->gpr[5];
        const std::uint32_t type = cpu->gpr[6];

        if (type == 0u || type == 2u) {
            try {
                for (std::uint32_t i = 0; i < length; ++i) {
                    Memory::Write8(buffer + i, 0u);
                }
            } catch (...) {
                // Match the pinned host HLE: an unmapped guest buffer is a
                // best-effort write failure, not an EXI transaction failure.
            }
        }

        cpu->gpr[3] = 1u;
    }
};

// The pinned host HLE treats DMA completion, sync, and unlock as immediate
// success while no physical EXI device is emulated on the host.
template <>
struct KnownNativeCpuCall<0x80168288u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        if (cpu) {
            cpu->gpr[3] = 1u;
        }
    }
};

template <>
struct KnownNativeCpuCall<0x80168380u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        if (cpu) {
            cpu->gpr[3] = 1u;
        }
    }
};

template <>
struct KnownNativeCpuCall<0x80169260u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        if (cpu) {
            cpu->gpr[3] = 1u;
        }
    }
};

// OSReport (PAL 0x801A25D0) is a native override in pinned WiiCompiled. Its
// implementation only formats/prints guest arguments on the host and leaves
// CpuContext and guest memory unchanged. The Switch fast-track intentionally
// sinks that host-only logging side effect while preserving exact guest-visible
// state so startup can continue without importing the desktop printf runtime.
template <>
struct KnownNativeCpuCall<0x801A25D0u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext*) noexcept {}
};

// OSGetConsoleType (PAL 0x8019F33C). Match the pinned native override exactly:
// read the guest physical MEM2 size and expose retail Wii vs NDEV/expanded MEM2
// through r3. MKW uses the NDEV value to enable its extra-memory heap path.
template <>
struct KnownNativeCpuCall<0x8019F33Cu> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        constexpr std::uint32_t kPhysicalMem2SizeAddr = 0x80003118u;
        constexpr std::uint32_t kRetailMem2Size = 64u * 1024u * 1024u;
        const std::uint32_t physicalMem2Size = Memory::Read32(kPhysicalMem2SizeAddr);
        cpu->gpr[3] = physicalMem2Size == kRetailMem2Size ? 0x00000012u : 0x10000012u;
    }
};

// OSGetResetCode (PAL 0x801A8A50). The Wii SDK implementation reads a
// Hollywood MMIO reset register that does not exist on Horizon. Pinned
// WiiCompiled replaces the whole entry point and always reports Cold Boot (0).
template <>
struct KnownNativeCpuCall<0x801A8A50u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        if (cpu) {
            cpu->gpr[3] = 0u;
        }
    }
};

// DCZeroRange (PAL 0x801A16E4). Match the pinned WiiCompiled HLE: align the
// guest range to 32-byte cache lines, zero the full aligned range, and notify
// the renderer-side DMA-write seam. Invalid guest ranges are best-effort and do
// not change guest registers or abort startup.
template <>
struct KnownNativeCpuCall<0x801A16E4u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        constexpr std::uint32_t kCacheLineSize = 32u;
        const std::uint32_t address = cpu->gpr[3];
        const std::uint32_t length = cpu->gpr[4];
        if (length == 0u) {
            return;
        }

        const std::uint32_t alignedAddress = address & ~(kCacheLineSize - 1u);
        const std::uint32_t alignedLength =
            ((address - alignedAddress) + length + (kCacheLineSize - 1u)) &
            ~(kCacheLineSize - 1u);
        if (alignedLength == 0u) {
            return;
        }

        // Upstream Memory::GetPointer throws AccessViolation for an unmapped
        // guest range; the Switch memory slice deliberately returns nullptr.
        // Preserve the upstream best-effort semantics by checking that result
        // before entering libc, otherwise memset(nullptr, ...) becomes a host
        // Data Abort instead of a skipped invalid guest cache operation.
        auto* destination = Memory::GetPointer(alignedAddress, alignedLength);
        if (!destination) {
            return;
        }

        std::memset(destination, 0, alignedLength);
        mkw_switch_gx_notify_guest_ram_dma_write(alignedAddress, alignedLength);
    }
};
