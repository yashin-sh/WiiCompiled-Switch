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
constexpr std::uint32_t gxPointer = 0x803886c8u;
constexpr std::uint32_t gxData = 0x70005000u;
const char* stage = nullptr;
const char* expectedReason = nullptr;
std::uint32_t expectedSize = 0;
CpuContext expectedCpu{};
int reportPipe = -1;
std::uint32_t nativeCalls = 0;
std::size_t resolvedLength = 0;
bool nullHostPointer = false;
bool throwOnInit = false;
#if MKW_LOCAL_RENDERED_FAST_TRACK
GXTexObj* previousIa8Host = nullptr;
GXTexObj* previousRgbHost = nullptr;
#endif
GXTexObj* currentHost = nullptr;
const void* currentData = nullptr;
std::uint16_t currentWidth = 0;
std::uint16_t currentHeight = 0;
GXTexFmt currentFormat = GX_TF_I4;
GXBool currentEdgeLod = GX_FALSE;
std::uint32_t expectedMap = 0u;
struct HostRegion {
    std::uint32_t base;
    std::vector<std::uint8_t> bytes;
};
std::vector<HostRegion> regions;

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
void WriteDescriptor(bool ia8) {
    const auto object = ia8 ? ia8Obj : rgbObj;
    const auto data = ia8 ? ia8Data : rgbData;
    const std::uint32_t width = ia8 ? 4u : 832u;
    const std::uint32_t height = ia8 ? 4u : 456u;
    const std::uint32_t format = ia8 ? 3u : 4u;
    const auto blocks = ((width + 3u) / 4u) * ((height + 3u) / 4u);
    // Independent field encoding; no texture payload from the game is used.
    const std::array<std::uint32_t, 8> words{
        (1u << 4u) | (4u << 5u) | (ia8 ? 1u << 8u : 0u), 0u,
        (width - 1u) | ((height - 1u) << 10u) | (format << 20u),
        data >> 5u, 0u, format, 0u, (blocks << 16u) | (2u << 8u) | 2u};
    for (std::uint32_t i = 0; i < words.size(); ++i) {
        WriteWord(object + 4u * i, words[i]);
    }
}
void InitMemory(std::uint32_t ia8Bytes = 32u, std::uint32_t rgbBytes = rgbSize,
                std::uint32_t descriptorBytes = 64u) {
    Memory::Config config;
    config.regions.push_back({"ia8-object", ia8Obj, descriptorBytes});
    config.regions.push_back({"rgb-object", rgbObj, 64u});
    if (ia8Bytes)
        config.regions.push_back({"ia8-data", ia8Data, ia8Bytes});
    if (rgbBytes)
        config.regions.push_back({"rgb-data", rgbData, rgbBytes});
    config.regions.push_back({"gx-pointer", gxPointer, 4u});
    config.regions.push_back({"gx-state", gxData, 0x604u});
    Memory::Init(config);
    WriteWord(gxPointer, gxData);
    WriteWord(gxData + 0x5fcu, 0x12345678u);
    Memory::Write16(gxData + 2u, 0xabcd);
    if (descriptorBytes >= 32u)
        WriteDescriptor(true);
    WriteDescriptor(false);
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
        close(descriptors[0]);
        reportPipe = descriptors[1];
        expectedReason = reason;
        expectedSize = size;
        std::memcpy(&expectedCpu, &cpu, sizeof(cpu));
        nativeCalls = 0u;
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
}
void CheckLoad(bool ia8, std::uint32_t tid = 0u) {
    const auto object = ia8 ? ia8Obj : rgbObj;
    const auto data = ia8 ? ia8Data : rgbData;
    const auto size = ia8 ? 32u : rgbSize;
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
    assert(currentWidth == (ia8 ? 4u : 832u) && currentHeight == (ia8 ? 4u : 456u));
    assert(currentFormat == (ia8 ? GX_TF_IA8 : GX_TF_RGB565));
    assert(currentEdgeLod == (ia8 ? GX_FALSE : GX_TRUE));
    auto*& previous = ia8 ? previousIa8Host : previousRgbHost;
    if (previous)
        assert(previous == currentHost);
    previous = currentHost;
    if (previousIa8Host && previousRgbHost)
        assert(previousIa8Host != previousRgbHost);
#else
    assert(nativeCalls == 0u);
#endif
    CheckStatus("load-pass", size);
#if MKW_LOCAL_RENDERED_FAST_TRACK
    assert(Status().find("tid=" + std::to_string(tid) + "\n") != std::string::npos);
#endif
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
    assert(nativeCalls++ == 0u);
    if (throwOnInit)
        throw std::runtime_error("test GX init failure");
    assert(s == GX_CLAMP && t == GX_CLAMP && mip == GX_FALSE);
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
    assert(nativeCalls++ == 1u && obj == currentHost);
    assert(min == GX_LINEAR && mag == GX_LINEAR && minLod == 0.f && maxLod == 0.f && bias == 0.f);
    assert(clamp == GX_FALSE && aniso == GX_ANISO_1);
    currentEdgeLod = edge;
}
extern "C" void GXInitTexObjUserData(GXTexObj* obj, void* data) {
    CheckStage();
    assert(nativeCalls++ == 2u && obj == currentHost && data == nullptr);
}
extern "C" void GXLoadTexObj(GXTexObj* obj, GXTexMapID id) {
    CheckStage();
    assert(nativeCalls++ == 3u && obj == currentHost && id == static_cast<GXTexMapID>(expectedMap));
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(
    const char* reason, std::uint32_t target, CpuContext* cpu) noexcept {
    assert(reportPipe >= 0);
    CheckStage();
    assert(std::strcmp(reason, expectedReason) == 0 && target == 0x80170f2cu);
    assert(std::memcmp(cpu, &expectedCpu, sizeof(*cpu)) == 0);
    assert(nativeCalls == (throwOnInit ? 1u : 0u));
    if (Memory::Contains(gxPointer, 4u)) {
        assert(Memory::Read32(gxData + 0x5fcu) == 0x12345678u);
        assert(Memory::Read16(gxData + 2u) == 0xabcdu);
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
    for (auto ia8 : {false, true, false, true})
        CheckLoad(ia8);
    for (std::uint32_t tid = 0u; tid < 8u; ++tid)
        CheckLoad(true, tid);
    // Each descriptor word/object variation and unapproved map must still stop.
    InitMemory();
    for (auto ia8 : {false, true}) {
        const auto object = ia8 ? ia8Obj : rgbObj;
        for (std::uint32_t word = 0; word < 8u; ++word) {
            const auto value = Memory::Read32(object + 4u * word);
            WriteWord(object + 4u * word, value ^ 1u);
            ExpectAbort(MakeCpu(object), "GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR", 0u);
            WriteWord(object + 4u * word, value);
        }
        for (auto tid : {8u, 0xffu, 0xffffffffu}) {
            ExpectAbort(MakeCpu(object, tid), "GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR", 0u);
        }
        if (!ia8)
            ExpectAbort(MakeCpu(object, 1u), "GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR", 0u);
        ExpectAbort(MakeCpu(object + 32u), "GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR", 0u);
    }
    for (auto bytes : {0u, 31u}) {
        InitMemory(bytes);
        ExpectAbort(MakeCpu(ia8Obj), "GX_LOAD_TEX_OBJ_DATA_UNMAPPED", 32u);
    }
    InitMemory(32u, rgbSize - 1u);
    ExpectAbort(MakeCpu(rgbObj), "GX_LOAD_TEX_OBJ_DATA_UNMAPPED", rgbSize);
    InitMemory(32u, rgbSize, 31u);
    ExpectAbort(MakeCpu(ia8Obj), "GX_LOAD_TEX_OBJ_GUEST_UNMAPPED", 0u);
    InitMemory();
#if MKW_LOCAL_RENDERED_FAST_TRACK
    nullHostPointer = true;
    ExpectAbort(MakeCpu(ia8Obj), "GX_LOAD_TEX_OBJ_DATA_POINTER_NULL", 32u);
    nullHostPointer = false;
    throwOnInit = true;
    ExpectAbort(MakeCpu(ia8Obj), "GX_LOAD_TEX_OBJ_HOST_EXCEPTION", 32u);
    throwOnInit = false;
#endif
    const auto previousStage = stage;
    KnownNativeCpuCall<0x80170f2cu>::Invoke(nullptr);
    assert(stage == previousStage);
    Memory::Reset();
}
