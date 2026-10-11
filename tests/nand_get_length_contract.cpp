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
constexpr std::uint32_t size = 65536, target = 0x8019bf4cu;
std::vector<std::uint8_t> bytes, refusedMemory;
bool active = false;
fs::path root;
unsigned valid = 0, refusals = 0, notes = 0, polls = 0, seeks = 0, tells = 0;
unsigned expectedSeeks = 0, expectedTells = 0;
unsigned failSeekCall = 0, failTellCall = 0;
constexpr std::uint32_t output = info - 4;
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
    cpu.gpr[4] = output;
    return cpu;
}
std::int32_t Invoke(CpuContext cpu, const std::vector<std::uint8_t>& expectedMemory) {
    const auto fileInfo = cpu.gpr[3], outPtr = cpu.gpr[4];
    auto expected = cpu;
    const auto oldNotes = notes, oldPolls = polls;
    InvokeDirectCpu<target>(&cpu);
    expected.gpr[3] = cpu.gpr[3];
    assert(std::memcmp(&expected, &cpu, sizeof(cpu)) == 0 && bytes == expectedMemory);
    assert(notes == oldNotes + 1 && polls == oldPolls + 1);
    assert(seeks == 2 && tells == 2);
    assert(stage && std::strcmp(stage, "RMCP01_NAND_GET_LENGTH") == 0);
    const auto result = static_cast<std::int32_t>(cpu.gpr[3]);
    const auto status = Read("sdmc:/switch/WiiCompiled-Switch/fast-track-nand-get-length-status.txt");
    assert(status.find("mode=1\naccess_type=" + std::to_string(Memory::Read8(fileInfo + 0x88)) + "\nopen_flag=1\n") != std::string::npos);
    assert(status.find("result=" + std::to_string(result) + "\n") != std::string::npos);
    assert(status.find("length=" + std::to_string(Memory::Read32(outPtr)) + "\n") != std::string::npos);
    assert(Read(root / "ordinary.bin") == Read("oracle.bin"));
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
constexpr std::int32_t NAND_RESULT_INVALID = -8, NAND_RESULT_OK = 0;
struct FileHandle {
    FILE* file;
};
struct NandFileExtent {
    long position = 0, size = 0;
};
FileHandle handle{};
FileHandle* ResolveNandFileHandle(const char* who, std::uint32_t p) {
    assert(std::strcmp(who, "NANDGetLength") == 0 && Memory::Contains(p, 4));
    return &handle;
}
#include "pinned-nand-extent.inc"
#include "pinned-nand-get-length.inc"
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
    if (failSeekCall == seeks)
        return -1;
    return __real_fseek(f, offset, origin);
}
extern "C" long __wrap_ftell(FILE* f) {
    ++tells;
    return failTellCall == tells ? -1 : __real_ftell(f);
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
    assert(t == target && std::strcmp(reason, "NAND_GET_LENGTH_UNPROVEN_CALL") == 0);
    assert(std::memcmp(cpu, &refusedCpu, sizeof(*cpu)) == 0 && bytes == refusedMemory);
    assert(seeks == expectedSeeks && tells == expectedTells);
    assert(Read("sdmc:/switch/WiiCompiled-Switch/fast-track-nand-get-length-status.txt").find("result=NOT_CALLED\n") != std::string::npos);
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
    auto Probe = [&](CpuContext cpu) {
        const auto fi = cpu.gpr[3], out = cpu.gpr[4];
        const auto before = bytes;
        seeks = tells = 0;
        const auto oracle = Pinned::NANDGetLength_HLE(fi, out);
        assert(oracle == 0 && seeks == 2 && tells == 2);
        const auto expected = bytes;
        bytes = before;
        seeks = tells = 0;
        assert(Invoke(cpu, expected) == oracle);
        failSeekCall = failTellCall = 0;
        assert(SeekSync(fi, 0, 1) == __real_ftell(Pinned::handle.file));
    };
    for (unsigned offset = 0; offset < 9; ++offset) {
        const auto fi = offset == 8 ? base + size - 139u : info + offset;
        assert(OpenSync(path, fi, 1) == 0 && SeekSync(fi, 7, 0) == 7);
        Pinned::handle.file = std::fopen("oracle.bin", "rb");
        assert(Pinned::handle.file && __real_fseek(Pinned::handle.file, 7, SEEK_SET) == 0);
        auto cpu = Cpu(fi);
        cpu.gpr[4] = offset == 8 ? base + size - 143u : output - offset;
        Probe(cpu);
        assert(std::fclose(Pinned::handle.file) == 0 && CloseSync(fi) == 0);
    }
    for (const auto length : {0u, 1u, 29344u, 0x2bc000u}) {
        const std::string data(length, static_cast<char>(length ^ 0x5a));
        for (const auto& p : {root / "ordinary.bin", fs::path("oracle.bin")}) {
            std::ofstream out(p, std::ios::binary | std::ios::trunc);
            out.write(data.data(), data.size());
            assert(out.good());
        }
        assert(OpenSync(path, info, 1) == 0 && SeekSync(info, 7, 0) == 7);
        Pinned::handle.file = std::fopen("oracle.bin", "rb");
        assert(Pinned::handle.file && __real_fseek(Pinned::handle.file, 7, SEEK_SET) == 0);
        Probe(Cpu());
        assert(Memory::Read32(output) == length);
        assert(std::fclose(Pinned::handle.file) == 0 && CloseSync(info) == 0);
    }
    Write(root / "ordinary.bin");
    Write("oracle.bin");
    assert(OpenSync(path, info, 1) == 0);
    Pinned::handle.file = std::fopen("oracle.bin", "rb");
    assert(Pinned::handle.file);
    for (unsigned fault = 0; fault < 4; ++fault) {
        assert(SeekSync(info, 7, 0) == 7 && __real_fseek(Pinned::handle.file, 7, SEEK_SET) == 0);
        failSeekCall = fault < 2 ? fault + 1 : 0;
        failTellCall = fault >= 2 ? fault - 1 : 0;
        Probe(Cpu());
        // Exercise the shared frontend against the same extracted oracle.
        assert(SeekSync(info, 7, 0) == 7 && __real_fseek(Pinned::handle.file, 7, SEEK_SET) == 0);
        failSeekCall = fault < 2 ? fault + 1 : 0;
        failTellCall = fault >= 2 ? fault - 1 : 0;
        seeks = tells = 0;
        assert(Pinned::NANDGetLength_HLE(info, output) == 0);
        const auto expected = bytes;
        Memory::Write32(output, 0xa5a5a5a5u);
        seeks = tells = 0;
        assert(GetLengthSync(info, output) == 0 && bytes == expected && seeks == 2 && tells == 2);
        failSeekCall = failTellCall = 0;
        assert(SeekSync(info, 0, 1) == __real_ftell(Pinned::handle.file));
    }
    for (unsigned offset = 0; offset < 256; ++offset) {
        if (offset != 1) {
            Memory::Write8(info + 0x8a, offset);
            Refusal(Cpu());
        }
    }
    Memory::Write8(info + 0x8a, 1);
    // The safe-open-specific access byte is opaque for ordinary opens.
    for (const auto opaque : {0u, 1u, 255u}) {
        Memory::Write8(info + 0x88, opaque);
        Probe(Cpu());
    }
    for (const auto p : {0u, 0xffffffffu, base - 1u, base + size - 138u})
        Refusal(Cpu(p));
    for (const auto p : {0u, 0xffffffffu, base - 1u, base + size - 3u}) {
        auto cpu = Cpu();
        cpu.gpr[4] = p;
        Refusal(cpu);
    }
    // Reject every output intersecting the complete opaque file-info span.
    for (std::uint32_t p = info - 3; p < info + 139; ++p) {
        auto cpu = Cpu();
        cpu.gpr[4] = p;
        Refusal(cpu);
    }
    assert(std::fclose(Pinned::handle.file) == 0);
    const auto fd = Memory::Read32(info);
    for (const auto bad : {0u, 0xffffffffu, fd + 1000u}) {
        Memory::Write32(info, bad);
        Refusal(Cpu());
    }
    Memory::Write32(info, fd);
    assert(CloseSync(info) == 0);
    Memory::Write8(info + 0x8a, 1);
    Refusal(Cpu()); // Stale fd cannot become an owned handle.
    for (const auto mode : {2u, 3u}) {
        assert(OpenSync(path, info, mode) == 0);
        Memory::Write8(info + 0x88, 1);
        Refusal(Cpu());
        assert(CloseSync(info) == 0);
    }
    assert(SafeOpenReadSync(path, info) == 0);
    Refusal(Cpu());
    Memory::Write8(info + 0x8a, 1);
    Refusal(Cpu()); // Safe read origin remains authoritative even with a forged ordinary flag.
    Memory::Write8(info + 0x8a, 3);
    assert(SafeCloseReadSync(info) == 0);
    assert(SafeOpenSync(path, info, 2) == 0);
    Refusal(Cpu());
    Memory::Write8(info + 0x8a, 1);
    Refusal(Cpu()); // Safe shadow remains excluded even with a forged ordinary flag.
    Memory::Write8(info + 0x8a, 3);
    assert(SafeCloseSync(info) == 0);
    assert(Read(root / "ordinary.bin") == std::string(payload, sizeof(payload)));
    Memory::Reset();
    std::printf("PASS: NANDGetLength valid=%u refusals=%u; actual owned-file extent, pinned seek/tell fault semantics, position restore, stale/mode/safe/span/overlap guards, CPU/guest/file preservation\n", valid, refusals);
}
