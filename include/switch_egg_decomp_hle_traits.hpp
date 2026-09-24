#pragma once

#include "abi_bridge.h"
#include "memory.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace mkw::egg_decomp_hle_detail {

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
inline void WriteDecodeStatus(
    const char* status,
    std::uint32_t src,
    std::uint32_t dst,
    std::uint32_t expandSize,
    std::uint32_t srcConsumed,
    std::uint32_t dstProduced) noexcept {
    constexpr const char* kPath =
        "sdmc:/switch/WiiCompiled-Switch/fast-track-szs-decode-status.txt";
    FILE* out = std::fopen(kPath, "w");
    if (!out) {
        return;
    }
    std::fprintf(
        out,
        "status=%s\n"
        "src=0x%08x\n"
        "dst=0x%08x\n"
        "expand_size=%u\n"
        "src_consumed=%u\n"
        "dst_produced=%u\n",
        status ? status : "<null>",
        src,
        dst,
        expandSize,
        srcConsumed,
        dstProduced);
    std::fclose(out);
}
#else
inline void WriteDecodeStatus(
    const char*,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t) noexcept {}
#endif

[[noreturn]] inline void AbortDecode(
    const char* status,
    CpuContext* cpu,
    std::uint32_t src,
    std::uint32_t dst,
    std::uint32_t expandSize,
    std::uint32_t srcConsumed,
    std::uint32_t dstProduced) noexcept {
    WriteDecodeStatus(
        status, src, dst, expandSize, srcConsumed, dstProduced);
    mkw_switch_report_unsupported_translated_dispatch(
        status, 0x80218C2Cu, cpu);
    std::abort();
}

inline std::uint8_t ReadByte(
    CpuContext* cpu,
    std::uint32_t address,
    std::uint32_t src,
    std::uint32_t dst,
    std::uint32_t expandSize,
    std::uint32_t srcConsumed,
    std::uint32_t dstProduced) {
    if (!Memory::Contains(address, 1u)) {
        AbortDecode(
            "SZS_SOURCE_UNMAPPED",
            cpu,
            src,
            dst,
            expandSize,
            srcConsumed,
            dstProduced);
    }
    return Memory::Read8(address);
}

inline void WriteByte(
    CpuContext* cpu,
    std::uint32_t address,
    std::uint8_t value,
    std::uint32_t src,
    std::uint32_t dst,
    std::uint32_t expandSize,
    std::uint32_t srcConsumed,
    std::uint32_t dstProduced) {
    if (!Memory::Contains(address, 1u)) {
        AbortDecode(
            "SZS_DEST_UNMAPPED",
            cpu,
            src,
            dst,
            expandSize,
            srcConsumed,
            dstProduced);
    }
    Memory::Write8(address, value);
}

inline std::uint32_t DecodeSZS(CpuContext* cpu) {
    if (!cpu) {
        return 0u;
    }

    const std::uint32_t src = cpu->gpr[3];
    const std::uint32_t dst = cpu->gpr[4];
    if (!Memory::Contains(src, 16u)) {
        AbortDecode("SZS_HEADER_UNMAPPED", cpu, src, dst, 0u, 0u, 0u);
    }

    const std::uint32_t expandSize =
        (static_cast<std::uint32_t>(Memory::Read8(src + 4u)) << 24u) |
        (static_cast<std::uint32_t>(Memory::Read8(src + 5u)) << 16u) |
        (static_cast<std::uint32_t>(Memory::Read8(src + 6u)) << 8u) |
        static_cast<std::uint32_t>(Memory::Read8(src + 7u));

    if (expandSize != 0u && !Memory::Contains(dst, expandSize)) {
        AbortDecode(
            "SZS_DEST_RANGE_UNMAPPED",
            cpu,
            src,
            dst,
            expandSize,
            16u,
            0u);
    }

    std::uint32_t srcIdx = 16u;
    std::uint32_t dstIdx = 0u;
    std::uint32_t mask = 0u;
    std::uint32_t flags = 0u;

    while (static_cast<std::int32_t>(dstIdx) <
           static_cast<std::int32_t>(expandSize)) {
        if (mask == 0u) {
            flags = ReadByte(
                cpu,
                src + srcIdx,
                src,
                dst,
                expandSize,
                srcIdx,
                dstIdx);
            ++srcIdx;
            mask = 0x80u;
        }

        if ((flags & mask) != 0u) {
            const std::uint8_t value = ReadByte(
                cpu,
                src + srcIdx,
                src,
                dst,
                expandSize,
                srcIdx,
                dstIdx);
            ++srcIdx;
            WriteByte(
                cpu,
                dst + dstIdx,
                value,
                src,
                dst,
                expandSize,
                srcIdx,
                dstIdx);
            ++dstIdx;
        } else {
            const std::uint32_t high = ReadByte(
                cpu,
                src + srcIdx,
                src,
                dst,
                expandSize,
                srcIdx,
                dstIdx);
            const std::uint32_t low = ReadByte(
                cpu,
                src + srcIdx + 1u,
                src,
                dst,
                expandSize,
                srcIdx + 1u,
                dstIdx);
            srcIdx += 2u;

            const std::uint32_t rep = (high << 8u) | low;
            const std::uint32_t distance = (rep & 0x0FFFu) + 1u;
            if (distance > dstIdx) {
                AbortDecode(
                    "SZS_BACKREF_BEFORE_OUTPUT",
                    cpu,
                    src,
                    dst,
                    expandSize,
                    srcIdx,
                    dstIdx);
            }

            std::uint32_t copyIdx = dstIdx - distance;
            std::uint32_t count = rep >> 12u;
            if (count != 0u) {
                count += 2u;
            } else {
                count =
                    static_cast<std::uint32_t>(ReadByte(
                        cpu,
                        src + srcIdx,
                        src,
                        dst,
                        expandSize,
                        srcIdx,
                        dstIdx)) +
                    18u;
                ++srcIdx;
            }

            for (std::uint32_t i = 0u; i < count; ++i) {
                if (dstIdx >= expandSize) {
                    AbortDecode(
                        "SZS_OUTPUT_OVERRUN",
                        cpu,
                        src,
                        dst,
                        expandSize,
                        srcIdx,
                        dstIdx);
                }
                const std::uint8_t value = ReadByte(
                    cpu,
                    dst + copyIdx,
                    src,
                    dst,
                    expandSize,
                    srcIdx,
                    dstIdx);
                ++copyIdx;
                WriteByte(
                    cpu,
                    dst + dstIdx,
                    value,
                    src,
                    dst,
                    expandSize,
                    srcIdx,
                    dstIdx);
                ++dstIdx;
            }
        }

        mask >>= 1u;
    }

    WriteDecodeStatus(
        "decode-pass", src, dst, expandSize, srcIdx, dstIdx);
    return expandSize;
}

} // namespace mkw::egg_decomp_hle_detail

// EGG::Decomp::decodeSZS (PAL 0x80218C2C). Pinned WiiCompiled deliberately
// native-overrides this exact Yaz0/SZS decoder. The first hardware hit is the
// freshly read /Boot/Strap/eu/English.szs buffer, so mirror only this boundary
// and publish the decoded byte count in guest r3.
template <>
struct KnownNativeCpuCall<0x80218C2Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (cpu) {
            cpu->gpr[3] = mkw::egg_decomp_hle_detail::DecodeSZS(cpu);
        }
    }
};
