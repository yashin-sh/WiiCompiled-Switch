#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"
#include <dolphin/gx.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>

namespace {
const char* stage = nullptr;
std::uint32_t calls = 0;
std::uint32_t calledTarget = 0;
GXClipMode clipMode = GX_CLIP_ENABLE;
GXBool enabled = GX_FALSE;
u8 alpha = 0;

void ExpectStage(const char* expected) {
    assert(stage != nullptr && std::strcmp(stage, expected) == 0);
}

template <std::uint32_t Target>
void InvokeAndCheck(std::uint32_t r3, std::uint32_t r4, const char* expectedStage) {
    CpuContext cpu;
    std::memset(&cpu, 0xa5, sizeof(cpu));
    cpu.gpr[3] = r3;
    cpu.gpr[4] = r4;
    std::array<unsigned char, sizeof(cpu)> before{};
    std::memcpy(before.data(), &cpu, sizeof(cpu));
    const auto previousCalls = calls;
    stage = nullptr;

    KnownNativeCpuCall<Target>::Invoke(&cpu);

    ExpectStage(expectedStage);
    assert(std::memcmp(before.data(), &cpu, sizeof(cpu)) == 0);
#if MKW_LOCAL_RENDERED_FAST_TRACK
    assert(calls == previousCalls + 1 && calledTarget == Target);
#else
    assert(calls == previousCalls);
#endif
}
} // namespace

extern "C" void mkw_switch_set_fast_track_stage(const char* value) noexcept {
    stage = value;
}

// These sinks exercise the real bridges, traits, CpuContext and Aurora types.
// They observe forwarding only; they do not simulate the Aurora FIFO/backend.
extern "C" void GXSetClipMode(GXClipMode mode) {
    ExpectStage("RMCP01_GX_SET_CLIP_MODE");
    ++calls;
    calledTarget = 0x8017351cu;
    clipMode = mode;
}

extern "C" void GXSetDither(GXBool value) {
    ExpectStage("RMCP01_GX_SET_DITHER");
    ++calls;
    calledTarget = 0x80172930u;
    enabled = value;
}

extern "C" void GXSetDstAlpha(GXBool value, u8 reference) {
    ExpectStage("RMCP01_GX_SET_DST_ALPHA");
    ++calls;
    calledTarget = 0x8017295cu;
    enabled = value;
    alpha = reference;
}

int main() {
    static_assert(KnownNativeCpuCall<0x8017351Cu>::kAvailable);
    static_assert(KnownNativeCpuCall<0x80172930u>::kAvailable);
    static_assert(KnownNativeCpuCall<0x8017295Cu>::kAvailable);
    static_assert(GX_CLIP_ENABLE == 0 && GX_CLIP_DISABLE == 1);

    KnownNativeCpuCall<0x8017351Cu>::Invoke(nullptr);
    KnownNativeCpuCall<0x80172930u>::Invoke(nullptr);
    KnownNativeCpuCall<0x8017295Cu>::Invoke(nullptr);
    assert(calls == 0 && stage == nullptr);

    for (auto mode : {0u, 1u}) {
        InvokeAndCheck<0x8017351Cu>(mode, 0x12345678u, "RMCP01_GX_SET_CLIP_MODE");
#if MKW_LOCAL_RENDERED_FAST_TRACK
        assert(clipMode == static_cast<GXClipMode>(mode));
#endif
    }

    for (auto value : {0u, 1u, 2u, 0x100u, 0xffffffffu}) {
        InvokeAndCheck<0x80172930u>(value, 0x12345678u, "RMCP01_GX_SET_DITHER");
#if MKW_LOCAL_RENDERED_FAST_TRACK
        assert(enabled == (value != 0u));
#endif
        for (auto reference : {0u, 1u, 0xffu, 0x100u, 0x1ffu, 0xffffffffu}) {
            InvokeAndCheck<0x8017295Cu>(value, reference, "RMCP01_GX_SET_DST_ALPHA");
#if MKW_LOCAL_RENDERED_FAST_TRACK
            assert(enabled == (value != 0u));
            assert(alpha == (reference & 0xffu));
#endif
        }
    }

    const auto previousCalls = calls;
    const auto previousStage = stage;
    KnownNativeCpuCall<0x8017351Cu>::Invoke(nullptr);
    KnownNativeCpuCall<0x80172930u>::Invoke(nullptr);
    KnownNativeCpuCall<0x8017295Cu>::Invoke(nullptr);
    assert(calls == previousCalls && stage == previousStage);
}
