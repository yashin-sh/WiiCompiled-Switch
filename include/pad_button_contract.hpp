#pragma once

#include <cstdint>

namespace mkw::pad_buttons {
// GC PAD wire masks, checked against the pinned Dolphin SDK in the executable
// contract. Keep this header independent of desktop controller declarations.
inline constexpr std::uint16_t Left = 0x0001;
inline constexpr std::uint16_t Right = 0x0002;
inline constexpr std::uint16_t Down = 0x0004;
inline constexpr std::uint16_t Up = 0x0008;
inline constexpr std::uint16_t Z = 0x0010;
inline constexpr std::uint16_t R = 0x0020;
inline constexpr std::uint16_t L = 0x0040;
inline constexpr std::uint16_t A = 0x0100;
inline constexpr std::uint16_t B = 0x0200;
inline constexpr std::uint16_t X = 0x0400;
inline constexpr std::uint16_t Y = 0x0800;
inline constexpr std::uint16_t Start = 0x1000;
} // namespace mkw::pad_buttons
