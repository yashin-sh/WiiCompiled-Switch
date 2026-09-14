#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <atomic>
#include <cstdint>

namespace {

std::atomic<std::uint16_t> g_dispCopyLeft{0u};
std::atomic<std::uint16_t> g_dispCopyTop{0u};
std::atomic<std::uint16_t> g_dispCopyWidth{0u};
std::atomic<std::uint16_t> g_dispCopyHeight{0u};

} // namespace

extern "C" void GX_HLE_FIFO_Write8(std::uint8_t value);
extern "C" void GX_HLE_FIFO_Write32(std::uint32_t value);

extern "C" void mkw_switch_hle_gx_set_disp_copy_src(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint16_t left = static_cast<std::uint16_t>(cpu->gpr[3]);
    const std::uint16_t top = static_cast<std::uint16_t>(cpu->gpr[4]);
    const std::uint16_t width = static_cast<std::uint16_t>(cpu->gpr[5]);
    const std::uint16_t height = static_cast<std::uint16_t>(cpu->gpr[6]);

    // Mirror pinned Aurora GXSetDispCopySrc host-side state. This is not guest
    // memory and does not create a framebuffer or renderer surface.
    g_dispCopyLeft.store(left, std::memory_order_release);
    g_dispCopyTop.store(top, std::memory_order_release);
    g_dispCopyWidth.store(width, std::memory_order_release);
    g_dispCopyHeight.store(height, std::memory_order_release);

    // Pinned GXSetDispCopySrc emits two BP/RAS writes. Keep the exact FIFO
    // command shape; the Switch fast-track FIFO helpers deliberately remain a
    // sink until M3 provides the real GX -> Switch backend.
    const std::uint32_t topLeft =
        0x49000000u |
        ((static_cast<std::uint32_t>(top) & 0x3FFu) << 10u) |
        (static_cast<std::uint32_t>(left) & 0x3FFu);
    const std::uint32_t size =
        0x4A000000u |
        (((static_cast<std::uint32_t>(height) - 1u) * 0x400u) & 0x000FFC00u) |
        ((static_cast<std::uint32_t>(width) - 1u) & 0x3FFu);

    GX_HLE_FIFO_Write8(0x61u);
    GX_HLE_FIFO_Write32(topLeft);
    GX_HLE_FIFO_Write8(0x61u);
    GX_HLE_FIFO_Write32(size);

    // The pinned override is void: preserve the guest GPRs, including r3.
}

#endif
