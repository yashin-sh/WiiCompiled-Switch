#pragma once

#include "abi_bridge.h"

// Bounded /dev/net/kd/request implementation lives in one translation unit so
// changing request behavior does not recompile every generated guest shard.
extern "C" void mkw_switch_hle_ios_open_kd_request(CpuContext*) noexcept;
extern "C" void mkw_switch_hle_ios_ioctl_kd_request(CpuContext*) noexcept;
extern "C" void mkw_switch_hle_ios_close_kd_request(CpuContext*) noexcept;

template <>
struct KnownNativeCpuCall<0x801938F8u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_ios_open_kd_request(cpu);
    }
};
template <>
struct KnownNativeCpuCall<0x80194290u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_ios_ioctl_kd_request(cpu);
    }
};
template <>
struct KnownNativeCpuCall<0x80193AD8u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_ios_close_kd_request(cpu);
    }
};
