#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "memory.h"
#include <array>
#include <bit>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <limits>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#if MKW_LOCAL_RENDERED_FAST_TRACK
#include "gx_internal.h"
#include "isa/big_endian.h"
#endif
namespace {
constexpr std::uint32_t target = 0x80084d20;
std::uint32_t regionBase = 0x70000000;
std::vector<std::uint8_t> bytes, savedMemory, packet, oracle;
bool active = false;
unsigned notes = 0, polls = 0, frameCalls = 0, publications = 0;
const char* lastStage = "";
CpuContext saved{};
const char* expectedReason = nullptr;
int proofFd = -1;
void SaveState();
bool Preserved();
CpuContext Cpu(unsigned count = 1, bool colors = false) {
    CpuContext cpu;
    std::memset(&cpu, 0xa5, sizeof(cpu));
    cpu.gpr[3] = regionBase + 1;
    cpu.gpr[4] = regionBase + 9;
    cpu.gpr[5] = count;
    cpu.gpr[6] = regionBase + 17;
    cpu.gpr[7] = colors ? regionBase + 273 : 0;
    cpu.gpr[8] = 255;
    return cpu;
}
void Refusal(CpuContext cpu, const char* reason, bool unknown = false) {
    saved = cpu;
    savedMemory = bytes;
    packet.clear();
    SaveState();
    expectedReason = reason;
    notes = polls = frameCalls = publications = 0;
    int ends[2];
    assert(pipe(ends) == 0);
    auto child = fork();
    assert(child >= 0);
    if (!child) {
        close(ends[0]);
        proofFd = ends[1];
        if (unknown)
            InvokeDirectCpu<0x12345678>(&cpu);
        else
            InvokeDirectCpu<target>(&cpu);
        _exit(9);
    }
    close(ends[1]);
    char proof = 0;
    assert(read(ends[0], &proof, 1) == 1 && proof == 'P');
    close(ends[0]);
    int status = 0;
    assert(waitpid(child, &status, 0) == child && WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
}
void Initialize(std::uint32_t base = 0x70000000, unsigned size = 320) {
    Memory::Reset();
    Memory::Config config;
    config.regions = {{"synthetic-layout-quad", base, size}};
    Memory::Init(config);
}
} // namespace
namespace GuestFlat {
bool IsActive() {
    return active;
}
void Initialize(const std::vector<RegionRequest>& requests) {
    assert(requests.size() == 1);
    regionBase = requests[0].base;
    bytes.assign(requests[0].size, 0xa5);
    active = true;
}
std::uint8_t* HostPointer(std::uint32_t address) {
    return address == regionBase ? bytes.data() : nullptr;
}
void Shutdown() noexcept {
    active = false;
    bytes.clear();
}
} // namespace GuestFlat
extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept {
    lastStage = stage;
}
extern "C" void mkw_switch_note_translated_dispatch(std::uint32_t address, CpuContext*) noexcept {
    assert(address == target);
    ++notes;
}
extern "C" void mkw_switch_hle_vi_poll_retrace(CpuContext*) noexcept {
    ++polls;
}
// These unrelated registry targets are never executed by this scoped contract.
#define UNEXERCISED(name)                        \
    extern "C" void name(CpuContext*) noexcept { \
        std::abort();                            \
    }
UNEXERCISED(mkw_switch_hle_gx_load_light_obj_imm)
UNEXERCISED(mkw_switch_hle_gx_load_nrm_mtx_imm)
UNEXERCISED(mkw_switch_hle_gx_set_z_texture)
UNEXERCISED(mkw_switch_hle_gx_set_tev_color_s10)
UNEXERCISED(mkw_switch_hle_nand_create)
#undef UNEXERCISED
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char* reason, std::uint32_t address, CpuContext* cpu) noexcept {
    if (!expectedReason || std::strcmp(reason, expectedReason) != 0)
        std::fprintf(stderr, "Expected refusal %s, got %s\n", expectedReason, reason);
    assert(expectedReason && std::strcmp(reason, expectedReason) == 0);
    assert(address == (std::strcmp(reason, "DIRECT") == 0 ? 0x12345678u : target));
    assert(std::memcmp(cpu, &saved, sizeof(saved)) == 0 && bytes == savedMemory && packet.empty() && Preserved());
    assert(!frameCalls && !publications && notes == (address == target) && polls == notes);
    if (address == target)
        assert(std::strcmp(lastStage, "RMCP01_LYT_DRAW_QUAD") == 0);
    assert(write(proofFd, "P", 1) == 1);
}
#if MKW_LOCAL_RENDERED_FAST_TRACK
HleGxState g_hleGxState;
GxDisplayListState g_dlRecordState;
bool g_alphaCompareValid = false;
std::atomic_bool g_auroraFrameActive{false}, g_auroraFrameHadWork{false};
namespace {
#include "pinned-gx-data.inc"
__GXData_struct nativeState{};
__GXData_struct* __gx = &nativeState;
bool nativeRecording = false;
unsigned drains = 0, dirtyFlushes = 0, primitiveFlushes = 0, alphas = 0, sourceDescs = 0, descs = 0, vats = 0;
std::array<std::uint8_t, sizeof(HleGxState)> hleBefore{};
std::array<std::uint8_t, sizeof(nativeState)> nativeBefore{};
bool alphaBefore = false, workBefore = false;
void __GXSetDirtyState() {
    ++dirtyFlushes;
    nativeState.dirtyState = 0;
}
void __GXSendFlushPrim() {
    ++primitiveFlushes;
    nativeState.vNum = 0;
    nativeState.vLim = 0;
}
void SaveState() {
    std::memcpy(hleBefore.data(), &g_hleGxState, sizeof(g_hleGxState));
    std::memcpy(nativeBefore.data(), &nativeState, sizeof(nativeState));
    alphaBefore = g_alphaCompareValid;
    workBefore = g_auroraFrameHadWork.load();
}
bool Preserved() {
    return std::memcmp(hleBefore.data(), &g_hleGxState, sizeof(g_hleGxState)) == 0 &&
           std::memcmp(nativeBefore.data(), &nativeState, sizeof(nativeState)) == 0 &&
           alphaBefore == g_alphaCompareValid && workBefore == g_auroraFrameHadWork.load();
}
} // namespace
namespace aurora::gx::fifo {
bool in_display_list() {
    return nativeRecording;
}
void write_data(const void*, std::uint32_t) {
    assert(false);
}
void drain() {
    ++drains;
}
void process(const std::uint8_t* data, std::uint32_t size, bool bigEndian) {
    assert(bigEndian && drains == 1 && descs == 25 && sourceDescs == 25 && vats == 16);
    packet.assign(data, data + size);
}
} // namespace aurora::gx::fifo
#include "pinned-call-dl.inc"
void EnsureAuroraFrameActive() {
    ++frameCalls;
}
void GXSetVtxDesc(GXAttr attr, GXAttrType type) {
    assert(attr != GX_VA_NBT && type == g_hleGxState.vtxDesc[attr]);
    ++publications;
    ++descs;
}
void GXSetSourceVtxDesc(GXAttr attr, GXAttrType type) {
    assert(attr != GX_VA_NBT && type == g_hleGxState.vtxDesc[attr]);
    ++publications;
    ++sourceDescs;
}
void GXSetVtxAttrFmt(GXVtxFmt fmt, GXAttr attr, GXCompCnt cnt, GXCompType type, u8 frac) {
    assert(fmt == GX_VTXFMT0 && attr != GX_VA_NBT && attr >= GX_VA_POS);
    const auto& f = g_hleGxState.vtxAttrFmt[0][attr];
    assert(f.cnt == cnt && f.type == type && f.frac == frac);
    ++publications;
    ++vats;
}
void GXSetAlphaCompare(GXCompare a, u8 refA, GXAlphaOp op, GXCompare b, u8 refB) {
    assert(a == GX_ALWAYS && b == GX_ALWAYS && op == GX_AOP_AND && !refA && !refB);
    ++publications;
    ++alphas;
}
namespace {
constexpr unsigned GX_DRAW_QUADS_CMD = 0x80;
// The Switch Memory slice exposes BE Read32; float decoding is bit-identical.
float ReadGuestFloat(std::uint32_t address) {
    return std::bit_cast<float>(Memory::Read32(address));
}
bool SubmitLytDrawDirect(float, float, float, float, int, std::uint32_t, const std::uint32_t*) {
    return false;
}
void SubmitLytDrawPacket(const std::uint8_t* data, std::uint32_t size) {
    oracle.assign(data, data + size);
}
#include "pinned-lyt-packet.inc"
void Layout(unsigned count, bool colors) {
    g_hleGxState = {};
    g_hleGxState.vtxDesc[GX_VA_POS] = GX_DIRECT;
    g_hleGxState.vtxAttrFmt[0][GX_VA_POS] = {GX_POS_XY, GX_F32, 137};
    if (colors) {
        g_hleGxState.vtxDesc[GX_VA_CLR0] = GX_DIRECT;
        g_hleGxState.vtxAttrFmt[0][GX_VA_CLR0] = {GX_CLR_RGBA, GX_RGBA8, 231};
    }
    for (unsigned i = 0; i < count; ++i) {
        g_hleGxState.vtxDesc[GX_VA_TEX0 + i] = GX_DIRECT;
        g_hleGxState.vtxAttrFmt[0][GX_VA_TEX0 + i] = {GX_TEX_ST, GX_F32, 83};
    }
}
void Float(std::uint32_t address, float value) {
    Memory::Write32(address, std::bit_cast<std::uint32_t>(value));
}
void Inputs(const CpuContext& cpu, float width = 3.f, float height = 7.f) {
    Float(cpu.gpr[3], 2.f);
    Float(cpu.gpr[3] + 4, 5.f);
    Float(cpu.gpr[4], width);
    Float(cpu.gpr[4] + 4, height);
    for (unsigned i = 0; i < cpu.gpr[5] * 8; ++i)
        Float(cpu.gpr[6] + i * 4, (float(i) - 17.f) / 8.f);
    if (cpu.gpr[7])
        for (unsigned i = 0; i < 16; ++i)
            Memory::Write8(cpu.gpr[7] + i, i * 17);
}
void Valid(CpuContext cpu) {
    Layout(cpu.gpr[5], cpu.gpr[7] != 0);
    std::array<std::uint32_t, 4> colors{};
    if (cpu.gpr[7])
        for (int i = 0; i < 4; ++i)
            colors[i] = ReadModulatedLytColor(cpu.gpr[7], i, cpu.gpr[8]);
    EmitLytDrawQuad(cpu.gpr[3], cpu.gpr[4], cpu.gpr[5], cpu.gpr[6], cpu.gpr[7] ? colors.data() : nullptr);
    auto before = cpu;
    savedMemory = bytes;
    nativeState = {};
    nativeState.dirtyState = 1;
    nativeState.vNum = 2;
    g_alphaCompareValid = cpu.gpr[8] & 1;
    const bool defaultAlpha = !g_alphaCompareValid;
    g_auroraFrameHadWork = false;
    SaveState();
    frameCalls = publications = notes = polls = drains = dirtyFlushes = primitiveFlushes = alphas = sourceDescs = descs = vats = 0;
    packet.clear();
    auto expectedNative = nativeState;
    std::memcpy(&expectedNative, &nativeState, sizeof(nativeState));
    expectedNative.dirtyState = 0;
    expectedNative.vNum = 0;
    expectedNative.vLim = 0;
    InvokeDirectCpu<target>(&cpu);
    assert(packet == oracle && packet.size() == 3 + 4 * (8 + (cpu.gpr[7] ? 4 : 0) + cpu.gpr[5] * 8));
    assert(notes == 1 && polls == 1 && frameCalls == 1 && drains == 1 && dirtyFlushes == 1 && primitiveFlushes == 1);
    assert(std::memcmp(&nativeState, &expectedNative, sizeof(nativeState)) == 0);
    assert(alphas == unsigned(defaultAlpha) && g_alphaCompareValid && g_auroraFrameHadWork);
    assert(std::memcmp(hleBefore.data(), &g_hleGxState, sizeof(g_hleGxState)) == 0);
    assert(std::memcmp(&cpu, &before, sizeof(cpu)) == 0 && bytes == savedMemory);
    assert(std::strcmp(lastStage, "RMCP01_LYT_DRAW_QUAD") == 0);
    // Independent winding/position and RGBA alpha oracle, including raw UV bits.
    unsigned cursor = 3;
    const float x = ReadGuestFloat(cpu.gpr[3]), y = ReadGuestFloat(cpu.gpr[3] + 4);
    const float right = x + ReadGuestFloat(cpu.gpr[4]), bottom = y - ReadGuestFloat(cpu.gpr[4] + 4);
    const auto word = [&]() { std::uint32_t v=0; for (unsigned i=0;i<4;++i) v=(v<<8)|packet.at(cursor++); return v; };
    for (unsigned corner : {0u, 1u, 3u, 2u}) {
        assert(word() == std::bit_cast<std::uint32_t>((corner & 1) ? right : x));
        assert(word() == std::bit_cast<std::uint32_t>((corner & 2) ? bottom : y));
        if (cpu.gpr[7]) {
            auto raw = Memory::Read32(cpu.gpr[7] + corner * 4);
            assert(word() == ((raw & 0xffffff00u) | ((raw & 255u) * (cpu.gpr[8] & 255u) / 255u)));
        }
        for (unsigned i = 0; i < cpu.gpr[5]; ++i)
            for (unsigned j = 0; j < 2; ++j)
                assert(word() == Memory::Read32(cpu.gpr[6] + i * 32 + corner * 8 + j * 4));
    }
    assert(cursor == packet.size());
}
} // namespace
#else
namespace {
void SaveState() {}
bool Preserved() {
    return true;
}
} // namespace
#endif
int main() {
    const rlimit noCore{0, 0};
    assert(setrlimit(RLIMIT_CORE, &noCore) == 0);
    auto entry = mkw_switch_find_missing_native_cpu_extension(target);
    assert(entry && !mkw_switch_find_missing_native_cpu_extension(0x12345678));
    entry(nullptr);
    assert(!notes && !polls && !frameCalls && !publications);
    Initialize();
#if MKW_LOCAL_RENDERED_FAST_TRACK
    unsigned cases = 0;
    for (unsigned a = 0; a < 256; ++a)
        for (unsigned b = 0; b < 256; ++b)
            assert(ScaleLytAlpha(a, b) == a * b / 255u && ScaleLytAlpha(a, 0xffffff00u | b) == a * b / 255u);
    for (unsigned count = 0; count <= 8; ++count)
        for (bool colors : {false, true})
            for (unsigned alpha : {0u, 1u, 127u, 254u, 255u, 0x100u, 0xffffffffu})
                for (float width : {-3.f, 0.f, 3.f}) {
                    auto cpu = Cpu(count, colors);
                    cpu.gpr[8] = alpha;
                    Inputs(cpu, width);
                    Valid(cpu);
                    ++cases;
                }
    for (float x : {16777215.f, -16777216.f, std::numeric_limits<float>::denorm_min(), -0.f}) {
        auto rounded = Cpu(8, true);
        Inputs(rounded, 1.f, -0.f);
        Float(rounded.gpr[3], x);
        Float(rounded.gpr[3] + 4, -0.f);
        Valid(rounded);
        ++cases;
    }
    auto atEnd = Cpu(0, false);
    atEnd.gpr[3] = regionBase + 312;
    atEnd.gpr[4] = regionBase + 304;
    Inputs(atEnd);
    Valid(atEnd);
    ++cases;
    auto cpu = Cpu(1, true);
    Inputs(cpu);
    Layout(1, true);
    for (unsigned value : {9u, 255u, 0x100u, 0xffffffffu}) {
        auto bad = cpu;
        bad.gpr[5] = value;
        Refusal(bad, "LYT_DRAW_QUAD_INVALID_TEXCOORD_COUNT");
    }
    g_hleGxState.inBegin = true;
    Refusal(cpu, "LYT_DRAW_QUAD_ACTIVE_STREAM");
    g_hleGxState.inBegin = false;
    g_hleGxState.fifoByteCount = 1;
    Refusal(cpu, "LYT_DRAW_QUAD_ACTIVE_STREAM");
    g_hleGxState.fifoByteCount = 0;
    g_dlRecordState.active = true;
    Refusal(cpu, "LYT_DRAW_QUAD_ACTIVE_STREAM");
    g_dlRecordState.active = false;
    nativeRecording = true;
    Refusal(cpu, "LYT_DRAW_QUAD_ACTIVE_STREAM");
    nativeRecording = false;
    for (unsigned attr = 0; attr < 26; ++attr) {
        const auto old = g_hleGxState.vtxDesc[attr];
        for (auto type : {GX_NONE, GX_DIRECT, GX_INDEX8, GX_INDEX16})
            if (type != old) {
                g_hleGxState.vtxDesc[attr] = type;
                Refusal(cpu, "LYT_DRAW_QUAD_UNSUPPORTED_LAYOUT");
            }
        g_hleGxState.vtxDesc[attr] = old;
    }
    for (unsigned attr : {unsigned(GX_VA_POS), unsigned(GX_VA_CLR0), unsigned(GX_VA_TEX0)}) {
        const auto old = g_hleGxState.vtxAttrFmt[0][attr];
        g_hleGxState.vtxAttrFmt[0][attr].cnt = static_cast<GXCompCnt>(1u - static_cast<unsigned>(old.cnt));
        Refusal(cpu, "LYT_DRAW_QUAD_UNSUPPORTED_LAYOUT");
        g_hleGxState.vtxAttrFmt[0][attr] = old;
        g_hleGxState.vtxAttrFmt[0][attr].type = GX_U16;
        Refusal(cpu, "LYT_DRAW_QUAD_UNSUPPORTED_LAYOUT");
        g_hleGxState.vtxAttrFmt[0][attr] = old;
    }
    for (unsigned reg : {3u, 4u, 6u, 7u})
        for (unsigned pointer : {0u, regionBase - 1, regionBase + 319, 0xfffffff9u}) {
            if (reg == 7 && pointer == 0)
                continue;
            auto bad = cpu;
            bad.gpr[reg] = pointer;
            Refusal(bad, reg == 7 && pointer == 0 ? "LYT_DRAW_QUAD_UNSUPPORTED_LAYOUT" : "LYT_DRAW_QUAD_UNREADABLE_INPUT");
        }
    for (unsigned reg : {3u, 4u, 6u}) {
        const auto old = Memory::Read32(cpu.gpr[reg]);
        for (unsigned raw : {0x7f800000u, 0xff800000u, 0x7fc01234u}) {
            Memory::Write32(cpu.gpr[reg], raw);
            Refusal(cpu, "LYT_DRAW_QUAD_NONFINITE_INPUT");
        }
        Memory::Write32(cpu.gpr[reg], old);
    }
    Float(cpu.gpr[3], std::numeric_limits<float>::max());
    Float(cpu.gpr[4], std::numeric_limits<float>::max());
    Refusal(cpu, "LYT_DRAW_QUAD_NONFINITE_EXTENT");
    Inputs(cpu);
    Refusal(cpu, "DIRECT", true);
    // Exact range boundaries, mapped zero, aliases and the end of 32-bit space.
    Initialize(0, 320);
    cpu = Cpu(8, true);
    cpu.gpr[3] = 0;
    cpu.gpr[7] = 304;
    Inputs(cpu);
    Valid(cpu);
    ++cases;
    cpu = Cpu(0, false);
    cpu.gpr[3] = 0;
    cpu.gpr[4] = 0;
    cpu.gpr[6] = 0xffffffffu;
    Inputs(cpu);
    Valid(cpu);
    ++cases;
    Initialize(0xfffffec0u, 320);
    cpu = Cpu(8, true);
    cpu.gpr[7] = 0xfffffff0u;
    Inputs(cpu);
    Valid(cpu);
    ++cases;
    Memory::Reset();
    Layout(cpu.gpr[5], true);
    Refusal(cpu, "LYT_DRAW_QUAD_UNREADABLE_INPUT");
    std::printf("PASS: LYT quad %u pinned/native and independent packets, exhaustive alpha, range/layout/stream/nonfinite refusals, CPU/memory/HLE preservation\n", cases);
#else
    Refusal(Cpu(), "LYT_DRAW_QUAD_REQUIRES_RENDERER");
    Refusal(Cpu(), "DIRECT", true);
    std::puts("PASS: LYT quad headless and unknown-target diagnostic stops");
#endif
}
