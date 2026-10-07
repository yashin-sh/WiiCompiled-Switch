#include "capture.hpp"

#include <algorithm>
#include <cstdio>
#include <stdexcept>

namespace {
void check(bool ok) {
    if (!ok) {
        throw std::runtime_error("format regression failed");
    }
}
template <class F>
void rejects(F&& fn) {
    bool rejected = false;
    try {
        fn();
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    check(rejected);
}
} // namespace

int main() {
    replay::Recorder recorder;
    recorder.drain(replay::Bytes{0});
    recorder.begin();
    recorder.drain(replay::Bytes{0});
    recorder.end();
    const auto good = recorder.finish();
    replay::Playback playback(good);
    unsigned calls = 0;
    playback.run([&](replay::Kind, auto) { ++calls; });
    check(calls == 4);
    for (std::size_t size = 0; size < good.size(); ++size) {
        rejects([&] { replay::Playback p{std::span(good).first(size)}; });
    }
    for (std::size_t pos = 0; pos < good.size(); ++pos) {
        auto changed = good;
        changed[pos] ^= 0x80;
        rejects([&] { replay::Playback p(changed); });
    }
    auto trailing = good;
    trailing.push_back(0);
    rejects([&] { replay::Playback p(trailing); });
    rejects([&] { recorder.drain(replay::Bytes{0}); });
    rejects([] { replay::Recorder{}.finish(); });
    rejects([] { replay::Recorder{}.begin(); });
    for (const auto& command : {replay::Bytes{0x40}, replay::Bytes{0x20}, replay::Bytes{0x50, 0, 0x34}, replay::Bytes{0x61, 0x52, 0, 0, 0}, replay::Bytes{0x08, 0xA0, 0, 0, 0, 0}}) {
        rejects([&] { replay::Recorder r; r.drain(command); });
    }
    rejects([] { replay::Recorder r; r.drain(replay::Bytes(replay::MaxBytes)); });
    // Pointer relocation and a changed resource must survive independent live
    // allocations, without flattening both reads into the final snapshot.
    replay::Bytes texture(64, 17);
    replay::Bytes command(37);
    command[0] = 0x50;
    command[2] = 0x30;
    command[15] = 4;
    command[19] = 4;
    command[23] = 6;
    auto address = reinterpret_cast<std::uintptr_t>(texture.data());
    for (unsigned i = 0; i < 8; ++i) {
        command[11 - i] = static_cast<std::uint8_t>(address >> (i * 8));
    }
    rejects([&] { replay::Recorder r; r.drain(command); });
    rejects([&] { replay::Recorder r; r.memory(std::span(texture).first(32)); r.drain(command); });
    replay::Recorder resourceRecorder;
    resourceRecorder.memory(texture);
    resourceRecorder.drain(command);
    resourceRecorder.begin();
    std::fill(texture.begin(), texture.end(), 91);
    resourceRecorder.drain(command);
    resourceRecorder.end();
    replay::Playback resources(resourceRecorder.finish());
    unsigned resourceReads = 0;
    std::uintptr_t previous = 0;
    resources.run([&](replay::Kind kind, auto data) {
        if (kind == replay::Kind::Fifo) {
            std::uintptr_t pointer = 0;
            for (unsigned i = 0; i < 8; ++i) {
                pointer = (pointer << 8) | data[4 + i];
            }
            check(pointer != address && (previous == 0 || previous == pointer));
            check(*reinterpret_cast<const std::uint8_t*>(pointer) == (resourceReads++ == 0 ? 17 : 91));
            previous = pointer;
        }
    });
    check(resourceReads == 2);
    for (const auto& [offset, value] : {std::pair<unsigned, unsigned>{3, 8}, {15, 0}, {23, 7}, {28, 1}}) {
        auto bad = command;
        bad[offset] = value;
        rejects([&] { replay::Recorder r; r.memory(texture); r.drain(bad); });
    }
    std::printf("PASS: %zu truncations, %zu corruptions, trailing data, lifecycle, unsupported commands and size bound\n", good.size(), good.size());
    std::puts("PASS: independent pointer relocation, same-address resource updates, bounds, slots, formats and mipmap refusals");
}
