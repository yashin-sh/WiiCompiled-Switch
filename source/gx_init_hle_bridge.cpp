#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

#include <cstdint>

namespace {

constexpr std::uint32_t kFifoObjAddr = 0x80343740u;
constexpr std::uint32_t kGxDataAddr = 0x803437C0u;
constexpr std::uint32_t kGxDataSize = 0x600u;
constexpr std::uint32_t kGxDataPtrAddr = 0x803886C8u;
constexpr std::uint32_t kHandlerTablePtrAddr = 0x803868F8u;
constexpr std::uint32_t kHandlerTableAddr = 0x80003040u;
constexpr std::uint32_t kInterruptMaskLoAddr = 0x800000C4u;
constexpr std::uint32_t kRunningContextAddr = 0x800000E4u;
constexpr std::uint32_t kGxThreadQueueAddr = 0x803867C0u;
constexpr std::uint32_t kGxCurrentThreadAddr = 0x803867C4u;
constexpr std::uint32_t kGxWrapFlagAddr = 0x803867B0u;
constexpr std::uint32_t kGxWrapFlag2Addr = 0x803867B1u;
constexpr std::uint32_t kGpFifoObjAddr = 0x80343DC0u;
constexpr std::uint32_t kCpuFifoObjAddr = 0x80343DE4u;
constexpr std::uint32_t kPeThreadQueueAddr = 0x803867D0u;

void ZeroWords(std::uint32_t address, std::uint32_t sizeBytes) noexcept {
    if (!Memory::Contains(address, sizeBytes)) {
        return;
    }
    for (std::uint32_t offset = 0u; offset < sizeBytes; offset += 4u) {
        Memory::Write32(address + offset, 0u);
    }
}

void InstallHandler(std::uint32_t interrupt, std::uint32_t handler) noexcept {
    if (interrupt >= 32u) {
        return;
    }

    std::uint32_t table = 0u;
    if (Memory::Contains(kHandlerTablePtrAddr, 4u)) {
        table = Memory::Read32(kHandlerTablePtrAddr);
        if (table == 0u) {
            table = kHandlerTableAddr;
            Memory::Write32(kHandlerTablePtrAddr, table);
        }
    }
    if (table == 0u) {
        table = kHandlerTableAddr;
    }

    const std::uint32_t entry = table + interrupt * 4u;
    if (Memory::Contains(entry, 4u)) {
        Memory::Write32(entry, handler);
    }
}

void Unmask(std::uint32_t mask) noexcept {
    if (Memory::Contains(kInterruptMaskLoAddr, 4u)) {
        Memory::Write32(kInterruptMaskLoAddr, Memory::Read32(kInterruptMaskLoAddr) & ~mask);
    }
}

} // namespace

extern "C" void mkw_switch_hle_gx_init(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    // Pinned WiiCompiled always returns the guest FIFO object address. The
    // stable fast-track remains headless. The rendered variant additionally
    // initializes the already-created Aurora GX backend before publishing the
    // same guest-visible SDK state below.
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    GXInit(nullptr, 0);
#endif
    cpu->gpr[3] = kFifoObjAddr;
    if (!Memory::IsInitialized()) {
        return;
    }

    ZeroWords(kGxDataAddr, kGxDataSize);
    if (Memory::Contains(kGxDataAddr + 0x5F8u, 3u)) {
        Memory::Write8(kGxDataAddr + 0x5F8u, 0u);
        Memory::Write8(kGxDataAddr + 0x5F9u, 1u);
        Memory::Write8(kGxDataAddr + 0x5FAu, 1u);
    }
    if (Memory::Contains(kGxDataPtrAddr, 4u)) {
        Memory::Write32(kGxDataPtrAddr, kGxDataAddr);
    }

    InstallHandler(0x11u, 0x8016C668u);
    Unmask(0x4000u);
    if (Memory::Contains(kGxCurrentThreadAddr, 4u)) {
        const std::uint32_t current = Memory::Contains(kRunningContextAddr, 4u) ? Memory::Read32(kRunningContextAddr) : 0u;
        Memory::Write32(kGxCurrentThreadAddr, current);
    }
    if (Memory::Contains(kGxThreadQueueAddr, 4u)) {
        Memory::Write32(kGxThreadQueueAddr, 0u);
    }
    ZeroWords(kCpuFifoObjAddr, 0x24u);
    ZeroWords(kGpFifoObjAddr, 0x24u);
    if (Memory::Contains(kGxWrapFlagAddr, 1u)) {
        Memory::Write8(kGxWrapFlagAddr, 0u);
    }
    if (Memory::Contains(kGxWrapFlag2Addr, 1u)) {
        Memory::Write8(kGxWrapFlag2Addr, 0u);
    }

    InstallHandler(0x12u, 0x8016ECCCu);
    InstallHandler(0x13u, 0x8016ED94u);
    Unmask(0x1000u);
    Unmask(0x2000u);
    if (Memory::Contains(kPeThreadQueueAddr, 4u)) {
        Memory::Write32(kPeThreadQueueAddr, 0u);
    }

    if (!Memory::Contains(kGxDataAddr, kGxDataSize)) {
        return;
    }

    Memory::Write16(kGxDataAddr + 0x0Au, static_cast<std::uint16_t>(Memory::Read16(kGxDataAddr + 0x0Au) | 0x000Fu));
    Memory::Write32(kGxDataAddr + 0x254u, 0u);
    Memory::Write32(kGxDataAddr + 0x174u, 0x0F0000FFu);
    Memory::Write32(kGxDataAddr + 0x07Cu, 0x22000000u);
    Memory::Write32(kGxDataAddr + 0x170u, 0x27000000u);
    Memory::Write32(kGxDataAddr + 0x5E4u, 0u);
    Memory::Write32(kGxDataAddr + 0x5E8u, 0u);
    Memory::Write32(kGxDataAddr + 0x5FCu, 0u);

    constexpr std::uint32_t baseRegs[] = {0x30u, 0x38u};
    for (std::uint32_t i = 0u; i < 2u; ++i) {
        const std::uint32_t r = baseRegs[i];
        const std::uint32_t s = i * 0x10u;
        Memory::Write32(kGxDataAddr + 0x108u + s, r << 24);
        Memory::Write32(kGxDataAddr + 0x128u + s, (r + 1u) << 24);
        Memory::Write32(kGxDataAddr + 0x10Cu + s, (r + 2u) << 24);
        Memory::Write32(kGxDataAddr + 0x12Cu + s, (r + 3u) << 24);
        Memory::Write32(kGxDataAddr + 0x110u + s, (r + 4u) << 24);
        Memory::Write32(kGxDataAddr + 0x130u + s, (r + 5u) << 24);
        Memory::Write32(kGxDataAddr + 0x114u + s, (r + 6u) << 24);
        Memory::Write32(kGxDataAddr + 0x134u + s, (r + 7u) << 24);
    }
}

#endif
