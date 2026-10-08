#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

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
constexpr std::uint32_t kGxInitTexObjLodAddress = 0x80170A4Cu;
constexpr std::uint32_t kGxInitTexObjWrapModeAddress = 0x80170B50u;
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
constexpr std::uint32_t kObservedTextureSize = 0x000B9400u;

// Later hardware load after GXSetChanAmbColor: one 4x4 IA8 tile.
// Only the captured descriptor is accepted; no texture payload is embedded.
constexpr std::uint32_t kObservedIa8LoadObj = 0x80384500u;
constexpr std::uint32_t kObservedIa8Data = 0x00384540u;
constexpr std::uint32_t kObservedIa8TextureSize = 32u;

// Captured in the Mii draw helper after viewport/depth setup. I4 stores
// 8x8 texels per 32-byte tile: 4x8 tiles require exactly 1,024 bytes.
constexpr std::uint32_t kObservedMiiI4LoadObj = 0x80397D80u;
constexpr std::uint32_t kObservedSecondMiiI4LoadObj = 0x80397DC0u;
// Next outer Mii pass: distinct captured object with the same tiled data.
constexpr std::uint32_t kObservedNextPassMiiI4LoadObj = 0x80397F80u;
constexpr std::uint32_t kObservedSecondNextPassMiiI4LoadObj = 0x80397FC0u;
constexpr std::uint32_t kObservedMiiI4Data = 0x109C1A40u;
// Two captured objects share a source 32 bytes earlier in another run.
constexpr std::uint32_t kObservedRelocatedMiiI4Data = 0x109C1A20u;
constexpr std::uint32_t kObservedMiiI4TextureSize = 1024u;

// Third captured Mii load: RGB5A3 uses 4x4 tiles of 32 bytes.
// The complete 44x32 image occupies 11x8 tiles (2,816 bytes).
constexpr std::uint32_t kObservedMiiRgb5a3LoadObj = 0x80397D40u;
constexpr std::uint32_t kObservedNextPassMiiRgb5a3LoadObj = 0x80397F40u;
constexpr std::uint32_t kObservedMiiRgb5a3Data = 0x109C0C40u;
constexpr std::uint32_t kObservedMiiRgb5a3TextureSize = 2816u;

// Fourth captured Mii load: I4 36x32 includes a partial right-edge tile.
// Ceil(36/8) x ceil(32/8) x 32 requires 640 bytes, not 576 texel bytes.
constexpr std::uint32_t kObservedMiiSmallI4LoadObj = 0x80397CC0u;
constexpr std::uint32_t kObservedSecondMiiSmallI4LoadObj = 0x80397D00u;
constexpr std::uint32_t kObservedMiiSmallI4Data = 0x109C1780u;
constexpr std::uint32_t kObservedMiiSmallI4TextureSize = 640u;

// Sixth captured Mii load: RGB5A3 38x32 includes a partial 4x4 tile.
// Ceil(38/4) x ceil(32/4) x 32 requires 2560 bytes; native width stays 38.
constexpr std::uint32_t kObservedMiiSmallRgb5a3LoadObj = 0x80397C40u;
constexpr std::uint32_t kObservedSecondMiiSmallRgb5a3LoadObj = 0x80397C80u;
constexpr std::uint32_t kObservedMiiSmallRgb5a3Data = 0x109C0200u;
constexpr std::uint32_t kObservedMiiSmallRgb5a3TextureSize = 2560u;

// Captured I4 16x16 load: two by two 8x8 tiles of 32 bytes.
constexpr std::uint32_t kObservedMiiTinyI4LoadObj = 0x80397E00u;
constexpr std::uint32_t kObservedMiiTinyI4Data = 0x109C1E80u;
constexpr std::uint32_t kObservedMiiTinyI4TextureSize = 128u;

constexpr std::uint32_t kObservedLodObj = 0x9018E120u;
constexpr std::uint32_t kObservedLodMinFilter = 1u;
constexpr std::uint32_t kObservedLodMagFilter = 1u;
constexpr std::uint32_t kObservedLodBiasClamp = 0u;
constexpr std::uint32_t kObservedLodEdgeLod = 0u;
constexpr std::uint32_t kObservedLodMaxAniso = 0u;
constexpr std::uint32_t kObservedLodWord0 = 0x00000095u;
constexpr std::uint32_t kObservedLodWord1 = 0x00000000u;
constexpr std::uint32_t kObservedLodWord2 = 0x0000FC3Fu;
constexpr std::uint32_t kObservedLodWord3 = 0x0080A997u;
constexpr std::uint32_t kObservedLodWord4 = 0x00000000u;
constexpr std::uint32_t kObservedLodWord5 = 0x00000000u;
constexpr std::uint32_t kObservedLodWord6 = 0x00000000u;
constexpr std::uint32_t kObservedLodWord7 = 0x00400102u;
constexpr std::uint32_t kObservedLodFloatBits = 0x00000000u;
constexpr std::uint32_t kObservedLodWord0After = 0x00000195u;
constexpr std::uint32_t kObservedLodWord1After = 0x00000000u;

constexpr std::uint32_t kObservedSecondLodObj = 0x9018E460u;
constexpr std::uint32_t kObservedSecondLodWord0 = 0x00000095u;
constexpr std::uint32_t kObservedSecondLodWord1 = 0x00000000u;
constexpr std::uint32_t kObservedSecondLodWord2 = 0x0000FC3Fu;
constexpr std::uint32_t kObservedSecondLodWord3 = 0x0080A88Fu;
constexpr std::uint32_t kObservedSecondLodWord4 = 0x00000000u;
constexpr std::uint32_t kObservedSecondLodWord5 = 0x00000000u;
constexpr std::uint32_t kObservedSecondLodWord6 = 0x00000000u;
constexpr std::uint32_t kObservedSecondLodWord7 = 0x00400102u;

constexpr std::uint32_t kObservedThirdLodObj = 0x9018E140u;
constexpr std::uint32_t kObservedThirdLodWord0 = 0x00000095u;
constexpr std::uint32_t kObservedThirdLodWord1 = 0x00000000u;
constexpr std::uint32_t kObservedThirdLodWord2 = 0x0000FC3Fu;
constexpr std::uint32_t kObservedThirdLodWord3 = 0x0080A998u;
constexpr std::uint32_t kObservedThirdLodWord4 = 0x00000000u;
constexpr std::uint32_t kObservedThirdLodWord5 = 0x00000000u;
constexpr std::uint32_t kObservedThirdLodWord6 = 0x00000000u;
constexpr std::uint32_t kObservedThirdLodWord7 = 0x00400102u;

constexpr std::uint32_t kObservedFourthLodObj = 0x9018E480u;
constexpr std::uint32_t kObservedFourthLodWord0 = 0x00000095u;
constexpr std::uint32_t kObservedFourthLodWord1 = 0x00000000u;
constexpr std::uint32_t kObservedFourthLodWord2 = 0x0000FC3Fu;
constexpr std::uint32_t kObservedFourthLodWord3 = 0x0080A890u;
constexpr std::uint32_t kObservedFourthLodWord4 = 0x00000000u;
constexpr std::uint32_t kObservedFourthLodWord5 = 0x00000000u;
constexpr std::uint32_t kObservedFourthLodWord6 = 0x00000000u;
constexpr std::uint32_t kObservedFourthLodWord7 = 0x00400102u;

constexpr std::uint32_t kObservedFifthLodObj = 0x908FA4E0u;
constexpr std::uint32_t kObservedFifthLodWord0 = 0x00000095u;
constexpr std::uint32_t kObservedFifthLodWord1 = 0x00000000u;
constexpr std::uint32_t kObservedFifthLodWord2 = 0x0000FC3Fu;
constexpr std::uint32_t kObservedFifthLodWord3 = 0x00845FB5u;
constexpr std::uint32_t kObservedFifthLodWord4 = 0x00000000u;
constexpr std::uint32_t kObservedFifthLodWord5 = 0x00000000u;
constexpr std::uint32_t kObservedFifthLodWord6 = 0x00000000u;
constexpr std::uint32_t kObservedFifthLodWord7 = 0x00400102u;

constexpr std::uint32_t kObservedSixthLodObj = 0x908FA5C0u;
constexpr std::uint32_t kObservedSixthLodWord0 = 0x00000095u;
constexpr std::uint32_t kObservedSixthLodWord1 = 0x00000000u;
constexpr std::uint32_t kObservedSixthLodWord2 = 0x0000FC3Fu;
constexpr std::uint32_t kObservedSixthLodWord3 = 0x00845FBCu;
constexpr std::uint32_t kObservedSixthLodWord4 = 0x00000000u;
constexpr std::uint32_t kObservedSixthLodWord5 = 0x00000000u;
constexpr std::uint32_t kObservedSixthLodWord6 = 0x00000000u;
constexpr std::uint32_t kObservedSixthLodWord7 = 0x00400102u;

constexpr std::uint32_t kObservedSeventhLodObj = 0x907938A0u;
constexpr std::uint32_t kObservedSeventhLodWord0 = 0x00000095u;
constexpr std::uint32_t kObservedSeventhLodWord1 = 0x00000000u;
constexpr std::uint32_t kObservedSeventhLodWord2 = 0x0000FC3Fu;
constexpr std::uint32_t kObservedSeventhLodWord3 = 0x0083AC53u;
constexpr std::uint32_t kObservedSeventhLodWord4 = 0x00000000u;
constexpr std::uint32_t kObservedSeventhLodWord5 = 0x00000000u;
constexpr std::uint32_t kObservedSeventhLodWord6 = 0x00000000u;
constexpr std::uint32_t kObservedSeventhLodWord7 = 0x00400102u;

constexpr std::uint32_t kObservedEighthLodObj = 0x908FA820u;
constexpr std::uint32_t kObservedEighthLodWord0 = 0x00000095u;
constexpr std::uint32_t kObservedEighthLodWord1 = 0x00000000u;
constexpr std::uint32_t kObservedEighthLodWord2 = 0x0000FC3Fu;
constexpr std::uint32_t kObservedEighthLodWord3 = 0x00845EADu;
constexpr std::uint32_t kObservedEighthLodWord4 = 0x00000000u;
constexpr std::uint32_t kObservedEighthLodWord5 = 0x00000000u;
constexpr std::uint32_t kObservedEighthLodWord6 = 0x00000000u;
constexpr std::uint32_t kObservedEighthLodWord7 = 0x00400102u;

constexpr std::uint32_t kObservedNinthLodObj = 0x90793BE0u;
constexpr std::uint32_t kObservedNinthLodWord0 = 0x00000095u;
constexpr std::uint32_t kObservedNinthLodWord1 = 0x00000000u;
constexpr std::uint32_t kObservedNinthLodWord2 = 0x0000FC3Fu;
constexpr std::uint32_t kObservedNinthLodWord3 = 0x0083AB4Bu;
constexpr std::uint32_t kObservedNinthLodWord4 = 0x00000000u;
constexpr std::uint32_t kObservedNinthLodWord5 = 0x00000000u;
constexpr std::uint32_t kObservedNinthLodWord6 = 0x00000000u;
constexpr std::uint32_t kObservedNinthLodWord7 = 0x00400102u;

constexpr std::uint32_t kObservedTenthLodObj = 0x909019C0u;
constexpr std::uint32_t kObservedTenthLodWord0 = 0x00000095u;
constexpr std::uint32_t kObservedTenthLodWord1 = 0x00000000u;
constexpr std::uint32_t kObservedTenthLodWord2 = 0x0000FC3Fu;
constexpr std::uint32_t kObservedTenthLodWord3 = 0x0084635Cu;
constexpr std::uint32_t kObservedTenthLodWord4 = 0x00000000u;
constexpr std::uint32_t kObservedTenthLodWord5 = 0x00000000u;
constexpr std::uint32_t kObservedTenthLodWord6 = 0x00000000u;
constexpr std::uint32_t kObservedTenthLodWord7 = 0x00400102u;

constexpr std::uint32_t kObservedEleventhLodObj = 0x908FA840u;
constexpr std::uint32_t kObservedEleventhLodWord0 = 0x00000095u;
constexpr std::uint32_t kObservedEleventhLodWord1 = 0x00000000u;
constexpr std::uint32_t kObservedEleventhLodWord2 = 0x0020FC3Fu;
constexpr std::uint32_t kObservedEleventhLodWord3 = 0x00845D15u;
constexpr std::uint32_t kObservedEleventhLodWord4 = 0x00000000u;
constexpr std::uint32_t kObservedEleventhLodWord5 = 0x00000002u;
constexpr std::uint32_t kObservedEleventhLodWord6 = 0x00000000u;
constexpr std::uint32_t kObservedEleventhLodWord7 = 0x00800202u;

constexpr std::uint32_t kObservedTwelfthLodObj = 0x9018E480u;
constexpr std::uint32_t kObservedTwelfthLodWord0 = 0x00000095u;
constexpr std::uint32_t kObservedTwelfthLodWord1 = 0x00000000u;
constexpr std::uint32_t kObservedTwelfthLodWord2 = 0x0020FC3Fu;
constexpr std::uint32_t kObservedTwelfthLodWord3 = 0x0080A6F7u;
constexpr std::uint32_t kObservedTwelfthLodWord4 = 0x00000000u;
constexpr std::uint32_t kObservedTwelfthLodWord5 = 0x00000002u;
constexpr std::uint32_t kObservedTwelfthLodWord6 = 0x00000000u;
constexpr std::uint32_t kObservedTwelfthLodWord7 = 0x00800202u;

constexpr std::uint32_t kObservedThirteenthLodObj = 0x908FAE00u;
constexpr std::uint32_t kObservedThirteenthLodWord0 = 0x00000095u;
constexpr std::uint32_t kObservedThirteenthLodWord1 = 0x00000000u;
constexpr std::uint32_t kObservedThirteenthLodWord2 = 0x0000FC3Fu;
constexpr std::uint32_t kObservedThirteenthLodWord3 = 0x00845FB5u;
constexpr std::uint32_t kObservedThirteenthLodWord4 = 0x00000000u;
constexpr std::uint32_t kObservedThirteenthLodWord5 = 0x00000000u;
constexpr std::uint32_t kObservedThirteenthLodWord6 = 0x00000000u;
constexpr std::uint32_t kObservedThirteenthLodWord7 = 0x00400102u;

constexpr std::uint32_t kObservedFourteenthLodObj = 0x908FB140u;
constexpr std::uint32_t kObservedFourteenthLodWord0 = 0x00000095u;
constexpr std::uint32_t kObservedFourteenthLodWord1 = 0x00000000u;
constexpr std::uint32_t kObservedFourteenthLodWord2 = 0x0000FC3Fu;
constexpr std::uint32_t kObservedFourteenthLodWord3 = 0x00845EEFu;
constexpr std::uint32_t kObservedFourteenthLodWord4 = 0x00000000u;
constexpr std::uint32_t kObservedFourteenthLodWord5 = 0x00000000u;
constexpr std::uint32_t kObservedFourteenthLodWord6 = 0x00000000u;
constexpr std::uint32_t kObservedFourteenthLodWord7 = 0x00400102u;

constexpr std::uint32_t kObservedFifteenthLodObj = 0x908FB160u;
constexpr std::uint32_t kObservedFifteenthLodWord0 = 0x00000095u;
constexpr std::uint32_t kObservedFifteenthLodWord1 = 0x00000000u;
constexpr std::uint32_t kObservedFifteenthLodWord2 = 0x0020FC3Fu;
constexpr std::uint32_t kObservedFifteenthLodWord3 = 0x00845D15u;
constexpr std::uint32_t kObservedFifteenthLodWord4 = 0x00000000u;
constexpr std::uint32_t kObservedFifteenthLodWord5 = 0x00000002u;
constexpr std::uint32_t kObservedFifteenthLodWord6 = 0x00000000u;
constexpr std::uint32_t kObservedFifteenthLodWord7 = 0x00800202u;

constexpr std::uint32_t kObservedSixteenthLodObj = 0x90901D00u;
constexpr std::uint32_t kObservedSixteenthLodWord0 = 0x00000095u;
constexpr std::uint32_t kObservedSixteenthLodWord1 = 0x00000000u;
constexpr std::uint32_t kObservedSixteenthLodWord2 = 0x0000FC3Fu;
constexpr std::uint32_t kObservedSixteenthLodWord3 = 0x00846254u;
constexpr std::uint32_t kObservedSixteenthLodWord4 = 0x00000000u;
constexpr std::uint32_t kObservedSixteenthLodWord5 = 0x00000000u;
constexpr std::uint32_t kObservedSixteenthLodWord6 = 0x00000000u;
constexpr std::uint32_t kObservedSixteenthLodWord7 = 0x00400102u;

constexpr std::uint32_t kObservedSeventeenthLodObj = 0x90825180u;
constexpr std::uint32_t kObservedSeventeenthLodWord0 = 0x00000095u;
constexpr std::uint32_t kObservedSeventeenthLodWord1 = 0x00000000u;
constexpr std::uint32_t kObservedSeventeenthLodWord2 = 0x0000FC3Fu;
constexpr std::uint32_t kObservedSeventeenthLodWord3 = 0x0083F51Au;
constexpr std::uint32_t kObservedSeventeenthLodWord4 = 0x00000000u;
constexpr std::uint32_t kObservedSeventeenthLodWord5 = 0x00000000u;
constexpr std::uint32_t kObservedSeventeenthLodWord6 = 0x00000000u;
constexpr std::uint32_t kObservedSeventeenthLodWord7 = 0x00400102u;

constexpr std::uint32_t kObservedWrapObj = 0x9018E120u;
constexpr std::uint32_t kObservedSecondWrapObj = 0x9018E460u;
constexpr std::uint32_t kObservedThirdWrapObj = 0x9018E140u;
constexpr std::uint32_t kObservedFourthWrapObj = 0x908FA4E0u;
constexpr std::uint32_t kObservedFifthWrapObj = 0x907938A0u;
constexpr std::uint32_t kObservedSixthWrapObj = 0x908FA5C0u;
constexpr std::uint32_t kObservedSeventhWrapObj = 0x908FA820u;
constexpr std::uint32_t kObservedEighthWrapObj = 0x909019C0u;
constexpr std::uint32_t kObservedNinthWrapObj = 0x908FA840u;
constexpr std::uint32_t kObservedTenthWrapObj = 0x9018E480u;
constexpr std::uint32_t kObservedEleventhWrapObj = 0x908FAE00u;
constexpr std::uint32_t kObservedTwelfthWrapObj = 0x908FB140u;
constexpr std::uint32_t kObservedThirteenthWrapObj = 0x9018E480u;
constexpr std::uint32_t kObservedWrapS = 0u;
constexpr std::uint32_t kObservedWrapT = 0u;
constexpr std::uint32_t kObservedWrapWord0Before = 0x00000195u;
constexpr std::uint32_t kObservedWrapWord0After = 0x00000190u;

#if defined(MKW_STRICT_GX_TEXTURE_OBSERVED_TUPLES) && MKW_STRICT_GX_TEXTURE_OBSERVED_TUPLES
constexpr bool kStrictObservedTextureTuples = true;
#else
constexpr bool kStrictObservedTextureTuples = false;
#endif

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
constexpr const char* kLodStatusPath =
    "sdmc:/switch/WiiCompiled-Switch/fast-track-gx-init-tex-obj-lod.txt";
constexpr const char* kWrapStatusPath =
    "sdmc:/switch/WiiCompiled-Switch/fast-track-gx-init-tex-obj-wrap-mode.txt";

std::mutex gTexObjMutex;
std::map<std::uint32_t, std::unique_ptr<GXTexObj>> gHostTexObjs;

GXTexObj* GetOrCreateHostTexObj(std::uint32_t guestAddr) {
    auto& entry = gHostTexObjs[guestAddr];
    if (!entry) {
        entry = std::make_unique<GXTexObj>();
    }
    return entry.get();
}

GXTexObj* FindLocalHostTexObj(std::uint32_t guestAddr) noexcept {
    const auto it = gHostTexObjs.find(guestAddr);
    return it != gHostTexObjs.end() ? it->second.get() : nullptr;
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

void WriteLodStatus(
    const char* status,
    std::uint32_t obj,
    std::uint32_t minFilter,
    std::uint32_t magFilter,
    std::uint32_t minLodBits,
    std::uint32_t maxLodBits,
    std::uint32_t lodBiasBits,
    std::uint32_t biasClamp,
    std::uint32_t edgeLod,
    std::uint32_t maxAniso) noexcept {
    FILE* out = std::fopen(kLodStatusPath, "w");
    if (!out) {
        return;
    }
    std::fprintf(
        out,
        "status=%s\n"
        "obj=0x%08x\n"
        "min_filter=%u\n"
        "mag_filter=%u\n"
        "min_lod_bits=0x%08x\n"
        "max_lod_bits=0x%08x\n"
        "lod_bias_bits=0x%08x\n"
        "bias_clamp=%u\n"
        "edge_lod=%u\n"
        "max_aniso=%u\n"
        "guest_word0=0x%08x\n"
        "guest_word1=0x%08x\n",
        status ? status : "<null>",
        obj,
        minFilter,
        magFilter,
        minLodBits,
        maxLodBits,
        lodBiasBits,
        biasClamp,
        edgeLod,
        maxAniso,
        Memory::Contains(obj, kGuestTexObjSize) ? Memory::Read32(obj + 0x00u) : 0u,
        Memory::Contains(obj, kGuestTexObjSize) ? Memory::Read32(obj + 0x04u) : 0u);
    std::fclose(out);
}

void WriteWrapStatus(
    const char* status,
    std::uint32_t obj,
    std::uint32_t wrapS,
    std::uint32_t wrapT,
    std::uint32_t word0Before) noexcept {
    FILE* out = std::fopen(kWrapStatusPath, "w");
    if (!out) {
        return;
    }
    std::fprintf(
        out,
        "status=%s\n"
        "obj=0x%08x\n"
        "wrap_s=%u\n"
        "wrap_t=%u\n"
        "word0_before=0x%08x\n"
        "guest_word0=0x%08x\n"
        "guest_word1=0x%08x\n",
        status ? status : "<null>",
        obj,
        wrapS,
        wrapT,
        word0Before,
        Memory::Contains(obj, kGuestTexObjSize) ? Memory::Read32(obj + 0x00u) : 0u,
        Memory::Contains(obj, kGuestTexObjSize) ? Memory::Read32(obj + 0x04u) : 0u);
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

void WriteLodStatus(
    const char*,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t) noexcept {}

void WriteWrapStatus(
    const char*,
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

[[noreturn]] void AbortLodBoundary(
    const char* status,
    CpuContext* cpu,
    std::uint32_t obj,
    std::uint32_t minFilter,
    std::uint32_t magFilter,
    std::uint32_t minLodBits,
    std::uint32_t maxLodBits,
    std::uint32_t lodBiasBits,
    std::uint32_t biasClamp,
    std::uint32_t edgeLod,
    std::uint32_t maxAniso) noexcept {
    WriteLodStatus(
        status,
        obj,
        minFilter,
        magFilter,
        minLodBits,
        maxLodBits,
        lodBiasBits,
        biasClamp,
        edgeLod,
        maxAniso);
    mkw_switch_report_unsupported_translated_dispatch(
        status, kGxInitTexObjLodAddress, cpu);
    std::abort();
}

[[noreturn]] void AbortWrapBoundary(
    const char* status,
    CpuContext* cpu,
    std::uint32_t obj,
    std::uint32_t wrapS,
    std::uint32_t wrapT,
    std::uint32_t word0Before) noexcept {
    WriteWrapStatus(status, obj, wrapS, wrapT, word0Before);
    mkw_switch_report_unsupported_translated_dispatch(
        status, kGxInitTexObjWrapModeAddress, cpu);
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

std::uint32_t FloatBits(float value) noexcept {
    std::uint32_t bits = 0u;
    static_assert(sizeof(bits) == sizeof(value));
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

bool IsValidWrapMode(std::uint32_t wrap) noexcept {
    return wrap <= 2u;
}

bool IsValidLodArgs(
    std::uint32_t minFilter,
    std::uint32_t magFilter,
    float minLod,
    float maxLod,
    float lodBias,
    std::uint32_t biasClamp,
    std::uint32_t edgeLod,
    std::uint32_t maxAniso) noexcept {
    constexpr float kMaxEncodedLod = 255.0f / 16.0f;
    if (minFilter > 5u || magFilter > 1u || maxAniso > 2u) {
        return false;
    }
    if (biasClamp > 1u || edgeLod > 1u) {
        return false;
    }
    if (!std::isfinite(minLod) || !std::isfinite(maxLod) ||
        !std::isfinite(lodBias)) {
        return false;
    }
    if (minLod < 0.0f || maxLod < minLod || maxLod > kMaxEncodedLod) {
        return false;
    }
    return lodBias >= -4.0f && lodBias <= 3.99f;
}

bool GetTexObjBlockLayout(
    std::uint32_t format,
    std::uint32_t& shiftX,
    std::uint32_t& shiftY,
    std::uint32_t& blockType) noexcept {
    switch (format) {
    case 0u:
    case 8u:
        shiftX = 3u;
        shiftY = 3u;
        blockType = 1u;
        return true;
    case 1u:
    case 2u:
    case 9u:
        shiftX = 3u;
        shiftY = 2u;
        blockType = 2u;
        return true;
    case 3u:
    case 4u:
    case 5u:
    case 10u:
        shiftX = 2u;
        shiftY = 2u;
        blockType = 2u;
        return true;
    case 6u:
    case 22u: // GX_TF_Z24X8: same 4x4, two-plane layout as RGBA8.
        shiftX = 2u;
        shiftY = 2u;
        blockType = 3u;
        return true;
    case 14u:
        shiftX = 3u;
        shiftY = 3u;
        blockType = 0u;
        return true;
    default:
        return false;
    }
}

bool IsStructurallyValidGuestTexObj(
    std::uint32_t word0,
    std::uint32_t word2,
    std::uint32_t word3,
    std::uint32_t word5,
    std::uint32_t word7) noexcept {
    if (!IsValidWrapMode(word0 & 0x3u) ||
        !IsValidWrapMode((word0 >> 2u) & 0x3u) ||
        (word3 & 0xFF000000u) != 0u) {
        return false;
    }

    const std::uint32_t format = word5;
    if (((word2 >> 20u) & 0xFu) != (format & 0xFu)) {
        return false;
    }

    std::uint32_t shiftX = 0u;
    std::uint32_t shiftY = 0u;
    std::uint32_t expectedBlockType = 0u;
    if (!GetTexObjBlockLayout(format, shiftX, shiftY, expectedBlockType)) {
        return false;
    }

    const std::uint32_t width = (word2 & 0x3FFu) + 1u;
    const std::uint32_t height = ((word2 >> 10u) & 0x3FFu) + 1u;
    const std::uint32_t blocksX =
        (width + ((1u << shiftX) - 1u)) >> shiftX;
    const std::uint32_t blocksY =
        (height + ((1u << shiftY) - 1u)) >> shiftY;
    const std::uint32_t expectedBlockCount =
        (blocksX * blocksY) & 0x7FFFu;

    const std::uint32_t blockCount = (word7 >> 16u) & 0xFFFFu;
    const std::uint32_t blockType = (word7 >> 8u) & 0xFFu;
    const std::uint32_t flags = word7 & 0xFFu;

    if (blockCount != expectedBlockCount ||
        blockType != expectedBlockType) {
        return false;
    }
    return flags == 0x02u || flags == 0x03u;
}

void ApplyGuestTexObjLodWords(
    std::uint32_t& word0,
    std::uint32_t& word1,
    std::uint32_t minFilter,
    std::uint32_t magFilter,
    float minLod,
    float maxLod,
    float lodBias,
    std::uint32_t biasClamp,
    std::uint32_t edgeLod,
    std::uint32_t maxAniso) noexcept {
    static constexpr std::uint8_t kGxToHwMinFilter[6] = {
        0u, 4u, 1u, 5u, 2u, 6u};

    const std::uint32_t minFilterBits = kGxToHwMinFilter[minFilter];
    word0 =
        (word0 & ~0xF0u) |
        ((magFilter & 1u) << 4u) |
        ((minFilterBits & 7u) << 5u);

    const auto scaledBias =
        static_cast<std::int8_t>(static_cast<int>(lodBias * 32.0f));
    word0 =
        (word0 & ~0x3FF00u) |
        ((edgeLod != 0u ? 0u : 1u) << 8u) |
        (static_cast<std::uint32_t>(
             static_cast<std::uint8_t>(scaledBias))
         << 9u);

    word0 =
        (word0 & ~0x380000u) |
        ((maxAniso & 0x3u) << 19u) |
        ((biasClamp != 0u ? 1u : 0u) << 21u);

    const auto minLodScaled =
        static_cast<std::uint32_t>(minLod * 16.0f) & 0xFFu;
    const auto maxLodScaled =
        static_cast<std::uint32_t>(maxLod * 16.0f) & 0xFFu;
    word1 =
        (word1 & 0xFFFF0000u) |
        minLodScaled |
        (maxLodScaled << 8u);
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

extern "C" void mkw_switch_hle_gx_init_tex_obj_lod(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t obj = cpu->gpr[3];
    const std::uint32_t minFilter = cpu->gpr[4];
    const std::uint32_t magFilter = cpu->gpr[5];
    const float minLod = static_cast<float>(cpu->fpr[1].d);
    const float maxLod = static_cast<float>(cpu->fpr[2].d);
    const float lodBias = static_cast<float>(cpu->fpr[3].d);
    const std::uint32_t biasClamp = cpu->gpr[6];
    const std::uint32_t edgeLod = cpu->gpr[7];
    const std::uint32_t maxAniso = cpu->gpr[8];

    const std::uint32_t minLodBits = FloatBits(minLod);
    const std::uint32_t maxLodBits = FloatBits(maxLod);
    const std::uint32_t lodBiasBits = FloatBits(lodBias);

    mkw_switch_set_fast_track_stage("RMCP01_GX_INIT_TEX_OBJ_LOD");

    if (!Memory::IsInitialized() || !Memory::Contains(obj, kGuestTexObjSize)) {
        AbortLodBoundary(
            "GX_INIT_TEX_OBJ_LOD_GUEST_UNMAPPED",
            cpu,
            obj,
            minFilter,
            magFilter,
            minLodBits,
            maxLodBits,
            lodBiasBits,
            biasClamp,
            edgeLod,
            maxAniso);
    }

    const std::uint32_t word0 = Memory::Read32(obj + 0x00u);
    const std::uint32_t word1 = Memory::Read32(obj + 0x04u);
    const std::uint32_t word2 = Memory::Read32(obj + 0x08u);
    const std::uint32_t word3 = Memory::Read32(obj + 0x0Cu);
    const std::uint32_t word4 = Memory::Read32(obj + 0x10u);
    const std::uint32_t word5 = Memory::Read32(obj + 0x14u);
    const std::uint32_t word6 = Memory::Read32(obj + 0x18u);
    const std::uint32_t word7 = Memory::Read32(obj + 0x1Cu);

    const bool exactObservedArgs =
        minFilter == kObservedLodMinFilter &&
        magFilter == kObservedLodMagFilter &&
        minLodBits == kObservedLodFloatBits &&
        maxLodBits == kObservedLodFloatBits &&
        lodBiasBits == kObservedLodFloatBits &&
        biasClamp == kObservedLodBiasClamp &&
        edgeLod == kObservedLodEdgeLod &&
        maxAniso == kObservedLodMaxAniso;

    const bool exactFirstObservedDescriptor =
        obj == kObservedLodObj &&
        word0 == kObservedLodWord0 &&
        word1 == kObservedLodWord1 &&
        word2 == kObservedLodWord2 &&
        word3 == kObservedLodWord3 &&
        word4 == kObservedLodWord4 &&
        word5 == kObservedLodWord5 &&
        word6 == kObservedLodWord6 &&
        word7 == kObservedLodWord7;

    const bool exactSecondObservedDescriptor =
        obj == kObservedSecondLodObj &&
        word0 == kObservedSecondLodWord0 &&
        word1 == kObservedSecondLodWord1 &&
        word2 == kObservedSecondLodWord2 &&
        word3 == kObservedSecondLodWord3 &&
        word4 == kObservedSecondLodWord4 &&
        word5 == kObservedSecondLodWord5 &&
        word6 == kObservedSecondLodWord6 &&
        word7 == kObservedSecondLodWord7;

    const bool exactThirdObservedDescriptor =
        obj == kObservedThirdLodObj &&
        word0 == kObservedThirdLodWord0 &&
        word1 == kObservedThirdLodWord1 &&
        word2 == kObservedThirdLodWord2 &&
        word3 == kObservedThirdLodWord3 &&
        word4 == kObservedThirdLodWord4 &&
        word5 == kObservedThirdLodWord5 &&
        word6 == kObservedThirdLodWord6 &&
        word7 == kObservedThirdLodWord7;

    const bool exactFourthObservedDescriptor =
        obj == kObservedFourthLodObj &&
        word0 == kObservedFourthLodWord0 &&
        word1 == kObservedFourthLodWord1 &&
        word2 == kObservedFourthLodWord2 &&
        word3 == kObservedFourthLodWord3 &&
        word4 == kObservedFourthLodWord4 &&
        word5 == kObservedFourthLodWord5 &&
        word6 == kObservedFourthLodWord6 &&
        word7 == kObservedFourthLodWord7;

    const bool exactFifthObservedDescriptor =
        obj == kObservedFifthLodObj &&
        word0 == kObservedFifthLodWord0 &&
        word1 == kObservedFifthLodWord1 &&
        word2 == kObservedFifthLodWord2 &&
        word3 == kObservedFifthLodWord3 &&
        word4 == kObservedFifthLodWord4 &&
        word5 == kObservedFifthLodWord5 &&
        word6 == kObservedFifthLodWord6 &&
        word7 == kObservedFifthLodWord7;

    const bool exactSixthObservedDescriptor =
        obj == kObservedSixthLodObj &&
        word0 == kObservedSixthLodWord0 &&
        word1 == kObservedSixthLodWord1 &&
        word2 == kObservedSixthLodWord2 &&
        word3 == kObservedSixthLodWord3 &&
        word4 == kObservedSixthLodWord4 &&
        word5 == kObservedSixthLodWord5 &&
        word6 == kObservedSixthLodWord6 &&
        word7 == kObservedSixthLodWord7;

    const bool exactSeventhObservedDescriptor =
        obj == kObservedSeventhLodObj &&
        word0 == kObservedSeventhLodWord0 &&
        word1 == kObservedSeventhLodWord1 &&
        word2 == kObservedSeventhLodWord2 &&
        word3 == kObservedSeventhLodWord3 &&
        word4 == kObservedSeventhLodWord4 &&
        word5 == kObservedSeventhLodWord5 &&
        word6 == kObservedSeventhLodWord6 &&
        word7 == kObservedSeventhLodWord7;

    const bool exactEighthObservedDescriptor =
        obj == kObservedEighthLodObj &&
        word0 == kObservedEighthLodWord0 &&
        word1 == kObservedEighthLodWord1 &&
        word2 == kObservedEighthLodWord2 &&
        word3 == kObservedEighthLodWord3 &&
        word4 == kObservedEighthLodWord4 &&
        word5 == kObservedEighthLodWord5 &&
        word6 == kObservedEighthLodWord6 &&
        word7 == kObservedEighthLodWord7;

    const bool exactNinthObservedDescriptor =
        obj == kObservedNinthLodObj &&
        word0 == kObservedNinthLodWord0 &&
        word1 == kObservedNinthLodWord1 &&
        word2 == kObservedNinthLodWord2 &&
        word3 == kObservedNinthLodWord3 &&
        word4 == kObservedNinthLodWord4 &&
        word5 == kObservedNinthLodWord5 &&
        word6 == kObservedNinthLodWord6 &&
        word7 == kObservedNinthLodWord7;

    const bool exactTenthObservedDescriptor =
        obj == kObservedTenthLodObj &&
        word0 == kObservedTenthLodWord0 &&
        word1 == kObservedTenthLodWord1 &&
        word2 == kObservedTenthLodWord2 &&
        word3 == kObservedTenthLodWord3 &&
        word4 == kObservedTenthLodWord4 &&
        word5 == kObservedTenthLodWord5 &&
        word6 == kObservedTenthLodWord6 &&
        word7 == kObservedTenthLodWord7;

    const bool exactEleventhObservedDescriptor =
        obj == kObservedEleventhLodObj &&
        word0 == kObservedEleventhLodWord0 &&
        word1 == kObservedEleventhLodWord1 &&
        word2 == kObservedEleventhLodWord2 &&
        word3 == kObservedEleventhLodWord3 &&
        word4 == kObservedEleventhLodWord4 &&
        word5 == kObservedEleventhLodWord5 &&
        word6 == kObservedEleventhLodWord6 &&
        word7 == kObservedEleventhLodWord7;

    const bool exactTwelfthObservedDescriptor =
        obj == kObservedTwelfthLodObj &&
        word0 == kObservedTwelfthLodWord0 &&
        word1 == kObservedTwelfthLodWord1 &&
        word2 == kObservedTwelfthLodWord2 &&
        word3 == kObservedTwelfthLodWord3 &&
        word4 == kObservedTwelfthLodWord4 &&
        word5 == kObservedTwelfthLodWord5 &&
        word6 == kObservedTwelfthLodWord6 &&
        word7 == kObservedTwelfthLodWord7;

    const bool exactThirteenthObservedDescriptor =
        obj == kObservedThirteenthLodObj &&
        word0 == kObservedThirteenthLodWord0 &&
        word1 == kObservedThirteenthLodWord1 &&
        word2 == kObservedThirteenthLodWord2 &&
        word3 == kObservedThirteenthLodWord3 &&
        word4 == kObservedThirteenthLodWord4 &&
        word5 == kObservedThirteenthLodWord5 &&
        word6 == kObservedThirteenthLodWord6 &&
        word7 == kObservedThirteenthLodWord7;

    const bool exactFourteenthObservedDescriptor =
        obj == kObservedFourteenthLodObj &&
        word0 == kObservedFourteenthLodWord0 &&
        word1 == kObservedFourteenthLodWord1 &&
        word2 == kObservedFourteenthLodWord2 &&
        word3 == kObservedFourteenthLodWord3 &&
        word4 == kObservedFourteenthLodWord4 &&
        word5 == kObservedFourteenthLodWord5 &&
        word6 == kObservedFourteenthLodWord6 &&
        word7 == kObservedFourteenthLodWord7;

    const bool exactFifteenthObservedDescriptor =
        obj == kObservedFifteenthLodObj &&
        word0 == kObservedFifteenthLodWord0 &&
        word1 == kObservedFifteenthLodWord1 &&
        word2 == kObservedFifteenthLodWord2 &&
        word3 == kObservedFifteenthLodWord3 &&
        word4 == kObservedFifteenthLodWord4 &&
        word5 == kObservedFifteenthLodWord5 &&
        word6 == kObservedFifteenthLodWord6 &&
        word7 == kObservedFifteenthLodWord7;

    const bool exactSixteenthObservedDescriptor =
        obj == kObservedSixteenthLodObj &&
        word0 == kObservedSixteenthLodWord0 &&
        word1 == kObservedSixteenthLodWord1 &&
        word2 == kObservedSixteenthLodWord2 &&
        word3 == kObservedSixteenthLodWord3 &&
        word4 == kObservedSixteenthLodWord4 &&
        word5 == kObservedSixteenthLodWord5 &&
        word6 == kObservedSixteenthLodWord6 &&
        word7 == kObservedSixteenthLodWord7;

    const bool exactSeventeenthObservedDescriptor =
        obj == kObservedSeventeenthLodObj &&
        word0 == kObservedSeventeenthLodWord0 &&
        word1 == kObservedSeventeenthLodWord1 &&
        word2 == kObservedSeventeenthLodWord2 &&
        word3 == kObservedSeventeenthLodWord3 &&
        word4 == kObservedSeventeenthLodWord4 &&
        word5 == kObservedSeventeenthLodWord5 &&
        word6 == kObservedSeventeenthLodWord6 &&
        word7 == kObservedSeventeenthLodWord7;

    const bool knownObservedDescriptor =
        exactFirstObservedDescriptor ||
        exactSecondObservedDescriptor ||
        exactThirdObservedDescriptor ||
        exactFourthObservedDescriptor ||
        exactFifthObservedDescriptor ||
        exactSixthObservedDescriptor ||
        exactSeventhObservedDescriptor ||
        exactEighthObservedDescriptor ||
        exactNinthObservedDescriptor ||
        exactTenthObservedDescriptor ||
        exactEleventhObservedDescriptor ||
        exactTwelfthObservedDescriptor ||
        exactThirteenthObservedDescriptor ||
        exactFourteenthObservedDescriptor ||
        exactFifteenthObservedDescriptor ||
        exactSixteenthObservedDescriptor ||
        exactSeventeenthObservedDescriptor;
    const bool knownObservedTuple =
        exactObservedArgs && knownObservedDescriptor;

    if (!IsValidLodArgs(
            minFilter,
            magFilter,
            minLod,
            maxLod,
            lodBias,
            biasClamp,
            edgeLod,
            maxAniso)) {
        AbortLodBoundary(
            "GX_INIT_TEX_OBJ_LOD_INVALID_ARGS",
            cpu,
            obj,
            minFilter,
            magFilter,
            minLodBits,
            maxLodBits,
            lodBiasBits,
            biasClamp,
            edgeLod,
            maxAniso);
    }

    if (!IsStructurallyValidGuestTexObj(
            word0, word2, word3, word5, word7)) {
        AbortLodBoundary(
            "GX_INIT_TEX_OBJ_LOD_INVALID_DESCRIPTOR",
            cpu,
            obj,
            minFilter,
            magFilter,
            minLodBits,
            maxLodBits,
            lodBiasBits,
            biasClamp,
            edgeLod,
            maxAniso);
    }

    if (kStrictObservedTextureTuples && !knownObservedTuple) {
        AbortLodBoundary(
            "GX_INIT_TEX_OBJ_LOD_UNPROVEN_TUPLE",
            cpu,
            obj,
            minFilter,
            magFilter,
            minLodBits,
            maxLodBits,
            lodBiasBits,
            biasClamp,
            edgeLod,
            maxAniso);
    }

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    try {
        std::scoped_lock lock(gTexObjMutex);
        GXTexObj* hostObj = FindLocalHostTexObj(obj);
        if (!hostObj) {
            AbortLodBoundary(
                "GX_INIT_TEX_OBJ_LOD_HOST_OBJ_MISSING",
                cpu,
                obj,
                minFilter,
                magFilter,
                minLodBits,
                maxLodBits,
                lodBiasBits,
                biasClamp,
                edgeLod,
                maxAniso);
        }
        GXInitTexObjLOD(
            hostObj,
            static_cast<GXTexFilter>(minFilter),
            static_cast<GXTexFilter>(magFilter),
            minLod,
            maxLod,
            lodBias,
            biasClamp != 0u ? GX_TRUE : GX_FALSE,
            edgeLod != 0u ? GX_TRUE : GX_FALSE,
            static_cast<GXAnisotropy>(maxAniso));
    } catch (...) {
        AbortLodBoundary(
            "GX_INIT_TEX_OBJ_LOD_HOST_EXCEPTION",
            cpu,
            obj,
            minFilter,
            magFilter,
            minLodBits,
            maxLodBits,
            lodBiasBits,
            biasClamp,
            edgeLod,
            maxAniso);
    }
#endif

    std::uint32_t expectedWord0 = word0;
    std::uint32_t expectedWord1 = word1;
    ApplyGuestTexObjLodWords(
        expectedWord0,
        expectedWord1,
        minFilter,
        magFilter,
        minLod,
        maxLod,
        lodBias,
        biasClamp,
        edgeLod,
        maxAniso);
    Memory::Write32(obj + 0x00u, expectedWord0);
    Memory::Write32(obj + 0x04u, expectedWord1);

    if (Memory::Read32(obj + 0x00u) != expectedWord0 ||
        Memory::Read32(obj + 0x04u) != expectedWord1) {
        AbortLodBoundary(
            "GX_INIT_TEX_OBJ_LOD_GUEST_STATE_MISMATCH",
            cpu,
            obj,
            minFilter,
            magFilter,
            minLodBits,
            maxLodBits,
            lodBiasBits,
            biasClamp,
            edgeLod,
            maxAniso);
    }

    WriteLodStatus(
        "lod-pass",
        obj,
        minFilter,
        magFilter,
        minLodBits,
        maxLodBits,
        lodBiasBits,
        biasClamp,
        edgeLod,
        maxAniso);
}

extern "C" void mkw_switch_hle_gx_init_tex_obj_wrap_mode(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t obj = cpu->gpr[3];
    const std::uint32_t wrapS = cpu->gpr[4];
    const std::uint32_t wrapT = cpu->gpr[5];

    mkw_switch_set_fast_track_stage("RMCP01_GX_INIT_TEX_OBJ_WRAP_MODE");

    if (!Memory::IsInitialized() || !Memory::Contains(obj, kGuestTexObjSize)) {
        AbortWrapBoundary(
            "GX_INIT_TEX_OBJ_WRAP_MODE_GUEST_UNMAPPED",
            cpu,
            obj,
            wrapS,
            wrapT,
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

    const bool exactFirstObservedTuple =
        obj == kObservedWrapObj &&
        wrapS == kObservedWrapS &&
        wrapT == kObservedWrapT &&
        word0 == kObservedWrapWord0Before &&
        word1 == kObservedLodWord1After &&
        word2 == kObservedLodWord2 &&
        word3 == kObservedLodWord3 &&
        word4 == kObservedLodWord4 &&
        word5 == kObservedLodWord5 &&
        word6 == kObservedLodWord6 &&
        word7 == kObservedLodWord7;

    const bool exactSecondObservedTuple =
        obj == kObservedSecondWrapObj &&
        wrapS == kObservedWrapS &&
        wrapT == kObservedWrapT &&
        word0 == kObservedWrapWord0Before &&
        word1 == kObservedLodWord1After &&
        word2 == kObservedSecondLodWord2 &&
        word3 == kObservedSecondLodWord3 &&
        word4 == kObservedSecondLodWord4 &&
        word5 == kObservedSecondLodWord5 &&
        word6 == kObservedSecondLodWord6 &&
        word7 == kObservedSecondLodWord7;

    const bool exactThirdObservedTuple =
        obj == kObservedThirdWrapObj &&
        wrapS == kObservedWrapS &&
        wrapT == kObservedWrapT &&
        word0 == kObservedWrapWord0Before &&
        word1 == kObservedLodWord1After &&
        word2 == kObservedThirdLodWord2 &&
        word3 == kObservedThirdLodWord3 &&
        word4 == kObservedThirdLodWord4 &&
        word5 == kObservedThirdLodWord5 &&
        word6 == kObservedThirdLodWord6 &&
        word7 == kObservedThirdLodWord7;

    const bool exactFourthObservedTuple =
        obj == kObservedFourthWrapObj &&
        wrapS == kObservedWrapS &&
        wrapT == kObservedWrapT &&
        word0 == kObservedWrapWord0Before &&
        word1 == kObservedLodWord1After &&
        word2 == kObservedFifthLodWord2 &&
        word3 == kObservedFifthLodWord3 &&
        word4 == kObservedFifthLodWord4 &&
        word5 == kObservedFifthLodWord5 &&
        word6 == kObservedFifthLodWord6 &&
        word7 == kObservedFifthLodWord7;

    const bool exactFifthObservedTuple =
        obj == kObservedFifthWrapObj &&
        wrapS == kObservedWrapS &&
        wrapT == kObservedWrapT &&
        word0 == kObservedWrapWord0Before &&
        word1 == kObservedLodWord1After &&
        word2 == kObservedSeventhLodWord2 &&
        word3 == kObservedSeventhLodWord3 &&
        word4 == kObservedSeventhLodWord4 &&
        word5 == kObservedSeventhLodWord5 &&
        word6 == kObservedSeventhLodWord6 &&
        word7 == kObservedSeventhLodWord7;

    const bool exactSixthObservedTuple =
        obj == kObservedSixthWrapObj &&
        wrapS == kObservedWrapS &&
        wrapT == kObservedWrapT &&
        word0 == kObservedWrapWord0Before &&
        word1 == kObservedLodWord1After &&
        word2 == kObservedSixthLodWord2 &&
        word3 == kObservedSixthLodWord3 &&
        word4 == kObservedSixthLodWord4 &&
        word5 == kObservedSixthLodWord5 &&
        word6 == kObservedSixthLodWord6 &&
        word7 == kObservedSixthLodWord7;

    const bool exactSeventhObservedTuple =
        obj == kObservedSeventhWrapObj &&
        wrapS == kObservedWrapS &&
        wrapT == kObservedWrapT &&
        word0 == kObservedWrapWord0Before &&
        word1 == kObservedLodWord1After &&
        word2 == kObservedEighthLodWord2 &&
        word3 == kObservedEighthLodWord3 &&
        word4 == kObservedEighthLodWord4 &&
        word5 == kObservedEighthLodWord5 &&
        word6 == kObservedEighthLodWord6 &&
        word7 == kObservedEighthLodWord7;

    const bool exactEighthObservedTuple =
        obj == kObservedEighthWrapObj &&
        wrapS == kObservedWrapS &&
        wrapT == kObservedWrapT &&
        word0 == kObservedWrapWord0Before &&
        word1 == kObservedLodWord1After &&
        word2 == kObservedTenthLodWord2 &&
        word3 == kObservedTenthLodWord3 &&
        word4 == kObservedTenthLodWord4 &&
        word5 == kObservedTenthLodWord5 &&
        word6 == kObservedTenthLodWord6 &&
        word7 == kObservedTenthLodWord7;

    const bool exactNinthObservedTuple =
        obj == kObservedNinthWrapObj &&
        wrapS == kObservedWrapS &&
        wrapT == kObservedWrapT &&
        word0 == kObservedWrapWord0Before &&
        word1 == kObservedLodWord1After &&
        word2 == kObservedEleventhLodWord2 &&
        word3 == kObservedEleventhLodWord3 &&
        word4 == kObservedEleventhLodWord4 &&
        word5 == kObservedEleventhLodWord5 &&
        word6 == kObservedEleventhLodWord6 &&
        word7 == kObservedEleventhLodWord7;

    const bool exactTenthObservedTuple =
        obj == kObservedTenthWrapObj &&
        wrapS == kObservedWrapS &&
        wrapT == kObservedWrapT &&
        word0 == kObservedWrapWord0Before &&
        word1 == kObservedLodWord1After &&
        word2 == kObservedFourthLodWord2 &&
        word3 == kObservedFourthLodWord3 &&
        word4 == kObservedFourthLodWord4 &&
        word5 == kObservedFourthLodWord5 &&
        word6 == kObservedFourthLodWord6 &&
        word7 == kObservedFourthLodWord7;

    const bool exactEleventhObservedTuple =
        obj == kObservedEleventhWrapObj &&
        wrapS == kObservedWrapS &&
        wrapT == kObservedWrapT &&
        word0 == kObservedWrapWord0Before &&
        word1 == kObservedLodWord1After &&
        word2 == kObservedThirteenthLodWord2 &&
        word3 == kObservedThirteenthLodWord3 &&
        word4 == kObservedThirteenthLodWord4 &&
        word5 == kObservedThirteenthLodWord5 &&
        word6 == kObservedThirteenthLodWord6 &&
        word7 == kObservedThirteenthLodWord7;

    const bool exactTwelfthObservedTuple =
        obj == kObservedTwelfthWrapObj &&
        wrapS == kObservedWrapS &&
        wrapT == kObservedWrapT &&
        word0 == kObservedWrapWord0Before &&
        word1 == kObservedLodWord1After &&
        word2 == kObservedFourteenthLodWord2 &&
        word3 == kObservedFourteenthLodWord3 &&
        word4 == kObservedFourteenthLodWord4 &&
        word5 == kObservedFourteenthLodWord5 &&
        word6 == kObservedFourteenthLodWord6 &&
        word7 == kObservedFourteenthLodWord7;

    const bool exactThirteenthObservedTuple =
        obj == kObservedThirteenthWrapObj &&
        wrapS == kObservedWrapS &&
        wrapT == kObservedWrapT &&
        word0 == kObservedWrapWord0Before &&
        word1 == kObservedLodWord1After &&
        word2 == kObservedTwelfthLodWord2 &&
        word3 == kObservedTwelfthLodWord3 &&
        word4 == kObservedTwelfthLodWord4 &&
        word5 == kObservedTwelfthLodWord5 &&
        word6 == kObservedTwelfthLodWord6 &&
        word7 == kObservedTwelfthLodWord7;

    const bool knownObservedTuple =
        exactFirstObservedTuple ||
        exactSecondObservedTuple ||
        exactThirdObservedTuple ||
        exactFourthObservedTuple ||
        exactFifthObservedTuple ||
        exactSixthObservedTuple ||
        exactSeventhObservedTuple ||
        exactEighthObservedTuple ||
        exactNinthObservedTuple ||
        exactTenthObservedTuple ||
        exactEleventhObservedTuple ||
        exactTwelfthObservedTuple ||
        exactThirteenthObservedTuple;

    if (!IsValidWrapMode(wrapS) || !IsValidWrapMode(wrapT)) {
        AbortWrapBoundary(
            "GX_INIT_TEX_OBJ_WRAP_MODE_INVALID_ARGS",
            cpu,
            obj,
            wrapS,
            wrapT,
            word0);
    }

    if (!IsStructurallyValidGuestTexObj(
            word0, word2, word3, word5, word7)) {
        AbortWrapBoundary(
            "GX_INIT_TEX_OBJ_WRAP_MODE_INVALID_DESCRIPTOR",
            cpu,
            obj,
            wrapS,
            wrapT,
            word0);
    }

    if (kStrictObservedTextureTuples && !knownObservedTuple) {
        AbortWrapBoundary(
            "GX_INIT_TEX_OBJ_WRAP_MODE_UNPROVEN_TUPLE",
            cpu,
            obj,
            wrapS,
            wrapT,
            word0);
    }

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    try {
        std::scoped_lock lock(gTexObjMutex);
        GXTexObj* hostObj = FindLocalHostTexObj(obj);
        if (!hostObj) {
            AbortWrapBoundary(
                "GX_INIT_TEX_OBJ_WRAP_MODE_HOST_OBJ_MISSING",
                cpu,
                obj,
                wrapS,
                wrapT,
                word0);
        }
        GXInitTexObjWrapMode(
            hostObj,
            static_cast<GXTexWrapMode>(wrapS),
            static_cast<GXTexWrapMode>(wrapT));
    } catch (...) {
        AbortWrapBoundary(
            "GX_INIT_TEX_OBJ_WRAP_MODE_HOST_EXCEPTION",
            cpu,
            obj,
            wrapS,
            wrapT,
            word0);
    }
#endif

    const std::uint32_t updatedWord0 =
        (word0 & ~0xFu) |
        (wrapS & 0x3u) |
        ((wrapT & 0x3u) << 2u);
    Memory::Write32(obj + 0x00u, updatedWord0);

    if (Memory::Read32(obj + 0x00u) != updatedWord0 ||
        Memory::Read32(obj + 0x04u) != word1) {
        AbortWrapBoundary(
            "GX_INIT_TEX_OBJ_WRAP_MODE_GUEST_STATE_MISMATCH",
            cpu,
            obj,
            wrapS,
            wrapT,
            word0);
    }

    WriteWrapStatus("wrap-pass", obj, wrapS, wrapT, word0);
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

    const bool exactOriginalDescriptor =
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

    // Pinned GXLoadTexObj guards eight binding slots, and Aurora indexes
    // eight-entry register tables. Forward each legal slot for this exact
    // observed IA8 descriptor; null/disable/out-of-range IDs still abort.
    const bool exactIa8Descriptor =
        obj == kObservedIa8LoadObj &&
        tid < 8u &&
        word0 == 0x00000190u &&
        word1 == 0x00000000u &&
        word2 == 0x00300C03u &&
        word3 == 0x0001C22Au &&
        word4 == 0x00000000u &&
        word5 == 0x00000003u &&
        word6 == 0x00000000u &&
        word7 == 0x00010202u &&
        data == kObservedIa8Data &&
        width == 4u && height == 4u &&
        format == 3u && formatWord2 == 3u &&
        wrapS == 0u && wrapT == 0u && mipmap == 0u;

    const bool exactMiiI4Descriptor =
        (obj == kObservedMiiI4LoadObj || obj == kObservedSecondMiiI4LoadObj ||
         obj == kObservedNextPassMiiI4LoadObj || obj == kObservedSecondNextPassMiiI4LoadObj) &&
        tid == 0u &&
        word0 == 0x00000190u &&
        word1 == 0x00000000u &&
        word2 == 0x0000FC1Fu &&
        ((word3 == 0x0084E0D2u && data == kObservedMiiI4Data) ||
         ((obj == kObservedMiiI4LoadObj || obj == kObservedSecondMiiI4LoadObj) &&
          word3 == 0x0084E0D1u &&
          data == kObservedRelocatedMiiI4Data)) &&
        word4 == 0x00000000u &&
        word5 == 0x00000000u &&
        word6 == 0x00000000u &&
        word7 == 0x00200102u &&
        width == 32u && height == 64u &&
        format == 0u && formatWord2 == 0u &&
        wrapS == 0u && wrapT == 0u && mipmap == 0u;

    const bool exactMiiRgb5a3Descriptor =
        (obj == kObservedMiiRgb5a3LoadObj || obj == kObservedNextPassMiiRgb5a3LoadObj) &&
        tid == 0u &&
        word0 == 0x00000190u &&
        word1 == 0x00000000u &&
        word2 == 0x00507C2Bu &&
        word3 == 0x0084E062u &&
        word4 == 0x00000000u &&
        word5 == 0x00000005u &&
        word6 == 0x00000000u &&
        word7 == 0x00580202u &&
        data == kObservedMiiRgb5a3Data &&
        width == 44u && height == 32u &&
        format == 5u && formatWord2 == 5u &&
        wrapS == 0u && wrapT == 0u && mipmap == 0u;

    const bool exactMiiSmallI4Descriptor =
        (obj == kObservedMiiSmallI4LoadObj || obj == kObservedSecondMiiSmallI4LoadObj) &&
        tid == 0u &&
        word0 == 0x00000190u &&
        word1 == 0x00000000u &&
        word2 == 0x00007C23u &&
        word3 == 0x0084E0BCu &&
        word4 == 0x00000000u &&
        word5 == 0x00000000u &&
        word6 == 0x00000000u &&
        word7 == 0x00140102u &&
        data == kObservedMiiSmallI4Data &&
        width == 36u && height == 32u &&
        format == 0u && formatWord2 == 0u &&
        wrapS == 0u && wrapT == 0u && mipmap == 0u;

    const bool exactMiiSmallRgb5a3Descriptor =
        (obj == kObservedMiiSmallRgb5a3LoadObj || obj == kObservedSecondMiiSmallRgb5a3LoadObj) &&
        tid == 0u &&
        word0 == 0x00000190u &&
        word1 == 0x00000000u &&
        word2 == 0x00507C25u &&
        word3 == 0x0084E010u &&
        word4 == 0x00000000u &&
        word5 == 0x00000005u &&
        word6 == 0x00000000u &&
        word7 == 0x00500202u &&
        data == kObservedMiiSmallRgb5a3Data &&
        width == 38u && height == 32u &&
        format == 5u && formatWord2 == 5u &&
        wrapS == 0u && wrapT == 0u && mipmap == 0u;

    const bool exactMiiTinyI4Descriptor =
        obj == kObservedMiiTinyI4LoadObj &&
        tid == 0u &&
        word0 == 0x00000190u &&
        word1 == 0x00000000u &&
        word2 == 0x00003C0Fu &&
        word3 == 0x0084E0F4u &&
        word4 == 0x00000000u &&
        word5 == 0x00000000u &&
        word6 == 0x00000000u &&
        word7 == 0x00040102u &&
        data == kObservedMiiTinyI4Data &&
        width == 16u && height == 16u &&
        format == 0u && formatWord2 == 0u &&
        wrapS == 0u && wrapT == 0u && mipmap == 0u;

    // Unknown descriptors have no proven size. Every admitted descriptor
    // checks its own complete tiled range before allocating or calling Aurora.
    const std::uint32_t textureSize = exactOriginalDescriptor ? kObservedTextureSize : (exactIa8Descriptor ? kObservedIa8TextureSize : (exactMiiI4Descriptor ? kObservedMiiI4TextureSize : (exactMiiRgb5a3Descriptor ? kObservedMiiRgb5a3TextureSize : (exactMiiSmallI4Descriptor ? kObservedMiiSmallI4TextureSize : (exactMiiSmallRgb5a3Descriptor ? kObservedMiiSmallRgb5a3TextureSize : (exactMiiTinyI4Descriptor ? kObservedMiiTinyI4TextureSize : 0u))))));
    if (!exactOriginalDescriptor && !exactIa8Descriptor && !exactMiiI4Descriptor && !exactMiiRgb5a3Descriptor && !exactMiiSmallI4Descriptor && !exactMiiSmallRgb5a3Descriptor && !exactMiiTinyI4Descriptor) {
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
            textureSize);
    }

    if (!Memory::Contains(data, textureSize)) {
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
            textureSize);
    }

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    try {
        std::scoped_lock lock(gTexObjMutex);
        GXTexObj* hostObj = GetOrCreateHostTexObj(obj);
        const void* hostData =
            GuestToHostPtr(data, textureSize);
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
                textureSize);
        }

        GXInitTexObj(
            hostObj,
            hostData,
            static_cast<std::uint16_t>(width),
            static_cast<std::uint16_t>(height),
            static_cast<GXTexFmt>(format),
            GX_CLAMP,
            GX_CLAMP,
            GX_FALSE);
        GXInitTexObjLOD(
            hostObj,
            GX_LINEAR,
            GX_LINEAR,
            0.0f,
            0.0f,
            0.0f,
            GX_FALSE,
            ((word0 >> 8u) & 1u) == 0u ? GX_TRUE : GX_FALSE,
            GX_ANISO_1);
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
            textureSize);
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
        textureSize);
}

#endif
