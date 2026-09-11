// Switch-only bridge for helper symbols pulled in once the local fast-track
// links WiiCompiled's generated base_dispatch table. Keep this translation unit
// narrow: importing the complete desktop ppc_helpers/fpu_helpers runtime would
// also pull host UI, logging, and the desktop guest-memory implementation.
#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK)

#include "memory_switch_slice.hpp"
#include "ppc_runtime.h"
#include "isa/ppc_isa_float.h"
#include "isa/ppc_isa_quantized.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace {
constexpr std::uint32_t kFpscrReadOnlySummaryMask = 0x60000000u;
}

extern "C" std::uint32_t OSSystemCall() {
    return 0u;
}

extern "C" void PPC_TrapWord(std::uint32_t trapOptions, std::uint32_t lhs, std::uint32_t rhs) {
    const bool trap =
        ((trapOptions & 0x10u) != 0u && static_cast<std::int32_t>(lhs) < static_cast<std::int32_t>(rhs)) ||
        ((trapOptions & 0x08u) != 0u && static_cast<std::int32_t>(lhs) > static_cast<std::int32_t>(rhs)) ||
        ((trapOptions & 0x04u) != 0u && lhs == rhs) ||
        ((trapOptions & 0x02u) != 0u && lhs < rhs) ||
        ((trapOptions & 0x01u) != 0u && lhs > rhs);
    if (trap) {
        throw std::runtime_error("PowerPC trap condition fired");
    }
}

extern "C" std::uint32_t PPC_LoadWordByteReverse(std::uint32_t addr) {
    return __builtin_bswap32(Memory::Read32(addr));
}

extern "C" void PPC_StoreWordByteReverse(std::uint32_t addr, std::uint32_t value) {
    Memory::Write32(addr, __builtin_bswap32(value));
}

extern "C" void PPC_StoreHalfwordByteReverse(std::uint32_t addr, std::uint32_t value) {
    const auto halfword = static_cast<std::uint16_t>(value & 0xFFFFu);
    Memory::Write16(addr, __builtin_bswap16(halfword));
}

extern "C" std::int32_t memset_zero_32(std::int32_t address) {
    const std::uint32_t addr = static_cast<std::uint32_t>(address);
    if (Memory::IsInitialized() && Memory::Contains(addr, 32u)) {
        std::memset(Memory::GetPointer(addr, 32u), 0, 32u);
    } else {
        for (std::uint32_t offset = 0u; offset < 32u; offset += 4u) {
            Memory::Write32(addr + offset, 0u);
        }
    }
    return address;
}

extern "C" double PPC_PsqL(std::uint32_t addr, std::uint32_t w, std::uint32_t i) {
    CpuContext* cpu = TryGetCpuContext();
    if (!cpu) {
        std::abort();
    }

    const std::uint32_t gqr = cpu->gqr[i & 7u];
    return w == 0u ? PPC_PsqLStateFallback<0u, 0u, false>(gqr, addr)
                   : PPC_PsqLStateFallback<1u, 0u, false>(gqr, addr);
}

extern "C" void PPC_PsqSt(std::uint32_t addr, double value, std::uint32_t w, std::uint32_t i) {
    CpuContext* cpu = TryGetCpuContext();
    if (!cpu) {
        std::abort();
    }

    const std::uint32_t gqr = cpu->gqr[i & 7u];
    if (w == 0u) {
        PPC_PsqStStateFallback<0u, 0u, false>(gqr, addr, value);
    } else {
        PPC_PsqStStateFallback<1u, 0u, false>(gqr, addr, value);
    }
}

extern "C" double PPC_PsSel(double lhs, double control, double rhs) {
    return PPC_PsSelInline(lhs, control, rhs);
}

extern "C" void PPC_Mtfsf(std::uint32_t fieldMask, double source) {
    CpuContext* cpu = TryGetCpuContext();
    if (!cpu) {
        return;
    }

    fieldMask &= 0xFFu;
    if (fieldMask == 0u) {
        return;
    }

    const std::uint32_t incoming = PPC_FprLowWordInline(source);
    if (fieldMask == 0xFFu) {
        cpu->fpscr = (cpu->fpscr & kFpscrReadOnlySummaryMask) |
                     (incoming & ~kFpscrReadOnlySummaryMask);
        MkwApplyHostNiMode(cpu->fpscr);
        return;
    }

    for (std::uint32_t field = 0u; field < 8u; ++field) {
        if ((fieldMask & (1u << (7u - field))) == 0u) {
            continue;
        }

        const std::uint32_t shift = (7u - field) * 4u;
        std::uint32_t mask = 0xFu << shift;
        if (field == 0u) {
            mask &= ~kFpscrReadOnlySummaryMask;
        }
        cpu->fpscr = (cpu->fpscr & ~mask) | (incoming & mask);
    }
    MkwApplyHostNiMode(cpu->fpscr);
}

extern "C" double PPC_Mffs() {
    CpuContext* cpu = TryGetCpuContext();
    return PpcBitCastToDoubleInline(static_cast<std::uint64_t>(cpu ? cpu->fpscr : 0u));
}

extern "C" void PPCMfhid2_HLE_8012e630(CpuContext* ctx) {
    if (!ctx) {
        return;
    }
    if (ctx->hid2 == 0u) {
        ctx->hid2 = 0x10000000u;
    }
    ctx->gpr[3] = ctx->hid2;
}

#endif
