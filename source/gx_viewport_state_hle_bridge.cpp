#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)
#include "abi_bridge.h"
#include <cstdlib>
#include <bit>
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include "gx_internal.h"
#include "runtime_log.h"
#include <algorithm>
namespace {
// Pinned dynamic_aspect.cpp screen-membership checks, slot predicates and
// bounded walks. Preserve repeated reads and per-frame exception/retry order.
constexpr uint32_t kMkwGfxDrawList = 0x809C1830u;
constexpr uint32_t kMkwGfxDrawListLinkOffset = 0x0Au;
constexpr uint32_t kMkwGfxOffscreenList = 0x809C183Cu;
constexpr uint32_t kMkwGfxOffscreenNodeScreenSlot = 0x10u;
constexpr uint32_t kEggScreenVTableOffset = 0x38u;
constexpr uint32_t kEggScreenVTable = 0x802A3F0Cu;
constexpr uint32_t kMkwScreenVTable = 0x808B4C20u;
constexpr uint32_t kEggScreenFlagsOffset = 0x34u;
constexpr uint16_t kEggScreenFlagFramebufferCanvas = 0x0008u;
constexpr uint16_t kEggScreenFlagKeepFrustumScale = 0x0040u;
constexpr uint32_t kMkwGfxNodeScreenSlots[] = {0x10u, 0x28u, 0x2Cu};
bool IsEggScreen(uint32_t address) {
    if (address == 0 || (address & 3u) != 0 || !Memory::Contains(address, 0x40u)) {
        return false;
    }
    const uint32_t vtable = Memory::Read32(address + kEggScreenVTableOffset);
    return vtable == kMkwScreenVTable || vtable == kEggScreenVTable;
}

void KeepFrustumScale(uint32_t screen) {
    if (!IsEggScreen(screen)) {
        return;
    }
    const uint16_t flags = Memory::Read16(screen + kEggScreenFlagsOffset);
    if ((flags & kEggScreenFlagKeepFrustumScale) != 0) {
        return;
    }
    Memory::Write16(screen + kEggScreenFlagsOffset,
                    static_cast<uint16_t>(flags | kEggScreenFlagKeepFrustumScale));
}

void KeepFrustumScaleOnFramebufferCanvasScreen(uint32_t screen) {
    if (!IsEggScreen(screen) ||
        (Memory::Read16(screen + kEggScreenFlagsOffset) &
         kEggScreenFlagFramebufferCanvas) == 0) {
        return;
    }
    KeepFrustumScale(screen);
}

// Walk one gfx-node list. On the on-screen list only screens that have already
// declared themselves framebuffer-canvas (bit 3) may be bypassed. On the
// offscreen list every screen is offscreen by construction, so no predicate is
// needed - and none would work anyway, because bit 3 is only set inside the
// bake itself.
void SweepGfxNodeList(uint32_t list, bool offscreenList) {
    if (!Memory::Contains(list, 0x0Cu)) {
        return;
    }
    const uint32_t linkOffset = Memory::Read16(list + kMkwGfxDrawListLinkOffset);
    // The on-screen walk also reads the menu node's parked-screen slots at
    // +0x28/+0x2C, which sit past the link words on small nodes.
    const uint32_t nodeSpan =
        std::max(linkOffset + 8u, offscreenList ? 0u : kMkwGfxNodeScreenSlots[2] + 4u);
    uint32_t node = Memory::Read32(list);
    for (int guard = 0; node != 0 && guard < 64; ++guard) {
        if (!Memory::Contains(node, nodeSpan)) {
            break;
        }
        if (offscreenList) {
            KeepFrustumScale(Memory::Read32(node + kMkwGfxOffscreenNodeScreenSlot));
        } else {
            for (const uint32_t slot : kMkwGfxNodeScreenSlots) {
                KeepFrustumScaleOnFramebufferCanvasScreen(Memory::Read32(node + slot));
            }
        }
        node = Memory::Read32(node + linkOffset + 4u);
    }
}

// Vertical expansion rides EGG::Screen's global scale, which any screen without bit 6 imports
// at projection-build time. MKW's offscreen passes only set bit 3 (fixed-size framebuffer canvas,
// record 2), so without this bypass they'd inherit the expansion and render vertically squashed.
// Re-arms every scene transition since the offscreen renderer is a per-scene singleton.
void AssertOffscreenScreenBypass() {
    SweepGfxNodeList(kMkwGfxDrawList, /*offscreenList=*/false);
    SweepGfxNodeList(kMkwGfxOffscreenList, /*offscreenList=*/true);
}

void AssertFrameOffscreenScreenBypass() {
    static int lastSweptFrame = -1;
    if (lastSweptFrame == g_gxFrameCount) {
        return;
    }
    lastSweptFrame = g_gxFrameCount;
    AssertOffscreenScreenBypass();
}

} // namespace
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept;
namespace {
[[noreturn]] void Refuse(const char* reason, std::uint32_t target, CpuContext* cpu) {
    mkw_switch_report_unsupported_translated_dispatch(reason, target, cpu);
    std::abort();
}
} // namespace
extern "C" void mkw_switch_hle_gx_get_viewport(CpuContext* cpu) noexcept {
    if (!cpu)
        return;
    mkw_switch_set_fast_track_stage("RMCP01_GX_GET_VIEWPORT");
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    try {
        static int lastBypassFrame = -1;
        if (lastBypassFrame != g_gxFrameCount) {
            lastBypassFrame = g_gxFrameCount;
            AssertFrameOffscreenScreenBypass();
        }
    } catch (...) {
        Refuse("GX_GET_VIEWPORT_BYPASS_EXCEPTION", 0x801733E0u, cpu);
    }
    const auto address = cpu->gpr[3];
    if (!address)
        return;
    // Pinned WriteGuestFloat: skip zero field addresses, catch AccessViolation
    // independently for each write; preserve unsigned wrap and partial writes.
    for (std::uint32_t i = 0; i < 6u; ++i) {
        const auto output = address + i * 4u;
        if (!output)
            continue;
        try {
            Memory::WriteFloat32(output, g_viewportState[i]);
        } catch (const Memory::AccessViolation& error) {
            LogMemoryError(RT_TAG_GX, "GX write", error);
        } catch (...) {
            Refuse("GX_GET_VIEWPORT_WRITE_EXCEPTION", 0x801733E0u, cpu);
        }
    }
#else
    Refuse("GX_GET_VIEWPORT_REQUIRES_RENDERER", 0x801733E0u, cpu);
#endif
}
extern "C" void mkw_switch_hle_gx_set_z_scale_offset(CpuContext* cpu) noexcept {
    if (!cpu)
        return;
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_Z_SCALE_OFFSET");
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    const float scale = static_cast<float>(cpu->fpr[1].d);
    const float offset = static_cast<float>(cpu->fpr[2].d);
    try {
        GXSetZScaleOffset(scale, offset);
    } catch (...) {
        Refuse("GX_Z_SCALE_OFFSET_NATIVE_EXCEPTION", 0x80173400u, cpu);
    }
    // Pinned native-before-memory order, float32 arithmetic and one best-effort
    // mirror transaction. Earlier writes survive a later failed access.
    try {
        const auto gd = Memory::Read32(kGXDataPtrAddr);
        if (gd) {
            constexpr float z24 = 16777215.0f;
            Memory::WriteFloat32(gd + 0x55Cu, z24 * offset);
            Memory::WriteFloat32(gd + 0x560u, 1.0f + z24 * scale);
            Memory::Write32(gd + 0x5FCu, Memory::Read32(gd + 0x5FCu) | 0x10000000u);
        }
    } catch (...) {
    }
#else
    Refuse("GX_Z_SCALE_OFFSET_REQUIRES_RENDERER", 0x80173400u, cpu);
#endif
}
extern "C" void mkw_switch_hle_gx_set_scissor_box_offset(CpuContext* cpu) noexcept {
    if (!cpu)
        return;
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_SCISSOR_BOX_OFFSET");
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    const auto x = std::bit_cast<std::int32_t>(cpu->gpr[3]);
    const auto y = std::bit_cast<std::int32_t>(cpu->gpr[4]);
    // The biased offset occupies a 10-bit half-pixel field. Admit its full
    // nonwrapping range, including odd values with pinned quantization.
    if (x < -342 || x > 1705 || y < -342 || y > 1705)
        Refuse("GX_SCISSOR_BOX_OFFSET_UNPROVEN_RANGE", 0x801734E0u, cpu);
    try {
        GXSetScissorBoxOffset(x, y);
    } catch (...) {
        Refuse("GX_SCISSOR_BOX_OFFSET_NATIVE_EXCEPTION", 0x801734E0u, cpu);
    }
    // Pinned GX__SetScissorBoxOffset: native call first, then best-effort
    // publication. Keep unsigned address wrap and swallowed memory errors.
    try {
        const auto gd = Memory::Read32(kGXDataPtrAddr);
        if (gd)
            Memory::Write16(gd + 2u, 0);
    } catch (...) {
    }
#else
    Refuse("GX_SCISSOR_BOX_OFFSET_REQUIRES_RENDERER", 0x801734E0u, cpu);
#endif
}

#endif
