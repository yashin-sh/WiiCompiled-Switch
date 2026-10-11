#include "guest_flat_memory.h"
#include "horizon_runtime_services.hpp"
#include "memory.h"
#include "nand_save_probe.h"
#include "switch_nand_runtime.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace {
constexpr unsigned base = 0x70000000, path = base + 16, info = base + 2048;
constexpr unsigned data = base + 4096, output = base + 3072;
std::vector<std::uint8_t> bytes;
fs::path root;
bool active = false;
unsigned cases = 0;
std::uint64_t saveAttempts = 0;
bool reportWritable = false;
const fs::path report = "sdmc:/switch/WiiCompiled-Switch/fast-track-nand-open-status.txt";
using ReportSnapshot = std::optional<std::pair<std::string, fs::file_time_type>>;
ReportSnapshot LastReport() {
    if (!fs::is_regular_file(report))
        return std::nullopt;
    std::ifstream stream(report, std::ios::binary);
    assert(stream.good());
    return std::pair{std::string(std::istreambuf_iterator<char>(stream), {}), fs::last_write_time(report)};
}
std::string DecodePath(const std::string& text) {
    std::string decoded;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] != '%') {
            decoded += text[i];
            continue;
        }
        assert(i + 2 < text.size());
        const auto digit = [](char c) -> unsigned {
            assert((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'));
            return c <= '9' ? c - '0' : c - 'A' + 10;
        };
        decoded += static_cast<char>((digit(text[i + 1]) << 4) | digit(text[i + 2]));
        i += 2;
    }
    return decoded;
}
void CheckReport(const fs::path& hostPath, RuntimeNandSave::ReadAction action,
                 int result, const ReportSnapshot& before) {
    if (hostPath.filename() != "rksys.dat") {
        assert(LastReport() == before);
        return;
    }
    ++saveAttempts;
    if (!reportWritable) {
        assert(!LastReport());
        return;
    }
    const auto current = LastReport();
    assert(current);
    std::map<std::string, std::string> fields;
    std::size_t begin = 0;
    const auto& text = current->first;
    while (begin < text.size()) {
        const auto end = text.find('\n', begin), equal = text.find('=', begin);
        assert(end != std::string::npos && equal < end);
        assert(fields.emplace(text.substr(begin, equal - begin), text.substr(equal + 1, end - equal - 1)).second);
        begin = end + 1;
    }
    assert(fields.size() == 8);
    assert(fields.at("status") == "rksys-read-open");
    assert(std::stoull(fields.at("attempt")) == saveAttempts);
    assert(fields.at("path_ptr") == "0x70000010");
    assert(DecodePath(fields.at("normalized_host_path")) == hostPath.generic_string());
    assert(fields.at("path_truncated") == "NO");
    const char* name = action == RuntimeNandSave::ReadAction::Proceed   ? "Proceed"
                       : action == RuntimeNandSave::ReadAction::Missing ? "Missing"
                       : action == RuntimeNandSave::ReadAction::Error   ? "Error"
                                                                        : "RecoveryNeeded";
    assert(fields.at("read_action") == name);
    assert(std::stoi(fields.at("result")) == result);
    assert(std::stoi(fields.at("fd")) == (result == 0 ? static_cast<int>(Memory::Read32(info)) : -1));
}

struct Snapshot {
    fs::path path;
    fs::file_type type;
    std::string payload;
    fs::file_time_type modified{};
    friend bool operator==(const Snapshot&, const Snapshot&) = default;
};
std::string Read(const fs::path& p) {
    std::ifstream stream(p, std::ios::binary);
    assert(stream.good());
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
std::vector<Snapshot> SnapshotTree() {
    std::vector<Snapshot> out;
    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        const auto type = entry.symlink_status().type();
        out.push_back({entry.path().lexically_relative(root), type,
                       type == fs::file_type::regular ? Read(entry.path()) : std::string{},
                       entry.last_write_time()});
    }
    std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) { return a.path < b.path; });
    return out;
}
void Write(const fs::path& p, const std::string& payload) {
    fs::create_directories(p.parent_path());
    std::ofstream stream(p, std::ios::binary);
    stream.write(payload.data(), payload.size());
    assert(stream.good());
    stream.close();
    // Deliberately old, distinct timestamp: tests prove the exact time survives.
    fs::last_write_time(p, fs::file_time_type::clock::now() - std::chrono::hours(72));
}
void Path(const std::string& p) {
    assert(p.size() < 1024);
    std::memcpy(bytes.data() + path - base, p.c_str(), p.size() + 1);
}
void Probe(const std::string& guestPath, const fs::path& hostPath) {
    using namespace mkw::switch_nand_runtime;
    const auto action = RuntimeNandSave::CheckRead(hostPath, 1);
    const auto expected = action == RuntimeNandSave::ReadAction::Missing ? -12
                          : action == RuntimeNandSave::ReadAction::Proceed
                              ? fs::exists(hostPath) ? 0 : -12
                              : -64;
    const auto disk = SnapshotTree();
    for (unsigned kind = 0; kind != 3; ++kind) {
        Path(guestPath);
        std::fill(bytes.begin() + info - base, bytes.begin() + info - base + 256, 0xa5);
        const auto before = bytes;
        const auto previousReport = LastReport();
        const int result = kind == 0   ? OpenSync(path, info, 1)
                           : kind == 1 ? SafeOpenReadSync(path, info)
                                       : SafeOpenSync(path, info, 1);
        assert(result == expected);
        CheckReport(hostPath, action, result, previousReport);
        if (result == 0) {
            const auto payload = Read(hostPath);
            assert(GetLengthSync(info, output) == 0);
            assert(Memory::Read32(output) == payload.size());
            const auto count = std::min<std::size_t>(payload.size(), 128);
            assert(ReadSync(info, data, count) == static_cast<int>(count));
            assert(std::memcmp(bytes.data() + data - base, payload.data(), count) == 0);
            assert(kind == 0 ? CloseSync(info) == 0 : SafeCloseReadSync(info) == 0);
        } else {
            assert(std::memcmp(bytes.data() + info - base, before.data() + info - base, 0x88) == 0);
            assert(Memory::Read8(info + 0x8a) == 0xa5);
        }
        assert(SnapshotTree() == disk);
        ++cases;
    }
}
} // namespace

namespace GuestFlat {
bool IsActive() {
    return active;
}
void Initialize(const std::vector<RegionRequest>& requests) {
    assert(requests.size() == 1 && requests[0].base == base && requests[0].size == 65536);
    bytes.assign(65536, 0);
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

extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept {}

int main() {
    Memory::Config config;
    config.regions = {{"blank-save", base, 65536}};
    Memory::Init(config);
    root = fs::absolute("nand%root\nfield=injection");
    const auto home = root / "title/00010004/524d4350/data";
    const auto save = home / "rksys.dat", shadow = home / "rksys.dat.nandsafe.tmp";
    fs::create_directories(home);
    Write(home / "banner.bin", std::string(29344, '\x5a'));
    Write(home / "rksys.dat.bak", "backup retained");
    Write(home / "rksys.dat.backup", "other backup retained");
    Write(root / "title/00000001/00000002/data/setting.txt", "identity retained");

    Probe("rksys.dat", save); // Missing report directory must not change the NAND result.
    fs::create_directories(report.parent_path());
    reportWritable = true;
    for (const std::size_t size : {0u, 1u, 4095u, 4096u, 4097u, 0x2bc000u}) {
        Write(save, std::string(size, '\0'));
        Probe("rksys.dat", save);
        Probe("/title/00010004/524d4350/data/rksys.dat", save);
        // Ordinary write opens remain possible for guest initialization, without writing here.
        const auto disk = SnapshotTree();
        const auto oldReport = LastReport();
        for (unsigned mode : {2u, 3u}) {
            Path("rksys.dat");
            assert(mkw::switch_nand_runtime::OpenSync(path, info, mode) == 0);
            assert(mkw::switch_nand_runtime::CloseSync(info) == 0);
            assert(SnapshotTree() == disk);
            assert(LastReport() == oldReport);
            ++cases;
        }
    }
    // This is a header-shaped synthetic file, not a claimed CRC-valid game save.
    std::string headerShaped(0x2bc000u, '\0');
    headerShaped.replace(0, 8, "RKSD0006");
    for (const auto& payload : {std::string("RKSD"), std::string("damaged-header"),
                                std::string(8192, '\0') + "x", headerShaped}) {
        Write(save, payload);
        Probe("rksys.dat", save);
        for (const auto& scratch : {std::string{}, std::string(4096, '\0'), std::string("recoverable shadow")}) {
            Write(shadow, scratch);
            Probe("rksys.dat", save);
        }
    }
    Write(save, "committed nonzero save");
    fs::remove(shadow);
    fs::create_directory(shadow);
    Probe("rksys.dat", save); // A committed nonzero original takes precedence over an invalid shadow.
    fs::remove(shadow);
    for (bool originalExists : {false, true}) {
        fs::remove(save);
        if (originalExists)
            Write(save, std::string(4096, '\0'));
        for (const auto& scratch : {std::string{}, std::string(4096, '\0'), std::string("recoverable shadow")}) {
            Write(shadow, scratch);
            Probe("rksys.dat", save);
            Probe("rksys.dat.nandsafe.tmp", shadow);
        }
        fs::remove(shadow);
        fs::create_directory(shadow);
        Probe("rksys.dat", save); // Error inspecting shadow must not imply missing.
        fs::remove(shadow);
    }
    fs::remove(save);
    fs::create_directory(save);
    Probe("rksys.dat", save); // Error inspecting the main file, with no descriptor published.
    fs::remove(save);
    for (const char* name : {"banner.bin", "rksys.dat.bak", "rksys.dat.backup", "rksys.dat2"}) {
        Write(home / name, std::string(4096, '\0'));
        Probe(name, home / name); // Exact basename restriction preserves unrelated blank files.
    }
    fs::remove(report);
    fs::create_directory(report); // A failed report fopen still must allow nonzero save reads.
    reportWritable = false;
    Write(save, "kept nonzero bytes");
    Probe("rksys.dat", save);
    Memory::Reset();
    std::printf("PASS: %u production NAND blank-save/open/read/preservation cases against pinned CheckRead\n", cases);
}
