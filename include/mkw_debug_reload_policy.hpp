#pragma once

namespace mkw::debug {
enum class ReloadMethod { Unavailable,
                          HbloaderNextLoad };
// Loader capability and a proven owner shutdown path are both required.
constexpr ReloadMethod choose_reload_method(bool owner_ready, bool loader_next_load) noexcept {
    return owner_ready && loader_next_load ? ReloadMethod::HbloaderNextLoad : ReloadMethod::Unavailable;
}
} // namespace mkw::debug
