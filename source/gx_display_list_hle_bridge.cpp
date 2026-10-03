#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include "gx_internal.h"
#include "gx_display_list_native.hpp"

extern "C" void mkw_switch_pinned_fifo_write_burst(const std::uint8_t*, std::uint32_t);

namespace {
constexpr std::uint32_t kSavedGxData = 0x80344110u;
constexpr std::uint32_t kGxDataSize = 0x600u;
std::uint32_t sGxData = 0;
std::uint8_t sSaveContext = 0;
std::uint8_t* sListHost = nullptr;
HleGxState sSavedHleState{};
} // namespace
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept;

namespace {
[[noreturn]] void Refuse(const char* reason, CpuContext* cpu = nullptr, std::uint32_t target = 0x80172E00u) {
    if (FILE* out = std::fopen("sdmc:/switch/WiiCompiled-Switch/fast-track-gx-display-list.txt", "w")) {
        std::fprintf(out, "status=refused\nreason=%s\n", reason);
        if (cpu) {
            std::fprintf(out, "r3=0x%08x\nr4=0x%08x\n", cpu->gpr[3], cpu->gpr[4]);
        }
        std::fclose(out);
    }
    mkw_switch_report_unsupported_translated_dispatch(reason, target, cpu);
    std::abort();
}

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
void Status(const char* status, std::uint32_t bytes) {
    if (FILE* out = std::fopen("sdmc:/switch/WiiCompiled-Switch/fast-track-gx-display-list.txt", "w")) {
        std::fprintf(out, "status=%s\nbase=0x%08x\ncapacity=%u\nbytes=%u\nsave_context=%u\n",
                     status, g_dlRecordState.base, g_dlRecordState.size, bytes, sSaveContext);
        std::fclose(out);
    }
}

bool Overlaps(const void* first, std::size_t firstSize, const void* second, std::size_t secondSize) {
    const auto a = reinterpret_cast<std::uintptr_t>(first);
    const auto b = reinterpret_cast<std::uintptr_t>(second);
    return a <= b ? b - a < firstSize : a - b < secondSize;
}

void CheckActive(CpuContext* cpu = nullptr, std::uint32_t target = 0x80172E00u) {
    const auto cursor = mkw_switch_gx_display_list_cursor();
    if (!g_dlRecordState.active || !cursor.active || cursor.buffer != sListHost ||
        cursor.capacity != g_dlRecordState.size || cursor.written > cursor.capacity) {
        Refuse("GX_DISPLAY_LIST_STATE", cpu, target);
    }
}

void SyncCursor() {
    g_dlRecordState.count = mkw_switch_gx_display_list_cursor().written;
    g_dlRecordState.writePtr = g_dlRecordState.base + g_dlRecordState.count;
}
#endif
} // namespace

extern "C" void mkw_switch_hle_gx_begin_display_list(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }
    mkw_switch_set_fast_track_stage("RMCP01_GX_BEGIN_DISPLAY_LIST");
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    const auto base = cpu->gpr[3];
    const auto size = cpu->gpr[4];
    if (IsDisplayListActive() || mkw_switch_gx_display_list_cursor().active) {
        Refuse("GX_DISPLAY_LIST_NESTED", cpu);
    }
    if ((base & 31u) != 0 || base == 0 || size == 0 || (size & 31u) != 0 ||
        !Memory::Contains(base, size)) {
        Refuse("GX_DISPLAY_LIST_RANGE", cpu);
    }
    if (!Memory::Contains(kGXDataPtrAddr, 4u)) {
        Refuse("GX_DISPLAY_LIST_GX_DATA_POINTER", cpu);
    }
    const auto gd = Memory::Read32(kGXDataPtrAddr);
    if (!Memory::Contains(gd, kGxDataSize) || !Memory::Contains(kSavedGxData, kGxDataSize) ||
        !Memory::Contains(kDlFifoAddr, 0x24u)) {
        Refuse("GX_DISPLAY_LIST_METADATA_RANGE", cpu);
    }
    const auto* gxHost = Memory::GetPointer(gd, kGxDataSize);
    if (Overlaps(gxHost, kGxDataSize, Memory::GetPointer(kSavedGxData, kGxDataSize), kGxDataSize) ||
        Overlaps(gxHost, kGxDataSize, Memory::GetPointer(kDlFifoAddr, 0x24u), 0x24u) ||
        Overlaps(gxHost, kGxDataSize, Memory::GetPointer(kGXDataPtrAddr, 4u), 4u)) {
        Refuse("GX_DISPLAY_LIST_GX_DATA_OVERLAP", cpu);
    }
    // The Switch closure has no guest __GXSetDirtyState decoder. Never silently
    // clear unhandled guest state or capture an unfinished immediate primitive.
    if (Memory::Read32(gd + 0x5FCu) != 0 || Memory::Read8(gd + 0x5F8u) != 0 ||
        g_hleGxState.inBegin || g_hleGxState.fifoByteCount != 0) {
        Refuse("GX_DISPLAY_LIST_PENDING_STATE", cpu);
    }
    const auto save = Memory::Read8(gd + 0x5F9u);
    if (save > 1u) {
        Refuse("GX_DISPLAY_LIST_SAVE_FLAG", cpu);
    }
    auto* list = Memory::GetPointer(base, size);
    const auto overlaps = [&](std::uint32_t addr, std::size_t length) {
        return Overlaps(list, size, Memory::GetPointer(addr, length), length);
    };
    // Compare host ranges as well: cached/uncached guest aliases must not let
    // commands overwrite the metadata that controls their own recording.
    if (overlaps(gd, kGxDataSize) || overlaps(kSavedGxData, kGxDataSize) ||
        overlaps(kDlFifoAddr, 0x24u) || overlaps(kGXDataPtrAddr, 4u)) {
        Refuse("GX_DISPLAY_LIST_METADATA_OVERLAP", cpu);
    }
    mkw_switch_gx_native_begin_display_list(list, size, save); // Flush dirty native state before redirection.
    sGxData = gd;
    sSaveContext = save;
    sListHost = list;
    if (save) {
        std::memcpy(Memory::GetPointer(kSavedGxData, kGxDataSize), Memory::GetPointer(gd, kGxDataSize), kGxDataSize);
        sSavedHleState = g_hleGxState;
    }
    for (std::uint32_t offset = 0; offset < 0x24u; offset += 4u) {
        Memory::Write32(kDlFifoAddr + offset, 0);
    }
    Memory::Write32(kDlFifoAddr, base);
    Memory::Write32(kDlFifoAddr + 4u, base + size - 4u);
    Memory::Write32(kDlFifoAddr + 8u, size);
    Memory::Write32(kDlFifoAddr + 0x14u, base);
    Memory::Write32(kDlFifoAddr + 0x18u, base);
    Memory::Write8(gd + 0x5F8u, 1);
    BeginDisplayListRecording(base, size);
    Status("begin-pass", 0);
#else
    Refuse("GX_DISPLAY_LIST_REQUIRES_RENDERER", cpu);
#endif
}

extern "C" void mkw_switch_hle_gx_end_display_list(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }
    mkw_switch_set_fast_track_stage("RMCP01_GX_END_DISPLAY_LIST");
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    CheckActive(cpu, 0x80172EB4u);
    if (Memory::Read32(kGXDataPtrAddr) != sGxData || Memory::Read8(sGxData + 0x5F9u) != sSaveContext ||
        mkw_switch_gx_display_list_cursor().save_context != sSaveContext) {
        Refuse("GX_DISPLAY_LIST_CONTEXT_CHANGED", cpu, 0x80172EB4u);
    }
    const auto bytes = mkw_switch_gx_native_end_display_list(); // Includes native dirty writes and zero padding.
    if (bytes > g_dlRecordState.size || (bytes & 31u) != 0) {
        Refuse("GX_DISPLAY_LIST_END_COUNT", cpu, 0x80172EB4u);
    }
    g_dlRecordState.count = bytes;
    g_dlRecordState.writePtr = g_dlRecordState.base + bytes;
    if (sSaveContext) {
        const auto word8 = Memory::Read32(sGxData + 8u);
        std::memcpy(Memory::GetPointer(sGxData, kGxDataSize), Memory::GetPointer(kSavedGxData, kGxDataSize), kGxDataSize);
        Memory::Write32(sGxData + 8u, word8); // Pinned SDK restoration exception.
        g_hleGxState = sSavedHleState;
    }
    Memory::Write8(sGxData + 0x5F8u, 0);
    EndDisplayListRecording();
    cpu->gpr[3] = bytes;
    Status("end-pass", bytes);
#else
    Refuse("GX_DISPLAY_LIST_REQUIRES_RENDERER", cpu);
#endif
}

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
extern "C" void mkw_switch_gx_record_begin(CpuContext* cpu) noexcept {
    if (!cpu)
        return;
    CheckActive(cpu, 0x8016F0F0u);
    const auto primitive = cpu->gpr[3];
    const auto format = cpu->gpr[4];
    const auto count = cpu->gpr[5];
    if ((primitive != GX_QUADS && primitive != GX_TRIANGLES && primitive != GX_TRIANGLESTRIP &&
         primitive != GX_TRIANGLEFAN && primitive != GX_LINES && primitive != GX_LINESTRIP &&
         primitive != GX_POINTS) ||
        format >= GX_MAX_VTXFMT || count > 0xFFFFu) {
        Refuse("GX_DISPLAY_LIST_PRIMITIVE", cpu, 0x8016F0F0u);
    }
    // Immediate-mode indexed attributes are expanded to direct Aurora layout.
    // Their raw recorded layout needs a separate implementation, not truncation.
    for (const auto type : g_hleGxState.vtxDesc) {
        if (type != GX_NONE && type != GX_DIRECT) {
            Refuse("GX_DISPLAY_LIST_INDEXED_VERTEX", cpu, 0x8016F0F0u);
        }
    }
    GXFlush(); // Pending native VCD/VAT/register bytes must precede geometry.
    WriteDisplayListData(primitive | format, 1);
    WriteDisplayListData(count, 2);
}

void BeginDisplayListRecording(std::uint32_t base, std::uint32_t size) {
    g_dlRecordState = {base, size, base, 0, base != 0 && size != 0};
}

void EndDisplayListRecording() {
    Memory::Write32(kDlWritePtrAddr, g_dlRecordState.writePtr);
    Memory::Write32(kDlCountAddr, g_dlRecordState.count);
    g_dlRecordState.active = false;
}

void WriteDisplayListData(std::uint32_t value, std::uint32_t size) {
    CheckActive();
    if (size != 1 && size != 2 && size != 4) {
        Refuse("GX_DISPLAY_LIST_WRITE_SIZE");
    }
    mkw_switch_gx_record_scalar(value, size);
    SyncCursor();
}

extern "C" void GX_HLE_FIFO_WriteBurst(const std::uint8_t* data, std::uint32_t size) {
    if (!data || size == 0) {
        return;
    }
    if (!IsDisplayListActive()) {
        mkw_switch_pinned_fifo_write_burst(data, size);
        return;
    }
    CheckActive();
    mkw_switch_gx_record_burst(data, size);
    SyncCursor();
}
#endif
#endif
