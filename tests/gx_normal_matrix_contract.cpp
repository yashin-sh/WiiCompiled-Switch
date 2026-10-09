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
[[maybe_unused]] void Emit(unsigned width, std::uint32_t bits) {
    for (unsigned i = 0; i < width; ++i)
        packet.push_back(bits >> ((width - 1 - i) * 8));
}
[[maybe_unused]] void Word(std::uint32_t address, std::uint32_t bits) {
    for (unsigned i = 0; i < 4; ++i)
        bytes.at(address - base + i) = bits >> (24 - i * 8);
}
CpuContext Cpu(std::uint32_t address, std::uint32_t id) {
    CpuContext cpu;
    std::memset(&cpu, 0xa5, sizeof(cpu));
    cpu.gpr[3] = address;
    cpu.gpr[4] = id;
    return cpu;
}
void Refusal(std::uint32_t address, std::uint32_t id, const char* reason, bool unknown = false) {
    auto cpu = Cpu(address, id);
    saved = cpu;
    savedMemory = bytes;
    packet.clear();
    expectedReason = reason;
    expectedTarget = unknown ? 0x12345678 : 0x80173188;
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
            InvokeDirectCpu<0x80173188>(&cpu);
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
    assert(target == 0x80173188);
    ++notes;
}
extern "C" void mkw_switch_hle_vi_poll_retrace(CpuContext*) noexcept {
    ++polls;
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char* reason, std::uint32_t target, CpuContext* cpu) noexcept {
    assert(expectedReason && std::strcmp(reason, expectedReason) == 0 && target == expectedTarget);
    assert(std::memcmp(cpu, &saved, sizeof(saved)) == 0 && bytes == savedMemory && packet.empty());
    assert(notes == (target == 0x80173188) && polls == notes);
    if (target == 0x80173188)
        assert(std::strcmp(lastStage, "RMCP01_GX_LOAD_NRM_MTX_IMM") == 0);
    assert(write(proofFd, "P", 1) == 1);
}
#if MKW_LOCAL_RENDERED_FAST_TRACK
// Actual pinned Aurora functions; link both registry entries without fake handlers.
#include "pinned-light-object.inc"
#define GX_WRITE_U8(value) Emit(1, static_cast<std::uint8_t>(value))
#define GX_WRITE_U32(value) Emit(4, static_cast<std::uint32_t>(value))
#define GX_WRITE_F32(value) Emit(4, std::bit_cast<std::uint32_t>(value))
#define CHECK(condition, ...) assert(condition)
extern "C" {
#include "pinned-light-native.inc"
#include "pinned-normal-native.inc"
}
#endif
int main() {
    Memory::Config config;
    config.regions = {{"synthetic-normal-matrix", base, 4096}};
    Memory::Init(config);
    auto entry = mkw_switch_find_missing_native_cpu_extension(0x80173188);
    assert(entry && !mkw_switch_find_missing_native_cpu_extension(0x12345678));
    entry(nullptr);
    assert(notes == 0 && polls == 0 && packet.empty());
#if MKW_LOCAL_RENDERED_FAST_TRACK
    unsigned cases = 0;
    // Independent guest words and expected wire bytes, including IEEE special values.
    const std::array<std::uint32_t, 12> values{0, 0x80000000, 0x3f800000, 0xbf800000, 1, 0x80000001,
                                               0x7f800000, 0xff800000, 0x7fc01234, 0x3eaaaaab, 0x80000000, 0xffc05678};
    for (auto address : {base + 128, base + 129, base + 4096 - 48})
        for (unsigned id = 0; id <= 27; ++id)
            for (unsigned rotation = 0; rotation < 12; ++rotation) {
                for (unsigned i = 0; i < 12; ++i)
                    Word(address + i * 4, values[(i + rotation) % 12]);
                auto cpu = Cpu(address, id);
                auto before = cpu;
                auto memory = bytes;
                const auto header = 0x00080400u + id * 3u;
                std::vector<std::uint8_t> expected{0x10,
                                                   static_cast<std::uint8_t>(header >> 24),
                                                   static_cast<std::uint8_t>(header >> 16),
                                                   static_cast<std::uint8_t>(header >> 8),
                                                   static_cast<std::uint8_t>(header)};
                // Independent guest-byte oracle: nine row-major words only;
                // poisoned translation words must never appear in this packet.
                for (unsigned row = 0; row < 3; ++row)
                    expected.insert(expected.end(), bytes.begin() + address - base + row * 16,
                                    bytes.begin() + address - base + row * 16 + 12);
                packet.clear();
                notes = polls = 0;
                InvokeDirectCpu<0x80173188>(&cpu);
                assert(packet.size() == 41 && packet == expected && notes == 1 && polls == 1);
                assert(std::strcmp(lastStage, "RMCP01_GX_LOAD_NRM_MTX_IMM") == 0);
                assert(std::memcmp(&cpu, &before, sizeof(cpu)) == 0 && bytes == memory);
                ++cases;
            }
    for (auto id : {28u, 30u, 0x100u, 0xffffffffu})
        Refusal(base + 128, id, "GX_LOAD_NRM_INVALID_ID");
    for (auto address : {0u, base - 1, base + 4096 - 47, 0xffffffd1u})
        Refusal(address, 0, "GX_LOAD_NRM_INVALID_MATRIX");
    Refusal(base + 128, 1, "DIRECT", true);
    Memory::Reset();
    Refusal(base + 128, 1, "GX_LOAD_NRM_INVALID_MATRIX");
    std::printf("PASS: normal matrix through actual pinned native FIFO, %u cases, all accepted IDs, skipped translation column, CPU/memory canaries and pre-mutation refusals\n", cases);
#else
    Refusal(base + 128, 1, "GX_LOAD_NRM_REQUIRES_RENDERER");
    Refusal(base + 128, 1, "DIRECT", true);
    std::puts("PASS: headless normal matrix refusal and unknown direct target remain diagnostic stops");
#endif
}
