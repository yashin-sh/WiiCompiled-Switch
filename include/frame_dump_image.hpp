#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace mkw::frame_dump {
struct Layout {
    std::uint32_t width, height, rowBytes;
    std::uint64_t bufferBytes;
};
struct Image {
    Layout layout{};
    std::vector<std::uint8_t> rgba;
    std::uint64_t nonBlackPixels = 0, nonOpaquePixels = 0;
    bool uniform = true;
};
// Readback is intentionally bounded to a single-sample 8-bit surface.
Layout layout(std::uint32_t width, std::uint32_t height);
Image unpack(Layout layout, std::span<const std::uint8_t> mapped, bool bgra);
std::vector<std::uint8_t> png(const Image& image);
void save(const char* output, const char* temporary, const Image& image);
void saveBytes(const char* output, const char* temporary, std::span<const std::uint8_t> bytes);
// Presence is sampled at startup by both controllers; previous captures stay
// intact when sdmc:/switch/WiiCompiled-Switch/render-captures-disabled.flag exists.
bool captureDisabled() noexcept;
} // namespace mkw::frame_dump
