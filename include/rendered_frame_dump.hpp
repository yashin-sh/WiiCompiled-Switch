#pragma once
#include "frame_dump_image.hpp"

namespace mkw::frame_dump {
void reset() noexcept;
bool enabled() noexcept;
void failure(const char* reason) noexcept;
void completed(Image surface, Image displayCopy, Image efbAfterCopy, std::uint64_t frame) noexcept;
} // namespace mkw::frame_dump
extern "C" void mkw_switch_frame_dump_checkpoint() noexcept;
