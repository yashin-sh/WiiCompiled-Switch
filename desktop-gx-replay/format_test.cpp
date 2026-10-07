#include "capture.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#include <cstdio>
#include <stdexcept>

extern "C" void mkw_replay_capture_drain(const unsigned char*, unsigned int) noexcept;

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
    recorder.init();
    recorder.drain(replay::Bytes{0});
    recorder.begin();
    recorder.drain(replay::Bytes{0});
    recorder.end();
    const auto good = recorder.finish();
    replay::Playback playback(good);
    unsigned calls = 0;
    playback.run([&](replay::Kind, auto) { ++calls; });
    check(calls == 5);
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
    rejects([] { replay::Recorder r; r.init(); r.begin(); r.begin(); });
    for (const auto& command : {replay::Bytes{0x40}, replay::Bytes{0x20}, replay::Bytes{0x61, 0x52, 0, 0, 0}, replay::Bytes{0x08, 0xA0, 0, 0, 0, 0}}) {
        rejects([&] { replay::Recorder r; r.init(); r.drain(command); });
    }
    rejects([] { replay::Recorder r; r.init(); r.drain(replay::Bytes(replay::MaxBytes)); });
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
    rejects([&] { replay::Recorder r; r.init(); r.drain(command); });
    rejects([&] { replay::Recorder r; r.init(); r.memory(std::span(texture).first(32)); r.drain(command); });
    replay::Recorder resourceRecorder;
    resourceRecorder.init();
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
        rejects([&] { replay::Recorder r; r.init(); r.memory(texture); r.drain(bad); });
    }
    // Failure hooks disable capture without throwing across a noexcept GX/HLE boundary.
    replay::Recorder observed;
    observed.init();
    replay::set_recorder(&observed);
    const std::uint8_t unknown = 0x40;
    mkw_replay_capture_drain(&unknown, 1);
    check(!replay::recording() && std::strlen(replay::failure()) != 0);
    mkw_replay_capture_drain(&unknown, 1);
    replay::set_recorder(nullptr);
    rejects([&] { observed.finish(); });
    rejects([&] { replay::Recorder r; r.memory(texture); r.memory(std::span(texture).subspan(1)); });
    // A resolver must prove the exact pointer and extent before any snapshot.
    replay::Recorder resolved;
    resolved.init();
    resolved.resolve_with([&](std::uint64_t pointer, std::size_t size) {
        check(pointer == address && size == texture.size());
        return std::span<const std::uint8_t>(texture);
    });
    resolved.begin();
    resolved.drain(command);
    resolved.end();
    replay::Playback resolvedPlayback(resolved.finish());
    replay::CopyState copy{};
    copy[3] = copy[4] = copy[5] = copy[6] = 4;
    copy[7] = 6;
    copy[14] = std::bit_cast<std::uint32_t>(1.f);
    copy[51] = std::bit_cast<std::uint32_t>(1.f);
    copy[52] = 0xffffff;
    copy[58] = UINT32_MAX;
    replay::Recorder copies(1280, 720);
    copies.init();
    copies.memory(texture);
    copies.begin();
    copies.copy(replay::Kind::CopyTex, address, copy);
    copies.copy(replay::Kind::CopyDisp, 0, copy);
    copies.end();
    replay::Playback copied(copies.finish());
    check(copied.width == 1280 && copied.height == 720);
    unsigned copyCalls = 0;
    copied.run([&](auto kind, auto payload) {
        if (kind == replay::Kind::CopyTex) {
            std::uint64_t pointer = 0;
            for (unsigned i = 0; i < 8; ++i)
                pointer = (pointer << 8) | payload[i];
            check(pointer != address && *reinterpret_cast<const std::uint8_t*>(pointer) == texture[0]);
            ++copyCalls;
        }
    });
    check(copyCalls == 1);
    for (auto index : {0, 3, 5, 7, 10, 11, 14, 17, 41, 48, 52, 58}) {
        auto bad = copy;
        bad[index] = UINT32_MAX;
        if (index == 58)
            bad[index] = 256;
        rejects([&] { replay::Recorder r; r.init(); r.memory(texture); r.begin(); r.copy(replay::Kind::CopyTex, address, bad); });
    }
    // Indexed XYZ/F32 positions use a relocated array with a CP stride.
    replay::Bytes indexed{0x08, 0x50, 0, 0, 4, 0, 0x08, 0x70, 0, 0, 0, 9, 0x08, 0xB0, 0, 0, 0, 12};
    auto array = command;
    array.resize(16);
    array[2] = 0x10;
    array[11] = 0;
    array[12] = 0;
    array[13] = 0;
    array[14] = 64;
    array[15] = 0;
    // Texture metadata has its pointer at byte 4, array metadata at byte 3.
    for (unsigned i = 0; i < 8; ++i)
        array[10 - i] = static_cast<std::uint8_t>(address >> (i * 8));
    indexed.insert(indexed.end(), array.begin(), array.end());
    indexed.insert(indexed.end(), {0x90, 0, 3, 0, 1, 2});
    replay::Recorder vertices;
    vertices.init();
    vertices.memory(texture);
    vertices.begin();
    vertices.drain(indexed);
    vertices.end();
    replay::Playback vertexPlayback(vertices.finish());
    auto badIndex = indexed;
    badIndex.back() = 6;
    rejects([&] { replay::Recorder r; r.init(); r.memory(texture); r.drain(badIndex); });
    replay::Recorder rawDraw;
    rawDraw.init();
    rawDraw.memory(texture);
    rawDraw.begin();
    rawDraw.drain(std::span(indexed).first(indexed.size() - 6));
    rawDraw.raw_draw(0x90, 0, replay::Bytes{0, 1, 2}, 3);
    rawDraw.end();
    replay::Playback submitted(rawDraw.finish());
    rejects([&] { replay::Recorder r; r.init(); r.raw_draw(0x91, 0, replay::Bytes{0}, 1); });
    rejects([&] { replay::Recorder r; r.init(); r.raw_draw(0x90, 8, replay::Bytes{0}, 1); });
    rejects([&] { replay::Recorder r; r.init(); r.raw_draw(0x90, 0, replay::Bytes{}, 1); });
    std::puts("PASS: nonthrowing capture failure, trusted resolution, overlap refusal, copy relocation/state bounds and indexed vertex bounds");
    std::printf("PASS: %zu truncations, %zu corruptions, trailing data, lifecycle, unsupported commands and size bound\n", good.size(), good.size());
    std::puts("PASS: independent pointer relocation, same-address resource updates, bounds, slots, formats and mipmap refusals");
}
