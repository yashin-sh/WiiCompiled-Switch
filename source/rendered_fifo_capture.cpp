#if defined(MKW_RENDERED_FIFO_CAPTURE) && MKW_RENDERED_FIFO_CAPTURE
#include "capture.hpp"
#include "memory.h"

#include <cstdio>
#include <memory>
#include <stdexcept>

namespace {
constexpr const char* Output = "sdmc:/switch/WiiCompiled-Switch/first-frame.mkwr";
constexpr const char* Temporary = "sdmc:/switch/WiiCompiled-Switch/first-frame.mkwr.tmp";
constexpr const char* Status = "sdmc:/switch/WiiCompiled-Switch/fifo-capture-status.txt";
std::unique_ptr<replay::Recorder> recorder;
bool attempted = false;
void status(const char* state, const char* reason = "", std::size_t bytes = 0) noexcept {
    if (auto* file = std::fopen(Status, "w")) {
        std::fprintf(file, "status=%s\nreason=%s\nbytes=%zu\nframe=first-from-GXInit\nformat=MKWRPL2\nlimit_bytes=%zu\n", state, reason, bytes, replay::MaxBytes);
        std::fclose(file);
    }
}
void invalid(const char* reason) noexcept {
    status("INVALID", reason);
}
std::span<const std::uint8_t> resolve(std::uint64_t pointer, std::size_t needed) {
    // Prove a host range belongs to an allocated guest region before reading it.
    // GetPointer does not resolve deferred EFB reads; these bytes are a RAM
    // snapshot, while ordered CopyTex events reproduce the GPU copy separately.
    for (const auto& region : Memory::DescribeRegions()) {
        if (!region.sizeBytes || !Memory::Contains(region.baseAddress, region.sizeBytes))
            continue;
        const auto base = reinterpret_cast<std::uintptr_t>(Memory::GetPointer(region.baseAddress, region.sizeBytes));
        if (pointer >= base && pointer - base < region.sizeBytes && needed <= region.sizeBytes - (pointer - base)) {
            return {reinterpret_cast<const std::uint8_t*>(pointer), needed};
        }
    }
    throw std::runtime_error("capture resource is outside mapped guest RAM");
}
} // namespace
extern "C" void mkw_switch_fifo_capture_start() noexcept {
    if (attempted) {
        if (replay::recording())
            replay::fail("repeated guest GXInit");
        return;
    }
    attempted = true;
    std::remove(Output);
    std::remove(Temporary);
    replay::set_failure_handler(invalid);
    try {
        recorder = std::make_unique<replay::Recorder>(1280, 720);
        recorder->resolve_with(resolve);
        recorder->begin();
        replay::set_recorder(recorder.get());
        status("RECORDING");
    } catch (const std::exception& error) {
        status("INVALID", error.what());
    } catch (...) {
        status("INVALID", "capture allocation failure");
    }
}
extern "C" void mkw_switch_fifo_capture_present(bool presented) noexcept {
    if (!recorder)
        return;
    if (!replay::recording()) {
        recorder.reset();
        return;
    }
    if (!presented) {
        replay::fail("first present failed");
        recorder.reset();
        return;
    }
    try {
        recorder->end();
        const auto bytes = recorder->finish();
        replay::Playback validate(bytes);
        auto* file = std::fopen(Temporary, "wb");
        if (!file)
            throw std::runtime_error("cannot open SD capture");
        const bool written = std::fwrite(bytes.data(), 1, bytes.size(), file) == bytes.size();
        const int closed = std::fclose(file);
        if (!written || closed != 0 || std::rename(Temporary, Output) != 0) {
            std::remove(Temporary);
            throw std::runtime_error("cannot save complete SD capture");
        }
        status("COMPLETE", "", bytes.size());
        replay::set_recorder(nullptr);
    } catch (const std::exception& error) {
        replay::fail(error.what());
    } catch (...) {
        replay::fail("capture save failure");
    }
    recorder.reset();
}
extern "C" void mkw_switch_fifo_capture_shutdown() noexcept {
    replay::fail("renderer stopped before first complete present");
    replay::set_recorder(nullptr);
    recorder.reset();
}
#endif
