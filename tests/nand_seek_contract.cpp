#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "horizon_runtime_services.hpp"
#include "memory.h"
#include "switch_nand_runtime.hpp"

#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
namespace {
constexpr std::uint32_t base = 0x70000000u, path = base + 16, info = base + 128;
constexpr std::uint32_t size = 65536, target = 0x8019b964u;
std::vector<std::uint8_t> bytes, refusedMemory;
bool active = false, failSeek = false, failTell = false;
fs::path root;
unsigned valid = 0, refusals = 0, notes = 0, polls = 0, seeks = 0, tells = 0;
unsigned expectedSeeks = 0, expectedTells = 0;
const char* stage = nullptr;
CpuContext refusedCpu{};
int proofFd = -1;
constexpr char payload[] = "binary\0payload\xff";
std::string Read(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void Write(const fs::path& p) {
    if (!p.parent_path().empty())
        fs::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary);
    out.write(payload, sizeof(payload));
    assert(out.good());
}
CpuContext Cpu(std::uint32_t p = info) {
    CpuContext cpu;
    std::memset(&cpu, 0xa5, sizeof(cpu));
    cpu.gpr[3] = p;
    cpu.gpr[4] = cpu.gpr[5] = 0;
    return cpu;
}
std::int32_t Invoke(CpuContext cpu) {
    const auto fileInfo = cpu.gpr[3];
    auto expected = cpu;
    const auto memory = bytes;
    const auto oldNotes = notes, oldPolls = polls, oldSeeks = seeks, oldTells = tells;
    InvokeDirectCpu<target>(&cpu);
    expected.gpr[3] = cpu.gpr[3];
    assert(std::memcmp(&expected, &cpu, sizeof(cpu)) == 0 && bytes == memory);
    assert(notes == oldNotes + 1 && polls == oldPolls + 1);
    assert(seeks == oldSeeks + 1 && tells == oldTells + (failSeek ? 0u : 1u));
    assert(stage && std::strcmp(stage, "RMCP01_NAND_SEEK") == 0);
    const auto result = static_cast<std::int32_t>(cpu.gpr[3]);
    const auto status = Read("sdmc:/switch/WiiCompiled-Switch/fast-track-nand-seek-status.txt");
    assert(status.find("mode=2\naccess_type=" + std::to_string(Memory::Read8(fileInfo + 0x88)) + "\nopen_flag=1\n") != std::string::npos);
    assert(status.find("result=" + std::to_string(result) + "\n") != std::string::npos);
    assert(Read(root / "ordinary.bin") == std::string(payload, sizeof(payload)));
    ++valid;
    return result;
}
void Refusal(CpuContext cpu) {
    refusedCpu = cpu;
    refusedMemory = bytes;
    expectedSeeks = seeks;
    expectedTells = tells;
    const auto retained = Read(root / "ordinary.bin");
    int fds[2];
    assert(pipe(fds) == 0);
    const auto child = fork();
    assert(child >= 0);
    if (!child) {
        close(fds[0]);
        proofFd = fds[1];
        const rlimit noCore{0, 0};
        assert(setrlimit(RLIMIT_CORE, &noCore) == 0 && prctl(PR_SET_DUMPABLE, 0) == 0);
        InvokeDirectCpu<target>(&cpu);
        _exit(9);
    }
    close(fds[1]);
    char proof = 0;
    assert(read(fds[0], &proof, 1) == 1 && proof == 'P');
    close(fds[0]);
    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    assert(Read(root / "ordinary.bin") == retained);
    ++refusals;
}
} // namespace
namespace Pinned {
constexpr std::int32_t NAND_RESULT_INVALID = -8, NAND_RESULT_UNKNOWN = -64;
struct FileHandle {
    FILE* file;
};
FileHandle handle{};
FileHandle* ResolveNandFileHandle(const char* who, std::uint32_t p) {
    assert(std::strcmp(who, "NANDSeek") == 0 && Memory::Contains(p, 4));
    return &handle;
}
#include "pinned-nand-origin.inc"
#include "pinned-nand-seek.inc"
} // namespace Pinned
namespace GuestFlat {
bool IsActive() {
    return active;
}
void Initialize(const std::vector<RegionRequest>& requests) {
    assert(requests.size() == 1 && requests[0].base == base && requests[0].size == size);
    bytes.assign(size, 0xa5);
    active = true;
}
std::uint8_t* HostPointer(std::uint32_t p) {
    return p == base ? bytes.data() : nullptr;
}
void Shutdown() noexcept {
    active = false;
    bytes.clear();
}
} // namespace GuestFlat
namespace mkw::horizon_runtime_services {
fs::path nand_root() {
    return root;
}
} // namespace mkw::horizon_runtime_services
extern "C" int __real_fseek(FILE*, long, int);
extern "C" long __real_ftell(FILE*);
extern "C" int __wrap_fseek(FILE* f, long offset, int origin) {
    ++seeks;
    if (failSeek) {
        assert(offset == 0 && origin == SEEK_SET);
        return -1;
    }
    return __real_fseek(f, offset, origin);
}
extern "C" long __wrap_ftell(FILE* f) {
    ++tells;
    return failTell ? -1 : __real_ftell(f);
}
extern "C" void mkw_switch_set_fast_track_stage(const char* p) noexcept {
    stage = p;
}
extern "C" void mkw_switch_note_translated_dispatch(std::uint32_t t, CpuContext*) noexcept {
    assert(t == target);
    ++notes;
}
extern "C" void mkw_switch_hle_vi_poll_retrace(CpuContext*) noexcept {
    ++polls;
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char* reason, std::uint32_t t, CpuContext* cpu) noexcept {
    assert(t == target && std::strcmp(reason, "NAND_SEEK_UNPROVEN_CALL") == 0);
    assert(std::memcmp(cpu, &refusedCpu, sizeof(*cpu)) == 0 && bytes == refusedMemory);
    assert(seeks == expectedSeeks && tells == expectedTells);
    assert(Read("sdmc:/switch/WiiCompiled-Switch/fast-track-nand-seek-status.txt").find("result=NOT_CALLED\n") != std::string::npos);
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
UNEXERCISED(mkw_switch_hle_nand_get_length)
UNEXERCISED(mkw_switch_hle_gx_set_tev_indirect)
#undef UNEXERCISED

int main() {
    static_assert(!KnownNativeCpuCall<target>::kAvailable);
    const auto entry = mkw_switch_find_missing_native_cpu_extension(target);
    assert(entry);
    entry(nullptr);
    assert(seeks == 0 && tells == 0);
    Memory::Config config;
    config.regions = {{"synthetic-seek", base, size}};
    Memory::Init(config);
    root = fs::absolute("production");
    std::memcpy(bytes.data() + path - base, "/ordinary.bin", 14);
    Write(root / "ordinary.bin");
    Write("oracle.bin");
    using namespace mkw::switch_nand_runtime;
    for (unsigned offset = 0; offset < 9; ++offset) {
        const auto fi = offset == 8 ? base + size - 139u : info + offset;
        assert(OpenSync(path, fi, 2) == 0);
        assert(SeekSync(fi, 7, 0) == 7);
        Pinned::handle.file = std::fopen("oracle.bin", "r+b");
        assert(Pinned::handle.file);
        assert(std::fseek(Pinned::handle.file, 7, SEEK_SET) == 0);
        const auto result = Pinned::NANDSeek_HLE(fi, 0, 0);
        assert(result == 0 && Invoke(Cpu(fi)) == result);
        assert(SeekSync(fi, 0, 1) == 0); // The real production stream actually rewound.
        assert(std::fclose(Pinned::handle.file) == 0 && CloseSync(fi) == 0);
    }
    assert(OpenSync(path, info, 2) == 0);
    Pinned::handle.file = std::fopen("oracle.bin", "r+b");
    assert(Pinned::handle.file);
    for (unsigned fault = 0; fault < 2; ++fault) {
        assert(SeekSync(info, 7, 0) == 7);
        assert(std::fseek(Pinned::handle.file, 7, SEEK_SET) == 0);
        failSeek = fault == 0;
        failTell = fault == 1;
        const auto oracle = Pinned::NANDSeek_HLE(info, 0, 0);
        assert(oracle == (failSeek ? -64 : -1) && Invoke(Cpu()) == oracle);
        failSeek = failTell = false;
        assert(SeekSync(info, 0, 1) == (fault == 0 ? 7 : 0));
    }
    failTell = true;
    assert(SeekSync(info, 0, 0) == -1); // Shared async backend also retains pinned ftell errors.
    failTell = false;
    assert(std::fclose(Pinned::handle.file) == 0);
    for (unsigned offset = 0; offset < 256; ++offset) {
        if (offset != 1) {
            Memory::Write8(info + 0x8a, offset);
            Refusal(Cpu());
        }
    }
    Memory::Write8(info + 0x8a, 1);
    // Ordinary OpenSync leaves this safe-open-specific byte opaque. The
    // authoritative access mode belongs to the owned host handle.
    for (const auto opaque : {0u, 2u, 255u}) {
        Memory::Write8(info + 0x88, opaque);
        assert(Invoke(Cpu()) == 0);
    }
    for (unsigned bit = 0; bit < 32; ++bit) {
        auto cpu = Cpu();
        cpu.gpr[4] = 1u << bit;
        Refusal(cpu);
        cpu = Cpu();
        cpu.gpr[5] = 1u << bit;
        Refusal(cpu);
    }
    for (const auto p : {0u, 0xffffffffu, base - 1u, base + size - 138u})
        Refusal(Cpu(p));
    const auto fd = Memory::Read32(info);
    for (const auto bad : {0u, 0xffffffffu, fd + 1000u}) {
        Memory::Write32(info, bad);
        Refusal(Cpu());
    }
    Memory::Write32(info, fd);
    assert(CloseSync(info) == 0);
    Memory::Write8(info + 0x8a, 1);
    Refusal(Cpu()); // Stale fd cannot become an owned handle.
    for (const auto mode : {1u, 3u}) {
        assert(OpenSync(path, info, mode) == 0);
        Memory::Write8(info + 0x88, 2);
        Refusal(Cpu());
        assert(CloseSync(info) == 0);
    }
    assert(SafeOpenSync(path, info, 2) == 0);
    Refusal(Cpu());
    Memory::Write8(info + 0x8a, 1);
    Refusal(Cpu()); // Safe shadow remains excluded even with a forged ordinary flag.
    Memory::Write8(info + 0x8a, 3);
    assert(SafeCloseSync(info) == 0);
    assert(Read(root / "ordinary.bin") == std::string(payload, sizeof(payload)));
    Memory::Reset();
    std::printf("PASS: NANDSeek valid=%u refusals=%u; actual owned-stream rewind, pinned fseek/ftell failures, stale/mode/safe/span/argument guards, CPU/guest/file preservation\n", valid, refusals);
}
