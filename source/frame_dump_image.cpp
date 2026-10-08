#include "frame_dump_image.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <sys/stat.h>

namespace mkw::frame_dump {
namespace {
void integer(std::vector<std::uint8_t>& out, std::uint32_t value) {
    for (int shift = 24; shift >= 0; shift -= 8)
        out.push_back(static_cast<std::uint8_t>(value >> shift));
}
std::uint32_t crc(std::span<const std::uint8_t> bytes) {
    std::uint32_t value = 0xffffffffu;
    for (auto byte : bytes) {
        value ^= byte;
        for (unsigned bit = 0; bit < 8; ++bit)
            value = (value >> 1) ^ (0xedb88320u & (0u - (value & 1u)));
    }
    return ~value;
}
void chunk(std::vector<std::uint8_t>& out, const char* type, std::span<const std::uint8_t> data) {
    integer(out, static_cast<std::uint32_t>(data.size()));
    const auto start = out.size();
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), data.begin(), data.end());
    integer(out, crc(std::span(out).subspan(start)));
}
} // namespace
Layout layout(std::uint32_t width, std::uint32_t height) {
    if (!width || !height || width > 2048u || height > 2048u)
        throw std::runtime_error("image dimensions outside 1..2048");
    const auto row = (width * 4u + 255u) & ~255u;
    return {width, height, row, std::uint64_t(row) * height};
}
Image unpack(Layout size, std::span<const std::uint8_t> mapped, bool bgra) {
    const auto expected = layout(size.width, size.height);
    if (size.rowBytes != expected.rowBytes || size.bufferBytes != expected.bufferBytes || mapped.size() < size.bufferBytes)
        throw std::runtime_error("invalid or short mapped readback");
    Image image{size, std::vector<std::uint8_t>(std::size_t(size.width) * size.height * 4u)};
    for (std::uint32_t y = 0; y < size.height; ++y) {
        for (std::uint32_t x = 0; x < size.width; ++x) {
            const auto source = std::size_t(y) * size.rowBytes + x * 4u;
            const auto target = (std::size_t(y) * size.width + x) * 4u;
            image.rgba[target] = mapped[source + (bgra ? 2u : 0u)];
            image.rgba[target + 1] = mapped[source + 1];
            image.rgba[target + 2] = mapped[source + (bgra ? 0u : 2u)];
            image.rgba[target + 3] = mapped[source + 3];
            image.nonBlackPixels += (image.rgba[target] | image.rgba[target + 1] | image.rgba[target + 2]) != 0;
            image.nonOpaquePixels += image.rgba[target + 3] != 255;
            image.uniform &= std::equal(image.rgba.begin(), image.rgba.begin() + 4, image.rgba.begin() + target);
        }
    }
    return image;
}
std::vector<std::uint8_t> png(const Image& image) {
    layout(image.layout.width, image.layout.height);
    if (image.rgba.size() != std::size_t(image.layout.width) * image.layout.height * 4u)
        throw std::runtime_error("invalid packed RGBA extent");
    std::vector<std::uint8_t> out{137, 80, 78, 71, 13, 10, 26, 10}, header;
    integer(header, image.layout.width);
    integer(header, image.layout.height);
    header.insert(header.end(), {8, 6, 0, 0, 0});
    chunk(out, "IHDR", header);
    std::vector<std::uint8_t> rows;
    const auto rowBytes = std::size_t(image.layout.width) * 4u;
    rows.reserve((rowBytes + 1u) * image.layout.height);
    for (std::uint32_t y = 0; y < image.layout.height; ++y) {
        rows.push_back(0); // PNG filter None; no compression library on Switch.
        rows.insert(rows.end(), image.rgba.begin() + y * rowBytes, image.rgba.begin() + (y + 1u) * rowBytes);
    }
    std::vector<std::uint8_t> deflate{0x78, 0x01};
    std::uint32_t a = 1, b = 0;
    for (auto byte : rows) {
        a = (a + byte) % 65521u;
        b = (b + a) % 65521u;
    }
    for (std::size_t offset = 0; offset < rows.size();) {
        const auto count = static_cast<std::uint16_t>(std::min<std::size_t>(65535u, rows.size() - offset));
        deflate.push_back(offset + count == rows.size() ? 1u : 0u);
        deflate.push_back(count & 255u);
        deflate.push_back(count >> 8);
        const auto inverse = static_cast<std::uint16_t>(~count);
        deflate.push_back(inverse & 255u);
        deflate.push_back(inverse >> 8);
        deflate.insert(deflate.end(), rows.begin() + offset, rows.begin() + offset + count);
        offset += count;
    }
    integer(deflate, (b << 16) | a);
    chunk(out, "IDAT", deflate);
    chunk(out, "IEND", {});
    return out;
}
void save(const char* output, const char* temporary, const Image& image) {
    const auto bytes = png(image);
    const std::string backup = std::string(temporary) + ".previous";
    struct stat existing = {};
    const bool hadPrevious = ::stat(output, &existing) == 0;
    if ((hadPrevious && !S_ISREG(existing.st_mode)) || (!hadPrevious && errno != ENOENT))
        throw std::runtime_error("image destination is not an accessible regular file");
    if (::stat(backup.c_str(), &existing) == 0 || errno != ENOENT)
        throw std::runtime_error("previous-image backup already exists or is inaccessible");
    auto* file = std::fopen(temporary, "wb");
    if (!file)
        throw std::runtime_error("cannot open temporary image");
    const bool written = std::fwrite(bytes.data(), 1, bytes.size(), file) == bytes.size();
    const bool closed = std::fclose(file) == 0;
    if (!written || !closed) {
        std::remove(temporary);
        throw std::runtime_error("cannot write and close complete image");
    }
    // Switch SD rename does not replace an existing file. Keep the last good
    // image in a sibling backup until the complete temporary image is installed.
    if (hadPrevious && std::rename(output, backup.c_str()) != 0) {
        const int error = errno;
        std::remove(temporary);
        throw std::runtime_error(std::string("cannot back up previous image: ") + std::strerror(error));
    }
    if (std::rename(temporary, output) != 0) {
        const int error = errno;
        const bool restored = !hadPrevious || std::rename(backup.c_str(), output) == 0;
        std::remove(temporary);
        if (!restored)
            throw std::runtime_error("cannot install image; previous image retained in .previous backup");
        throw std::runtime_error(std::string("cannot install complete image: ") + std::strerror(error));
    }
    if (hadPrevious && std::remove(backup.c_str()) != 0)
        throw std::runtime_error("image installed but previous-image backup cleanup failed");
}
} // namespace mkw::frame_dump
