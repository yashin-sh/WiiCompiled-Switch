#pragma once
#include <cstddef>
#include <cstdint>
#include "mkw_debug_reload_policy.hpp"

namespace mkw::debug {
// The listener is disabled unless Config/debug.ini contains debug_server=1.
// Call start after SD initialization; stop joins the worker before socketExit.
// Supply argv[0] before start so runtime identity belongs to the executed file.
void set_launch_path(const char* path) noexcept;
bool start() noexcept;
// Worker only requests. After guest/render work is quiescent, the owning thread
// prepares next-load, then stop() joins the worker before normal exit.
// The runtime enables this only after it can stop every guest/render worker
// without unwinding noexcept bridges or abandoning held locks.
void set_reload_owner_available(bool available) noexcept;
ReloadMethod reload_request_method() noexcept;
bool reload_requested() noexcept;
bool prepare_reload_on_owner() noexcept;
// False retains worker/resources for retry; never return to the loader then.
bool stop() noexcept;
bool enabled() noexcept;
void note_stage(const char* stage) noexcept;
void note_dispatch(std::uint64_t count, std::uint32_t guest_pc,
                   const char* frontier) noexcept;
void note_present() noexcept;
// Best-effort bounded copy: producers never wait for networking or a lock.
void log(const char* bytes, std::size_t size) noexcept;
#if !defined(__SWITCH__)
// POSIX harness seam; no SD or Switch service dependency. Port 0 is ephemeral.
struct HostConfig {
    bool enabled = false;
    const char* root = "WiiCompiled-Switch";
    const char* nro_path = "WiiCompiled-Switch/WiiCompiled-Switch.nro";
    std::uint16_t port = 6543;
};
bool start(const HostConfig& config) noexcept;
std::uint16_t bound_port() noexcept;
#endif
} // namespace mkw::debug
