#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "memory.h"
#include "switch_gx_hle_traits.hpp"
#include <dolphin/gx.h>

#include <array>
#include <cassert>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {
constexpr std::uint32_t ia8Obj = 0x80384500u;
constexpr std::uint32_t ia8Data = 0x00384540u;
constexpr std::uint32_t rgbObj = 0x901136b4u;
constexpr std::uint32_t rgbData = 0x00f103e0u;
constexpr std::uint32_t rgbSize = 208u * 114u * 32u;
enum class Texture { Rgb,
                     Ia8,
                     MiiI4,
                     SecondMiiI4,
                     MiiRgb5a3,
                     MiiSmallI4,
                     SecondMiiSmallI4,
                     MiiSmallRgb5a3,
                     SecondMiiSmallRgb5a3,
                     MiiTinyI4,
                     NextPassMiiI4,
                     SecondNextPassMiiI4,
                     NextPassMiiRgb5a3,
                     NextPassMiiSmallI4,
                     GuardedNextMiiSmallI4,
                     RelocatedMiiI4,
                     SecondRelocatedMiiI4 };
struct Fixture {
    std::uint32_t object, data, width, height, format, size;
    bool disableEdgeLod;
    bool repeat = false;
};
constexpr Fixture GetFixture(Texture texture) {
    switch (texture) {
    case Texture::Rgb:
        return {rgbObj, rgbData, 832u, 456u, 4u, rgbSize, false};
    case Texture::Ia8:
        return {ia8Obj, ia8Data, 4u, 4u, 3u, 32u, true};
    case Texture::MiiI4:
        return {0x80397d80u, 0x109c1a40u, 32u, 64u, 0u, 4u * 8u * 32u, true};
    case Texture::SecondMiiI4:
        return {0x80397dc0u, 0x109c1a40u, 32u, 64u, 0u, 4u * 8u * 32u, true};
    case Texture::NextPassMiiI4:
        return {0x80397f80u, 0x109c1a40u, 32u, 64u, 0u, 4u * 8u * 32u, true};
    case Texture::SecondNextPassMiiI4:
        return {0x80397fc0u, 0x109c1a40u, 32u, 64u, 0u, 4u * 8u * 32u, true};
    case Texture::RelocatedMiiI4:
        return {0x80397d80u, 0x109c1a20u, 32u, 64u, 0u, 4u * 8u * 32u, true};
    case Texture::SecondRelocatedMiiI4:
        return {0x80397dc0u, 0x109c1a20u, 32u, 64u, 0u, 4u * 8u * 32u, true};
    case Texture::MiiRgb5a3:
        return {0x80397d40u, 0x109c0c40u, 44u, 32u, 5u, 11u * 8u * 32u, true};
    case Texture::NextPassMiiRgb5a3:
        return {0x80397f40u, 0x109c0c40u, 44u, 32u, 5u, 11u * 8u * 32u, true};
    case Texture::MiiSmallI4:
        return {0x80397cc0u, 0x109c1780u, 36u, 32u, 0u, ((36u + 7u) / 8u) * 4u * 32u, true};
    case Texture::SecondMiiSmallI4:
        return {0x80397d00u, 0x109c1780u, 36u, 32u, 0u, ((36u + 7u) / 8u) * 4u * 32u, true};
    case Texture::NextPassMiiSmallI4:
        return {0x80397ec0u, 0x109c1780u, 36u, 32u, 0u, ((36u + 7u) / 8u) * 4u * 32u, true};
    case Texture::GuardedNextMiiSmallI4:
        return {0x80397f00u, 0x109c1780u, 36u, 32u, 0u, 640u, true};
    case Texture::MiiSmallRgb5a3:
        return {0x80397c40u, 0x109c0200u, 38u, 32u, 5u, ((38u + 3u) / 4u) * 8u * 32u, true};
    case Texture::SecondMiiSmallRgb5a3:
        return {0x80397c80u, 0x109c0200u, 38u, 32u, 5u, ((38u + 3u) / 4u) * 8u * 32u, true};
    case Texture::MiiTinyI4:
        return {0x80397e00u, 0x109c1e80u, 16u, 16u, 0u, 2u * 2u * 32u, true};
    }
    std::abort();
}
constexpr std::uint32_t gxPointer = 0x803886c8u;
constexpr std::uint32_t gxData = 0x70005000u;
const char* stage = nullptr;
const char* expectedReason = nullptr;
std::uint32_t expectedSize = 0;
CpuContext expectedCpu{};
int reportPipe = -1;
std::uint32_t nativeCalls = 0;
std::uint32_t validCalls = 0;
std::uint32_t diagnosedAborts = 0;
std::size_t resolvedLength = 0;
bool nullHostPointer = false;
bool throwOnInit = false;
#if MKW_LOCAL_RENDERED_FAST_TRACK
std::array<GXTexObj*, 15> previousHosts{};
#endif
GXTexObj* currentHost = nullptr;
const void* currentData = nullptr;
std::uint16_t currentWidth = 0;
std::uint16_t currentHeight = 0;
GXTexFmt currentFormat = GX_TF_I4;
GXBool currentEdgeLod = GX_FALSE;
GXTexWrapMode currentWrapS = GX_CLAMP, currentWrapT = GX_CLAMP;
std::uint32_t expectedMap = 0u;
struct HostRegion {
    std::uint32_t base;
    std::vector<std::uint8_t> bytes;
};
std::vector<HostRegion> regions;
std::vector<HostRegion> expectedRegions;

void CheckStage() {
    assert(stage && std::strcmp(stage, "RMCP01_GX_LOAD_TEX_OBJ") == 0);
}
CpuContext MakeCpu(std::uint32_t object, std::uint32_t tid = 0u) {
    CpuContext cpu;
    std::memset(&cpu, 0xa5, sizeof(cpu));
    cpu.gpr[3] = object;
    cpu.gpr[4] = tid;
    return cpu;
}
void WriteWord(std::uint32_t pointer, std::uint32_t value) {
    for (std::uint32_t i = 0; i < 4u; ++i) {
        Memory::Write8(pointer + i, value >> (24u - 8u * i));
    }
}
void WriteDescriptorFixture(const Fixture& f) {
    const auto columns = f.format == 0u || f.format == 2u ? 8u : 4u;
    const auto rows = f.format == 0u ? 8u : 4u;
    const auto blocks = ((f.width + columns - 1u) / columns) *
                        ((f.height + rows - 1u) / rows);
    const auto tileType = f.format == 0u ? 1u : 2u;
    // Independent tiled field encoding; no game texture payload is used.
    const std::array<std::uint32_t, 8> words{
        (1u << 4u) | (4u << 5u) | (f.disableEdgeLod ? 1u << 8u : 0u) | (f.repeat ? 5u : 0u), 0u,
        (f.width - 1u) | ((f.height - 1u) << 10u) | (f.format << 20u),
        f.data >> 5u, 0u, f.format, 0u, ((blocks & 0x7fffu) << 16u) | (tileType << 8u) | 2u};
    for (std::uint32_t i = 0; i < words.size(); ++i) {
        WriteWord(f.object + 4u * i, words[i]);
    }
}
void WriteDescriptor(Texture texture) {
    WriteDescriptorFixture(GetFixture(texture));
}
void InitMemory(std::uint32_t ia8Bytes = 32u, std::uint32_t rgbBytes = rgbSize,
                std::uint32_t descriptorBytes = 64u, std::uint32_t i4Bytes = 1024u,
                std::uint32_t i4DescriptorBytes = 64u, std::uint32_t secondI4DescriptorBytes = 64u,
                std::uint32_t rgb5a3Bytes = 2816u, std::uint32_t rgb5a3DescriptorBytes = 64u,
                std::uint32_t smallI4Bytes = 640u, std::uint32_t smallI4DescriptorBytes = 64u,
                std::uint32_t secondSmallI4DescriptorBytes = 64u,
                std::uint32_t smallRgb5a3Bytes = 2560u, std::uint32_t smallRgb5a3DescriptorBytes = 64u,
                std::uint32_t secondSmallRgb5a3DescriptorBytes = 64u,
                std::uint32_t tinyI4Bytes = 128u, std::uint32_t tinyI4DescriptorBytes = 64u,
                std::uint32_t nextPassI4DescriptorBytes = 64u,
                bool relocatedI4 = false,
                std::uint32_t secondNextPassI4DescriptorBytes = 64u,
                std::uint32_t nextPassRgb5a3DescriptorBytes = 64u,
                std::uint32_t nextPassSmallI4DescriptorBytes = 64u) {
    Memory::Config config;
    config.regions.push_back({"ia8-object", ia8Obj, descriptorBytes});
    config.regions.push_back({"rgb-object", rgbObj, 64u});
    const auto primaryI4 = relocatedI4 ? Texture::RelocatedMiiI4 : Texture::MiiI4;
    const auto i4 = GetFixture(primaryI4);
    const auto secondaryI4 = relocatedI4 ? Texture::SecondRelocatedMiiI4 : Texture::SecondMiiI4;
    const auto secondI4 = GetFixture(secondaryI4);
    const auto rgb5a3 = GetFixture(Texture::MiiRgb5a3);
    const auto nextPassRgb5a3 = GetFixture(Texture::NextPassMiiRgb5a3);
    const auto smallI4 = GetFixture(Texture::MiiSmallI4);
    const auto secondSmallI4 = GetFixture(Texture::SecondMiiSmallI4);
    const auto nextPassSmallI4 = GetFixture(Texture::NextPassMiiSmallI4);
    const auto smallRgb5a3 = GetFixture(Texture::MiiSmallRgb5a3);
    const auto secondSmallRgb5a3 = GetFixture(Texture::SecondMiiSmallRgb5a3);
    const auto nextPassI4 = GetFixture(Texture::NextPassMiiI4);
    const auto secondNextPassI4 = GetFixture(Texture::SecondNextPassMiiI4);
    const auto tinyI4 = GetFixture(Texture::MiiTinyI4);
    if (i4DescriptorBytes)
        config.regions.push_back({"i4-object", i4.object, i4DescriptorBytes});
    if (secondI4DescriptorBytes)
        config.regions.push_back({"second-i4-object", secondI4.object, secondI4DescriptorBytes});
    if (rgb5a3DescriptorBytes)
        config.regions.push_back({"rgb5a3-object", rgb5a3.object, rgb5a3DescriptorBytes});
    if (nextPassRgb5a3DescriptorBytes)
        config.regions.push_back({"next-pass-rgb5a3-object", nextPassRgb5a3.object, nextPassRgb5a3DescriptorBytes});
    if (rgb5a3Bytes)
        config.regions.push_back({"rgb5a3-physical-mem2-data", rgb5a3.data, rgb5a3Bytes});
    if (smallI4DescriptorBytes)
        config.regions.push_back({"small-i4-object", smallI4.object, smallI4DescriptorBytes});
    if (secondSmallI4DescriptorBytes)
        config.regions.push_back({"second-small-i4-object", secondSmallI4.object, secondSmallI4DescriptorBytes});
    if (nextPassSmallI4DescriptorBytes)
        config.regions.push_back({"next-pass-small-i4-object", nextPassSmallI4.object, nextPassSmallI4DescriptorBytes});
    config.regions.push_back({"latest-guarded-small-i4-object", GetFixture(Texture::GuardedNextMiiSmallI4).object, 64u});
    if (smallI4Bytes)
        config.regions.push_back({"small-i4-physical-mem2-data", smallI4.data, smallI4Bytes});
    if (smallRgb5a3DescriptorBytes)
        config.regions.push_back({"small-rgb5a3-object", smallRgb5a3.object, smallRgb5a3DescriptorBytes});
    if (secondSmallRgb5a3DescriptorBytes)
        config.regions.push_back({"second-small-rgb5a3-object", secondSmallRgb5a3.object, secondSmallRgb5a3DescriptorBytes});
    if (smallRgb5a3Bytes)
        config.regions.push_back({"small-rgb5a3-physical-mem2-data", smallRgb5a3.data, smallRgb5a3Bytes});
    if (nextPassI4DescriptorBytes)
        config.regions.push_back({"next-pass-i4-object", nextPassI4.object, nextPassI4DescriptorBytes});
    if (secondNextPassI4DescriptorBytes)
        config.regions.push_back({"second-next-pass-i4-object", secondNextPassI4.object, secondNextPassI4DescriptorBytes});
    if (tinyI4DescriptorBytes)
        config.regions.push_back({"tiny-i4-object", tinyI4.object, tinyI4DescriptorBytes});
    if (tinyI4Bytes)
        config.regions.push_back({"tiny-i4-physical-mem2-data", tinyI4.data, tinyI4Bytes});
    if (ia8Bytes)
        config.regions.push_back({"ia8-data", ia8Data, ia8Bytes});
    if (rgbBytes)
        config.regions.push_back({"rgb-data", rgbData, rgbBytes});
    if (i4Bytes)
        config.regions.push_back({"i4-physical-mem2-data", i4.data, i4Bytes});
    config.regions.push_back({"gx-pointer", gxPointer, 4u});
    config.regions.push_back({"gx-state", gxData, 0x604u});
    Memory::Init(config);
    WriteWord(gxPointer, gxData);
    WriteWord(gxData + 0x5fcu, 0x12345678u);
    Memory::Write16(gxData + 2u, 0xabcd);
    if (descriptorBytes >= 32u)
        WriteDescriptor(Texture::Ia8);
    WriteDescriptor(Texture::Rgb);
    WriteDescriptor(Texture::GuardedNextMiiSmallI4);
    if (i4DescriptorBytes >= 32u)
        WriteDescriptor(primaryI4);
    if (secondI4DescriptorBytes >= 32u)
        WriteDescriptor(secondaryI4);
    if (rgb5a3DescriptorBytes >= 32u)
        WriteDescriptor(Texture::MiiRgb5a3);
    if (nextPassRgb5a3DescriptorBytes >= 32u)
        WriteDescriptor(Texture::NextPassMiiRgb5a3);
    if (smallI4DescriptorBytes >= 32u)
        WriteDescriptor(Texture::MiiSmallI4);
    if (secondSmallI4DescriptorBytes >= 32u)
        WriteDescriptor(Texture::SecondMiiSmallI4);
    if (nextPassSmallI4DescriptorBytes >= 32u)
        WriteDescriptor(Texture::NextPassMiiSmallI4);
    if (smallRgb5a3DescriptorBytes >= 32u)
        WriteDescriptor(Texture::MiiSmallRgb5a3);
    if (secondSmallRgb5a3DescriptorBytes >= 32u)
        WriteDescriptor(Texture::SecondMiiSmallRgb5a3);
    if (nextPassI4DescriptorBytes >= 32u)
        WriteDescriptor(Texture::NextPassMiiI4);
    if (secondNextPassI4DescriptorBytes >= 32u)
        WriteDescriptor(Texture::SecondNextPassMiiI4);
    if (tinyI4DescriptorBytes >= 32u)
        WriteDescriptor(Texture::MiiTinyI4);
}
#if MKW_LOCAL_RENDERED_FAST_TRACK
std::string Status() {
    std::ifstream file("sdmc:/switch/WiiCompiled-Switch/fast-track-gx-load-tex-obj.txt");
    return std::string(std::istreambuf_iterator<char>(file), {});
}
#endif
void CheckStatus(const char* reason, std::uint32_t size) {
#if MKW_LOCAL_RENDERED_FAST_TRACK
    const auto text = Status();
    assert(text.find(std::string("status=") + reason + "\n") != std::string::npos);
    char field[32]{};
    std::snprintf(field, sizeof(field), "size=0x%08x\n", size);
    assert(text.find(field) != std::string::npos);
#else
    (void)reason;
    (void)size;
#endif
}
void ExpectAbort(CpuContext cpu, const char* reason, std::uint32_t size) {
    int descriptors[2]{};
    assert(pipe(descriptors) == 0);
    const auto child = fork();
    assert(child >= 0);
    if (child == 0) {
        // Linux core-pipe handlers can ignore RLIMIT_CORE; retain real SIGABRT
        // without sending expected-refusal children to the system core collector.
        assert(prctl(PR_SET_DUMPABLE, 0) == 0);
        close(descriptors[0]);
        reportPipe = descriptors[1];
        expectedReason = reason;
        expectedSize = size;
        std::memcpy(&expectedCpu, &cpu, sizeof(cpu));
        nativeCalls = 0u;
        expectedRegions = regions;
        KnownNativeCpuCall<0x80170f2cu>::Invoke(&cpu);
        _exit(1);
    }
    close(descriptors[1]);
    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    char reported = 0;
    assert(read(descriptors[0], &reported, 1) == 1 && reported == 'R');
    close(descriptors[0]);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    CheckStatus(reason, size);
    ++diagnosedAborts;
}
void CheckFixtureLoad(const Fixture& f, std::uint32_t tid = 0u, [[maybe_unused]] std::size_t hostIndex = ~std::size_t{0}) {
    const auto object = f.object;
    const auto data = f.data;
    const auto size = f.size;
    WriteWord(gxData + 0x5fcu, 0x12345678u);
    Memory::Write16(gxData + 2u, 0xabcd);
    auto cpu = MakeCpu(object, tid);
    expectedMap = tid;
    std::array<unsigned char, sizeof(cpu)> before{};
    std::memcpy(before.data(), &cpu, sizeof(cpu));
    std::array<std::uint8_t, 32> descriptor{};
    std::memcpy(descriptor.data(), Memory::GetPointer(object, 32u), 32u);
    std::vector<std::uint8_t> payload(size);
    for (std::uint32_t i = 0; i < size; ++i)
        payload[i] = (i * 37u + 19u) & 0xffu;
    std::memcpy(Memory::GetPointer(data, size), payload.data(), size);
    std::array<std::uint8_t, 0x604> state{};
    std::memcpy(state.data(), Memory::GetPointer(gxData, state.size()), state.size());
    state[2] = 0u;
    state[3] = 0u;
    state[0x5ff] |= 1u;
    nativeCalls = 0u;
    KnownNativeCpuCall<0x80170f2cu>::Invoke(&cpu);
    CheckStage();
    assert(std::memcmp(before.data(), &cpu, sizeof(cpu)) == 0);
    assert(std::memcmp(descriptor.data(), Memory::GetPointer(object, 32u), 32u) == 0);
    assert(std::memcmp(payload.data(), Memory::GetPointer(data, size), size) == 0);
    assert(std::memcmp(state.data(), Memory::GetPointer(gxData, state.size()), state.size()) == 0);
#if MKW_LOCAL_RENDERED_FAST_TRACK
    assert(nativeCalls == 4u && resolvedLength == size);
    assert(currentData == Memory::GetPointer(data, size));
    assert(currentWidth == f.width && currentHeight == f.height);
    assert(currentFormat == static_cast<GXTexFmt>(f.format));
    assert(currentWrapS == (f.repeat ? GX_REPEAT : GX_CLAMP));
    assert(currentWrapT == (f.repeat ? GX_REPEAT : GX_CLAMP));
    assert(currentEdgeLod == (f.disableEdgeLod ? GX_FALSE : GX_TRUE));
    if (hostIndex < previousHosts.size()) {
        auto*& previous = previousHosts[hostIndex];
        if (previous)
            assert(previous == currentHost);
        previous = currentHost;
        for (std::size_t i = 0; i < previousHosts.size(); ++i)
            if (i != hostIndex && previousHosts[i])
                assert(previousHosts[i] != currentHost);
    }
#else
    assert(nativeCalls == 0u);
#endif
    CheckStatus("load-pass", size);
#if MKW_LOCAL_RENDERED_FAST_TRACK
    assert(Status().find("tid=" + std::to_string(tid) + "\n") != std::string::npos);
#endif
    ++validCalls;
}
void CheckLoad(Texture texture, std::uint32_t tid = 0u) {
    const auto identity = texture == Texture::RelocatedMiiI4 ? Texture::MiiI4
                                                             : (texture == Texture::SecondRelocatedMiiI4 ? Texture::SecondMiiI4 : texture);
    CheckFixtureLoad(GetFixture(texture), tid, static_cast<std::size_t>(identity));
}
void InitFamily(const Fixture& f, std::uint32_t bytes) {
    Memory::Config config;
    config.regions = {{"family-object", f.object, 64u}, {"gx-pointer", gxPointer, 4u}, {"gx-state", gxData, 0x604u}};
    if (bytes)
        config.regions.push_back({"family-data", f.data, bytes});
    Memory::Init(config);
    WriteWord(gxPointer, gxData);
    WriteWord(gxData + 0x5fcu, 0x12345678u);
    Memory::Write16(gxData + 2u, 0xabcdu);
    WriteDescriptorFixture(f);
}
void CheckFamily() {
    std::uint32_t identity = 0x80401000u;
    // Sources and objects absent from the hardware-address lists. Exercise
    // whole and partial tiles, small/large extents and all native binding slots.
    for (auto mode : {std::pair{0u, false}, {2u, false}, {2u, true}, {3u, false}, {3u, true}, {5u, false}}) {
        const auto format = mode.first;
        for (auto dims : {std::pair{1u, 1u}, {4u, 7u}, {8u, 8u}, {9u, 9u}, {36u, 32u}, {32u, 32u}, {511u, 17u}, {1024u, 64u}}) {
            const auto columns = format == 0u || format == 2u ? 8u : 4u;
            const auto rows = format == 0u ? 8u : 4u;
            Fixture f{identity, 0x10500000u + (identity - 0x80401000u) * 16u, dims.first, dims.second, format,
                      ((dims.first + columns - 1u) / columns) * ((dims.second + rows - 1u) / rows) * 32u, true, mode.second};
            identity += 128u;
            InitFamily(f, f.size + 32u);
            for (std::uint32_t slot = 0; slot < 8u; ++slot)
                CheckFixtureLoad(f, slot);
            auto* previous = currentHost;
            const auto* previousData = currentData;
            f.data += 32u;
            WriteDescriptorFixture(f);
            CheckFixtureLoad(f);
#if MKW_LOCAL_RENDERED_FAST_TRACK
            assert(currentHost == previous && currentData != previousData);
#else
            (void)previous;
            (void)previousData;
#endif
            InitFamily(f, f.size - 1u);
            ExpectAbort(MakeCpu(f.object), "GX_LOAD_TEX_OBJ_DATA_UNMAPPED", f.size);
            InitFamily(f, f.size);
            const auto count = Memory::Read32(f.object + 28u);
            WriteWord(f.object + 28u, count ^ 0x00010000u);
            ExpectAbort(MakeCpu(f.object), "GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR", 0u);
            WriteWord(f.object + 28u, count);
            WriteWord(f.object + 12u, 0u);
            ExpectAbort(MakeCpu(f.object), "GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR", 0u);
            // A mapped but unaligned object must still be refused.
            f.object += 1u;
            InitFamily(f, f.size);
            ExpectAbort(MakeCpu(f.object), "GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR", 0u);
        }
    }
    for (auto format : {3u, 2u}) {
        Fixture observed{0x80398eb8u, 0x1114f2a0u, 32u, 32u, format, format == 2u ? 1024u : 2048u, true, true};
        for (bool repeat : {true, false}) {
            observed.repeat = repeat;
            observed.data = format == 2u ? 0x116e7de0u : (repeat ? 0x1114f2a0u : 0x1114f2c0u);
            InitFamily(observed, observed.size);
            const std::array<std::uint32_t, 8> captured{repeat ? 0x195u : 0x190u, 0u, format == 2u ? 0x00207c1fu : 0x00307c1fu,
                                                        format == 2u ? 0x008b73efu : (repeat ? 0x0088a795u : 0x0088a796u), 0u, format, 0u, format == 2u ? 0x00200202u : 0x00400202u};
            for (std::uint32_t word = 0; word < captured.size(); ++word)
                assert(Memory::Read32(observed.object + word * 4u) == captured[word]);
            for (std::uint32_t slot = 0; slot < 8u; ++slot)
                CheckFixtureLoad(observed, slot);
            for (std::uint32_t word = 0; word < captured.size(); ++word) {
                for (std::uint32_t bit = 0; bit < 32u; ++bit) {
                    if ((word == 2u && bit < 20u) || (word == 3u && bit < 24u))
                        continue;
                    WriteWord(observed.object + word * 4u, captured[word] ^ (1u << bit));
                    ExpectAbort(MakeCpu(observed.object), "GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR", 0u);
                }
                WriteWord(observed.object + word * 4u, captured[word]);
            }
        }
        // Switching the sampler on one object must refresh the existing native
        // allocation, in both directions, rather than retaining the first mode.
        auto* previous = currentHost;
        for (bool repeat : {true, false, true}) {
            observed.repeat = repeat;
            WriteDescriptorFixture(observed);
            CheckFixtureLoad(observed);
#if MKW_LOCAL_RENDERED_FAST_TRACK
            assert(currentHost == previous);
#else
            (void)previous;
#endif
        }
    }
    Fixture wrapped{identity, 0x10600000u, 1024u, 512u, 5u, 32768u * 32u, true};
    InitFamily(wrapped, wrapped.size);
    ExpectAbort(MakeCpu(wrapped.object), "GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR", 0u);
    wrapped.format = 2u;
    wrapped.height = 1024u;
    wrapped.repeat = true;
    InitFamily(wrapped, wrapped.size);
    ExpectAbort(MakeCpu(wrapped.object), "GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR", 0u);
}
void CheckDescriptorRefusals(Texture texture) {
    const auto fixture = GetFixture(texture);
    const auto object = fixture.object;
    const bool family = fixture.format == 0u || fixture.format == 3u || fixture.format == 5u;
    for (std::uint32_t word = 0; word < 8u; ++word) {
        const auto value = Memory::Read32(object + 4u * word);
        for (std::uint32_t bit = 0; bit < 32u; ++bit) {
            if (family && ((word == 2u && bit < 20u) || (word == 3u && bit < 24u)))
                continue; // Dimensions/source are variable; exercised below.
            WriteWord(object + 4u * word, value ^ (1u << bit));
            ExpectAbort(MakeCpu(object), "GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR", 0u);
        }
        WriteWord(object + 4u * word, value);
    }
    for (auto tid : {8u, 0xffu, 0xffffffffu}) {
        ExpectAbort(MakeCpu(object, tid), "GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR", 0u);
    }
    if (texture == Texture::Rgb)
        for (std::uint32_t tid = 1u; tid < 8u; ++tid)
            ExpectAbort(MakeCpu(object, tid), "GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR", 0u);
    std::memcpy(Memory::GetPointer(object + 32u, 32u), Memory::GetPointer(object, 32u), 32u);
    if (family) {
        auto relocated = fixture;
        relocated.object += 32u;
        CheckFixtureLoad(relocated);
        for (std::uint32_t slot = 1u; slot < 8u; ++slot)
            CheckFixtureLoad(fixture, slot);
        const auto saved = Memory::Read32(object + 12u);
        WriteWord(object + 12u, 0x00200000u);
        ExpectAbort(MakeCpu(object), "GX_LOAD_TEX_OBJ_DATA_UNMAPPED", fixture.size);
        WriteWord(object + 12u, saved);
    } else {
        ExpectAbort(MakeCpu(object + 32u), "GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR", 0u);
    }
}
} // namespace

// Allocation-only host seam; the actual Switch Memory checks and endian reads run.
// Regions are independent test allocations; no Horizon alias mapping is emulated.
namespace GuestFlat {
bool IsActive() {
    return !regions.empty();
}
void Initialize(const std::vector<RegionRequest>& requests) {
    regions.clear();
    for (const auto& r : requests)
        regions.push_back({r.base, std::vector<std::uint8_t>(r.size)});
}
std::uint8_t* HostPointer(std::uint32_t pointer) {
    for (auto& r : regions) {
        if (pointer >= r.base && std::uint64_t(pointer) < std::uint64_t(r.base) + r.bytes.size()) {
            return r.bytes.data() + (pointer - r.base);
        }
    }
    return nullptr;
}
void Shutdown() noexcept {
    regions.clear();
}
} // namespace GuestFlat
extern "C" void mkw_switch_set_fast_track_stage(const char* value) noexcept {
    stage = value;
}
void* GuestToHostPtr(std::uint32_t pointer, std::size_t length) {
    resolvedLength = length;
    return nullHostPointer ? nullptr : Memory::GetPointer(pointer, length);
}
extern "C" void GXInitTexObj(GXTexObj* obj, const void* data, u16 width, u16 height,
                             GXTexFmt format, GXTexWrapMode s, GXTexWrapMode t, GXBool mip) {
    CheckStage();
    assert(Memory::Read32(gxData + 0x5fcu) == 0x12345678u);
    assert(Memory::Read16(gxData + 2u) == 0xabcdu);
    assert(nativeCalls++ == 0u);
    if (throwOnInit)
        throw std::runtime_error("test GX init failure");
    assert(mip == GX_FALSE);
    currentWrapS = s;
    currentWrapT = t;
    currentHost = obj;
    currentData = data;
    currentWidth = width;
    currentHeight = height;
    currentFormat = format;
}
extern "C" void GXInitTexObjLOD(GXTexObj* obj, GXTexFilter min, GXTexFilter mag,
                                float minLod, float maxLod, float bias, GXBool clamp,
                                GXBool edge, GXAnisotropy aniso) {
    CheckStage();
    assert(Memory::Read32(gxData + 0x5fcu) == 0x12345678u);
    assert(Memory::Read16(gxData + 2u) == 0xabcdu);
    assert(nativeCalls++ == 1u && obj == currentHost);
    assert(min == GX_LINEAR && mag == GX_LINEAR && minLod == 0.f && maxLod == 0.f && bias == 0.f);
    assert(clamp == GX_FALSE && aniso == GX_ANISO_1);
    currentEdgeLod = edge;
}
extern "C" void GXInitTexObjUserData(GXTexObj* obj, void* data) {
    CheckStage();
    assert(Memory::Read32(gxData + 0x5fcu) == 0x12345678u);
    assert(Memory::Read16(gxData + 2u) == 0xabcdu);
    assert(nativeCalls++ == 2u && obj == currentHost && data == nullptr);
}
extern "C" void GXLoadTexObj(GXTexObj* obj, GXTexMapID id) {
    CheckStage();
    assert(Memory::Read32(gxData + 0x5fcu) == 0x12345678u);
    assert(Memory::Read16(gxData + 2u) == 0xabcdu);
    assert(nativeCalls++ == 3u && obj == currentHost && id == static_cast<GXTexMapID>(expectedMap));
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(
    const char* reason, std::uint32_t target, CpuContext* cpu) noexcept {
    assert(reportPipe >= 0);
    CheckStage();
    assert(std::strcmp(reason, expectedReason) == 0 && target == 0x80170f2cu);
    assert(std::memcmp(cpu, &expectedCpu, sizeof(*cpu)) == 0);
    assert(nativeCalls == (throwOnInit ? 1u : 0u));
    assert(expectedRegions.size() == regions.size());
    for (std::size_t i = 0; i < regions.size(); ++i) {
        assert(expectedRegions[i].base == regions[i].base);
        assert(expectedRegions[i].bytes == regions[i].bytes);
    }
    CheckStatus(reason, expectedSize);
    assert(write(reportPipe, "R", 1) == 1);
}

int main() {
    static_assert(KnownNativeCpuCall<0x80170f2cu>::kAvailable);
    const rlimit noCore{0, 0};
    assert(setrlimit(RLIMIT_CORE, &noCore) == 0);
    KnownNativeCpuCall<0x80170f2cu>::Invoke(nullptr);
    assert(stage == nullptr && nativeCalls == 0u);
    ExpectAbort(MakeCpu(ia8Obj), "GX_LOAD_TEX_OBJ_GUEST_UNMAPPED", 0u);
    InitMemory();
    for (auto texture : {Texture::Rgb, Texture::Ia8, Texture::MiiI4, Texture::SecondMiiI4, Texture::MiiRgb5a3, Texture::MiiSmallI4, Texture::SecondMiiSmallI4, Texture::MiiSmallRgb5a3, Texture::SecondMiiSmallRgb5a3, Texture::MiiTinyI4, Texture::NextPassMiiI4, Texture::SecondNextPassMiiI4, Texture::NextPassMiiRgb5a3, Texture::NextPassMiiSmallI4, Texture::GuardedNextMiiSmallI4,
                         Texture::Rgb, Texture::Ia8, Texture::MiiI4, Texture::SecondMiiI4, Texture::MiiRgb5a3, Texture::MiiSmallI4, Texture::SecondMiiSmallI4, Texture::MiiSmallRgb5a3, Texture::SecondMiiSmallRgb5a3, Texture::MiiTinyI4, Texture::NextPassMiiI4, Texture::SecondNextPassMiiI4, Texture::NextPassMiiRgb5a3, Texture::NextPassMiiSmallI4, Texture::GuardedNextMiiSmallI4})
        CheckLoad(texture);
    for (std::uint32_t tid = 0u; tid < 8u; ++tid)
        CheckLoad(Texture::Ia8, tid);
    // Unsupported fields/maps stop; family identities, sources and legal slots vary.
    InitMemory();
    for (auto texture : {Texture::Rgb, Texture::Ia8, Texture::MiiI4, Texture::SecondMiiI4, Texture::MiiRgb5a3, Texture::MiiSmallI4, Texture::SecondMiiSmallI4, Texture::MiiSmallRgb5a3, Texture::SecondMiiSmallRgb5a3, Texture::MiiTinyI4, Texture::NextPassMiiI4, Texture::SecondNextPassMiiI4, Texture::NextPassMiiRgb5a3, Texture::NextPassMiiSmallI4, Texture::GuardedNextMiiSmallI4}) {
        CheckDescriptorRefusals(texture);
    }
    for (auto bytes : {0u, 31u}) {
        InitMemory(bytes);
        ExpectAbort(MakeCpu(ia8Obj), "GX_LOAD_TEX_OBJ_DATA_UNMAPPED", 32u);
    }
    for (auto bytes : {0u, 512u, 992u, 1023u}) {
        InitMemory(32u, rgbSize, 64u, bytes);
        for (auto texture : {Texture::MiiI4, Texture::SecondMiiI4, Texture::NextPassMiiI4, Texture::SecondNextPassMiiI4})
            ExpectAbort(MakeCpu(GetFixture(texture).object), "GX_LOAD_TEX_OBJ_DATA_UNMAPPED", 1024u);
    }
    for (auto bytes : {0u, 31u}) {
        InitMemory(32u, rgbSize, 64u, 1024u, bytes);
        ExpectAbort(MakeCpu(GetFixture(Texture::MiiI4).object), "GX_LOAD_TEX_OBJ_GUEST_UNMAPPED", 0u);
    }
    for (auto bytes : {0u, 31u}) {
        InitMemory(32u, rgbSize, 64u, 1024u, 64u, bytes);
        ExpectAbort(MakeCpu(GetFixture(Texture::SecondMiiI4).object), "GX_LOAD_TEX_OBJ_GUEST_UNMAPPED", 0u);
    }
    for (auto bytes : {0u, 2048u, 2784u, 2815u}) {
        InitMemory(32u, rgbSize, 64u, 1024u, 64u, 64u, bytes);
        for (auto texture : {Texture::MiiRgb5a3, Texture::NextPassMiiRgb5a3})
            ExpectAbort(MakeCpu(GetFixture(texture).object), "GX_LOAD_TEX_OBJ_DATA_UNMAPPED", 2816u);
    }
    for (auto bytes : {0u, 31u}) {
        InitMemory(32u, rgbSize, 64u, 1024u, 64u, 64u, 2816u, bytes);
        ExpectAbort(MakeCpu(GetFixture(Texture::MiiRgb5a3).object), "GX_LOAD_TEX_OBJ_GUEST_UNMAPPED", 0u);
    }
    for (auto bytes : {0u, 576u, 639u}) {
        InitMemory(32u, rgbSize, 64u, 1024u, 64u, 64u, 2816u, 64u, bytes);
        for (auto texture : {Texture::MiiSmallI4, Texture::SecondMiiSmallI4, Texture::NextPassMiiSmallI4, Texture::GuardedNextMiiSmallI4})
            ExpectAbort(MakeCpu(GetFixture(texture).object), "GX_LOAD_TEX_OBJ_DATA_UNMAPPED", 640u);
    }
    for (auto bytes : {0u, 31u}) {
        InitMemory(32u, rgbSize, 64u, 1024u, 64u, 64u, 2816u, 64u, 640u, bytes);
        ExpectAbort(MakeCpu(GetFixture(Texture::MiiSmallI4).object), "GX_LOAD_TEX_OBJ_GUEST_UNMAPPED", 0u);
    }
    for (auto bytes : {0u, 31u}) {
        InitMemory(32u, rgbSize, 64u, 1024u, 64u, 64u, 2816u, 64u, 640u, 64u, bytes);
        ExpectAbort(MakeCpu(GetFixture(Texture::SecondMiiSmallI4).object), "GX_LOAD_TEX_OBJ_GUEST_UNMAPPED", 0u);
    }
    for (auto bytes : {0u, 2432u, 2559u}) {
        InitMemory(32u, rgbSize, 64u, 1024u, 64u, 64u, 2816u, 64u, 640u, 64u, 64u, bytes);
        for (auto texture : {Texture::MiiSmallRgb5a3, Texture::SecondMiiSmallRgb5a3})
            ExpectAbort(MakeCpu(GetFixture(texture).object), "GX_LOAD_TEX_OBJ_DATA_UNMAPPED", 2560u);
    }
    for (auto bytes : {0u, 31u}) {
        InitMemory(32u, rgbSize, 64u, 1024u, 64u, 64u, 2816u, 64u, 640u, 64u, 64u, 2560u, bytes);
        ExpectAbort(MakeCpu(GetFixture(Texture::MiiSmallRgb5a3).object), "GX_LOAD_TEX_OBJ_GUEST_UNMAPPED", 0u);
    }
    for (auto bytes : {0u, 31u}) {
        InitMemory(32u, rgbSize, 64u, 1024u, 64u, 64u, 2816u, 64u, 640u, 64u, 64u, 2560u, 64u, bytes);
        ExpectAbort(MakeCpu(GetFixture(Texture::SecondMiiSmallRgb5a3).object), "GX_LOAD_TEX_OBJ_GUEST_UNMAPPED", 0u);
    }
    for (auto bytes : {0u, 64u, 127u}) {
        InitMemory(32u, rgbSize, 64u, 1024u, 64u, 64u, 2816u, 64u, 640u, 64u, 64u, 2560u, 64u, 64u, bytes);
        ExpectAbort(MakeCpu(GetFixture(Texture::MiiTinyI4).object), "GX_LOAD_TEX_OBJ_DATA_UNMAPPED", 128u);
    }
    for (auto bytes : {0u, 31u}) {
        InitMemory(32u, rgbSize, 64u, 1024u, 64u, 64u, 2816u, 64u, 640u, 64u, 64u, 2560u, 64u, 64u, 128u, bytes);
        ExpectAbort(MakeCpu(GetFixture(Texture::MiiTinyI4).object), "GX_LOAD_TEX_OBJ_GUEST_UNMAPPED", 0u);
    }
    for (auto bytes : {0u, 31u}) {
        InitMemory(32u, rgbSize, 64u, 1024u, 64u, 64u, 2816u, 64u, 640u, 64u, 64u, 2560u, 64u, 64u, 128u, 64u, bytes);
        ExpectAbort(MakeCpu(GetFixture(Texture::NextPassMiiI4).object), "GX_LOAD_TEX_OBJ_GUEST_UNMAPPED", 0u);
    }
    for (auto bytes : {0u, 31u}) {
        InitMemory(32u, rgbSize, 64u, 1024u, 64u, 64u, 2816u, 64u, 640u,
                   64u, 64u, 2560u, 64u, 64u, 128u, 64u, 64u, false, bytes);
        ExpectAbort(MakeCpu(GetFixture(Texture::SecondNextPassMiiI4).object),
                    "GX_LOAD_TEX_OBJ_GUEST_UNMAPPED", 0u);
    }
    for (auto bytes : {0u, 31u}) {
        InitMemory(32u, rgbSize, 64u, 1024u, 64u, 64u, 2816u, 64u, 640u,
                   64u, 64u, 2560u, 64u, 64u, 128u, 64u, 64u, false, 64u, bytes);
        ExpectAbort(MakeCpu(GetFixture(Texture::NextPassMiiRgb5a3).object),
                    "GX_LOAD_TEX_OBJ_GUEST_UNMAPPED", 0u);
    }
    for (auto bytes : {0u, 31u}) {
        InitMemory(32u, rgbSize, 64u, 1024u, 64u, 64u, 2816u, 64u, 640u,
                   64u, 64u, 2560u, 64u, 64u, 128u, 64u, 64u, false, 64u, 64u, bytes);
        ExpectAbort(MakeCpu(GetFixture(Texture::NextPassMiiSmallI4).object),
                    "GX_LOAD_TEX_OBJ_GUEST_UNMAPPED", 0u);
    }
    InitMemory(32u, rgbSize - 1u);
    ExpectAbort(MakeCpu(rgbObj), "GX_LOAD_TEX_OBJ_DATA_UNMAPPED", rgbSize);
    InitMemory(32u, rgbSize, 31u);
    ExpectAbort(MakeCpu(ia8Obj), "GX_LOAD_TEX_OBJ_GUEST_UNMAPPED", 0u);
    InitMemory();
#if MKW_LOCAL_RENDERED_FAST_TRACK
    for (auto texture : {Texture::Rgb, Texture::Ia8, Texture::MiiI4, Texture::SecondMiiI4, Texture::MiiRgb5a3, Texture::MiiSmallI4, Texture::SecondMiiSmallI4, Texture::MiiSmallRgb5a3, Texture::SecondMiiSmallRgb5a3, Texture::MiiTinyI4, Texture::NextPassMiiI4, Texture::SecondNextPassMiiI4, Texture::NextPassMiiRgb5a3, Texture::NextPassMiiSmallI4, Texture::GuardedNextMiiSmallI4}) {
        const auto f = GetFixture(texture);
        nullHostPointer = true;
        ExpectAbort(MakeCpu(f.object), "GX_LOAD_TEX_OBJ_DATA_POINTER_NULL", f.size);
        nullHostPointer = false;
        throwOnInit = true;
        ExpectAbort(MakeCpu(f.object), "GX_LOAD_TEX_OBJ_HOST_EXCEPTION", f.size);
        throwOnInit = false;
    }
#endif
    // Alternate the captured data tuples on the same guest object. The
    // host allocation must remain stable while Init/LOD/UserData/Load refresh.
    const auto initRelocated = [](std::uint32_t dataBytes = 1024u,
                                  std::uint32_t objectBytes = 64u,
                                  std::uint32_t secondObjectBytes = 64u) {
        InitMemory(32u, rgbSize, 64u, dataBytes, objectBytes, secondObjectBytes, 2816u,
                   64u, 640u, 64u, 64u, 2560u, 64u, 64u, 128u, 64u, 64u, true);
    };
    initRelocated();
    CheckLoad(Texture::RelocatedMiiI4);
    CheckLoad(Texture::SecondRelocatedMiiI4);
    InitMemory();
    CheckLoad(Texture::MiiI4);
    CheckLoad(Texture::SecondMiiI4);
    CheckFamily();
    initRelocated();
    CheckLoad(Texture::RelocatedMiiI4);
    CheckLoad(Texture::SecondRelocatedMiiI4);
    CheckLoad(Texture::RelocatedMiiI4);
    CheckLoad(Texture::SecondRelocatedMiiI4);
    initRelocated();
    CheckDescriptorRefusals(Texture::RelocatedMiiI4);
    CheckDescriptorRefusals(Texture::SecondRelocatedMiiI4);
    // A relocated source is valid for every object in the audited family.
    // Object identity does not change texture memory or sampler semantics.
    for (auto texture : {Texture::NextPassMiiI4, Texture::SecondNextPassMiiI4}) {
        const auto object = GetFixture(texture).object;
        std::memcpy(Memory::GetPointer(object, 32u),
                    Memory::GetPointer(GetFixture(Texture::RelocatedMiiI4).object, 32u), 32u);
        auto fixture = GetFixture(texture);
        fixture.data = GetFixture(Texture::RelocatedMiiI4).data;
        CheckFixtureLoad(fixture);
    }
    for (auto bytes : {0u, 512u, 992u, 1023u}) {
        initRelocated(bytes);
        for (auto texture : {Texture::RelocatedMiiI4, Texture::SecondRelocatedMiiI4})
            ExpectAbort(MakeCpu(GetFixture(texture).object),
                        "GX_LOAD_TEX_OBJ_DATA_UNMAPPED", 1024u);
    }
    for (auto bytes : {0u, 31u}) {
        initRelocated(1024u, bytes);
        ExpectAbort(MakeCpu(GetFixture(Texture::RelocatedMiiI4).object),
                    "GX_LOAD_TEX_OBJ_GUEST_UNMAPPED", 0u);
    }
    for (auto bytes : {0u, 31u}) {
        initRelocated(1024u, 64u, bytes);
        ExpectAbort(MakeCpu(GetFixture(Texture::SecondRelocatedMiiI4).object),
                    "GX_LOAD_TEX_OBJ_GUEST_UNMAPPED", 0u);
    }
#if MKW_LOCAL_RENDERED_FAST_TRACK
    initRelocated();
    for (auto texture : {Texture::RelocatedMiiI4, Texture::SecondRelocatedMiiI4}) {
        nullHostPointer = true;
        ExpectAbort(MakeCpu(GetFixture(texture).object),
                    "GX_LOAD_TEX_OBJ_DATA_POINTER_NULL", 1024u);
        nullHostPointer = false;
        throwOnInit = true;
        ExpectAbort(MakeCpu(GetFixture(texture).object),
                    "GX_LOAD_TEX_OBJ_HOST_EXCEPTION", 1024u);
        throwOnInit = false;
    }
#endif
    InitMemory();
    CheckLoad(Texture::MiiI4);
    CheckLoad(Texture::SecondMiiI4);
    const auto previousStage = stage;
    KnownNativeCpuCall<0x80170f2cu>::Invoke(nullptr);
    assert(stage == previousStage);
    Memory::Reset();
    assert(validCalls >= 46u && diagnosedAborts > 100u);
    std::printf("PASS: texture-load valid=%u diagnosed-aborts=%u rendered=%d\n", validCalls,
                diagnosedAborts, MKW_LOCAL_RENDERED_FAST_TRACK);
}
