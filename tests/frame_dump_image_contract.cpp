#include "frame_dump_image.hpp"
#include "rendered_frame_dump.hpp"

#include <array>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {
bool failReplacement = false;
bool failRestore = false;
} // namespace
extern "C" int __real_rename(const char*, const char*);
extern "C" int __wrap_rename(const char* from, const char* to) {
    // Switch SD rename refuses an existing destination, unlike POSIX rename.
    if (std::filesystem::exists(to)) {
        errno = EEXIST;
        return -1;
    }
    if ((failReplacement && std::string(from) == "rollback.tmp") || (failRestore && std::string(from) == "rollback.tmp.previous")) {
        errno = EIO;
        return -1;
    }
    return __real_rename(from, to);
}

using namespace mkw::frame_dump;
namespace {
template <class F>
void rejected(F call) {
    bool caught = false;
    try {
        call();
    } catch (const std::runtime_error&) {
        caught = true;
    }
    assert(caught);
}
std::string read(const char* path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), {}};
}
} // namespace
int main() {
    const auto size = layout(3, 2);
    assert(size.rowBytes == 256 && size.bufferBytes == 512);
    std::vector<std::uint8_t> padded(size.bufferBytes, 0xa5);
    constexpr std::array<std::uint8_t, 24> expected{255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 128, 0, 0, 0, 255, 37, 41, 59, 255, 0, 0, 0, 0};
    for (unsigned y = 0; y < 2; ++y)
        std::copy_n(expected.data() + y * 12, 12, padded.data() + y * size.rowBytes);
    const auto rgba = unpack(size, padded, false);
    assert(std::equal(rgba.rgba.begin(), rgba.rgba.end(), expected.begin()));
    assert(rgba.nonBlackPixels == 4 && rgba.nonOpaquePixels == 2 && !rgba.uniform);
    save("rgba.png", "rgba.tmp", rgba);
    for (unsigned y = 0; y < 2; ++y)
        for (unsigned x = 0; x < 3; ++x)
            std::swap(padded[y * size.rowBytes + x * 4], padded[y * size.rowBytes + x * 4 + 2]);
    const auto bgra = unpack(size, padded, true);
    assert(bgra.rgba == rgba.rgba);
    save("bgra.png", "bgra.tmp", bgra);
    assert(read("rgba.png") == read("bgra.png"));
    // Exercise multiple stored DEFLATE blocks and an unaligned large surface.
    const auto wide = layout(617, 341);
    std::vector<std::uint8_t> data(wide.bufferBytes, 0xa5);
    for (unsigned y = 0; y < wide.height; ++y)
        for (unsigned x = 0; x < wide.width; ++x) {
            const auto offset = y * wide.rowBytes + x * 4;
            data[offset] = x & 255;
            data[offset + 1] = y & 255;
            data[offset + 2] = (x ^ y) & 255;
            data[offset + 3] = 255;
        }
    save("wide.png", "wide.tmp", unpack(wide, data, false));
    for (auto dimensions : {std::pair{0u, 1u}, {1u, 0u}, {2049u, 1u}, {1u, 0xffffffffu}})
        rejected([&] { layout(dimensions.first, dimensions.second); });
    rejected([&] { unpack(size, std::span(padded).first(511), false); });
    auto invalid = size;
    --invalid.rowBytes;
    rejected([&] { unpack(invalid, padded, false); });
    auto shortImage = rgba;
    shortImage.rgba.pop_back();
    rejected([&] { png(shortImage); });
    rejected([&] { save("missing/rgba.png", "missing/rgba.tmp", rgba); });
    std::filesystem::create_directory("destination-directory");
    rejected([&] { save("destination-directory", "failed.tmp", rgba); });
    assert(!std::filesystem::exists("failed.tmp"));
    const auto original = read("rgba.png");
    save("rollback.png", "rollback.tmp", rgba);
    failReplacement = true;
    rejected([&] { save("rollback.png", "rollback.tmp", rgba); });
    failReplacement = false;
    assert(read("rollback.png") == original);
    assert(!std::filesystem::exists("rollback.tmp"));
    assert(!std::filesystem::exists("rollback.tmp.previous"));
    failReplacement = failRestore = true;
    rejected([&] { save("rollback.png", "rollback.tmp", rgba); });
    failReplacement = failRestore = false;
    assert(read("rollback.tmp.previous") == original);
    assert(!std::filesystem::exists("rollback.png") && !std::filesystem::exists("rollback.tmp"));
    assert(__real_rename("rollback.tmp.previous", "rollback.png") == 0);

    std::filesystem::create_directories("sdmc:/switch/WiiCompiled-Switch");
    std::ofstream("sdmc:/switch/WiiCompiled-Switch/surface-image.tmp.previous") << "retired backup";
    reset();
    assert(!std::filesystem::exists("sdmc:/switch/WiiCompiled-Switch/surface-image.tmp.previous"));
    assert(enabled());
    completed(rgba, rgba, rgba, 1);
    const auto first = read("sdmc:/switch/WiiCompiled-Switch/surface-first.png");
    const auto blackSize = layout(2, 2);
    std::vector<std::uint8_t> black(blackSize.bufferBytes);
    for (unsigned y = 0; y < 2; ++y)
        for (unsigned x = 0; x < 2; ++x)
            black[y * blackSize.rowBytes + x * 4 + 3] = 255;
    const auto opaqueBlack = unpack(blackSize, black, false);
    assert(opaqueBlack.uniform && !opaqueBlack.nonBlackPixels && !opaqueBlack.nonOpaquePixels);
    completed(opaqueBlack, rgba, opaqueBlack, 2);
    assert(read("sdmc:/switch/WiiCompiled-Switch/surface-first.png") == first);
    auto status = read("sdmc:/switch/WiiCompiled-Switch/frame-dump-status.txt");
    assert(status.find("frame=2\n") != std::string::npos && status.find("latest_png_frame=1\n") != std::string::npos);
    mkw_switch_frame_dump_checkpoint();
    status = read("sdmc:/switch/WiiCompiled-Switch/frame-dump-status.txt");
    assert(status.find("status=COMPLETE\n") != std::string::npos && status.find("latest_png_frame=2\n") != std::string::npos);
    completed(rgba, rgba, opaqueBlack, 30);
    status = read("sdmc:/switch/WiiCompiled-Switch/frame-dump-status.txt");
    assert(enabled() && status.find("latest_png_frame=30\n") != std::string::npos);
    assert(read("sdmc:/switch/WiiCompiled-Switch/surface-latest.png") == first);
    assert(!std::filesystem::exists("sdmc:/switch/WiiCompiled-Switch/surface-image.tmp.previous"));
    assert(read("sdmc:/switch/WiiCompiled-Switch/display-copy-latest.png") == first);
    assert(read("sdmc:/switch/WiiCompiled-Switch/efb-after-copy-latest.png") != first);
    assert(status.find("display_latest_png_frame=30\n") != std::string::npos && status.find("efb_latest_png_frame=30\n") != std::string::npos);
    failure("injected GPU map timeout");
    assert(!enabled());
    mkw_switch_frame_dump_checkpoint();
    assert(read("sdmc:/switch/WiiCompiled-Switch/frame-dump-status.txt").find("reason=injected GPU map timeout\n") != std::string::npos);
    reset();
    assert(!std::filesystem::exists("sdmc:/switch/WiiCompiled-Switch/surface-first.png"));
    assert(!std::filesystem::exists("sdmc:/switch/WiiCompiled-Switch/surface-latest.png"));
    std::filesystem::remove_all("sdmc:/switch/WiiCompiled-Switch");
    completed(rgba, rgba, rgba, 1);
    assert(!enabled());
    std::puts("PASS: padded RGBA/BGRA, PNG extent, black/alpha statistics, SD replacement rollback, first/latest checkpoint and fresh-run retirement");
}
