#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"

#include <atomic>
#include <cstdint>

namespace {

constexpr std::uint32_t kViInitializedFlagAddr = 0x80386B38u;
constexpr std::uint32_t kViTimingGuardAddr = 0x80386B44u;
constexpr std::uint32_t kViTvFormatAddr = 0x80386BA8u;
constexpr std::uint32_t kViRenderWidthAddr = 0x80350864u;
constexpr std::uint32_t kViRenderHeightAddr = 0x80350866u;
constexpr std::uint32_t kViXfbWidthAddr = 0x80350872u;
constexpr std::uint32_t kViXfbHeightAddr = 0x8035087Cu;
constexpr std::uint32_t kViRetraceCountAddr = 0x80386BE4u;
constexpr std::uint32_t kViPreRetraceCallbackAddr = 0x80386BB8u;
constexpr std::uint32_t kViPostRetraceCallbackAddr = 0x80386BB4u;
constexpr std::uint32_t kViNextFrameBufferAddr = 0x80386BA0u;
constexpr std::uint32_t kViNextFrameBufferHwAddr = 0x80350890u;

std::atomic<bool> g_viInitialized{false};
std::atomic<bool> g_viPendingBlack{false};
std::atomic<std::uint32_t> g_viPendingNextFrameBuffer{0u};
std::atomic<bool> g_viFlushArmed{false};

void Write8IfMapped(std::uint32_t address, std::uint8_t value) noexcept {
    if (Memory::Contains(address, 1u)) {
        Memory::Write8(address, value);
    }
}

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

std::uint32_t Read32IfMapped(std::uint32_t address) noexcept {
    if (!Memory::Contains(address, 4u)) {
        return 0u;
    }

    try {
        return Memory::Read32(address);
    } catch (...) {
        return 0u;
    }
}

} // namespace

extern "C" void mkw_switch_hle_vi_init(CpuContext* cpu) noexcept {
    if (cpu) {
        cpu->gpr[3] = 0u;
    }

    if (!Memory::IsInitialized()) {
        return;
    }

    bool expected = false;
    if (!g_viInitialized.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        return;
    }

    // Pinned WiiCompiled's VIInit/__VIInit HLE skips Wii VI MMIO and seeds the
    // guest-visible defaults from ViState::EnsureInitializedLocked(). Keep the
    // Switch fast-track headless: this is VI bookkeeping only, not a presenter.
    Write8IfMapped(kViInitializedFlagAddr, 1u);
    Write8IfMapped(kViTimingGuardAddr, 1u);
    Write32IfMapped(kViTvFormatAddr, 0u);
    Write16IfMapped(kViRenderWidthAddr, 640u);
    Write16IfMapped(kViRenderHeightAddr, 480u);
    Write16IfMapped(kViXfbWidthAddr, 640u);
    Write16IfMapped(kViXfbHeightAddr, 480u);
    Write32IfMapped(kViRetraceCountAddr, 0u);
    Write32IfMapped(kViPreRetraceCallbackAddr, 0u);
    Write32IfMapped(kViPostRetraceCallbackAddr, 0u);
    Write32IfMapped(kViNextFrameBufferAddr, 0u);
    Write32IfMapped(kViNextFrameBufferHwAddr, 0u);
}

extern "C" void mkw_switch_hle_vi_set_black(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const bool makeBlack = cpu->gpr[3] != 0u;

    // Pinned VISetBlack writes only the pending VI state. It becomes active
    // after VIFlush/retrace; do not fabricate either event in the headless
    // fast-track. Ensure the same minimal VI initialization has happened first.
    mkw_switch_hle_vi_init(cpu);
    g_viPendingBlack.store(makeBlack, std::memory_order_release);
    cpu->gpr[3] = 0u;
}

extern "C" void mkw_switch_hle_vi_flush(CpuContext* cpu) noexcept {
    // Pinned VIFlush first ensures VI state exists, opportunistically recovers a
    // pending framebuffer from the SDK-visible guest globals when the internal
    // pending slot is still zero, then only arms pending state for a later
    // retrace. It does not itself commit VI state or present a frame.
    mkw_switch_hle_vi_init(cpu);

    if (Memory::IsInitialized() &&
        g_viPendingNextFrameBuffer.load(std::memory_order_acquire) == 0u) {
        std::uint32_t guestNextFrameBuffer = Read32IfMapped(kViNextFrameBufferAddr);
        if (guestNextFrameBuffer == 0u) {
            guestNextFrameBuffer = Read32IfMapped(kViNextFrameBufferHwAddr);
        }
        if (guestNextFrameBuffer != 0u) {
            g_viPendingNextFrameBuffer.store(guestNextFrameBuffer, std::memory_order_release);
        }
    }

    g_viFlushArmed.store(true, std::memory_order_release);

    if (cpu) {
        cpu->gpr[3] = 0u;
    }
}

#endif
