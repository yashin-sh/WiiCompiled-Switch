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
constexpr const char* DisplayFirst = "sdmc:/switch/WiiCompiled-Switch/display-copy-first.png";
constexpr const char* DisplayLatest = "sdmc:/switch/WiiCompiled-Switch/display-copy-latest.png";
constexpr const char* EfbLatest = "sdmc:/switch/WiiCompiled-Switch/efb-after-copy-latest.png";
constexpr const char* NonBlackFirst = "sdmc:/switch/WiiCompiled-Switch/surface-first-nonblack.png";
constexpr const char* DisplayNonBlackFirst = "sdmc:/switch/WiiCompiled-Switch/display-copy-first-nonblack.png";
constexpr const char* Temporary = "sdmc:/switch/WiiCompiled-Switch/surface-image.tmp";
constexpr const char* Backup = "sdmc:/switch/WiiCompiled-Switch/surface-image.tmp.previous";
constexpr const char* Status = "sdmc:/switch/WiiCompiled-Switch/frame-dump-status.txt";
std::mutex mutex;
Image latest, display, efb;
std::uint64_t runId = 0, frame = 0, savedFrame = 0, firstFrame = 0, displaySavedFrame = 0, efbSavedFrame = 0, displayFirstFrame = 0;
std::uint64_t nonBlackFirstFrame = 0, displayNonBlackFirstFrame = 0;
bool active = false;
char lastError[256]{};
void status(const char* state, const char* reason = "") noexcept {
    if (auto* file = std::fopen(Status, "w")) {
        std::fprintf(file, "status=%s\nreason=%s\nrun_id=%llu\nsource=NVK surface after selected XFB presentation\nphase=last completed present\nframe=%llu\nfirst_png_frame=%llu\nlatest_png_frame=%llu\nwidth=%u\nheight=%u\nrow_bytes=%u\nrgba_bytes=%zu\nnonblack_rgb_pixels=%llu\nnonopaque_pixels=%llu\nuniform=%u\npartial_active_frame_captured=NO\n",
                     state, reason, static_cast<unsigned long long>(runId), static_cast<unsigned long long>(frame),
                     static_cast<unsigned long long>(firstFrame), static_cast<unsigned long long>(savedFrame),
                     latest.layout.width, latest.layout.height, latest.layout.rowBytes, latest.rgba.size(),
                     static_cast<unsigned long long>(latest.nonBlackPixels), static_cast<unsigned long long>(latest.nonOpaquePixels), latest.uniform ? 1u : 0u);
        std::fprintf(file, "display_first_png_frame=%llu\ndisplay_latest_png_frame=%llu\nefb_latest_png_frame=%llu\nefb_phase=after GXCopyDisp clear; not a pre-clear EFB image\npresentation_policy=selected XFB RGB scaled to surface; opaque alpha\n",
                     static_cast<unsigned long long>(displayFirstFrame), static_cast<unsigned long long>(displaySavedFrame), static_cast<unsigned long long>(efbSavedFrame));
        std::fprintf(file, "first_nonblack_png_frame=%llu\ndisplay_first_nonblack_png_frame=%llu\n",
                     static_cast<unsigned long long>(nonBlackFirstFrame), static_cast<unsigned long long>(displayNonBlackFirstFrame));
        for (const auto& item : {std::pair{"display", &display}, std::pair{"efb", &efb}})
            std::fprintf(file, "%s_width=%u\n%s_height=%u\n%s_nonblack_rgb_pixels=%llu\n%s_nonopaque_pixels=%llu\n%s_uniform=%u\n",
                         item.first, item.second->layout.width, item.first, item.second->layout.height,
                         item.first, static_cast<unsigned long long>(item.second->nonBlackPixels),
                         item.first, static_cast<unsigned long long>(item.second->nonOpaquePixels), item.first, item.second->uniform ? 1u : 0u);
        std::fclose(file);
    }
}
void save_latest() {
    if (!frame)
        return;
    if (frame != savedFrame) {
        save(Latest, Temporary, latest);
        savedFrame = frame;
    }
    if (frame != displaySavedFrame) {
        save(DisplayLatest, Temporary, display);
        displaySavedFrame = frame;
    }
    if (frame != efbSavedFrame) {
        save(EfbLatest, Temporary, efb);
        efbSavedFrame = frame;
    }
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
    display = {};
    efb = {};
    frame = savedFrame = firstFrame = displaySavedFrame = efbSavedFrame = displayFirstFrame = 0;
    nonBlackFirstFrame = displayNonBlackFirstFrame = 0;
    lastError[0] = '\0';
    runId = static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    if (captureDisabled()) {
        status("DISABLED", "render-captures-disabled.flag present");
        return;
    }
    for (auto* path : {First, Latest, DisplayFirst, DisplayLatest, EfbLatest, NonBlackFirst, DisplayNonBlackFirst, Temporary, Backup}) {
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
void completed(Image surface, Image displayCopy, Image efbAfterCopy, std::uint64_t number) noexcept {
    std::scoped_lock lock(mutex);
    if (!active)
        return;
    latest = std::move(surface);
    display = std::move(displayCopy);
    efb = std::move(efbAfterCopy);
    frame = number;
    try {
        if (!firstFrame) {
            save(First, Temporary, latest);
            firstFrame = frame;
        }
        if (!displayFirstFrame) {
            save(DisplayFirst, Temporary, display);
            displayFirstFrame = frame;
        }
        if (!nonBlackFirstFrame && latest.nonBlackPixels) {
            save(NonBlackFirst, Temporary, latest);
            nonBlackFirstFrame = frame;
        }
        if (!displayNonBlackFirstFrame && display.nonBlackPixels) {
            save(DisplayNonBlackFirst, Temporary, display);
            displayNonBlackFirstFrame = frame;
        }
        // Retain the exact latest completed pixels of all three stages in RAM.
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
