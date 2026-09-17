#include <cstdint>

#if (defined(MKW_LOCAL_FAST_TRACK) && MKW_LOCAL_FAST_TRACK) || \
    (defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK)

#include <switch.h>

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

namespace {

constexpr const char* kHeartbeatPath =
    "sdmc:/switch/WiiCompiled-Switch/fast-track-heartbeat.txt";
constexpr const char* kHeartbeatHistoryPath =
    "sdmc:/switch/WiiCompiled-Switch/fast-track-heartbeat-history.txt";
constexpr std::int64_t kPollIntervalNs = 1'000'000'000LL;
constexpr std::size_t kHeartbeatBufferSize = 2048u;
constexpr std::uint64_t kMaxHistorySamples = 180u;
constexpr std::uint64_t kFsyncStride = 5u;

Thread g_watchdog_thread{};
bool g_history_initialized = false;
bool g_have_previous = false;
std::uint64_t g_sample_index = 0u;
std::uint64_t g_first_sample_tick = 0u;
std::uint64_t g_previous_hash = 0u;
std::uint64_t g_previous_dispatch = 0u;
std::uint32_t g_previous_target = 0u;
std::uint32_t g_stale_seconds = 0u;

std::uint64_t HashBytes(const char* data, std::size_t size) noexcept {
    constexpr std::uint64_t kOffset = 1469598103934665603ull;
    constexpr std::uint64_t kPrime = 1099511628211ull;
    std::uint64_t hash = kOffset;
    for (std::size_t i = 0u; i < size; ++i) {
        hash ^= static_cast<std::uint8_t>(data[i]);
        hash *= kPrime;
    }
    return hash;
}

bool ReadHeartbeat(char* buffer, std::size_t capacity, std::size_t* out_size) noexcept {
    if (!buffer || capacity < 2u || !out_size) {
        return false;
    }

    for (int attempt = 0; attempt < 2; ++attempt) {
        const int fd = ::open(kHeartbeatPath, O_RDONLY);
        if (fd >= 0) {
            const ssize_t rc = ::read(fd, buffer, capacity - 1u);
            ::close(fd);
            if (rc > 0) {
                *out_size = static_cast<std::size_t>(rc);
                buffer[*out_size] = '\0';
                return true;
            }
        }

        // The normal heartbeat writer truncates then rewrites atomically enough
        // for diagnostics. A short retry avoids treating that tiny window as a
        // translated-runtime stall.
        svcSleepThread(10'000'000LL);
    }

    return false;
}

bool ParseU64Field(const char* text, const char* label, std::uint64_t* out) noexcept {
    if (!text || !label || !out) {
        return false;
    }
    const char* value = std::strstr(text, label);
    if (!value) {
        return false;
    }
    value += std::strlen(label);
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(value, &end, 0);
    if (end == value) {
        return false;
    }
    *out = static_cast<std::uint64_t>(parsed);
    return true;
}

bool ParseU32Field(const char* text, const char* label, std::uint32_t* out) noexcept {
    std::uint64_t value = 0u;
    if (!ParseU64Field(text, label, &value)) {
        return false;
    }
    *out = static_cast<std::uint32_t>(value);
    return true;
}

void WriteAll(int fd, const char* data, std::size_t size) noexcept {
    std::size_t written = 0u;
    while (written < size) {
        const ssize_t rc = ::write(fd, data + written, size - written);
        if (rc <= 0) {
            return;
        }
        written += static_cast<std::size_t>(rc);
    }
}

void InitializeHistoryFile() noexcept {
    if (g_history_initialized) {
        return;
    }

    ::unlink(kHeartbeatHistoryPath);
    const int fd = ::open(kHeartbeatHistoryPath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        return;
    }

    constexpr char kHeader[] =
        "WiiCompiled-Switch independent liveness watchdog\n"
        "================================================\n"
        "status ACTIVE = translated heartbeat changed since previous watchdog sample\n"
        "status STALE  = watchdog is alive but translated heartbeat did not change\n";
    WriteAll(fd, kHeader, sizeof(kHeader) - 1u);
    ::fsync(fd);
    ::close(fd);
    g_history_initialized = true;
}

void AppendHistorySample(std::uint64_t now,
                         std::uint64_t frequency,
                         std::uint64_t dispatch,
                         std::uint64_t post_main,
                         std::uint32_t target,
                         std::uint32_t guest_pc,
                         std::uint32_t r1,
                         std::uint32_t r3,
                         std::uint64_t hash) noexcept {
    if (g_sample_index >= kMaxHistorySamples) {
        return;
    }

    InitializeHistoryFile();
    if (!g_history_initialized) {
        return;
    }

    if (g_first_sample_tick == 0u) {
        g_first_sample_tick = now;
    }

    const bool changed =
        !g_have_previous || hash != g_previous_hash || dispatch != g_previous_dispatch;
    if (changed) {
        g_stale_seconds = 0u;
    } else if (g_stale_seconds != 0xFFFFFFFFu) {
        ++g_stale_seconds;
    }

    const std::uint64_t dispatch_delta =
        g_have_previous && dispatch >= g_previous_dispatch
            ? dispatch - g_previous_dispatch
            : 0u;
    const bool same_target = g_have_previous && target == g_previous_target;
    const std::uint64_t elapsed_ms =
        frequency != 0u && now >= g_first_sample_tick
            ? ((now - g_first_sample_tick) * 1000u) / frequency
            : g_sample_index * 1000u;

    ++g_sample_index;
    char line[512];
    const int n = std::snprintf(
        line,
        sizeof(line),
        "[%03llu] host_ms=%llu status=%s stale_s=%u dispatch=%llu delta=%llu "
        "post_main=%llu target=0x%08x same_target=%s pc=0x%08x r1=0x%08x r3=0x%08x\n",
        static_cast<unsigned long long>(g_sample_index),
        static_cast<unsigned long long>(elapsed_ms),
        changed ? "ACTIVE" : "STALE",
        g_stale_seconds,
        static_cast<unsigned long long>(dispatch),
        static_cast<unsigned long long>(dispatch_delta),
        static_cast<unsigned long long>(post_main),
        target,
        same_target ? "YES" : "NO",
        guest_pc,
        r1,
        r3);
    if (n <= 0) {
        return;
    }

    const std::size_t size =
        static_cast<std::size_t>(n) < sizeof(line)
            ? static_cast<std::size_t>(n)
            : sizeof(line) - 1u;
    const int fd = ::open(kHeartbeatHistoryPath, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd >= 0) {
        WriteAll(fd, line, size);
        // Keep normal ACTIVE sampling cheap, but make stall evidence durable
        // immediately and checkpoint active history every few seconds.
        if (!changed || g_sample_index == 1u || (g_sample_index % kFsyncStride) == 0u) {
            ::fsync(fd);
        }
        ::close(fd);
    }

    g_previous_hash = hash;
    g_previous_dispatch = dispatch;
    g_previous_target = target;
    g_have_previous = true;
}

void WatchdogThread(void*) noexcept {
    while (true) {
        char heartbeat[kHeartbeatBufferSize]{};
        std::size_t heartbeat_size = 0u;
        if (ReadHeartbeat(heartbeat, sizeof(heartbeat), &heartbeat_size)) {
            std::uint64_t dispatch = 0u;
            std::uint64_t post_main = 0u;
            std::uint32_t target = 0u;
            std::uint32_t guest_pc = 0u;
            std::uint32_t r1 = 0u;
            std::uint32_t r3 = 0u;

            const bool parsed =
                ParseU64Field(heartbeat, "dispatch count        : ", &dispatch) &&
                ParseU64Field(heartbeat, "post-main dispatch    : ", &post_main) &&
                ParseU32Field(heartbeat, "last target           : ", &target) &&
                ParseU32Field(heartbeat, "guest pc              : ", &guest_pc) &&
                ParseU32Field(heartbeat, "r1                    : ", &r1) &&
                ParseU32Field(heartbeat, "r3                    : ", &r3);

            if (parsed) {
                const std::uint64_t now = armGetSystemTick();
                AppendHistorySample(
                    now,
                    armGetSystemTickFreq(),
                    dispatch,
                    post_main,
                    target,
                    guest_pc,
                    r1,
                    r3,
                    HashBytes(heartbeat, heartbeat_size));
            }
        }

        svcSleepThread(kPollIntervalNs);
    }
}

#if defined(MKW_LOCAL_FAST_TRACK) && MKW_LOCAL_FAST_TRACK
__attribute__((constructor)) void StartFastTrackLivenessWatchdog() noexcept {
    // Diagnostics only: one low-duty Horizon thread, 32 KiB auto-allocated
    // stack, default process core. It never mutates guest CPU/memory/scheduler
    // state and therefore cannot manufacture progress in the translated path.
    const Result create_rc = threadCreate(
        &g_watchdog_thread,
        WatchdogThread,
        nullptr,
        nullptr,
        0x8000u,
        0x3Bu,
        -2);
    if (R_FAILED(create_rc)) {
        return;
    }
    const Result start_rc = threadStart(&g_watchdog_thread);
    if (R_FAILED(start_rc)) {
        threadClose(&g_watchdog_thread);
    }
}
#endif

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK
extern "C" __attribute__((used)) bool mkw_switch_liveness_watchdog_synthetic_probe() noexcept {
    constexpr char kSample[] =
        "dispatch count        : 37148\n"
        "post-main dispatch    : 36543\n"
        "last target           : 0x8020fcd4\n"
        "guest pc              : 0x8024373c\n"
        "r1                    : 0x90112608\n"
        "r3                    : 0x00000f8f\n";

    std::uint64_t dispatch = 0u;
    std::uint64_t post_main = 0u;
    std::uint32_t target = 0u;
    return ParseU64Field(kSample, "dispatch count        : ", &dispatch) &&
           ParseU64Field(kSample, "post-main dispatch    : ", &post_main) &&
           ParseU32Field(kSample, "last target           : ", &target) &&
           dispatch == 37148u && post_main == 36543u && target == 0x8020FCD4u &&
           HashBytes(kSample, sizeof(kSample) - 1u) != 0u;
}
#endif

} // namespace

#endif
