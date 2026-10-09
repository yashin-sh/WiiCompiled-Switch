#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "memory.h"
#include <dolphin/gx.h>
#include <array>
#include <bit>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {
constexpr std::uint32_t target = 0x80171e70;
std::uint32_t regionBase = 0x70000000;
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
[[maybe_unused]] void Emit(unsigned width, std::uint32_t value) {
    for (unsigned i = 0; i < width; ++i)
        packet.push_back(value >> ((width - i - 1) * 8));
}
CpuContext Cpu(std::uint32_t id, std::uint32_t pointer) {
    CpuContext cpu;
    std::memset(&cpu, 0xa5, sizeof(cpu));
    cpu.gpr[3] = id;
    cpu.gpr[4] = pointer;
    return cpu;
}
void Refusal(std::uint32_t id, std::uint32_t pointer, const char* reason, bool unknown = false) {
    auto cpu = Cpu(id, pointer);
    saved = cpu;
    savedMemory = bytes;
    packet.clear();
    SaveNativeState();
    expectedReason = reason;
    expectedTarget = unknown ? 0x12345678 : target;
    notes = polls = 0;
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
void Initialize(std::uint32_t base, unsigned size) {
    Memory::Reset();
    Memory::Config config;
    config.regions = {{"synthetic-signed-tev-color", base, size}};
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
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char* reason, std::uint32_t address, CpuContext* cpu) noexcept {
    assert(expectedReason && std::strcmp(reason, expectedReason) == 0 && address == expectedTarget);
    assert(std::memcmp(cpu, &saved, sizeof(saved)) == 0 && bytes == savedMemory && packet.empty() && NativeStatePreserved());
    assert(notes == (address == target) && polls == notes);
    if (address == target)
        assert(std::strcmp(lastStage, "RMCP01_GX_SET_TEV_COLOR_S10") == 0);
    assert(write(proofFd, "P", 1) == 1);
}

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

constexpr unsigned MaxTevRegs = 4;
struct DecodedState {
    std::uint32_t guard = 0x1234abcd;
    std::array<std::array<float, 4>, 4> colorRegs{}, kcolors{};
    bool stateDirty = false;
    std::uint32_t tail = 0x4321dcba;
} g_gxState;
#include "pinned-bp-get.inc"
void Decode(u32 regId, u32 value) {
    switch (regId) {
#include "pinned-tev-s10-decode.inc"
    default:
        assert(false);
    }
}
void Valid(unsigned id, std::uint32_t pointer, const std::array<std::uint16_t, 4>& colors, unsigned flag) {
    for (unsigned component = 0; component < 4; ++component) {
        bytes.at(pointer - regionBase + component * 2) = colors[component] >> 8;
        bytes.at(pointer - regionBase + component * 2 + 1) = colors[component];
    }
    const auto memory = bytes;
    auto cpu = Cpu(id, pointer);
    const auto before = cpu;
    std::memset(&nativeState, 0xa5, sizeof(nativeState));
    nativeState.nrmType = GX_NONE;
    nativeState.bpSent = flag;
    auto expectedNative = nativeState;
    std::memcpy(&expectedNative, &nativeState, sizeof(nativeState));
    expectedNative.bpSent = 1;
    // Independent full BP packet oracle: RA and BG ordering, eleven bits each.
    const std::uint32_t ra = ((0xe0u + id * 2) << 24) | (colors[0] % 2048u) | ((colors[3] % 2048u) << 12);
    const std::uint32_t bg = ((0xe1u + id * 2) << 24) | (colors[2] % 2048u) | ((colors[1] % 2048u) << 12);
    std::vector<std::uint8_t> expected;
    for (auto word : {ra, bg}) {
        expected.push_back(0x61);
        for (unsigned i = 0; i < 4; ++i)
            expected.push_back(word >> (24 - 8 * i));
    }
    packet.clear();
    notes = polls = 0;
    InvokeDirectCpu<target>(&cpu);
    assert(packet == expected && packet.size() == 10 && notes == 1 && polls == 1);
    assert(std::memcmp(&cpu, &before, sizeof(cpu)) == 0 && bytes == memory);
    assert(std::memcmp(&nativeState, &expectedNative, sizeof(nativeState)) == 0);
    assert(std::strcmp(lastStage, "RMCP01_GX_SET_TEV_COLOR_S10") == 0);

    // Execute actual pinned BP decoder cases on a small state seam. Untouched
    // registers and K colors remain canaries; no GPU pixel claim is made here.
    g_gxState = {};
    for (unsigned reg = 0; reg < 4; ++reg) {
        g_gxState.colorRegs[reg].fill(19.f);
        g_gxState.kcolors[reg].fill(-23.f);
    }
    auto expectedDecoded = g_gxState;
    std::memcpy(&expectedDecoded, &g_gxState, sizeof(g_gxState));
    for (unsigned component = 0; component < 4; ++component) {
        const int unsignedBits = colors[component] % 2048u;
        const int signedValue = unsignedBits >= 1024 ? unsignedBits - 2048 : unsignedBits;
        expectedDecoded.colorRegs[id][component] = static_cast<float>(signedValue) / 255.f;
    }
    expectedDecoded.stateDirty = true;
    Decode(ra >> 24, ra & 0xffffff);
    Decode(bg >> 24, bg & 0xffffff);
    assert(std::memcmp(&g_gxState, &expectedDecoded, sizeof(g_gxState)) == 0);
}
#else
void SaveNativeState() {}
bool NativeStatePreserved() {
    return true;
}
#endif
} // namespace

int main() {
    const rlimit noCore{0, 0};
    assert(setrlimit(RLIMIT_CORE, &noCore) == 0);
    auto entry = mkw_switch_find_missing_native_cpu_extension(target);
    assert(entry && !mkw_switch_find_missing_native_cpu_extension(0x12345678));
    entry(nullptr);
    assert(notes == 0 && polls == 0 && packet.empty());
#if MKW_LOCAL_RENDERED_FAST_TRACK
    Initialize(0x70000000, 64);
    unsigned cases = 0;
    // Every 16-bit encoding reaches each component; rotating IDs and BP flags
    // exercises all native register pairs without guessing guest color values.
    for (unsigned value = 0; value < 65536; ++value) {
        const std::array<std::uint16_t, 4> colors{static_cast<std::uint16_t>(value),
                                                  static_cast<std::uint16_t>(value ^ 0x8000), static_cast<std::uint16_t>(value ^ 0x5555),
                                                  static_cast<std::uint16_t>(value ^ 0xffff)};
        for (auto offset : {0u, 1u, 56u}) {
            Valid(value % 4, regionBase + offset, colors, value % 2);
            ++cases;
        }
    }
    for (auto id : {4u, 255u, 0x100u, 0xffffffffu}) {
        Refusal(id, regionBase, "GX_SET_TEV_COLOR_S10_INVALID_ID");
        Refusal(id, 0xffffffffu, "GX_SET_TEV_COLOR_S10_INVALID_ID");
    }
    for (auto pointer : {0u, regionBase - 1, regionBase + 57, 0xfffffff9u})
        Refusal(1, pointer, "GX_SET_TEV_COLOR_S10_UNREADABLE_COLOR");
    Refusal(1, regionBase, "DIRECT", true);
    Initialize(0, 8);
    Valid(0, 0, {0, 0xffff, 0x400, 0x7fff}, 0);
    Initialize(0xfffffff0u, 16);
    Valid(3, 0xfffffff8u, {0x8000, 0x3ff, 0xfc00, 0x800}, 1);
    Refusal(1, 0xfffffff9u, "GX_SET_TEV_COLOR_S10_UNREADABLE_COLOR");
    Initialize(0x70000000, 7);
    Refusal(1, regionBase, "GX_SET_TEV_COLOR_S10_UNREADABLE_COLOR");
    Memory::Reset();
    Refusal(1, 0x70000000, "GX_SET_TEV_COLOR_S10_UNREADABLE_COLOR");
    std::printf("PASS: signed TEV color, %u native BP writer/decoder packets plus zero/end-of-address-space boundaries, CPU/memory/native-state canaries and pre-output refusals\n", cases + 2);
#else
    Initialize(0x70000000, 64);
    Refusal(1, regionBase, "GX_SET_TEV_COLOR_S10_REQUIRES_RENDERER");
    Refusal(1, regionBase, "DIRECT", true);
    std::puts("PASS: signed TEV headless and unknown-target diagnostic stops");
#endif
}
