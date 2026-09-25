#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include "gx_internal.h"

#include <dolphin/gx.h>

#include <map>
#include <memory>
#include <mutex>
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

namespace {

constexpr std::uint32_t kGxInitTexObjAddress = 0x801707F8u;
constexpr std::uint32_t kGxLoadTexObjAddress = 0x80170F2Cu;
constexpr std::uint32_t kGuestTexObjSize = 0x20u;
constexpr std::uint32_t kGxDataPtrAddr = 0x803886C8u;

constexpr std::uint32_t kObservedLoadObj = 0x901136B4u;
constexpr std::uint32_t kObservedLoadTid = 0u;
constexpr std::uint32_t kObservedWord0 = 0x00000090u;
constexpr std::uint32_t kObservedWord1 = 0x00000000u;
constexpr std::uint32_t kObservedWord2 = 0x00471F3Fu;
constexpr std::uint32_t kObservedWord3 = 0x0007881Fu;
constexpr std::uint32_t kObservedWord4 = 0x00000000u;
constexpr std::uint32_t kObservedWord5 = 0x00000004u;
constexpr std::uint32_t kObservedWord6 = 0x00000000u;
constexpr std::uint32_t kObservedWord7 = 0x5CA00202u;
constexpr std::uint32_t kObservedData = 0x00F103E0u;
constexpr std::uint16_t kObservedWidth = 832u;
constexpr std::uint16_t kObservedHeight = 456u;
constexpr std::uint32_t kObservedFormat = 4u;
constexpr std::uint32_t kObservedTextureSize = 0x00172800u;

std::uint32_t CanonicalizeGuestMainRamAddress(std::uint32_t addr) noexcept {
    if (addr < 0x01800000u) {
        return addr;
    }
    if (addr >= Memory::kMem2PhysicalBase && addr < Memory::kMem2PhysicalEnd) {
        return addr;
    }
    if (addr >= 0x80000000u && addr < 0x81800000u) {
        return addr - 0x80000000u;
    }
    if (addr >= Memory::kMem2CachedBase && addr < Memory::kMem2CachedEnd) {
        return addr - 0x80000000u;
    }
    if (addr >= 0xC0000000u && addr < 0xC1800000u) {
        return addr - 0xC0000000u;
    }
    if (addr >= Memory::kMem2UncachedBase && addr < Memory::kMem2UncachedEnd) {
        return addr - 0xC0000000u;
    }
    return addr;
}

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
constexpr const char* kStatusPath =
    "sdmc:/switch/WiiCompiled-Switch/fast-track-gx-init-tex-obj.txt";
constexpr const char* kLoadStatusPath =
    "sdmc:/switch/WiiCompiled-Switch/fast-track-gx-load-tex-obj.txt";

std::mutex gTexObjMutex;
std::map<std::uint32_t, std::unique_ptr<GXTexObj>> gHostTexObjs;

GXTexObj* GetOrCreateHostTexObj(std::uint32_t guestAddr) {
    auto& entry = gHostTexObjs[guestAddr];
    if (!entry) {
        entry = std::make_unique<GXTexObj>();
    }
    return entry.get();
}

void WriteStatus(
    const char* status,
    std::uint32_t obj,
    std::uint32_t data,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t format,
    std::uint32_t wrapS,
    std::uint32_t wrapT,
    std::uint32_t mipmap) noexcept {
    FILE* out = std::fopen(kStatusPath, "w");
    if (!out) {
        return;
    }

    std::fprintf(
        out,
        "status=%s\n"
        "obj=0x%08x\n"
        "data=0x%08x\n"
        "width=%u\n"
        "height=%u\n"
        "format=%u\n"
        "wrap_s=%u\n"
        "wrap_t=%u\n"
        "mipmap=%u\n",
        status ? status : "<null>",
        obj,
        data,
        width,
        height,
        format,
        wrapS,
        wrapT,
        mipmap);

    if (Memory::Contains(obj, kGuestTexObjSize)) {
        std::fprintf(
            out,
            "guest_word0=0x%08x\n"
            "guest_word1=0x%08x\n"
            "guest_word2=0x%08x\n"
            "guest_word3=0x%08x\n"
            "guest_format=0x%08x\n"
            "guest_blocks=%u\n"
            "guest_block_type=%u\n"
            "guest_flags=%u\n",
            Memory::Read32(obj + 0x00u),
            Memory::Read32(obj + 0x04u),
            Memory::Read32(obj + 0x08u),
            Memory::Read32(obj + 0x0Cu),
            Memory::Read32(obj + 0x14u),
            static_cast<unsigned>(Memory::Read16(obj + 0x1Cu)),
            static_cast<unsigned>(Memory::Read8(obj + 0x1Eu)),
            static_cast<unsigned>(Memory::Read8(obj + 0x1Fu)));
    }

    std::fclose(out);
}

void WriteLoadStatus(
    const char* status,
    std::uint32_t obj,
    std::uint32_t tid,
    std::uint32_t data,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t format,
    std::uint32_t wrapS,
    std::uint32_t wrapT,
    std::uint32_t mipmap,
    std::uint32_t size) noexcept {
    FILE* out = std::fopen(kLoadStatusPath, "w");
    if (!out) {
        return;
    }

    std::fprintf(
        out,
        "status=%s\n"
        "obj=0x%08x\n"
        "tid=%u\n"
        "data=0x%08x\n"
        "width=%u\n"
        "height=%u\n"
        "format=%u\n"
        "wrap_s=%u\n"
        "wrap_t=%u\n"
        "mipmap=%u\n"
        "size=0x%08x\n",
        status ? status : "<null>",
        obj,
        tid,
        data,
        width,
        height,
        format,
        wrapS,
        wrapT,
        mipmap,
        size);
    std::fclose(out);
}
#else
void WriteStatus(
    const char*,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t) noexcept {}

void WriteLoadStatus(
    const char*,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t) noexcept {}
#endif

[[noreturn]] void AbortBoundary(
    const char* status,
    CpuContext* cpu,
    std::uint32_t obj,
    std::uint32_t data,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t format,
    std::uint32_t wrapS,
    std::uint32_t wrapT,
    std::uint32_t mipmap) noexcept {
    WriteStatus(
        status,
        obj,
        data,
        width,
        height,
        format,
        wrapS,
        wrapT,
        mipmap);
    mkw_switch_report_unsupported_translated_dispatch(
        status, kGxInitTexObjAddress, cpu);
    std::abort();
}

[[noreturn]] void AbortLoadBoundary(
    const char* status,
    CpuContext* cpu,
    std::uint32_t obj,
    std::uint32_t tid,
    std::uint32_t data,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t format,
    std::uint32_t wrapS,
    std::uint32_t wrapT,
    std::uint32_t mipmap,
    std::uint32_t size) noexcept {
    WriteLoadStatus(
        status,
        obj,
        tid,
        data,
        width,
        height,
        format,
        wrapS,
        wrapT,
        mipmap,
        size);
    mkw_switch_report_unsupported_translated_dispatch(
        status, kGxLoadTexObjAddress, cpu);
    std::abort();
}

void PublishTextureLoadGuestState() noexcept {
    if (!Memory::IsInitialized() || !Memory::Contains(kGxDataPtrAddr, 4u)) {
        return;
    }
    try {
        const std::uint32_t gxData = Memory::Read32(kGxDataPtrAddr);
        if (gxData == 0u ||
            !Memory::Contains(gxData + 0x5FCu, 4u) ||
            !Memory::Contains(gxData + 2u, 2u)) {
            return;
        }
        Memory::Write32(
            gxData + 0x5FCu,
            Memory::Read32(gxData + 0x5FCu) | 1u);
        Memory::Write16(gxData + 2u, 0u);
    } catch (...) {
    }
}

void WriteGuestTexObj(
    CpuContext* cpu,
    std::uint32_t obj,
    std::uint32_t data,
    std::uint16_t width,
    std::uint16_t height,
    std::uint32_t format,
    std::uint32_t wrapS,
    std::uint32_t wrapT,
    bool mipmap) {
    if (!Memory::Contains(obj, kGuestTexObjSize)) {
        AbortBoundary(
            "GX_INIT_TEX_OBJ_GUEST_UNMAPPED",
            cpu,
            obj,
            data,
            width,
            height,
            format,
            wrapS,
            wrapT,
            mipmap ? 1u : 0u);
    }

    const std::uint32_t canonicalData =
        CanonicalizeGuestMainRamAddress(data);

    for (std::uint32_t i = 0u; i < 8u; ++i) {
        Memory::Write32(obj + i * 4u, 0u);
    }

    std::uint32_t word0 =
        (wrapS & 0x3u) | ((wrapT & 0x3u) << 2u) | 0x10u;
    std::uint32_t word1 = 0u;

    if (!mipmap) {
        word0 =
            (word0 & 0xFFFFFF10u) |
            (wrapS & 0x3u) |
            ((wrapT & 0x3u) << 2u) |
            0x90u;
    } else {
        word0 =
            (word0 & 0xFFFFFF10u) |
            (wrapS & 0x3u) |
            ((wrapT & 0x3u) << 2u) |
            (((format - 8u) < 3u) ? 0xB0u : 0xD0u);

        const std::uint32_t maxDim =
            std::max<std::uint32_t>(width, height);
        const std::uint32_t maxLod =
            maxDim > 0u
                ? 31u - static_cast<std::uint32_t>(__builtin_clz(maxDim))
                : 0u;
        word1 |=
            std::min<std::uint32_t>(maxLod * 16u, 0xFFu) << 8u;
    }

    std::uint32_t blockShiftX = 2u;
    std::uint32_t blockShiftY = 2u;
    std::uint8_t blockType = 2u;
    switch (format & 0xFu) {
    case 0u:
    case 8u:
        blockShiftX = 3u;
        blockShiftY = 3u;
        blockType = 1u;
        break;
    case 1u:
    case 2u:
    case 9u:
        blockShiftX = 3u;
        blockShiftY = 2u;
        blockType = 2u;
        break;
    case 3u:
    case 4u:
    case 5u:
    case 10u:
        blockShiftX = 2u;
        blockShiftY = 2u;
        blockType = 2u;
        break;
    case 6u:
        blockShiftX = 2u;
        blockShiftY = 2u;
        blockType = 3u;
        break;
    case 14u:
        blockShiftX = 3u;
        blockShiftY = 3u;
        blockType = 0u;
        break;
    default:
        break;
    }

    const std::uint32_t word2 =
        ((static_cast<std::uint32_t>(width) - 1u) & 0x3FFu) |
        (((static_cast<std::uint32_t>(height) - 1u) & 0x3FFu) << 10u) |
        ((format & 0xFu) << 20u);
    const std::uint32_t word3 =
        (canonicalData >> 5u) & 0x00FFFFFFu;

    const std::uint32_t blocksX =
        (static_cast<std::uint32_t>(width) +
         ((1u << blockShiftX) - 1u)) >>
        blockShiftX;
    const std::uint32_t blocksY =
        (static_cast<std::uint32_t>(height) +
         ((1u << blockShiftY) - 1u)) >>
        blockShiftY;
    const std::uint16_t blockCount =
        static_cast<std::uint16_t>((blocksX * blocksY) & 0x7FFFu);
    const std::uint8_t flags = mipmap ? 0x03u : 0x02u;

    Memory::Write32(obj + 0x00u, word0);
    Memory::Write32(obj + 0x04u, word1);
    Memory::Write32(obj + 0x08u, word2);
    Memory::Write32(obj + 0x0Cu, word3);
    Memory::Write32(obj + 0x14u, format);
    Memory::Write16(obj + 0x1Cu, blockCount);
    Memory::Write8(obj + 0x1Eu, blockType);
    Memory::Write8(obj + 0x1Fu, flags);
}

} // namespace

extern "C" void mkw_switch_hle_gx_init_tex_obj(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t obj = cpu->gpr[3];
    const std::uint32_t data = cpu->gpr[4];
    const std::uint32_t width = cpu->gpr[5];
    const std::uint32_t height = cpu->gpr[6];
    const std::uint32_t format = cpu->gpr[7];
    const std::uint32_t wrapS = cpu->gpr[8];
    const std::uint32_t wrapT = cpu->gpr[9];
    const std::uint32_t mipmap = cpu->gpr[10];

    mkw_switch_set_fast_track_stage("RMCP01_GX_INIT_TEX_OBJ");

    const auto width16 = static_cast<std::uint16_t>(width);
    const auto height16 = static_cast<std::uint16_t>(height);
    const bool hasMipmaps = mipmap != 0u;

    WriteGuestTexObj(
        cpu,
        obj,
        data,
        width16,
        height16,
        format,
        wrapS,
        wrapT,
        hasMipmaps);

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    if (data != 0u && !Memory::Contains(data, 1u)) {
        AbortBoundary(
            "GX_INIT_TEX_OBJ_DATA_UNMAPPED",
            cpu,
            obj,
            data,
            width,
            height,
            format,
            wrapS,
            wrapT,
            mipmap);
    }

    try {
        std::scoped_lock lock(gTexObjMutex);
        GXTexObj* hostObj = GetOrCreateHostTexObj(obj);
        const void* hostData =
            data != 0u ? GuestToHostPtr(data, 1u) : nullptr;
        GXInitTexObj(
            hostObj,
            hostData,
            width16,
            height16,
            static_cast<GXTexFmt>(format),
            static_cast<GXTexWrapMode>(wrapS),
            static_cast<GXTexWrapMode>(wrapT),
            static_cast<GXBool>(mipmap));
    } catch (...) {
        AbortBoundary(
            "GX_INIT_TEX_OBJ_HOST_EXCEPTION",
            cpu,
            obj,
            data,
            width,
            height,
            format,
            wrapS,
            wrapT,
            mipmap);
    }
#endif

    WriteStatus(
        "init-pass",
        obj,
        data,
        width,
        height,
        format,
        wrapS,
        wrapT,
        mipmap);
}

extern "C" void mkw_switch_hle_gx_load_tex_obj(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t obj = cpu->gpr[3];
    const std::uint32_t tid = cpu->gpr[4];
    mkw_switch_set_fast_track_stage("RMCP01_GX_LOAD_TEX_OBJ");

    if (!Memory::IsInitialized() || !Memory::Contains(obj, kGuestTexObjSize)) {
        AbortLoadBoundary(
            "GX_LOAD_TEX_OBJ_GUEST_UNMAPPED",
            cpu,
            obj,
            tid,
            0u,
            0u,
            0u,
            0u,
            0u,
            0u,
            0u,
            0u);
    }

    const std::uint32_t word0 = Memory::Read32(obj + 0x00u);
    const std::uint32_t word1 = Memory::Read32(obj + 0x04u);
    const std::uint32_t word2 = Memory::Read32(obj + 0x08u);
    const std::uint32_t word3 = Memory::Read32(obj + 0x0Cu);
    const std::uint32_t word4 = Memory::Read32(obj + 0x10u);
    const std::uint32_t word5 = Memory::Read32(obj + 0x14u);
    const std::uint32_t word6 = Memory::Read32(obj + 0x18u);
    const std::uint32_t word7 = Memory::Read32(obj + 0x1Cu);

    const std::uint32_t data =
        CanonicalizeGuestMainRamAddress((word3 & 0x00FFFFFFu) << 5u);
    const std::uint32_t width = (word2 & 0x3FFu) + 1u;
    const std::uint32_t height = ((word2 >> 10u) & 0x3FFu) + 1u;
    const std::uint32_t formatWord2 = (word2 >> 20u) & 0xFu;
    const std::uint32_t format = word5;
    const std::uint32_t wrapS = word0 & 0x3u;
    const std::uint32_t wrapT = (word0 >> 2u) & 0x3u;
    const std::uint32_t mipmap = word7 & 0x1u;

    const bool exactObservedDescriptor =
        obj == kObservedLoadObj &&
        tid == kObservedLoadTid &&
        word0 == kObservedWord0 &&
        word1 == kObservedWord1 &&
        word2 == kObservedWord2 &&
        word3 == kObservedWord3 &&
        word4 == kObservedWord4 &&
        word5 == kObservedWord5 &&
        word6 == kObservedWord6 &&
        word7 == kObservedWord7 &&
        data == kObservedData &&
        width == kObservedWidth &&
        height == kObservedHeight &&
        format == kObservedFormat &&
        formatWord2 == kObservedFormat &&
        wrapS == 0u &&
        wrapT == 0u &&
        mipmap == 0u;

    if (!exactObservedDescriptor) {
        AbortLoadBoundary(
            "GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR",
            cpu,
            obj,
            tid,
            data,
            width,
            height,
            format,
            wrapS,
            wrapT,
            mipmap,
            kObservedTextureSize);
    }

    if (!Memory::Contains(data, kObservedTextureSize)) {
        AbortLoadBoundary(
            "GX_LOAD_TEX_OBJ_DATA_UNMAPPED",
            cpu,
            obj,
            tid,
            data,
            width,
            height,
            format,
            wrapS,
            wrapT,
            mipmap,
            kObservedTextureSize);
    }

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    try {
        std::scoped_lock lock(gTexObjMutex);
        GXTexObj* hostObj = GetOrCreateHostTexObj(obj);
        const void* hostData =
            GuestToHostPtr(data, kObservedTextureSize);
        if (!hostData) {
            AbortLoadBoundary(
                "GX_LOAD_TEX_OBJ_DATA_POINTER_NULL",
                cpu,
                obj,
                tid,
                data,
                width,
                height,
                format,
                wrapS,
                wrapT,
                mipmap,
                kObservedTextureSize);
        }

        GXInitTexObj(
            hostObj,
            hostData,
            kObservedWidth,
            kObservedHeight,
            static_cast<GXTexFmt>(kObservedFormat),
            static_cast<GXTexWrapMode>(0u),
            static_cast<GXTexWrapMode>(0u),
            static_cast<GXBool>(0u));
        GXInitTexObjLOD(
            hostObj,
            static_cast<GXTexFilter>(1u),
            static_cast<GXTexFilter>(1u),
            0.0f,
            0.0f,
            0.0f,
            GX_FALSE,
            GX_TRUE,
            static_cast<GXAnisotropy>(0u));
        GXInitTexObjUserData(hostObj, nullptr);
        GXLoadTexObj(hostObj, static_cast<GXTexMapID>(tid));
    } catch (...) {
        AbortLoadBoundary(
            "GX_LOAD_TEX_OBJ_HOST_EXCEPTION",
            cpu,
            obj,
            tid,
            data,
            width,
            height,
            format,
            wrapS,
            wrapT,
            mipmap,
            kObservedTextureSize);
    }
#endif

    PublishTextureLoadGuestState();
    WriteLoadStatus(
        "load-pass",
        obj,
        tid,
        data,
        width,
        height,
        format,
        wrapS,
        wrapT,
        mipmap,
        kObservedTextureSize);
}

#endif
