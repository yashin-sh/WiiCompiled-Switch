// Narrow Switch bridge for the generic PPC helpers that translated WiiCompiled
// shards can call before the full desktop runtime is available.
//
// Do not include runtime/src/ppc_helpers.cpp here: that translation unit pulls
// the desktop memory/logging stack, which conflicts with the Switch guest-memory
// slice. The implementations below intentionally mirror the pinned WiiCompiled
// semantics for this CPU-only subset (time base + SPR access) without RT_LOG.
#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK)

#include "ppc_runtime.h"
#include "isa/ppc_isa_int.h"
#include "timebase_contract.h"

#include <array>
#include <chrono>
#include <cstdint>

namespace {

constexpr std::uint32_t kBroadwayPvr = 0x00087200u;
std::array<std::uint32_t, 2048> g_sprShadow{};
const auto g_timeBaseStart = std::chrono::steady_clock::now();

std::uint64_t GetTimeBase() noexcept {
    const auto elapsed = std::chrono::steady_clock::now() - g_timeBaseStart;
    const auto nanoseconds =
        std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
    return TimeBaseContract::NanosecondsToTicks(
        static_cast<std::uint64_t>(nanoseconds));
}

} // namespace

extern "C" std::uint32_t PPC_Mftb() {
    return static_cast<std::uint32_t>(GetTimeBase());
}

extern "C" std::uint32_t PPC_Mftbu() {
    return static_cast<std::uint32_t>(GetTimeBase() >> 32);
}

extern "C" std::uint32_t PPC_ReadSpr(std::uint32_t spr) {
    CpuContext* cpu = TryGetCpuContext();

    const auto shadowValue = [&]() -> std::uint32_t {
        return spr < g_sprShadow.size() ? g_sprShadow[spr] : 0u;
    };

    if (spr >= 912u && spr <= 919u) {
        return cpu ? cpu->gqr[spr - 912u] : 0u;
    }

    switch (spr) {
        case 8: return cpu ? cpu->lr : 0u;
        case 9: return cpu ? cpu->ctr : 0u;
        case 1: return cpu ? cpu->xer : 0u;
        case 22: return shadowValue();
        case 26: return cpu ? cpu->srr0 : 0u;
        case 27: return cpu ? cpu->srr1 : 0u;
        case 268: return PPC_Mftb();
        case 269: return PPC_Mftbu();
        case 287: return kBroadwayPvr;
        case 920: return cpu ? cpu->hid2 : 0u;
        case 1008: return cpu ? cpu->hid0 : 0u;
        case 1009: return cpu ? cpu->hid1 : 0u;
        case 1011:
        case 1017:
        case 952:
        case 953:
        case 954:
        case 956:
        case 957:
        case 958:
        default:
            return shadowValue();
    }
}

extern "C" void PPC_WriteSpr(std::uint32_t spr, std::uint32_t value) {
    CpuContext* cpu = TryGetCpuContext();

    if (spr >= 912u && spr <= 919u) {
        if (cpu) {
            cpu->gqr[spr - 912u] = value;
        }
        return;
    }

    switch (spr) {
        case 8: if (cpu) cpu->lr = value; return;
        case 9: if (cpu) cpu->ctr = value; return;
        case 1: if (cpu) cpu->xer = value; return;
        case 22: break;
        case 26: if (cpu) cpu->srr0 = value; return;
        case 27: if (cpu) cpu->srr1 = value; return;
        case 920: if (cpu) cpu->hid2 = value; return;
        case 1008: if (cpu) cpu->hid0 = value; return;
        case 1009: if (cpu) cpu->hid1 = value; return;
        case 1011:
        case 1017:
        case 952:
        case 953:
        case 954:
        case 956:
        case 957:
        case 958:
        default:
            break;
    }

    if (spr < g_sprShadow.size()) {
        g_sprShadow[spr] = value;
    }
}

#endif
