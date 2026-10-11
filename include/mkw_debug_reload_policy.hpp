#pragma once

namespace mkw::debug {
// Application context includes hbmenu title override. Forwarder intent must be
// supplied explicitly; context and system version cannot establish that intent.
constexpr bool program_restart_allowed(bool explicit_forwarder_opt_in,
                                       bool application_context, bool supported_system) noexcept {
    return explicit_forwarder_opt_in && application_context && supported_system;
}
} // namespace mkw::debug
