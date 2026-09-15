#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"

#include <atomic>
#include <cstdint>

namespace {

constexpr std::uint32_t kRenderModeBytes = 0x39u;
constexpr std::uint32_t kViTvFormatAddr = 0x80386BA8u;
constexpr std::uint32_t kViRenderWidthAddr = 0x80350864u;
constexpr std::uint32_t kViRenderHeightAddr = 0x80350866u;
constexpr std::uint32_t kViXfbWidthAddr = 0x80350872u;
constexpr std::uint32_t kViXfbHeightAddr = 0x8035087Cu;

std::atomic<std::uint32_t> g_pendingTvFormat{0u};
std::atomic<std::uint32_t> g_pendingRenderWidth{640u};
std::atomic<std::uint32_t> g_pendingRenderHeight{480u};
std::atomic<std::uint32_t> g_pendingViXOrigin{0u};
std::atomic<std::uint32_t> g_pendingViYOrigin{0u};
std::atomic<std::uint32_t> g_pendingXfbWidth{640u};
std::atomic<std::uint32_t> g_pendingXfbHeight{480u};

void Write16IfMapped(std::uint32_t address, std::uint16_t value) noexcept {
    if (Memory::Contains(address, 2u)) {
        Memory::Write16(address, value);
    }
}

void Write32IfMapped(std::uint32_t address, std::uint32_t value) noexcept {
    if (Memory::Contains(address, 4u)) {
        Memory::Write32(address, value);
    }
}

} // namespace

extern "C" void mkw_switch_hle_vi_init(CpuContext* cpu) noexcept;

extern "C" void mkw_switch_hle_vi_configure(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t ptr = cpu->gpr[3];
    if (!Memory::IsInitialized() || ptr == 0u || !Memory::Contains(ptr, kRenderModeBytes)) {
        cpu->gpr[3] = 0u;
        return;
    }

    try {
        const std::uint32_t tvMode = Memory::Read32(ptr + 0x00u);
        const std::uint16_t fbWidth = Memory::Read16(ptr + 0x04u);
        const std::uint16_t efbHeight = Memory::Read16(ptr + 0x06u);
        const std::uint16_t xfbHeight = Memory::Read16(ptr + 0x08u);
        const std::uint16_t viXOrigin = Memory::Read16(ptr + 0x0Au);
        const std::uint16_t viYOrigin = Memory::Read16(ptr + 0x0Cu);
        const std::uint16_t viWidth = Memory::Read16(ptr + 0x0Eu);
        const std::uint16_t viHeight = Memory::Read16(ptr + 0x10u);

        mkw_switch_hle_vi_init(cpu);

        g_pendingTvFormat.store((tvMode >> 2u) & 0x7u, std::memory_order_release);
        g_pendingRenderWidth.store(viWidth != 0u ? viWidth : fbWidth, std::memory_order_release);
        g_pendingRenderHeight.store(viHeight != 0u ? viHeight : xfbHeight,
                                    std::memory_order_release);
        g_pendingViXOrigin.store(viXOrigin, std::memory_order_release);
        g_pendingViYOrigin.store(viYOrigin, std::memory_order_release);
        g_pendingXfbWidth.store(fbWidth, std::memory_order_release);
        g_pendingXfbHeight.store(xfbHeight != 0u ? xfbHeight : efbHeight,
                                 std::memory_order_release);
    } catch (...) {
        // Match the pinned HLE's rejected-access path: leave pending state
        // unchanged and still return zero to the guest caller.
    }

    // The pinned host presenter call is intentionally omitted in headless fast-track.
    cpu->gpr[3] = 0u;
}

// Commit the VIConfigure pending geometry at a retrace boundary. The pinned
// AdvanceRetrace path writes these active values back to the guest-visible VI
// globals after a VIFlush arm. Keep that bookkeeping separate from rendering.
extern "C" std::uint32_t mkw_switch_hle_vi_commit_pending_config() noexcept {
    const std::uint32_t tvFormat = g_pendingTvFormat.load(std::memory_order_acquire);
    const std::uint32_t renderWidth = g_pendingRenderWidth.load(std::memory_order_acquire);
    const std::uint32_t renderHeight = g_pendingRenderHeight.load(std::memory_order_acquire);
    const std::uint32_t xfbWidth = g_pendingXfbWidth.load(std::memory_order_acquire);
    const std::uint32_t xfbHeight = g_pendingXfbHeight.load(std::memory_order_acquire);

    if (Memory::IsInitialized()) {
        Write32IfMapped(kViTvFormatAddr, tvFormat);
        Write16IfMapped(kViRenderWidthAddr, static_cast<std::uint16_t>(renderWidth));
        Write16IfMapped(kViRenderHeightAddr, static_cast<std::uint16_t>(renderHeight));
        Write16IfMapped(kViXfbWidthAddr, static_cast<std::uint16_t>(xfbWidth));
        Write16IfMapped(kViXfbHeightAddr, static_cast<std::uint16_t>(xfbHeight));
    }

    return tvFormat;
}

#endif
