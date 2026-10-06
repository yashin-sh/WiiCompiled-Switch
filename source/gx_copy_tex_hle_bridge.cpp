#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"
#include "switch_texture_copy_lifetime.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include "gx_internal.h"

#include <map>
#include <mutex>
#include <vector>

namespace {
constexpr std::uint32_t kCopyBytes = 128u * 128u * 2u;
std::mutex destinationsMutex;
// Address overlap uses physical identity; destruction must use the exact host
// pointer handed to Aurora, since Switch guest aliases have different VAs.
std::map<std::uint32_t, void*> destinations;

std::uint32_t Physical(std::uint32_t address) {
    return CanonicalizeGxMainRamAddress(address);
}

bool ObservedConfiguration() {
    return g_texCopyState.srcLeft == 0 && g_texCopyState.srcTop == 0 &&
           g_texCopyState.srcWidth == 128 && g_texCopyState.srcHeight == 128 &&
           g_texCopyState.dstWidth == 128 && g_texCopyState.dstHeight == 128 &&
           g_texCopyState.dstFormat == 5 && g_texCopyState.dstMipmap == 0;
}

void Report(const char* status, std::uint32_t address, std::uint32_t clear) {
    static const char* previousStatus = nullptr;
    static std::uint32_t previousAddress = 0, previousClear = 0;
    if (previousStatus == status && previousAddress == address && previousClear == clear)
        return;
    if (FILE* out = std::fopen("sdmc:/switch/WiiCompiled-Switch/fast-track-gx-copy-tex.txt", "w")) {
        std::fprintf(out,
                     "status=%s\ndestination=0x%08x\nchecked_bytes=%u\nclear=%u\n"
                     "source=0,0,128,128\ndestination_shape=128,128,5,0\n"
                     "copy_backend=native-aurora\nlarge_copy=GPU-only\n",
                     status, address, kCopyBytes, clear);
        std::fclose(out);
        previousStatus = status;
        previousAddress = address;
        previousClear = clear;
    }
}
} // namespace
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept;

namespace {
[[noreturn]] void Refuse(const char* reason, CpuContext* cpu) {
    mkw_switch_report_unsupported_translated_dispatch(reason, 0x8016FD74u, cpu);
    std::abort();
}
} // namespace

extern "C" void mkw_switch_hle_gx_copy_tex(CpuContext* cpu) noexcept {
    if (!cpu)
        return;
    mkw_switch_set_fast_track_stage("RMCP01_GX_COPY_TEX");
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    const auto address = cpu->gpr[3], clear = cpu->gpr[4];
    // First hardware tuple only. Do not silently broaden format, dimensions,
    // mipmaps, clear behavior or GPU resource identity to unobserved aliases.
    if (!ObservedConfiguration() || clear != 1 || address < 0x90000000u || address >= 0x94000000u)
        Refuse("GX_COPY_TEX_UNPROVEN_ARGS", cpu);
    if (static_cast<std::uint64_t>(address) + kCopyBytes > 0x94000000ull)
        Refuse("GX_COPY_TEX_DESTINATION_RANGE", cpu);
    auto* destination = Memory::GetPointer(address, kCopyBytes);
    if (!destination)
        Refuse("GX_COPY_TEX_DESTINATION_RANGE", cpu);

    try {
        EnsureAuroraFrameActive();
        if (!mkw_switch_renderer_has_active_frame())
            Refuse("GX_COPY_TEX_FRAME_INACTIVE", cpu);
        GXDrawDone();
        // A draw-done callback must not change the size behind our preflight.
        if (!ObservedConfiguration())
            Refuse("GX_COPY_TEX_CONFIGURATION_CHANGED", cpu);
        GXSetTexCopySrc(0, 0, 128, 128);
        GXCopyTex(destination, GX_TRUE);
        {
            std::lock_guard lock(destinationsMutex);
            destinations[Physical(address)] = destination;
        }
        GXSetTexCopySrc(0, 0, 128, 128);
        Report("copy-pass", address, clear);
    } catch (...) {
        // Never publish a successful copy after a native/resource exception.
        Refuse("GX_COPY_TEX_NATIVE_EXCEPTION", cpu);
    }
#else
    Refuse("GX_COPY_TEX_REQUIRES_RENDERER", cpu);
#endif
}

extern "C" void mkw_switch_gx_invalidate_copy_destinations(std::uint32_t address, std::uint32_t bytes) noexcept {
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    if (!bytes)
        return;
    const std::uint64_t start = Physical(address), end = start + bytes;
    std::vector<void*> retired;
    {
        std::lock_guard lock(destinationsMutex);
        // Copies have the same bounded extent. Erase overlap, including a
        // flush beginning inside a copy or covering several destinations.
        const auto earliest = start > kCopyBytes ? start - kCopyBytes : 0;
        for (auto it = destinations.lower_bound(static_cast<std::uint32_t>(earliest));
             it != destinations.end() && it->first < end;) {
            if (start >= static_cast<std::uint64_t>(it->first) + kCopyBytes) {
                ++it;
                continue;
            }
            retired.push_back(it->second);
            it = destinations.erase(it);
        }
    }
    // Aurora emits FIFO-ordered destroy commands. Avoid calling it while the
    // bookkeeping mutex is held: drain/callback paths can notify writes too.
    for (auto* destination : retired)
        GXDestroyCopyTex(destination);
#else
    (void)address;
    (void)bytes;
#endif
}

extern "C" void mkw_switch_gx_forget_copy_destinations() noexcept {
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    std::lock_guard lock(destinationsMutex);
    destinations.clear();
#endif
}

extern "C" void mkw_switch_hle_dc_range(CpuContext* cpu) noexcept {
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    if (!cpu || !cpu->gpr[4])
        return;
    // Pinned os_cache.cpp: 32-byte alignment, u64 overflow refusal, complete
    // mapped-range validation, then a best-effort GX write notification.
    const std::uint32_t start = cpu->gpr[3] & ~31u;
    const std::uint64_t end = (static_cast<std::uint64_t>(cpu->gpr[3]) + cpu->gpr[4] + 31u) & ~std::uint64_t{31u};
    if (end <= start || end >= 0x100000000ull)
        return;
    const auto bytes = static_cast<std::uint32_t>(end - start);
    if (Memory::Contains(start, bytes))
        mkw_switch_gx_invalidate_copy_destinations(start, bytes);
#else
    (void)cpu;
#endif
}

#endif
