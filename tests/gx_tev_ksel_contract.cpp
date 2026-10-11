#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"
#include <dolphin/gx.h>
#include <array>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
namespace {
struct Registers {
    std::uint32_t before;
    std::array<std::uint32_t, 8> tevKsel;
    std::uint16_t bpSent;
    std::array<unsigned char, 14> unrelated;
} registers{};
[[maybe_unused]] Registers* __gx = &registers;
std::vector<unsigned char> packet;
std::array<unsigned char, sizeof(registers)> savedNative{};
#if MKW_LOCAL_RENDERED_FAST_TRACK
constexpr unsigned MaxTevSwap = 4, MaxTevStages = 16;
struct Swap {
    GXTevColorChan red, green, blue, alpha;
};
struct Stage {
    GXTevKColorSel kcSel;
    GXTevKAlphaSel kaSel;
};
struct DecodedState {
    std::uint32_t before;
    std::array<Swap, MaxTevSwap> tevSwapTable;
    std::array<Stage, MaxTevStages> tevStages;
    bool pipelineDirty;
    std::array<unsigned char, 15> unrelated;
} g_gxState{};
void mark_pipeline_state_dirty() {
    g_gxState.pipelineDirty = true;
}
#include "pinned-ksel-bp-get.inc"
void Decode(unsigned regId, unsigned value) {
    switch (regId) {
#include "pinned-ksel-decode.inc"
    default:
        assert(false);
    }
}
#endif
CpuContext saved{};
const char* stage = nullptr;
unsigned notes = 0, polls = 0, stageCalls = 0;
int proofFd = -1;
bool expectedAlpha = false;
void* liveCpu = nullptr;
[[maybe_unused]] void Emit(std::uint32_t value) {
    if (proofFd >= 0)
        _exit(91); // Never accept a refused native write as SIGABRT.
    packet.push_back(0x61);
    for (unsigned i = 0; i < 4; ++i)
        packet.push_back(value >> (24 - i * 8));
}
void Seed(unsigned pattern, unsigned flag) {
    std::memset(&registers, pattern, sizeof(registers));
    for (unsigned i = 0; i < 8; ++i)
        registers.tevKsel[i] = ((0xf6u + i) << 24) | (pattern ? 0x00d35ba6u : 0x002ac495u);
    registers.bpSent = flag;
    std::memcpy(savedNative.data(), &registers, sizeof(registers));
    packet.clear();
    stage = nullptr;
    notes = polls = stageCalls = 0;
}
CpuContext Cpu(unsigned id, unsigned selector) {
    CpuContext cpu;
    std::memset(&cpu, 0xa5, sizeof(cpu));
    cpu.gpr[3] = id;
    cpu.gpr[4] = selector;
    return cpu;
}
void Invoke(bool alpha, CpuContext* cpu) {
    if (alpha)
        InvokeDirectCpu<0x80171F80u>(cpu);
    else
        InvokeDirectCpu<0x80171F30u>(cpu);
}
bool Allowed(bool alpha, unsigned selector) {
    return selector < 8 || (selector >= (alpha ? 16u : 12u) && selector < 32);
}
void Valid(bool alpha, unsigned id, unsigned selector, unsigned pattern, unsigned flag) {
    Seed(pattern, flag);
    auto cpu = Cpu(id, selector);
    saved = cpu;
    expectedAlpha = alpha;
    Registers expected;
    std::memcpy(&expected, &registers, sizeof(expected));
#if MKW_LOCAL_RENDERED_FAST_TRACK
    // Independently specify each half of the shared stage-pair register.
    const unsigned shift = alpha ? (id % 2 ? 19 : 9) : (id % 2 ? 14 : 4);
    const auto mask = 31u << shift;
    const auto word = (expected.tevKsel[id / 2] & ~mask) | (selector << shift);
    expected.tevKsel[id / 2] = word;
    expected.bpSent = 1;
#endif
    Invoke(alpha, &cpu);
    assert(std::memcmp(&cpu, &saved, sizeof(cpu)) == 0);
    assert(std::memcmp(&registers, &expected, sizeof(registers)) == 0);
    assert(notes == 1 && polls == 1 && stageCalls == 1 && stage);
    assert(std::strcmp(stage, alpha ? "RMCP01_GX_SET_TEV_K_ALPHA_SEL" : "RMCP01_GX_SET_TEV_K_COLOR_SEL") == 0);
#if MKW_LOCAL_RENDERED_FAST_TRACK
    const std::vector<unsigned char> bytes{0x61, static_cast<unsigned char>(word >> 24), static_cast<unsigned char>(word >> 16),
                                           static_cast<unsigned char>(word >> 8), static_cast<unsigned char>(word)};
    assert(packet == bytes);
    // Establish a consistent decoder/cache baseline from all prior packets.
    // The next packet must change only this selector and the pipeline flag,
    // preserving the paired stage and every shared swap-table channel.
    std::memset(&g_gxState, 0xa5, sizeof(g_gxState));
    Registers beforeRegisters;
    std::memcpy(&beforeRegisters, savedNative.data(), sizeof(beforeRegisters));
    for (auto prior : beforeRegisters.tevKsel)
        Decode(prior >> 24, prior & 0xffffff);
    g_gxState.pipelineDirty = false;
    DecodedState decoded;
    std::memcpy(&decoded, &g_gxState, sizeof(decoded));
    if (alpha)
        decoded.tevStages[id].kaSel = static_cast<GXTevKAlphaSel>(selector);
    else
        decoded.tevStages[id].kcSel = static_cast<GXTevKColorSel>(selector);
    decoded.pipelineDirty = true;
    Decode(word >> 24, word & 0xffffff);
    assert(std::memcmp(&g_gxState, &decoded, sizeof(decoded)) == 0);
#else
    assert(packet.empty());
#endif
}
void Refusal(bool alpha, unsigned id, unsigned selector) {
    Seed(1, 0xabcd);
    expectedAlpha = alpha;
    void* memory = mmap(nullptr, sizeof(CpuContext), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    assert(memory != MAP_FAILED);
    auto* cpu = static_cast<CpuContext*>(memory);
    auto initial = Cpu(id, selector);
    std::memcpy(cpu, &initial, sizeof(initial));
    saved = initial;
    liveCpu = cpu;
    int ends[2];
    assert(pipe(ends) == 0);
    auto child = fork();
    assert(child >= 0);
    if (!child) {
        assert(prctl(PR_SET_DUMPABLE, 0) == 0);
        close(ends[0]);
        proofFd = ends[1];
        Invoke(alpha, cpu);
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
    assert(munmap(memory, sizeof(initial)) == 0);
}
} // namespace
extern "C" void mkw_switch_hle_vi_poll_retrace(CpuContext*) noexcept {
    ++polls;
}
extern "C" void mkw_switch_note_translated_dispatch(std::uint32_t target, CpuContext*) noexcept {
    assert(target == (expectedAlpha ? 0x80171F80u : 0x80171F30u));
    ++notes;
}
extern "C" void mkw_switch_set_fast_track_stage(const char* value) noexcept {
    stage = value;
    ++stageCalls;
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char* reason, std::uint32_t target, CpuContext* cpu) noexcept {
    assert(proofFd >= 0 && cpu == liveCpu && notes == 1 && polls == 1 && stageCalls == 1);
    assert(target == (expectedAlpha ? 0x80171F80u : 0x80171F30u));
    assert(std::strcmp(reason, expectedAlpha ? "GX_SET_TEV_K_ALPHA_SEL_UNPROVEN_ARGS" : "GX_SET_TEV_K_COLOR_SEL_UNPROVEN_ARGS") == 0);
    assert(std::memcmp(cpu, &saved, sizeof(saved)) == 0 && packet.empty());
    assert(std::memcmp(&registers, savedNative.data(), sizeof(registers)) == 0);
    assert(write(proofFd, "P", 1) == 1);
}
#if MKW_LOCAL_RENDERED_FAST_TRACK
#define GX_WRITE_RAS_REG(value) Emit(value)
#include "pinned-tev-ksel.inc"
#endif
int main() {
    static_assert(KnownNativeCpuCall<0x80171F30u>::kAvailable && KnownNativeCpuCall<0x80171F80u>::kAvailable);
    static_assert(GX_TEV_KCSEL_8_8 == 0 && GX_TEV_KCSEL_1_8 == 7 && GX_TEV_KCSEL_K0 == 12 && GX_TEV_KCSEL_K3_A == 31);
    static_assert(GX_TEV_KASEL_8_8 == 0 && GX_TEV_KASEL_1_8 == 7 && GX_TEV_KASEL_K0_R == 16 && GX_TEV_KASEL_K3_A == 31);
    const rlimit noCore{0, 0};
    assert(setrlimit(RLIMIT_CORE, &noCore) == 0);
    Seed(1, 1);
    KnownNativeCpuCall<0x80171F30u>::Invoke(nullptr);
    KnownNativeCpuCall<0x80171F80u>::Invoke(nullptr);
    assert(notes == 0 && polls == 0 && stageCalls == 0 && packet.empty());
    unsigned valid = 0, rejected = 0;
    for (bool alpha : {false, true})
        for (unsigned id = 0; id < 16; ++id)
            for (unsigned sel = 0; sel < 32; ++sel) {
                if (!Allowed(alpha, sel))
                    continue;
                for (unsigned pattern : {0u, 0xa5u})
                    for (unsigned flag : {0u, 1u, 0xffffu}) {
                        Valid(alpha, id, sel, pattern, flag);
                        ++valid;
                    }
            }
    for (bool alpha : {false, true}) {
        for (unsigned id : {16u, 17u, 0x100u, 0x10000u, 0xffffffffu}) {
            Refusal(alpha, id, 0);
            ++rejected;
        }
        for (unsigned id : {0u, 15u})
            for (unsigned sel = 8; sel < (alpha ? 16u : 12u); ++sel) {
                Refusal(alpha, id, sel);
                ++rejected;
            }
        for (unsigned id : {0u, 15u})
            for (unsigned sel : {32u, 33u, 0x100u, 0x10000u, 0x1001fu, 0xffffffffu}) {
                Refusal(alpha, id, sel);
                ++rejected;
            }
    }
    assert(valid == 4992 && rejected == 58);
    std::printf("PASS: TEV KSEL rendered=%d native-state/packet cases=%u diagnosed refusals=%u\n", MKW_LOCAL_RENDERED_FAST_TRACK, valid, rejected);
}
