#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "memory.h"
#include "switch_gx_hle_traits.hpp"
#include "gx_internal.h"
#include "dolphin/gx/__gx.h"

#include <algorithm>
#include <cassert>
#include <csignal>
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
GxDisplayListState g_dlRecordState{};
extern "C" void __GXSetDirtyState() {
    aurora::gx::fifo::write_u8(0x61);
    aurora::gx::fifo::write_u32(0x12345678);
    __gx->dirtyState = 0;
}
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
CpuContext* activeCpu = nullptr;
CpuContext frozenCpu{};
const char* stage = nullptr;
std::uint8_t mainFifo[256]{};

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
    nativeState.dlSaveContext = 1;
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
            aurora::gx::fifo::detail::sDlWritePos != frozenCursor)
            _exit(94);
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
    nativeState.dirtyState = 1;
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
    nativeState.dirtyState = 1;
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
    Memory::Write32(gd + 0x5FCu, 1);
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
    Memory::Reset();
    std::printf("PASS: GX display list rendered=%d cases=%u refusals=%u\n", MKW_LOCAL_RENDERED_FAST_TRACK, cases, refusals);
}
