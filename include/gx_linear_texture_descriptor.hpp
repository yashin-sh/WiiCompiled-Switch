#pragma once

#include <array>
#include <cstdint>

namespace mkw::gx {
// The audited non-paletted family uses linear filtering, clamp for I4/RGB5A3
// or clamp/repeat on both axes for IA4/IA8, disabled edge LOD, zero LOD/bias,
// no user data/TLUT and no mipmaps. Object and source addresses are identities,
// not admission criteria.
constexpr std::uint32_t linearTextureBytes(const std::array<std::uint32_t, 8>& words, std::uint32_t slot) noexcept {
    const auto format = words[5];
    if (slot >= 8u || (format != 0u && format != 2u && format != 3u && format != 5u) ||
        (words[0] != 0x190u && ((format != 2u && format != 3u) || words[0] != 0x195u)) || words[1] != 0u || words[4] != 0u || words[6] != 0u ||
        words[3] == 0u || (words[3] & 0xff000000u) != 0u)
        return 0u;
    const auto width = (words[2] & 0x3ffu) + 1u;
    const auto height = ((words[2] >> 10u) & 0x3ffu) + 1u;
    if (words[2] != ((width - 1u) | ((height - 1u) << 10u) | (format << 20u)))
        return 0u;
    const auto tileWidth = format == 0u || format == 2u ? 8u : 4u;
    const auto tileHeight = format == 0u ? 8u : 4u;
    const auto tiles = ((width + tileWidth - 1u) / tileWidth) * ((height + tileHeight - 1u) / tileHeight);
    // The SDK stores only 15 bits of tile count. Refuse wrapped counts rather
    // than interpreting zero as a valid declaration of a large backing range.
    if (tiles > 0x7fffu || words[7] != ((tiles << 16u) | ((format == 0u ? 1u : 2u) << 8u) | 2u))
        return 0u;
    return tiles * 32u;
}
} // namespace mkw::gx
