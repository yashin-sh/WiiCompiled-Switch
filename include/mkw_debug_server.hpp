#pragma once
#include <cstddef>
#include <cstdint>

namespace mkw::debug {
// The listener is disabled unless Config/debug.ini contains debug_server=1.
// Call start after SD initialization; stop joins the worker before socketExit.
bool start() noexcept;
void stop() noexcept;
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
