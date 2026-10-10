// Exercise the production Switch bridge against the actual Aurora GX decoder.
#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "gx_internal.h"
#include "memory.h"
#include <bit>
#include <cstring>
#include <stdexcept>

extern "C" void mkw_switch_hle_lyt_draw_quad(CpuContext*) noexcept;
HleGxState g_hleGxState;
GxDisplayListState g_dlRecordState;
bool g_alphaCompareValid = false;
std::atomic_bool g_auroraFrameActive{false}, g_auroraFrameHadWork{false};
namespace {
std::vector<std::uint8_t> memory;
bool active = false;
void Require(bool condition) {
    if (!condition)
        throw std::runtime_error("production LYT quad bridge contract failed");
}
void Float(unsigned address, float value) {
    Memory::Write32(address, std::bit_cast<std::uint32_t>(value));
}
} // namespace
namespace GuestFlat {
bool IsActive() {
    return active;
}
void Initialize(const std::vector<RegionRequest>& regions) {
    Require(regions.size() == 1 && regions[0].base == 0);
    memory.assign(regions[0].size, 0xa5);
    active = true;
}
std::uint8_t* HostPointer(std::uint32_t address) {
    return address == 0 ? memory.data() : nullptr;
}
void Shutdown() noexcept {
    active = false;
    memory.clear();
}
} // namespace GuestFlat
extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept {}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char*, std::uint32_t, CpuContext*) noexcept {
    std::abort();
}
void EnsureAuroraFrameActive() {
    Require(g_auroraFrameActive);
}
void draw_lyt_quad(float center, bool colors, bool blue) {
    Memory::Config config;
    config.regions = {{"synthetic-layout-quad", 0, 128}};
    Memory::Init(config);
    CpuContext cpu;
    std::memset(&cpu, 0xa5, sizeof(cpu));
    cpu.gpr[3] = 1;
    cpu.gpr[4] = 9;
    cpu.gpr[5] = 1;
    cpu.gpr[6] = 17;
    cpu.gpr[7] = colors ? 49 : 0;
    cpu.gpr[8] = 0xffffff7f;
    Float(1, center - 0.4f);
    Float(5, 0.7f);
    Float(9, 0.8f);
    Float(13, 1.4f);
    for (unsigned i = 0; i < 8; ++i)
        Float(17 + i * 4, 0.5f);
    for (unsigned corner = 0; corner < 4; ++corner)
        Memory::Write32(49 + corner * 4, blue ? 0x0000ffff : 0xff0000ff);
    g_hleGxState = {};
    g_hleGxState.vtxDesc[GX_VA_POS] = GX_DIRECT;
    g_hleGxState.vtxAttrFmt[0][GX_VA_POS] = {GX_POS_XY, GX_F32, 0};
    g_hleGxState.vtxDesc[GX_VA_TEX0] = GX_DIRECT;
    g_hleGxState.vtxAttrFmt[0][GX_VA_TEX0] = {GX_TEX_ST, GX_F32, 0};
    if (colors) {
        g_hleGxState.vtxDesc[GX_VA_CLR0] = GX_DIRECT;
        g_hleGxState.vtxAttrFmt[0][GX_VA_CLR0] = {GX_CLR_RGBA, GX_RGBA8, 0};
    }
    const auto beforeMemory = memory;
    const auto beforeCpu = cpu;
    std::array<std::uint8_t, sizeof(HleGxState)> beforeHle{};
    std::memcpy(beforeHle.data(), &g_hleGxState, sizeof(g_hleGxState));
    g_auroraFrameActive = true;
    g_auroraFrameHadWork = false;
    mkw_switch_hle_lyt_draw_quad(&cpu);
    Require(g_auroraFrameHadWork && g_alphaCompareValid);
    Require(memory == beforeMemory && std::memcmp(&cpu, &beforeCpu, sizeof(cpu)) == 0);
    Require(std::memcmp(beforeHle.data(), &g_hleGxState, sizeof(g_hleGxState)) == 0);
    Memory::Reset();
}
