#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK && defined(MKW_RENDERED_FRAME_DUMP) && MKW_RENDERED_FRAME_DUMP
#include "rendered_frame_dump.hpp"

#include <chrono>
#include <cerrno>
#include <cstdio>
#include <exception>
#include <mutex>
#include <utility>

namespace mkw::frame_dump {
namespace {
constexpr const char* First = "sdmc:/switch/WiiCompiled-Switch/surface-first.png";
constexpr const char* Latest = "sdmc:/switch/WiiCompiled-Switch/surface-latest.png";
constexpr const char* Temporary = "sdmc:/switch/WiiCompiled-Switch/surface-image.tmp";
constexpr const char* Backup = "sdmc:/switch/WiiCompiled-Switch/surface-image.tmp.previous";
constexpr const char* Status = "sdmc:/switch/WiiCompiled-Switch/frame-dump-status.txt";
std::mutex mutex;
Image latest;
std::uint64_t runId = 0, frame = 0, savedFrame = 0, firstFrame = 0;
bool active = false;
char lastError[256]{};
void status(const char* state, const char* reason = "") noexcept {
    if (auto* file = std::fopen(Status, "w")) {
        std::fprintf(file, "status=%s\nreason=%s\nrun_id=%llu\nsource=NVK surface after Aurora render\nphase=last completed present\nframe=%llu\nfirst_png_frame=%llu\nlatest_png_frame=%llu\nwidth=%u\nheight=%u\nrow_bytes=%u\nrgba_bytes=%zu\nnonblack_rgb_pixels=%llu\nnonopaque_pixels=%llu\nuniform=%u\npartial_active_frame_captured=NO\n",
                     state, reason, static_cast<unsigned long long>(runId), static_cast<unsigned long long>(frame),
                     static_cast<unsigned long long>(firstFrame), static_cast<unsigned long long>(savedFrame),
                     latest.layout.width, latest.layout.height, latest.layout.rowBytes, latest.rgba.size(),
                     static_cast<unsigned long long>(latest.nonBlackPixels), static_cast<unsigned long long>(latest.nonOpaquePixels), latest.uniform ? 1u : 0u);
        std::fclose(file);
    }
}
void save_latest() {
    if (!frame || frame == savedFrame)
        return;
    save(Latest, Temporary, latest);
    savedFrame = frame;
}
void fail(const char* reason) noexcept {
    active = false;
    std::snprintf(lastError, sizeof(lastError), "%s", reason);
    status("FAILED", lastError);
}
} // namespace
void reset() noexcept {
    std::scoped_lock lock(mutex);
    active = false;
    latest = {};
    frame = savedFrame = firstFrame = 0;
    lastError[0] = '\0';
    runId = static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    for (auto* path : {First, Latest, Temporary, Backup}) {
        // Never leave a previous run's image labelled as this run's output.
        if (std::remove(path) != 0 && errno != ENOENT) {
            fail("cannot retire previous image");
            return;
        }
    }
    active = true;
    status("WAITING");
}
bool enabled() noexcept {
    std::scoped_lock lock(mutex);
    return active;
}
void failure(const char* reason) noexcept {
    std::scoped_lock lock(mutex);
    fail(reason);
}
void completed(Image image, std::uint64_t number) noexcept {
    std::scoped_lock lock(mutex);
    if (!active)
        return;
    latest = std::move(image);
    frame = number;
    try {
        if (!firstFrame) {
            save(First, Temporary, latest);
            firstFrame = frame;
        }
        // Keep only two files; retain the exact latest completed pixels in RAM.
        // Checkpoint on a diagnosed stop saves frames between these intervals.
        if (frame == 1u || frame % 30u == 0u)
            save_latest();
        status("READBACK_COMPLETE");
    } catch (const std::exception& error) {
        fail(error.what());
    } catch (...) {
        fail("image save allocation failure");
    }
}
void checkpoint() noexcept {
    // This may be called by an unsupported dispatch. Do not acquire the
    // renderer lock or submit/complete the partially recorded guest frame.
    std::unique_lock lock(mutex, std::try_to_lock);
    if (!lock || !frame)
        return;
    try {
        save_latest();
        status(active ? "COMPLETE" : "FAILED", active ? "" : lastError);
    } catch (const std::exception& error) {
        fail(error.what());
    } catch (...) {
        fail("checkpoint allocation failure");
    }
}
} // namespace mkw::frame_dump
extern "C" void mkw_switch_frame_dump_checkpoint() noexcept {
    mkw::frame_dump::checkpoint();
}
#endif
