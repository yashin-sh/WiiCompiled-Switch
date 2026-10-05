#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "memory.h"
#include "switch_gx_hle_traits.hpp"
#include "gx_internal.h"
#include "dolphin/gx/__gx.h"

#include <algorithm>
#include <cassert>
#include <csignal>
#include <cmath>
#include <array>
#include <limits>
#include <type_traits>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <string>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace aurora::gx::fifo::detail {
std::uint8_t* sBufferData = nullptr;
std::uint32_t sBufferSize = 0, sBufferCapacity = 0;
bool sInDisplayList = false;
std::uint8_t* sDlBuffer = nullptr;
std::uint32_t sDlSize = 0, sDlWritePos = 0;
} // namespace aurora::gx::fifo::detail
namespace aurora::gx::fifo {
void write_data_grow(const void*, std::uint32_t) {
    assert(false);
}
} // namespace aurora::gx::fifo
__GXData_struct nativeState{};
__GXData_struct* __gx = &nativeState;
HleGxState g_hleGxState{};
bool g_alphaCompareValid = false;
GxDisplayListState g_dlRecordState{};
// Cache comparisons only: recording always emits VCD/VAT regardless of cache.
struct HostAttr {
    GXCompCnt cnt{};
    GXCompType type{};
    u8 frac{};
};
struct HostFormat {
    HostAttr attrs[GX_VA_MAX_ATTR]{};
};
struct HostCache {
    GXAttrType sourceVtxDesc[GX_VA_MAX_ATTR]{};
    GXAttrType vtxDesc[GX_VA_MAX_ATTR]{};
    HostFormat vtxFmts[8]{};
} g_gxState;
template <class T>
constexpr auto underlying(T value) {
    return static_cast<std::underlying_type_t<T>>(value);
}
struct HostLog {
    template <class... T>
    void warn(T...) {
        assert(false);
    }
} Log;
namespace aurora::gx::fifo {
u32 get_buffer_size() {
    return detail::sBufferSize;
} // Actual live-only pinned accessor.
void drain() {
    assert(false);
} // This contract only draws into display lists.
} // namespace aurora::gx::fifo
extern "C" void __GXUpdateBPMask() {
    assert(false);
}
#include "pinned-sphere.inc"
#include "pinned-display-list.inc"
namespace {
constexpr std::uint32_t list = 0x80394F00u;
constexpr std::uint32_t gd = 0x803437C0u;
struct Region {
    std::uint32_t base;
    std::vector<std::uint8_t> bytes;
};
std::vector<Region> regions;
unsigned refusals = 0, cases = 0, liveBursts = 0;
int refusalPipe = -1;
std::vector<std::vector<std::uint8_t>> frozenRegions;
__GXData_struct frozenNative{};
GxDisplayListState frozenDl{};
HleGxState frozenHle{};
std::uint32_t frozenCursor = 0;
bool frozenAlphaCompareValid = false;
CpuContext* activeCpu = nullptr;
CpuContext frozenCpu{};
const char* stage = nullptr;
std::uint8_t mainFifo[1024]{};
std::vector<std::uint8_t> frozenMainFifo;
std::uint32_t frozenMainSize = 0;

void Reset(std::uint32_t listSize = 16416) {
    Memory::Config c;
    c.regions = {{"metadata", 0x80343000u, 0x2000u}, {"pointer", kGXDataPtrAddr, 4}, {"list-with-sentinel", list - 32u, listSize + 32u}};
    Memory::Init(c);
    for (auto& r : regions)
        std::fill(r.bytes.begin(), r.bytes.end(), 0xa5);
    std::memset(Memory::GetPointer(gd, 0x600), 0, 0x600);
    Memory::Write32(kGXDataPtrAddr, gd);
    Memory::Write8(gd + 0x5F9u, 1);
    nativeState = {};
    g_alphaCompareValid = false;
    nativeState.dlSaveContext = 1;
    std::fill(std::begin(nativeState.texmapId), std::end(nativeState.texmapId), 0xFFu);
    for (unsigned i = 0; i < 8; ++i) {
        nativeState.suTs0[i] = (0x30u + i * 2u) << 24u;
        nativeState.suTs1[i] = (0x31u + i * 2u) << 24u;
    }
    g_hleGxState = {};
    g_dlRecordState = {};
    using namespace aurora::gx::fifo::detail;
    sInDisplayList = false;
    sDlBuffer = nullptr;
    sDlSize = sDlWritePos = 0;
    sBufferData = mainFifo;
    sBufferCapacity = sizeof(mainFifo);
    sBufferSize = 0;
}
CpuContext Cpu(std::uint32_t size = 16384) {
    CpuContext c;
    std::memset(&c, 0xa5, sizeof(c));
    c.gpr[3] = list;
    c.gpr[4] = size;
    return c;
}
void Begin(CpuContext& c) {
    KnownNativeCpuCall<0x80172E00u>::Invoke(&c);
}
void End(CpuContext& c) {
    KnownNativeCpuCall<0x80172EB4u>::Invoke(&c);
}
std::string Report(const char* name) {
    std::ifstream f(std::string("sdmc:/switch/WiiCompiled-Switch/") + name);
    return std::string(std::istreambuf_iterator<char>(f), {});
}
void Refusal(const std::function<void()>& f, const char* reason = nullptr) {
    if (activeCpu)
        std::memcpy(&frozenCpu, activeCpu, sizeof(frozenCpu));
    frozenRegions.clear();
    for (const auto& r : regions)
        frozenRegions.push_back(r.bytes);
    std::memcpy(&frozenNative, &nativeState, sizeof(nativeState));
    std::memcpy(&frozenDl, &g_dlRecordState, sizeof(g_dlRecordState));
    std::memcpy(&frozenHle, &g_hleGxState, sizeof(g_hleGxState));
    frozenCursor = aurora::gx::fifo::detail::sDlWritePos;
    frozenAlphaCompareValid = g_alphaCompareValid;
    frozenMainSize = aurora::gx::fifo::detail::sBufferSize;
    frozenMainFifo.assign(std::begin(mainFifo), std::end(mainFifo));
    int p[2]{};
    assert(pipe(p) == 0);
    const auto child = fork();
    assert(child >= 0);
    if (child == 0) {
        assert(prctl(PR_SET_DUMPABLE, 0) == 0);
        close(p[0]);
        refusalPipe = p[1];
        f();
        _exit(91);
    }
    close(p[1]);
    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    char mark = 0;
    assert(read(p[0], &mark, 1) == 1 && mark == 'R');
    close(p[0]);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    if (reason)
        assert(Report("fast-track-gx-display-list.txt").find(reason) != std::string::npos);
    ++refusals;
}
} // namespace
namespace GuestFlat {
bool IsActive() {
    return !regions.empty();
}
void Initialize(const std::vector<RegionRequest>& requests) {
    regions.clear();
    for (const auto& r : requests)
        regions.push_back({r.base, std::vector<std::uint8_t>(r.size)});
}
std::uint8_t* HostPointer(std::uint32_t ptr) {
    for (auto& r : regions)
        if (ptr >= r.base && std::uint64_t(ptr) < std::uint64_t(r.base) + r.bytes.size())
            return r.bytes.data() + ptr - r.base;
    return nullptr;
}
void Shutdown() noexcept {
    regions.clear();
}
} // namespace GuestFlat
extern "C" [[noreturn]] void __real_abort();
extern "C" [[noreturn]] void __wrap_abort() {
    if (refusalPipe >= 0) {
        if (activeCpu && std::memcmp(activeCpu, &frozenCpu, sizeof(frozenCpu)) != 0)
            _exit(97);
        if (regions.size() != frozenRegions.size() ||
            std::memcmp(&nativeState, &frozenNative, sizeof(nativeState)) != 0 ||
            std::memcmp(&g_dlRecordState, &frozenDl, sizeof(g_dlRecordState)) != 0 ||
            std::memcmp(&g_hleGxState, &frozenHle, sizeof(g_hleGxState)) != 0 ||
            aurora::gx::fifo::detail::sDlWritePos != frozenCursor || g_alphaCompareValid != frozenAlphaCompareValid)
            _exit(94);
        if (frozenMainSize != aurora::gx::fifo::detail::sBufferSize ||
            !std::equal(frozenMainFifo.begin(), frozenMainFifo.end(), std::begin(mainFifo)))
            _exit(98);
        for (std::size_t i = 0; i < regions.size(); ++i)
            if (regions[i].bytes != frozenRegions[i])
                _exit(95);
        if (write(refusalPipe, "R", 1) != 1)
            _exit(96);
    }
    __real_abort();
}
extern "C" void mkw_switch_set_fast_track_stage(const char* s) noexcept {
    stage = s;
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char*, std::uint32_t, CpuContext*) noexcept {}
extern "C" void mkw_switch_pinned_fifo_write_burst(const std::uint8_t* data, std::uint32_t size) {
    assert(data && size);
    ++liveBursts;
}

extern "C" void mkw_switch_gx_record_begin(CpuContext*) noexcept;

int main() {
    Reset();
    mkw_switch_hle_gx_begin_display_list(nullptr);
    mkw_switch_hle_gx_end_display_list(nullptr);
    assert(!g_dlRecordState.active);
    auto c = Cpu();
    activeCpu = &c;
#if MKW_LOCAL_RENDERED_FAST_TRACK
    const auto before = c;
    nativeState.dirtyState = 4;
    Begin(c);
    assert(std::memcmp(&c, &before, sizeof(c)) == 0);
    assert(aurora::gx::fifo::detail::sBufferSize == 5);
    assert(mainFifo[0] == 0x61); // Begin's dirty flush precedes recording.
    assert(Report("fast-track-gx-display-list.txt").find("status=begin-pass") != std::string::npos);
    auto primitiveCpu = Cpu();
    primitiveCpu.gpr[3] = GX_TRIANGLES;
    primitiveCpu.gpr[4] = 0;
    primitiveCpu.gpr[5] = 2;
    mkw_switch_gx_record_begin(&primitiveCpu);
    aurora::gx::fifo::write_u32(0x3f800000); // Native and guest use one cursor.
    const std::uint8_t burst[] = {0x40, 0, 0, 0, 0x41};
    GX_HLE_FIFO_WriteBurst(burst, sizeof(burst));
    assert(g_dlRecordState.count == 12);
    assert(aurora::gx::fifo::detail::sDlWritePos == 12);
    const auto savedNative = nativeState;
    nativeState.genMode = 0x87654321;
    g_hleGxState.vtxDesc[GX_VA_POS] = GX_DIRECT;
    Memory::Write32(gd, 0x11223344);
    Memory::Write32(gd + 8u, 0x55667788);
    End(c);
    auto expected = before;
    expected.gpr[3] = 32;
    assert(std::memcmp(&c, &expected, sizeof(c)) == 0);
    assert(!g_dlRecordState.active && !aurora::gx::fifo::in_display_list());
    assert(nativeState.genMode == savedNative.genMode);
    assert(g_hleGxState.vtxDesc[GX_VA_POS] == GX_NONE);
    assert(Memory::Read32(gd) == 0 && Memory::Read32(gd + 8u) == 0x55667788);
    assert(Memory::Read8(gd + 0x5F8u) == 0);
    assert(Memory::Read32(kDlCountAddr) == 32 && Memory::Read32(kDlWritePtrAddr) == list + 32u);
    const std::uint8_t packet[] = {0x90, 0, 2, 0x3f, 0x80, 0, 0, 0x40, 0, 0, 0, 0x41};
    assert(std::memcmp(Memory::GetPointer(list, 12), packet, 12) == 0);
    for (unsigned i = 12; i < 32; ++i)
        assert(Memory::Read8(list + i) == 0);
    assert(Memory::Read8(list - 1u) == 0xa5 && Memory::Read8(list + 32u) == 0xa5);
    GX_HLE_FIFO_WriteBurst(burst, sizeof(burst));
    assert(liveBursts == 1); // Decoder remains selected after End.
    ++cases;

    // Real AlphaCompare bridge and pinned native BP emission: the producer's
    // validity flag follows the same save policy as its restored GX shadow.
    for (const unsigned save : {0u, 1u}) {
        for (const bool initial : {false, true}) {
            Reset();
            c = Cpu(32);
            Memory::Write8(gd + 0x5F9u, save);
            g_alphaCompareValid = initial;
            Begin(c);
            auto alphaCpu = Cpu();
            alphaCpu.gpr[3] = alphaCpu.gpr[6] = GX_ALWAYS;
            alphaCpu.gpr[4] = alphaCpu.gpr[5] = alphaCpu.gpr[7] = 0;
            const auto alphaBefore = alphaCpu;
            KnownNativeCpuCall<0x80172088u>::Invoke(&alphaCpu);
            assert(std::memcmp(&alphaCpu, &alphaBefore, sizeof(alphaCpu)) == 0);
            assert(g_alphaCompareValid);
            assert(Memory::Read8(list) == 0x61 && Memory::Read32(list + 1u) == 0xF33F0000u);
            End(c);
            assert(c.gpr[3] == 32);
            assert(g_alphaCompareValid == (save ? initial : true));
            ++cases;
        }
    }

    // Actual pinned SU size/bias emission, before Begin and within End, on every
    // coordinate, legal wrap mode and save policy. Native GX is authoritative;
    // only coordinates emitted by its active TEV/indirect references are mirrored.
    for (const bool atEnd : {false, true}) {
        for (unsigned coord = 0; coord < 8; ++coord) {
            for (unsigned sw = 0; sw < 3; ++sw) {
                for (unsigned tw = 0; tw < 3; ++tw) {
                    for (unsigned save = 0; save < 2; ++save) {
                        Reset();
                        c = Cpu();
                        Memory::Write8(gd + 0x5F9u, save);
                        Memory::Write16(gd + 2u, 0x7e7e);
                        nativeState.texmapId[0] = (7u - coord) | 0x100u;
                        nativeState.texmapValid = 1;
                        nativeState.tref[0] = coord << 3u;
                        const auto map = 7u - coord;
                        const unsigned widthMinus1 = (coord & 1u) ? 1023u : 0u;
                        const unsigned heightMinus1 = (coord & 2u) ? 1023u : coord;
                        nativeState.tImage0[map] = widthMinus1 | (heightMinus1 << 10u);
                        nativeState.tMode0[map] = sw | (tw << 2u);
                        nativeState.suTs0[coord] |= 0x700ffu;
                        nativeState.suTs1[coord] |= 0x7aaffu;
                        for (unsigned i = 0; i < 8; ++i) {
                            Memory::Write32(gd + 0x108u + 4u * i, 0xDEAD0000u + i);
                            Memory::Write32(gd + 0x128u + 4u * i, 0xBEEF0000u + i);
                        }
                        const auto cpuBefore = c;
                        std::vector<std::uint8_t> guestBefore(Memory::GetPointer(gd, 0x600),
                                                              Memory::GetPointer(gd, 0x600) + 0x600);
                        if (!atEnd) {
                            Memory::Write32(gd + 0x5FCu, 1);
                        }
                        Begin(c);
                        assert(std::memcmp(&c, &cpuBefore, sizeof(c)) == 0);
                        if (atEnd) {
                            Memory::Write32(gd + 0x5FCu, 1);
                        }
                        const auto sWord = ((0x30u + coord * 2u) << 24u) | 0x60000u |
                                           ((sw == 1u) ? 0x10000u : 0u) | widthMinus1;
                        const auto tWord = ((0x31u + coord * 2u) << 24u) | 0x60000u |
                                           ((tw == 1u) ? 0x10000u : 0u) | heightMinus1;
                        if (!atEnd) {
                            assert(Memory::Read32(gd + 0x108u + 4u * coord) == sWord);
                            assert(Memory::Read32(gd + 0x128u + 4u * coord) == tWord);
                            assert(Memory::Read16(gd + 2u) == 0);
                            assert(Memory::Read32(gd + 0x5FCu) == 0);
                            guestBefore.assign(Memory::GetPointer(gd, 0x600),
                                               Memory::GetPointer(gd, 0x600) + 0x600);
                            guestBefore[0x5F8] = 0;
                        }
                        End(c);
                        auto cpuAfter = cpuBefore;
                        cpuAfter.gpr[3] = atEnd ? 32u : 0u;
                        assert(std::memcmp(&c, &cpuAfter, sizeof(c)) == 0);
                        if (atEnd && !save) {
                            assert(Memory::Read32(gd + 0x108u + 4u * coord) == sWord);
                            assert(Memory::Read32(gd + 0x128u + 4u * coord) == tWord);
                            assert(Memory::Read16(gd + 2u) == 0);
                            assert(Memory::Read32(gd + 0x5FCu) == 0);
                            const auto beWrite = [&](unsigned offset, unsigned value) {
                                for (unsigned i = 0; i < 4; ++i)
                                    guestBefore[offset + i] = value >> (24u - 8u * i);
                            };
                            beWrite(0x108u + 4u * coord, sWord);
                            beWrite(0x128u + 4u * coord, tWord);
                            beWrite(0x5FCu, 0);
                            guestBefore[2] = guestBefore[3] = 0;
                            assert(std::memcmp(Memory::GetPointer(gd, 0x600), guestBefore.data(), 0x600) == 0);
                        } else {
                            assert(std::memcmp(Memory::GetPointer(gd, 0x600), guestBefore.data(), 0x600) == 0);
                        }
                        for (unsigned i = 0; i < 8; ++i) {
                            if (i != coord) {
                                assert(Memory::Read32(gd + 0x108u + 4u * i) == 0xDEAD0000u + i);
                                assert(Memory::Read32(gd + 0x128u + 4u * i) == 0xBEEF0000u + i);
                            }
                        }
                        auto* packetData = atEnd ? Memory::GetPointer(list, 32) : mainFifo;
                        assert(packetData[0] == 0x61 && packetData[5] == 0x61);
                        const auto readBe = [](const std::uint8_t* bytes) {
                            return (std::uint32_t(bytes[0]) << 24u) | (std::uint32_t(bytes[1]) << 16u) |
                                   (std::uint32_t(bytes[2]) << 8u) | bytes[3];
                        };
                        assert(readBe(packetData + 1) == sWord && readBe(packetData + 6) == tWord);
                        assert(aurora::gx::fifo::detail::sBufferSize == (atEnd ? 0u : 10u));
                        assert(Memory::Read8(list + (atEnd ? 32u : 0u)) == 0xa5);
                        assert(Report("fast-track-gx-su-flush.txt").find(atEnd ? "phase=end" : "phase=begin") != std::string::npos);
                        ++cases;
                    }
                }
            }
        }
    }

    // Four indirect layouts plus direct odd/even stages; repeated coordinates
    // preserve native ordering and publish the final shadow value once.
    for (unsigned indirect = 0; indirect <= 4; ++indirect) {
        Reset();
        c = Cpu();
        nativeState.genMode = (indirect << 16u) | (1u << 10u);
        nativeState.texmapValid = 3;
        nativeState.texmapId[0] = 4;
        nativeState.texmapId[1] = 5;
        nativeState.tref[0] = (0u << 3u) | (7u << 15u);
        for (unsigned i = 0; i < 8; ++i) {
            nativeState.tImage0[i] = (i + 10u) | ((i + 20u) << 10u);
            nativeState.tMode0[i] = 1u | (2u << 2u);
        }
        for (unsigned i = 0; i < indirect; ++i)
            nativeState.iref |= i << (6u * i) | i << (6u * i + 3u);
        Memory::Write32(gd + 0x5FCu, 1);
        Begin(c);
        assert(aurora::gx::fifo::detail::sBufferSize == 10u * (indirect + 2u));
        assert(Memory::Read32(gd + 0x108u) == 0x3001000Eu); // Direct map 4 wins over indirect map 0.
        assert(Memory::Read32(gd + 0x108u + 4u * 7u) == 0x3E01000Fu);
        End(c);
        assert(c.gpr[3] == 0);
        ++cases;
    }

    // Manual, null-map and disabled-coordinate paths emit nothing and preserve
    // guest SU/BP fields, while consuming only the handled dirty marker.
    for (const unsigned kind : {0u, 1u, 2u}) {
        Reset();
        c = Cpu();
        nativeState.texmapId[0] = kind == 1u ? 0xFFu : 0u;
        nativeState.texmapValid = kind == 2u ? 0u : 1u;
        if (kind == 0u) {
            nativeState.tcsManEnab = 0xFF;
            Memory::Write32(gd + 0x5E4u, 0xFF);
        }
        Memory::Write16(gd + 2u, 0x2345);
        Memory::Write32(gd + 0x108u, 0x11223344);
        Memory::Write32(gd + 0x5FCu, 1);
        Begin(c);
        assert(aurora::gx::fifo::detail::sBufferSize == 0);
        assert(Memory::Read32(gd + 0x108u) == 0x11223344 && Memory::Read16(gd + 2u) == 0x2345);
        assert(Memory::Read32(gd + 0x5FCu) == 0);
        End(c);
        ++cases;
    }
    Reset();
    c = Cpu();
    Memory::Write32(gd + 0x5FCu, 1);
    nativeState.texmapId[0] = 0;
    nativeState.texmapValid = 1;
    nativeState.dirtyState = 5; // SU handled separately; gen mode remains for native Begin.
    Begin(c);
    assert(aurora::gx::fifo::detail::sBufferSize == 15 && mainFifo[10] == 0x61);
    assert(nativeState.dirtyState == 0);
    End(c);
    ++cases;

    // Validation failures must happen before SU emission or guest mutation.
    Reset();
    c = Cpu();
    Memory::Write32(gd + 0x5FCu, 1);
    Memory::Write32(gd + 0x5E4u, 1);
    Refusal([&] { Begin(c); }, "GX_DISPLAY_LIST_NATIVE_SU_STATE");
    Memory::Write32(gd + 0x5E4u, 256);
    Refusal([&] { Begin(c); }, "GX_DISPLAY_LIST_NATIVE_SU_STATE");
    Memory::Write32(gd + 0x5E4u, 0);
    nativeState.texmapValid = 1;
    nativeState.texmapId[0] = 8;
    Refusal([&] { Begin(c); }, "GX_DISPLAY_LIST_NATIVE_SU_STATE");
    nativeState.texmapId[0] = 0;
    nativeState.genMode = 5u << 16u;
    Refusal([&] { Begin(c); }, "GX_DISPLAY_LIST_NATIVE_SU_STATE");
    nativeState.genMode = 0;
    for (const unsigned dirty : {2u, 3u, 0xFFFFFFFFu}) {
        Memory::Write32(gd + 0x5FCu, dirty);
        Refusal([&] { Begin(c); }, "GX_DISPLAY_LIST_PENDING_STATE");
    }
    Memory::Write32(gd + 0x5FCu, 0);
    Begin(c);
    Memory::Write32(gd + 0x5FCu, 2);
    Refusal([&] { End(c); }, "GX_DISPLAY_LIST_PENDING_STATE");
    Memory::Write32(gd + 0x5FCu, 1);
    nativeState.texmapId[0] = 8;
    Refusal([&] { End(c); }, "GX_DISPLAY_LIST_NATIVE_SU_STATE");
    nativeState.texmapId[0] = 0;
    End(c);

    // Exhaustive padding residues, empty lists, exact capacity, repeated lists,
    // both save flags and the hardware-observed 16 KiB stack buffer.
    for (const unsigned capacity : {32u, 64u, 16384u}) {
        for (unsigned count = 0; count <= 64 && count <= capacity; ++count) {
            for (const unsigned save : {0u, 1u}) {
                Reset();
                c = Cpu(capacity);
                Memory::Write8(gd + 0x5F9u, save);
                Begin(c);
                for (unsigned i = 0; i < count; ++i)
                    WriteDisplayListData(i, 1);
                nativeState.genMode = 123;
                g_hleGxState.vtxDesc[GX_VA_POS] = GX_DIRECT;
                Memory::Write32(gd, 456);
                End(c);
                const unsigned padded = (count + 31u) & ~31u;
                assert(c.gpr[3] == padded);
                assert(nativeState.genMode == (save ? 0u : 123u));
                assert(Memory::Read32(gd) == (save ? 0u : 456u));
                assert(g_hleGxState.vtxDesc[GX_VA_POS] == (save ? GX_NONE : GX_DIRECT));
                for (unsigned i = 0; i < count; ++i)
                    assert(Memory::Read8(list + i) == (i & 255u));
                for (unsigned i = count; i < padded; ++i)
                    assert(Memory::Read8(list + i) == 0);
                assert(Memory::Read8(list + padded) == 0xa5);
                ++cases;
            }
        }
    }
    Reset();
    c = Cpu();
    Begin(c);
    std::vector<std::uint8_t> fill(16384, 0x67);
    GX_HLE_FIFO_WriteBurst(fill.data(), fill.size());
    Refusal([&] { WriteDisplayListData(1, 1); });
    assert(Report("fast-track-gx-display-list-overflow.txt").find("requested=1") != std::string::npos);
    Refusal([&] { aurora::gx::fifo::write_u32(0x1234); });
    Refusal([&] { GX_HLE_FIFO_WriteBurst(fill.data(), 4); });
    End(c);
    assert(c.gpr[3] == 16384 && Memory::Read8(list + 16384) == 0xa5);
    ++cases;

    Reset();
    c = Cpu(32);
    Begin(c);
    for (unsigned i = 0; i < 30; ++i)
        WriteDisplayListData(i, 1);
    Refusal([&] { WriteDisplayListData(0x12345678, 4); });
    assert(Memory::Read8(list + 30) == 0xa5 && Memory::Read8(list + 32) == 0xa5);
    WriteDisplayListData(0x1234, 2);
    End(c);
    assert(c.gpr[3] == 32);
    ++cases;

    Reset();
    c = Cpu(64);
    Begin(c);
    GX_HLE_FIFO_WriteBurst(fill.data(), 16);
    GX_HLE_FIFO_WriteBurst(Memory::GetPointer(list, 16), 16); // Aliased nested bytes.
    End(c);
    assert(c.gpr[3] == 32);
    ++cases;

    Reset();
    c = Cpu();
    Begin(c);
    primitiveCpu.gpr[3] = GX_TRIANGLES;
    primitiveCpu.gpr[4] = 0;
    primitiveCpu.gpr[5] = 1;
    activeCpu = &primitiveCpu;
    nativeState.dirtyState = 4;
    mkw_switch_gx_record_begin(&primitiveCpu);
    assert(Memory::Read8(list) == 0x61 && Memory::Read8(list + 5) == GX_TRIANGLES);
    assert(Memory::Read16(list + 6) == 1);
    for (const unsigned invalid : {0u, 0x81u, 0xffu}) {
        primitiveCpu.gpr[3] = invalid;
        Refusal([&] { mkw_switch_gx_record_begin(&primitiveCpu); }, "GX_DISPLAY_LIST_PRIMITIVE");
    }
    primitiveCpu.gpr[3] = GX_TRIANGLES;
    primitiveCpu.gpr[4] = 8;
    Refusal([&] { mkw_switch_gx_record_begin(&primitiveCpu); }, "GX_DISPLAY_LIST_PRIMITIVE");
    primitiveCpu.gpr[4] = 0;
    primitiveCpu.gpr[5] = 65536;
    Refusal([&] { mkw_switch_gx_record_begin(&primitiveCpu); }, "GX_DISPLAY_LIST_PRIMITIVE");
    primitiveCpu.gpr[5] = 1;
    g_hleGxState.vtxDesc[GX_VA_POS] = GX_INDEX16;
    Refusal([&] { mkw_switch_gx_record_begin(&primitiveCpu); }, "GX_DISPLAY_LIST_INDEXED_VERTEX");
    g_hleGxState.vtxDesc[GX_VA_POS] = GX_DIRECT;
    g_hleGxState.vtxDesc[GX_VA_PNMTXIDX] = GX_DIRECT;
    Refusal([&] { mkw_switch_gx_record_begin(&primitiveCpu); }, "GX_DISPLAY_LIST_MATRIX_INDEX");
    g_hleGxState.vtxDesc[GX_VA_PNMTXIDX] = GX_NONE;
    g_hleGxState.vtxDesc[GX_VA_TEX0] = GX_DIRECT;
    mkw_switch_gx_record_begin(&primitiveCpu);
    activeCpu = &c;
    End(c);
    ++cases;

    Reset();
    c = Cpu();
    Refusal([&] { End(c); }, "GX_DISPLAY_LIST_STATE");
    for (const auto badSize : {0u, 1u, 31u, 33u, 0xffffffe0u}) {
        c = Cpu(badSize);
        Refusal([&] { Begin(c); }, "GX_DISPLAY_LIST_RANGE");
    }
    c = Cpu();
    c.gpr[3] += 1;
    Refusal([&] { Begin(c); }, "GX_DISPLAY_LIST_RANGE");
    c = Cpu();
    c.gpr[3] = 0xffffffe0u;
    Refusal([&] { Begin(c); }, "GX_DISPLAY_LIST_RANGE");
    c = Cpu();
    c.gpr[3] = gd;
    c.gpr[4] = 32;
    Refusal([&] { Begin(c); }, "GX_DISPLAY_LIST_METADATA_OVERLAP");
    c = Cpu();
    Memory::Write32(kGXDataPtrAddr, 0);
    Refusal([&] { Begin(c); }, "GX_DISPLAY_LIST_METADATA_RANGE");
    Memory::Write32(kGXDataPtrAddr, 0x80344110u);
    Refusal([&] { Begin(c); }, "GX_DISPLAY_LIST_GX_DATA_OVERLAP");
    Memory::Write32(kGXDataPtrAddr, gd);
    Memory::Write32(gd + 0x5FCu, 2);
    Refusal([&] { Begin(c); }, "GX_DISPLAY_LIST_PENDING_STATE");
    Memory::Write32(gd + 0x5FCu, 0);
    g_hleGxState.inBegin = true;
    Refusal([&] { Begin(c); }, "GX_DISPLAY_LIST_PENDING_STATE");
    g_hleGxState.inBegin = false;
    g_hleGxState.fifoByteCount = 1;
    Refusal([&] { Begin(c); }, "GX_DISPLAY_LIST_PENDING_STATE");
    g_hleGxState.fifoByteCount = 0;
    Memory::Write8(gd + 0x5F9u, 2);
    Refusal([&] { Begin(c); }, "GX_DISPLAY_LIST_SAVE_FLAG");
    Memory::Write8(gd + 0x5F9u, 1);
    Begin(c);
    Refusal([&] { Begin(c); }, "GX_DISPLAY_LIST_NESTED");
    Refusal([&] { WriteDisplayListData(0, 3); }, "GX_DISPLAY_LIST_WRITE_SIZE");
    Memory::Write8(gd + 0x5F9u, 0);
    Refusal([&] { End(c); }, "GX_DISPLAY_LIST_CONTEXT_CHANGED");
    Memory::Write8(gd + 0x5F9u, 1);
    End(c);
    Refusal([&] { WriteDisplayListData(0, 1); }, "GX_DISPLAY_LIST_STATE");
#else
    Refusal([&] { Begin(c); }, "GX_DISPLAY_LIST_REQUIRES_RENDERER");
    Refusal([&] { End(c); }, "GX_DISPLAY_LIST_REQUIRES_RENDERER");
#endif
#if MKW_LOCAL_RENDERED_FAST_TRACK
    for (unsigned major : {4u, 8u}) {
        const unsigned minor = 2u * major;
        for (unsigned texture : {0u, 1u, 2u, 3u}) {
            for (unsigned save : {0u, 1u}) {
                Reset();
                Memory::Write8(gd + 0x5F9u, save);
                GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
                GXSetVtxDesc(GX_VA_NRM, GX_INDEX8);
                GXSetVtxDesc(GX_VA_TEX0, static_cast<GXAttrType>(texture));
                GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
                GXSetVtxDesc(GX_VA_TEX7MTXIDX, GX_DIRECT);
                GXSetVtxAttrFmt(GX_VTXFMT3, GX_VA_POS, GX_POS_XY, GX_S16, 7);
                GXSetVtxAttrFmt(GX_VTXFMT3, GX_VA_NRM, GX_NRM_NBT3, GX_S8, 6);
                GXSetVtxAttrFmt(GX_VTXFMT3, GX_VA_TEX0, GX_TEX_S, GX_U16, 4);
                const auto savedNative = nativeState;
                c = Cpu();
                Begin(c);
                std::vector<u8> guestBefore(Memory::GetPointer(gd, 0x600), Memory::GetPointer(gd, 0x600) + 0x600);
                const auto hleBefore = g_hleGxState;
                c.gpr[3] = major;
                c.gpr[4] = minor;
                const auto sphereCpu = c;
                KnownNativeCpuCall<0x80172A30u>::Invoke(&c);
                assert(std::memcmp(&c, &sphereCpu, sizeof(c)) == 0);
                assert(std::memcmp(&g_hleGxState, &hleBefore, sizeof(hleBefore)) == 0);
                assert(std::equal(guestBefore.begin(), guestBefore.end(), Memory::GetPointer(gd, 0x600)));
                assert(nativeState.vcdLo == savedNative.vcdLo && nativeState.vcdHi == savedNative.vcdHi);
                assert(nativeState.vatA[3] == savedNative.vatA[3]);
                assert(nativeState.vatB[3] == savedNative.vatB[3] && nativeState.vatC[3] == savedNative.vatC[3]);
                assert(nativeState.hasNrms == savedNative.hasNrms && nativeState.hasBiNrms == savedNative.hasBiNrms);
                const auto written = g_dlRecordState.count;
                assert(written == aurora::gx::fifo::detail::sDlWritePos);
                const auto* bytes = Memory::GetPointer(list, 16384);
                // Parse independently: actual CP/XF state, strip headers, all
                // positions/normals/UVs, closure and restored-state commands.
                unsigned offset = 0, strips = 0;
                auto word = [&](unsigned at) { return (u32(bytes[at]) << 24) | (u32(bytes[at + 1]) << 16) | (u32(bytes[at + 2]) << 8) | bytes[at + 3]; };
                auto value = [&](unsigned at) { return std::bit_cast<float>(word(at)); };
                while (offset < written) {
                    const auto op = bytes[offset++];
                    if (op == 8) {
                        offset += 5;
                        continue;
                    }
                    if (op == 0x10) {
                        const auto header = word(offset);
                        offset += 4 + 4 * ((header >> 16) + 1u);
                        continue;
                    }
                    assert(op == (unsigned(GX_TRIANGLESTRIP) | unsigned(GX_VTXFMT3)));
                    const auto vertices = (unsigned(bytes[offset]) << 8) | bytes[offset + 1];
                    offset += 2;
                    assert(vertices == 2 * (minor + 1));
                    for (unsigned v = 0; v < vertices; ++v) {
                        const auto latitude = strips + ((v & 1) ? 0 : 1);
                        const float a = strips * (3.1415927f / major);
                        const float theta = (v & 1) ? a : a + 3.1415927f / major;
                        const float longitude = (v / 2) * (6.2831855f / minor);
                        const float expected[3] = {std::cos(longitude) * std::sin(theta), std::sin(longitude) * std::sin(theta), std::cos(theta)};
                        float length = 0;
                        for (unsigned k = 0; k < 3; ++k) {
                            assert(std::abs(value(offset + 4 * k) - expected[k]) < 0.000002f);
                            assert(word(offset + 4 * k) == word(offset + 12 + 4 * k));
                            length += value(offset + 4 * k) * value(offset + 4 * k);
                        }
                        assert(std::abs(length - 1.0f) < 0.000002f);
                        offset += 24;
                        if (texture) {
                            assert(std::abs(value(offset) - float(v / 2) / minor) < 0.000001f);
                            assert(std::abs(value(offset + 4) - float(latitude) / major) < 0.000001f);
                            offset += 8;
                        }
                    }
                    ++strips;
                }
                assert(offset == written && strips == major);
                assert(written == 39 + major * (3 + 2 * (minor + 1) * (texture ? 32 : 24)));
                assert(Report("fast-track-gx-sphere.txt").find("status=sphere-pass") != std::string::npos);
                End(c);
                assert(c.gpr[3] == ((written + 39 + 31) & ~31u));
                assert(bytes[written] == 8 && bytes[written + 1] == 0x50 && word(written + 2) == savedNative.vcdLo);
                for (unsigned i = written + 39; i < c.gpr[3]; ++i)
                    assert(bytes[i] == 0);
                assert(bytes[c.gpr[3]] == 0xa5 && bytes[16384] == 0xa5);
                ++cases;
            }
        }
    }
    Reset();
    c = Cpu();
    Begin(c);
    c.gpr[3] = 4;
    c.gpr[4] = 8;
    for (const auto args : {std::pair{0u, 8u}, std::pair{4u, 0u}, std::pair{260u, 8u}, std::pair{4u, 264u}, std::pair{4u, 16u}}) {
        c.gpr[3] = args.first;
        c.gpr[4] = args.second;
        Refusal([&] { KnownNativeCpuCall<0x80172A30u>::Invoke(&c); }, "GX_SPHERE_DOMAIN_OR_CAPACITY");
    }
    c.gpr[3] = 4;
    c.gpr[4] = 8;
    nativeState.dirtyState = 1;
    Refusal([&] { KnownNativeCpuCall<0x80172A30u>::Invoke(&c); }, "GX_SPHERE_DOMAIN_OR_CAPACITY");
    nativeState.dirtyState = 0;
    Memory::Write32(gd + 0x5FCu, 1);
    Refusal([&] { KnownNativeCpuCall<0x80172A30u>::Invoke(&c); }, "GX_SPHERE_CONTEXT");
    Memory::Write32(gd + 0x5FCu, 0);
    End(c);
    Refusal([&] { KnownNativeCpuCall<0x80172A30u>::Invoke(&c); }, "GX_DISPLAY_LIST_STATE");
    Reset();
    c = Cpu(32);
    Begin(c);
    c.gpr[3] = 4;
    c.gpr[4] = 8;
    Refusal([&] { KnownNativeCpuCall<0x80172A30u>::Invoke(&c); }, "GX_SPHERE_DOMAIN_OR_CAPACITY");
    End(c);
#else
    Refusal([&] { KnownNativeCpuCall<0x80172A30u>::Invoke(&c); }, "GX_SPHERE_REQUIRES_RENDERER");
#endif
    Memory::Reset();
    std::printf("PASS: GX display list rendered=%d cases=%u refusals=%u\n", MKW_LOCAL_RENDERED_FAST_TRACK, cases, refusals);
}
