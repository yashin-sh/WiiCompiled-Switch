#if defined(MKW_RENDERED_FIFO_CAPTURE) && MKW_RENDERED_FIFO_CAPTURE
#include "capture.hpp"
#include "memory.h"
#include "frame_dump_image.hpp"

#include <cstdio>
#include <chrono>
#include <cerrno>
#include <mutex>
#include <string>
#include <utility>
#include <memory>
#include <stdexcept>

extern "C" void mkw_switch_fifo_capture_checkpoint() noexcept;

namespace {
constexpr const char* Output = "sdmc:/switch/WiiCompiled-Switch/first-frame.mkwr";
constexpr const char* Temporary = "sdmc:/switch/WiiCompiled-Switch/first-frame.mkwr.tmp";
constexpr const char* Latest = "sdmc:/switch/WiiCompiled-Switch/latest-frames.mkwr";
constexpr const char* Status = "sdmc:/switch/WiiCompiled-Switch/fifo-capture-status.txt";
std::unique_ptr<replay::Recorder> recorder;
bool attempted = false;
std::mutex captureMutex;
thread_local bool captureLockHeld = false;
struct CaptureLockScope {
    bool previous = std::exchange(captureLockHeld, true);
    ~CaptureLockScope() {
        captureLockHeld = previous;
    }
    void release() noexcept {
        captureLockHeld = previous;
    }
};
replay::Bytes lastComplete;
std::uint64_t runId = 0, completedFrames = 0, savedFrame = 0;
std::size_t savedBytes = 0;
char failureReason[256]{};
void status(const char* state, const char* reason = "", std::size_t bytes = 0) noexcept {
    if (auto* file = std::fopen(Status, "w")) {
        std::fprintf(file, "status=%s\nreason=%s\nbytes=%zu\nrun_id=%llu\nframe=prefix-from-GXInit\ncompleted_frames=%llu\nsaved_frame=%llu\nformat=MKWRPL3\nlimit_bytes=%zu\npartial_active_frame_captured=NO\n",
                     state, reason, bytes, static_cast<unsigned long long>(runId),
                     static_cast<unsigned long long>(completedFrames), static_cast<unsigned long long>(savedFrame), replay::MaxBytes);
        std::fclose(file);
    }
}
void invalid(const char* reason) noexcept {
    std::snprintf(failureReason, sizeof(failureReason), "%s", reason);
    status("INVALID", reason, savedBytes);
    // Raw-drain failures can persist the last complete prefix immediately.
    // If present owns the lock, it retries after releasing that lock below.
    mkw_switch_fifo_capture_checkpoint();
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
    runId = std::chrono::steady_clock::now().time_since_epoch().count();
    if (mkw::frame_dump::captureDisabled()) {
        status("DISABLED", "render-captures-disabled.flag present");
        return;
    }
    const std::string backup = std::string(Temporary) + ".previous";
    for (auto* path : {Output, Latest, Temporary, backup.c_str()}) {
        if (std::remove(path) != 0 && errno != ENOENT) {
            invalid("cannot retire previous capture");
            return;
        }
    }
    replay::set_failure_handler(invalid);
    try {
        recorder = std::make_unique<replay::Recorder>(1280, 720);
        recorder->resolve_with(resolve);
        replay::set_recorder(recorder.get());
        status("RECORDING");
    } catch (const std::exception& error) {
        status("INVALID", error.what());
    } catch (...) {
        status("INVALID", "capture allocation failure");
    }
}
extern "C" void mkw_switch_fifo_capture_begin_frame() noexcept {
    if (!recorder || !replay::recording())
        return;
    try {
        recorder->begin();
    } catch (const std::exception& error) {
        replay::fail(error.what());
    } catch (...) {
        replay::fail("capture begin allocation failure");
    }
}
namespace {
void save_latest() {
    if (lastComplete.empty() || savedFrame == completedFrames)
        return;
    mkw::frame_dump::saveBytes(Latest, Temporary, lastComplete);
    savedFrame = completedFrames;
    savedBytes = lastComplete.size();
}
} // namespace
extern "C" void mkw_switch_fifo_capture_present(bool presented) noexcept {
    std::unique_lock lock(captureMutex);
    CaptureLockScope scope;
    if (!recorder || !replay::recording())
        return;
    if (!presented) {
        replay::fail("present failed");
        lock.unlock();
        scope.release();
        mkw_switch_fifo_capture_checkpoint();
        return;
    }
    try {
        recorder->frame();
        auto bytes = recorder->checkpoint();
        replay::Playback validate(bytes);
        lastComplete = std::move(bytes);
        ++completedFrames;
        if (completedFrames == 1u)
            mkw::frame_dump::saveBytes(Output, Temporary, lastComplete);
        if (completedFrames == 1u || completedFrames % 30u == 0u)
            save_latest();
        status("RECORDING", "", savedBytes);
    } catch (const std::exception& error) {
        replay::fail(error.what());
    } catch (...) {
        replay::fail("capture checkpoint allocation failure");
    }
    lock.unlock();
    scope.release();
    if (!replay::recording())
        mkw_switch_fifo_capture_checkpoint();
}
extern "C" void mkw_switch_fifo_capture_checkpoint() noexcept {
    // Never try_lock a non-recursive mutex already owned by this thread.
    if (captureLockHeld)
        return;
    std::unique_lock lock(captureMutex, std::try_to_lock);
    if (!lock || lastComplete.empty())
        return;
    CaptureLockScope scope;
    try {
        save_latest();
        status(failureReason[0] ? "INVALID" : "COMPLETE", failureReason, savedBytes);
    } catch (const std::exception& error) {
        invalid(error.what());
    } catch (...) {
        invalid("capture save allocation failure");
    }
}
extern "C" void mkw_switch_fifo_capture_shutdown() noexcept {
    if (!recorder)
        return;
    mkw_switch_fifo_capture_checkpoint();
    if (!completedFrames)
        replay::fail("renderer stopped before first complete present");
    replay::set_recorder(nullptr);
    recorder.reset();
}
#endif
