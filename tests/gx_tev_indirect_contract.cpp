#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "memory.h"
#include "switch_gx_hle_traits.hpp"
#include <dolphin/gx.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <type_traits>
#include <unistd.h>
#include <vector>
namespace {
constexpr unsigned target = 0x801717ACu, stack = 0x70001000u;
#include "pinned-indirect-gx-data.inc"
using Registers = __GXData_struct;
Registers registers{};
[[maybe_unused]] Registers* __gx = &registers;
std::array<unsigned char, sizeof(registers)> savedNative{};
std::vector<unsigned char> packet;
#if MKW_LOCAL_RENDERED_FAST_TRACK
constexpr unsigned MaxTevStages = 16;
struct Stage {
    GXIndTexStageID indTexStage;
    GXIndTexFormat indTexFormat;
    GXIndTexBiasSel indTexBiasSel;
    GXIndTexAlphaSel indTexAlphaSel;
    GXIndTexMtxID indTexMtxId;
    GXIndTexWrap indTexWrapS, indTexWrapT;
    bool indTexUseOrigLOD, indTexAddPrev;
    std::array<unsigned char, 6> unrelated;
};
struct DecodedState {
    unsigned before;
    std::array<Stage, MaxTevStages> tevStages;
    bool stateDirty;
    std::uint32_t pipelineStateGeneration;
    std::array<unsigned char, 15> unrelated;
} g_gxState{};
std::array<unsigned char, sizeof(g_gxState)> savedDecoded{};
#include "pinned-indirect-epoch.inc"
#include "pinned-indirect-pipeline-dirty.inc"
#include "pinned-indirect-bp-get.inc"
void Decode(unsigned regId, unsigned value) {
    switch (regId) {
#include "pinned-indirect-decode.inc"
    default:
        assert(false);
    }
}
#endif
struct HostRegion {
    unsigned base;
    std::size_t size;
    unsigned char* data;
};
std::vector<HostRegion> regions;
std::vector<std::vector<unsigned char>> savedMemory;
#if MKW_LOCAL_RENDERED_FAST_TRACK
extern "C" void GX__SetTevIndirect_801717ac(std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
#endif
CpuContext savedCpu{};
const char* stage = nullptr;
const char* refusedReason = nullptr;
unsigned notes = 0, polls = 0, stageCalls = 0, validCount = 0, refusedCount = 0;
int proofFd = -1;
CpuContext* liveCpu = nullptr;
void SaveMemory() {
    savedMemory.clear();
    for (const auto& r : regions) {
        if (r.size)
            savedMemory.emplace_back(r.data, r.data + r.size);
        else
            savedMemory.emplace_back();
    }
}
void CheckMemory() {
    assert(savedMemory.size() == regions.size());
    for (unsigned i = 0; i < regions.size(); ++i)
        if (regions[i].size)
            assert(std::memcmp(regions[i].data, savedMemory[i].data(), regions[i].size) == 0);
}
[[maybe_unused]] void Emit(unsigned word) {
    if (proofFd >= 0)
        _exit(91);
    packet.push_back(0x61);
    for (unsigned i = 0; i < 4; ++i)
        packet.push_back(word >> (24 - 8 * i));
}
void Seed(unsigned flag = 0xffffu) {
    std::memset(&registers, 0xa5, sizeof(registers));
    registers.bpSent = flag;
    std::memcpy(savedNative.data(), &registers, sizeof(registers));
#if MKW_LOCAL_RENDERED_FAST_TRACK
    g_gxState = {};
    g_gxState.before = 0x12345678u;
    g_gxState.pipelineStateGeneration = next_gx_state_epoch();
    g_gxState.unrelated.fill(0xa5);
    for (auto& s : g_gxState.tevStages) {
        s.indTexStage = GX_INDTEXSTAGE1;
        s.indTexFormat = GX_ITF_5;
        s.indTexBiasSel = GX_ITB_SU;
        s.indTexAlphaSel = GX_ITBA_T;
        s.indTexMtxId = GX_ITM_T1;
        s.indTexWrapS = GX_ITW_64;
        s.indTexWrapT = GX_ITW_16;
        s.indTexUseOrigLOD = true;
        s.indTexAddPrev = true;
        s.unrelated.fill(0xa5);
    }
    std::memcpy(savedDecoded.data(), &g_gxState, sizeof(g_gxState));
#endif
    packet.clear();
    stage = nullptr;
    notes = polls = stageCalls = 0;
    SaveMemory();
}
using Args = std::array<unsigned, 10>;
Args Baseline() {
    return {0, 0, 2, 5, 6, 2, 5, 0x100, 0x100, 2};
}
void Encode(unsigned location, unsigned value) {
    auto* bytes = Memory::GetPointer(location, 4);
    assert(bytes);
    for (unsigned i = 0; i < 4; ++i)
        bytes[i] = value >> (24 - 8 * i);
}
CpuContext Cpu(const Args& a, unsigned sp) {
    CpuContext cpu;
    std::memset(&cpu, 0xa5, sizeof(cpu));
    cpu.gpr[1] = sp;
    for (unsigned i = 0; i < 8; ++i)
        cpu.gpr[3 + i] = a[i];
    return cpu;
}
void Valid(const Args& a, unsigned sp = stack, unsigned flag = 0xffffu) {
    Encode(sp + 8, a[8]);
    Encode(sp + 12, a[9]);
    Seed(flag);
    auto cpu = Cpu(a, sp);
    std::memcpy(&savedCpu, &cpu, sizeof(cpu));
    Registers expected;
    std::memcpy(&expected, &registers, sizeof(expected));
#if MKW_LOCAL_RENDERED_FAST_TRACK
    expected.bpSent = 1;
    const unsigned ts = std::min(a[0], 15u), is = std::min(a[1], 3u);
    const unsigned word = ((ts + 0x10u) << 24) | is | (a[2] << 2) | (a[3] << 4) |
                          (a[9] << 7) | (a[4] << 9) | (a[5] << 13) | (a[6] << 16) |
                          (unsigned(a[8] != 0) << 19) | (unsigned(a[7] != 0) << 20);
    DecodedState decoded;
    std::memcpy(&decoded, &g_gxState, sizeof(decoded));
    auto& s = decoded.tevStages[ts];
    s.indTexStage = static_cast<GXIndTexStageID>(is);
    s.indTexFormat = static_cast<GXIndTexFormat>(a[2]);
    s.indTexBiasSel = static_cast<GXIndTexBiasSel>(a[3]);
    s.indTexAlphaSel = static_cast<GXIndTexAlphaSel>(a[9]);
    s.indTexMtxId = static_cast<GXIndTexMtxID>(a[4]);
    s.indTexWrapS = static_cast<GXIndTexWrap>(a[5]);
    s.indTexWrapT = static_cast<GXIndTexWrap>(a[6]);
    s.indTexUseOrigLOD = a[8] != 0;
    s.indTexAddPrev = a[7] != 0;
    decoded.stateDirty = true;
    decoded.pipelineStateGeneration = g_gxState.pipelineStateGeneration + 1u;
#endif
    InvokeDirectCpu<target>(&cpu);
    assert(std::memcmp(&cpu, &savedCpu, sizeof(cpu)) == 0);
    assert(std::memcmp(&registers, &expected, sizeof(expected)) == 0);
    assert(notes == 1 && polls == 1 && stageCalls == 1 && stage);
    assert(std::strcmp(stage, "RMCP01_GX_SET_TEV_INDIRECT") == 0);
    CheckMemory();
#if MKW_LOCAL_RENDERED_FAST_TRACK
    const std::vector<unsigned char> bytes{0x61, static_cast<unsigned char>(word >> 24),
                                           static_cast<unsigned char>(word >> 16), static_cast<unsigned char>(word >> 8), static_cast<unsigned char>(word)};
    assert(packet == bytes);
    Decode(word >> 24, word & 0xffffffu);
    assert(std::memcmp(&g_gxState, &decoded, sizeof(decoded)) == 0);
    // Execute the exact pinned ten-u32 wrapper as a second independent path:
    // its clamps and Aurora bool casts must produce the same single packet.
    packet.clear();
    std::memcpy(&registers, savedNative.data(), sizeof(registers));
    GX__SetTevIndirect_801717ac(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9]);
    assert(packet == bytes && std::memcmp(&registers, &expected, sizeof(registers)) == 0);
    assert(std::memcmp(&cpu, &savedCpu, sizeof(cpu)) == 0);
    CheckMemory();
#else
    assert(packet.empty());
#endif
    ++validCount;
}
void Refusal(const Args& a, unsigned sp, const char* reason) {
    Seed();
    refusedReason = reason;
    auto* cpu = static_cast<CpuContext*>(mmap(nullptr, sizeof(CpuContext), PROT_READ | PROT_WRITE,
                                              MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    assert(cpu != MAP_FAILED);
    const auto initial = Cpu(a, sp);
    std::memcpy(cpu, &initial, sizeof(initial));
    std::memcpy(&savedCpu, &initial, sizeof(initial));
    liveCpu = cpu;
    int ends[2];
    assert(pipe(ends) == 0);
    const auto child = fork();
    assert(child >= 0);
    if (!child) {
        assert(prctl(PR_SET_DUMPABLE, 0) == 0);
        close(ends[0]);
        proofFd = ends[1];
        InvokeDirectCpu<target>(cpu);
        _exit(92);
    }
    close(ends[1]);
    int result;
    assert(waitpid(child, &result, 0) == child);
    unsigned char proof;
    assert(read(ends[0], &proof, 1) == 1 && proof == 'P');
    close(ends[0]);
    assert(WIFSIGNALED(result) && WTERMSIG(result) == SIGABRT);
    assert(std::memcmp(cpu, &initial, sizeof(initial)) == 0);
    CheckMemory();
    assert(munmap(cpu, sizeof(initial)) == 0);
    ++refusedCount;
}
void Map(const Memory::Config& config) {
    Memory::Init(config);
}
} // namespace
// Replace only the platform allocation boundary. Checked range lookups and
// spill reads use the actual Switch Memory implementation. MAP_SHARED permits
// checking CPU and guest backing again after an aborted refusal subprocess.
namespace GuestFlat {
bool IsActive() {
    return !regions.empty();
}
void Shutdown() noexcept {
    for (const auto& r : regions)
        if (r.size)
            assert(munmap(r.data, r.size) == 0);
    regions.clear();
}
void Initialize(const std::vector<RegionRequest>& requests) {
    Shutdown();
    for (const auto& r : requests) {
        unsigned char* data = nullptr;
        if (r.size) {
            data = static_cast<unsigned char*>(mmap(nullptr, r.size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
            assert(data != MAP_FAILED);
            std::memset(data, 0x5a, r.size);
        }
        regions.push_back({r.base, r.size, data});
    }
}
std::uint8_t* HostPointer(unsigned address) {
    for (auto& r : regions)
        if (address >= r.base && std::uint64_t(address) < std::uint64_t(r.base) + r.size)
            return r.data + (address - r.base);
    return nullptr;
}
} // namespace GuestFlat
extern "C" void mkw_switch_hle_vi_poll_retrace(CpuContext*) {
    ++polls;
}
extern "C" void mkw_switch_note_translated_dispatch(unsigned address, CpuContext*) {
    assert(address == target);
    ++notes;
}
extern "C" void mkw_switch_set_fast_track_stage(const char* value) noexcept {
    stage = value;
    ++stageCalls;
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char* reason, unsigned address, CpuContext* cpu) {
    assert(proofFd >= 0 && cpu == liveCpu && address == target);
    assert(notes == 1 && polls == 1 && stageCalls == 1 && stage);
    assert(std::strcmp(stage, "RMCP01_GX_SET_TEV_INDIRECT") == 0 && std::strcmp(reason, refusedReason) == 0);
    assert(std::memcmp(cpu, &savedCpu, sizeof(savedCpu)) == 0 && packet.empty());
    assert(std::memcmp(&registers, savedNative.data(), sizeof(registers)) == 0);
#if MKW_LOCAL_RENDERED_FAST_TRACK
    assert(std::memcmp(&g_gxState, savedDecoded.data(), sizeof(g_gxState)) == 0);
#endif
    CheckMemory();
    assert(write(proofFd, "P", 1) == 1);
}
#define UNEXERCISED(function)                        \
    extern "C" void function(CpuContext*) noexcept { \
        std::abort();                                \
    }
UNEXERCISED(mkw_switch_hle_gx_load_light_obj_imm)
UNEXERCISED(mkw_switch_hle_gx_load_nrm_mtx_imm)
UNEXERCISED(mkw_switch_hle_gx_set_z_texture)
UNEXERCISED(mkw_switch_hle_gx_set_tev_color_s10)
UNEXERCISED(mkw_switch_hle_lyt_draw_quad)
UNEXERCISED(mkw_switch_hle_nand_create)
UNEXERCISED(mkw_switch_hle_nand_write)
UNEXERCISED(mkw_switch_hle_nand_move)
UNEXERCISED(mkw_switch_hle_nand_seek)
UNEXERCISED(mkw_switch_hle_nand_get_length)
#undef UNEXERCISED
#if MKW_LOCAL_RENDERED_FAST_TRACK
#define GX_WRITE_RAS_REG(value) Emit(value)
#include "pinned-tev-indirect.inc"
#endif
int main() {
    static_assert(!KnownNativeCpuCall<target>::kAvailable);
    static_assert(std::is_same_v<GXBool, bool>);
    static_assert(GX_ITF_8 == 0 && GX_ITF_3 == 3 && GX_ITB_NONE == 0 && GX_ITB_STU == 7);
    static_assert(GX_ITM_OFF == 0 && GX_ITM_2 == 3 && GX_ITM_S0 == 5 && GX_ITM_S2 == 7 && GX_ITM_T0 == 9 && GX_ITM_T2 == 11);
    static_assert(GX_ITW_OFF == 0 && GX_ITW_0 == 6 && GX_ITBA_OFF == 0 && GX_ITBA_U == 3);
    const rlimit noCore{0, 0};
    assert(setrlimit(RLIMIT_CORE, &noCore) == 0);
    Seed();
    const auto entry = mkw_switch_find_missing_native_cpu_extension(target);
    assert(entry && !mkw_switch_find_missing_native_cpu_extension(0x12345678u));
    entry(nullptr);
    assert(!notes && !polls && !stageCalls && packet.empty());
    auto a = Baseline();
    Refusal(a, stack, "GX_SET_TEV_INDIRECT_STACK_UNREADABLE");
    Memory::Config config;
    config.regions = {{"spill", stack, 64}, {"end", 0xfffffff0u, 16}, {"low", 0, 32}};
    Map(config);
    constexpr std::array<unsigned, 10> matrices{0, 1, 2, 3, 5, 6, 7, 9, 10, 11};
    for (unsigned ts = 0; ts < 16; ++ts)
        for (unsigned is = 0; is < 4; ++is)
            for (unsigned fmt = 0; fmt < 4; ++fmt)
                for (unsigned bias = 0; bias < 8; ++bias)
                    for (unsigned alpha = 0; alpha < 4; ++alpha) {
                        a = Baseline();
                        a[0] = ts;
                        a[1] = is;
                        a[2] = fmt;
                        a[3] = bias;
                        a[9] = alpha;
                        a[4] = matrices[(ts + is + fmt + bias + alpha) % matrices.size()];
                        a[5] = (ts + fmt + bias) % 7;
                        a[6] = (is + bias + alpha) % 7;
                        Valid(a);
                    }
    for (unsigned ts : {0u, 15u, 16u, 255u, 0x10000u, 0xffffffffu})
        for (unsigned is : {0u, 3u, 4u, 255u, 0x10000u, 0xffffffffu})
            for (unsigned ap : {0u, 1u, 2u, 0x100u, 0xffffffffu})
                for (unsigned il : {0u, 1u, 2u, 0x100u, 0xffffffffu}) {
                    a = Baseline();
                    a[0] = ts;
                    a[1] = is;
                    a[7] = ap;
                    a[8] = il;
                    Valid(a);
                }
    for (auto matrix : matrices)
        for (unsigned ws = 0; ws < 7; ++ws)
            for (unsigned wt = 0; wt < 7; ++wt) {
                a = Baseline();
                a[4] = matrix;
                a[5] = ws;
                a[6] = wt;
                Valid(a);
            }
    for (auto sp : {stack, stack + 1, stack + 48, 0xfffffff0u, 0u})
        for (unsigned flag : {0u, 1u, 0xffffu})
            Valid(Baseline(), sp, flag);
    // Direct/non-indirect tuple forwards rather than skipping the native write.
    Valid(Args{});
    for (unsigned field : {2u, 3u, 4u, 5u, 6u, 9u})
        for (unsigned value : {4u, 7u, 8u, 12u, 255u, 0x100u, 0x10000u, 0xffffffffu}) {
            const bool allowed = field == 2 || field == 9 ? value < 4 : field == 3 ? value < 8
                                                                    : field == 4   ? std::find(matrices.begin(), matrices.end(), value) != matrices.end()
                                                                                   : value < 7;
            if (allowed)
                continue;
            a = Baseline();
            a[field] = value;
            Encode(stack + 8, a[8]);
            Encode(stack + 12, a[9]);
            Refusal(a, stack, "GX_SET_TEV_INDIRECT_UNPROVEN_ARGS");
        }
    for (unsigned hole : {4u, 8u}) {
        a = Baseline();
        a[4] = hole;
        Refusal(a, stack, "GX_SET_TEV_INDIRECT_UNPROVEN_ARGS");
    }
    for (unsigned sp : {0xfffffff1u, 0xfffffff8u, 0xfffffffcu, 0xffffffffu})
        Refusal(Baseline(), sp, "GX_SET_TEV_INDIRECT_STACK_OVERFLOW");
    Refusal(Baseline(), stack + 49, "GX_SET_TEV_INDIRECT_STACK_UNREADABLE");
    Refusal(Baseline(), 0x71000000u, "GX_SET_TEV_INDIRECT_STACK_UNREADABLE");
    for (unsigned size = 0; size < 8; ++size) {
        config.regions = {{"short", stack + 8, size}};
        Map(config);
        Refusal(Baseline(), stack, "GX_SET_TEV_INDIRECT_STACK_UNREADABLE");
    }
    config.regions = {{"first", stack + 8, 4}, {"second", stack + 12, 4}};
    Map(config);
    Refusal(Baseline(), stack, "GX_SET_TEV_INDIRECT_STACK_UNREADABLE");
    Memory::Reset();
    assert(validCount == 9598 && refusedCount == 61);
    std::printf("PASS: TEV indirect rendered=%d actual-memory/native/decoded cases=%u diagnosed refusals=%u\n", MKW_LOCAL_RENDERED_FAST_TRACK, validCount, refusedCount);
}
