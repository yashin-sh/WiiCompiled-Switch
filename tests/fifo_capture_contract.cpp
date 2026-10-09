#include "capture.hpp"
#include "memory.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

extern "C" void mkw_switch_fifo_capture_start() noexcept;
extern "C" void mkw_switch_fifo_capture_begin_frame() noexcept;
extern "C" void mkw_switch_fifo_capture_present(bool) noexcept;
extern "C" void mkw_switch_fifo_capture_checkpoint() noexcept;
extern "C" void mkw_switch_fifo_capture_shutdown() noexcept;
extern "C" void mkw_replay_capture_init() noexcept;
extern "C" void mkw_replay_capture_drain(const unsigned char*, unsigned) noexcept;
extern "C" void mkw_replay_capture_unsupported(const char*) noexcept;

// This controller test has no resources. Actual mapped-memory/relocation
// contracts run in the texture and replay-format tests, not in this seam.
std::vector<Memory::RegionConfig> Memory::DescribeRegions() {
    return {};
}
bool Memory::Contains(std::uint32_t, std::size_t) {
    return false;
}
std::uint8_t* Memory::GetPointer(std::uint32_t, std::size_t) {
    return nullptr;
}

namespace {
std::string read(const char* name) {
    std::ifstream file(std::string("sdmc:/switch/WiiCompiled-Switch/") + name, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), {}};
}
unsigned frames(const std::string& bytes) {
    assert(bytes.size() >= 100 && bytes[6] == '3');
    const auto data = std::span(reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size());
    unsigned count = 0;
    replay::Playback(data).run([&](replay::Kind kind, auto) { count += kind == replay::Kind::Frame || kind == replay::Kind::End; });
    return count;
}
void work() {
    constexpr unsigned char nop = 0;
    mkw_replay_capture_drain(&nop, 1);
}
} // namespace
int main(int argc, char** argv) {
    assert(argc == 2);
    const std::string mode = argv[1];
    std::filesystem::create_directories("sdmc:/switch/WiiCompiled-Switch");
    if (mode == "disabled") {
        std::ofstream("sdmc:/switch/WiiCompiled-Switch/first-frame.mkwr") << "previous capture";
        std::ofstream("sdmc:/switch/WiiCompiled-Switch/render-captures-disabled.flag");
        mkw_switch_fifo_capture_start();
        mkw_switch_fifo_capture_begin_frame();
        mkw_replay_capture_init();
        work();
        mkw_switch_fifo_capture_present(true);
        mkw_switch_fifo_capture_shutdown();
        assert(read("first-frame.mkwr") == "previous capture");
        assert(read("fifo-capture-status.txt").find("status=DISABLED\n") != std::string::npos);
        assert(!std::filesystem::exists("sdmc:/switch/WiiCompiled-Switch/latest-frames.mkwr"));
        std::puts("PASS: SD capture disable marker preserves prior files and keeps hooks inactive");
        return 0;
    }
    mkw_switch_fifo_capture_start();
    mkw_switch_fifo_capture_begin_frame();
    mkw_replay_capture_init();
    work();
    mkw_switch_fifo_capture_present(true);
    assert(frames(read("first-frame.mkwr")) == 1 && frames(read("latest-frames.mkwr")) == 1);
    const auto first = read("first-frame.mkwr");
    if (mode == "flush-failure" || mode == "flush-present-failure") {
        for (unsigned i = 0; i < 3; ++i) {
            mkw_switch_fifo_capture_begin_frame();
            work();
            mkw_switch_fifo_capture_present(true);
        }
        assert(frames(read("latest-frames.mkwr")) == 1);
        mkw_switch_fifo_capture_begin_frame();
        work();
        if (mode == "flush-failure")
            mkw_replay_capture_unsupported("injected failure before a fifth complete frame");
        else
            mkw_switch_fifo_capture_present(false);
        assert(frames(read("latest-frames.mkwr")) == 4);
        assert(read("first-frame.mkwr") == first);
        const auto status = read("fifo-capture-status.txt");
        assert(status.find("status=INVALID\n") != std::string::npos && status.find("saved_frame=4\n") != std::string::npos);
        mkw_switch_fifo_capture_shutdown();
        std::puts("PASS: capture failure saves the last completed prefix without a later present or normal shutdown");
        return 0;
    }
    mkw_switch_fifo_capture_begin_frame();
    work();
    if (mode == "partial") {
        mkw_switch_fifo_capture_present(true);
        mkw_switch_fifo_capture_begin_frame();
        work(); // unfinished third frame
        mkw_switch_fifo_capture_checkpoint();
        assert(frames(read("latest-frames.mkwr")) == 2);
        const auto status = read("fifo-capture-status.txt");
        assert(status.find("status=COMPLETE\n") != std::string::npos && status.find("saved_frame=2\n") != std::string::npos);
    } else {
        if (mode == "invalid")
            mkw_replay_capture_unsupported("injected unsupported command");
        else {
            assert(mode == "failed-present");
            mkw_switch_fifo_capture_present(false);
        }
        mkw_switch_fifo_capture_checkpoint();
        assert(frames(read("latest-frames.mkwr")) == 1);
        assert(read("fifo-capture-status.txt").find("status=INVALID\n") != std::string::npos);
    }
    assert(read("first-frame.mkwr") == first);
    assert(!std::filesystem::exists("sdmc:/switch/WiiCompiled-Switch/first-frame.mkwr.tmp.previous"));
    mkw_switch_fifo_capture_shutdown();
    std::puts("PASS: real SD capture controller preserves completed prefix and first file across partial/failing later frames");
}
