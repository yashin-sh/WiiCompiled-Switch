#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "memory.h"
#include <dolphin/gx.h>
#include <array>
#include <bit>
#include <cassert>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
namespace {
constexpr std::uint32_t base = 0x70000000u;
std::vector<std::uint8_t> bytes, savedMemory, packet;
bool active = false;
unsigned notes = 0, polls = 0;
const char* lastStage = "";
CpuContext saved{};
const char* expectedReason = nullptr;
std::uint32_t expectedTarget = 0;
int proofFd = -1;
void SaveNativeState();
bool NativeStatePreserved();
[[maybe_unused]] void Emit(unsigned width, std::uint32_t bits) {
    for (unsigned i = 0; i < width; ++i)
        packet.push_back(bits >> ((width - 1 - i) * 8));
}
[[maybe_unused]] void Word(std::uint32_t address, std::uint32_t bits) {
    for (unsigned i = 0; i < 4; ++i)
        bytes.at(address - base + i) = bits >> (24 - i * 8);
}
CpuContext Cpu(std::uint32_t address, std::uint32_t id, std::uint32_t bias = 0) {
    CpuContext cpu;
    std::memset(&cpu, 0xa5, sizeof(cpu));
    cpu.gpr[3] = address;
    cpu.gpr[4] = id;
    cpu.gpr[5] = bias;
    return cpu;
}
void Refusal(std::uint32_t address, std::uint32_t id, const char* reason, bool unknown = false) {
    auto cpu = Cpu(address, id);
    saved = cpu;
    savedMemory = bytes;
    packet.clear();
    SaveNativeState();
    expectedReason = reason;
    expectedTarget = unknown ? 0x12345678 : 0x801720c0;
    notes = polls = 0;
    int ends[2];
    assert(pipe(ends) == 0);
    auto child = fork();
    assert(child >= 0);
    if (!child) {
        close(ends[0]);
        proofFd = ends[1];
        const rlimit noCore{0, 0};
        assert(setrlimit(RLIMIT_CORE, &noCore) == 0);
        if (unknown)
            InvokeDirectCpu<0x12345678>(&cpu);
        else
            InvokeDirectCpu<0x801720c0>(&cpu);
        _exit(9);
    }
    close(ends[1]);
    char proof = 0;
    assert(read(ends[0], &proof, 1) == 1 && proof == 'P');
    close(ends[0]);
    int status = 0;
    assert(waitpid(child, &status, 0) == child && WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
}
} // namespace
#if MKW_LOCAL_RENDERED_FAST_TRACK
#include "gx_native_extensions_fixture.inc"
#endif
namespace {
#if MKW_LOCAL_RENDERED_FAST_TRACK
std::array<std::uint8_t, sizeof(nativeState)> nativeBefore{};
void SaveNativeState() {
    std::memcpy(nativeBefore.data(), &nativeState, sizeof(nativeState));
}
bool NativeStatePreserved() {
    return std::memcmp(nativeBefore.data(), &nativeState, sizeof(nativeState)) == 0;
}
#else
void SaveNativeState() {}
bool NativeStatePreserved() {
    return true;
}
#endif
} // namespace
namespace GuestFlat {
bool IsActive() {
    return active;
}
void Initialize(const std::vector<RegionRequest>& requests) {
    assert(requests.size() == 1 && requests[0].base == base && requests[0].size == 4096);
    bytes.assign(4096, 0xa5);
    active = true;
}
std::uint8_t* HostPointer(std::uint32_t address) {
    return address == base ? bytes.data() : nullptr;
}
void Shutdown() noexcept {
    active = false;
    bytes.clear();
}
} // namespace GuestFlat
extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept {
    lastStage = stage;
}
extern "C" void mkw_switch_note_translated_dispatch(std::uint32_t target, CpuContext*) noexcept {
    assert(target == 0x801720c0);
    ++notes;
}
extern "C" void mkw_switch_hle_vi_poll_retrace(CpuContext*) noexcept {
    ++polls;
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char* reason, std::uint32_t target, CpuContext* cpu) noexcept {
    assert(expectedReason && std::strcmp(reason, expectedReason) == 0 && target == expectedTarget);
    assert(std::memcmp(cpu, &saved, sizeof(saved)) == 0 && bytes == savedMemory && packet.empty() && NativeStatePreserved());
    assert(notes == (target == 0x801720c0) && polls == notes);
    if (target == 0x801720c0)
        assert(std::strcmp(lastStage, "RMCP01_GX_SET_Z_TEXTURE") == 0);
    assert(write(proofFd, "P", 1) == 1);
}

#if MKW_LOCAL_RENDERED_FAST_TRACK
namespace {
struct DecodedState {
    std::uint32_t guard = 0x1234abcd;
    GXZTexOp zTextureOp = GX_ZT_REPLACE;
    u8 zTextureFmt = 2;
    u32 zTextureBias = 0x123456;
    bool stateDirty = false;
    std::uint64_t pipelineStateGeneration = 0;
    std::uint32_t tail = 0x4321dcba;
} g_gxState;
std::uint64_t epoch = 0;
std::uint64_t next_gx_state_epoch() {
    return ++epoch;
}
#include "pinned-bp-get.inc"
#include "pinned-pipeline-dirty.inc"
void Decode(u32 regId, u32 value) {
    switch (regId) {
#include "pinned-z-texture-decode.inc"
    default:
        assert(false);
    }
}
void CheckDecoded(u32 op, u8 format, u32 bias) {
    g_gxState = {};
    epoch = 0;
    for (unsigned offset : {0u, 5u}) {
        assert(packet[offset] == 0x61);
        u32 word = 0;
        for (unsigned i = 1; i <= 4; ++i)
            word = (word << 8) | packet[offset + i];
        Decode(word >> 24, word & 0x00ffffffu);
    }
    assert(g_gxState.zTextureOp == op && g_gxState.zTextureFmt == format && g_gxState.zTextureBias == bias);
    assert(g_gxState.stateDirty && g_gxState.pipelineStateGeneration == 2 && epoch == 2);
    assert(g_gxState.guard == 0x1234abcd && g_gxState.tail == 0x4321dcba);
}
} // namespace
#endif
int main() {
    Memory::Config config;
    config.regions = {{"synthetic-z-texture", base, 4096}};
    Memory::Init(config);
    auto entry = mkw_switch_find_missing_native_cpu_extension(0x801720c0);
    assert(entry && !mkw_switch_find_missing_native_cpu_extension(0x12345678));
    entry(nullptr);
    assert(notes == 0 && polls == 0 && packet.empty());
#if MKW_LOCAL_RENDERED_FAST_TRACK
    unsigned cases = 0;
    // Canonical formats, low-word defaults and wide aliases. Compare full words
    // before normalization: 0x10011 is a default, never an alias of Z8.
    std::vector<std::uint32_t> formats;
    for (std::uint32_t value = 0; value < 256; ++value)
        formats.push_back(value);
    for (auto value : {0x10011u, 0x10013u, 0x80000011u, 0xffffff13u, 0xffffffffu})
        formats.push_back(value);
    for (unsigned op = 0; op < 3; ++op)
        for (auto format : formats)
            for (auto bias : {0u, 1u, 0x00ffffffu, 0x01000000u, 0xffffffffu})
                for (auto previousBp : {0u, 0xbeefu}) {
                    std::memset(&nativeState, 0xa5, sizeof(nativeState));
                    nativeState.nrmType = GX_NONE;
                    nativeState.bpSent = previousBp;
                    SaveNativeState();
                    auto expectedNative = nativeBefore;
                    const u16 sent = 1;
                    std::memcpy(expectedNative.data() + offsetof(__GXData_struct, bpSent), &sent, sizeof(sent));
                    auto cpu = Cpu(op, format, bias);
                    auto before = cpu;
                    auto memory = bytes;
                    const std::uint8_t formatCode = format == 17 ? 0 : format == 19 ? 1
                                                                                    : 2;
                    const std::vector<std::uint8_t> expected{0x61, 0xf4,
                                                             static_cast<std::uint8_t>(bias >> 16), static_cast<std::uint8_t>(bias >> 8),
                                                             static_cast<std::uint8_t>(bias), 0x61, 0xf5, 0, 0,
                                                             static_cast<std::uint8_t>(op * 4 + formatCode)};
                    packet.clear();
                    notes = polls = 0;
                    InvokeDirectCpu<0x801720c0>(&cpu);
                    assert(packet == expected && notes == 1 && polls == 1);
                    assert(std::strcmp(lastStage, "RMCP01_GX_SET_Z_TEXTURE") == 0);
                    assert(std::memcmp(&cpu, &before, sizeof(cpu)) == 0 && bytes == memory);
                    assert(std::memcmp(expectedNative.data(), &nativeState, sizeof(nativeState)) == 0);
                    CheckDecoded(op, formatCode, bias & 0x00ffffffu);
                    ++cases;
                }
    for (auto op : {3u, 4u, 0x100u, 0x80000000u, 0xffffffffu})
        Refusal(op, 17, "GX_SET_Z_TEXTURE_INVALID_OP");
    Refusal(0, 17, "DIRECT", true);
    std::printf("PASS: Z texture through actual pinned BP writer and decoder, %u packets, all operations, format defaults, bias masking, CPU/memory/native-state canaries and refusals\n", cases);
#else
    Refusal(0, 17, "GX_SET_Z_TEXTURE_REQUIRES_RENDERER");
    Refusal(base + 128, 1, "DIRECT", true);
    std::puts("PASS: headless Z texture refusal and unknown direct target remain diagnostic stops");
#endif
}
