#include "switch_ios_kd_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

// Mapping/link coverage for the three public IOS entry points. Executable
// Nintendo-data-free contracts separately exercise live handle lifetime,
// full buffer checks and the pinned KD scheduler phase across close/reopen.
static_assert(KnownNativeCpuCall<0x801938F8u>::kAvailable);
static_assert(KnownNativeCpuCall<0x80194290u>::kAvailable);
static_assert(KnownNativeCpuCall<0x80193AD8u>::kAvailable);

extern "C" __attribute__((used)) bool synthetic_ios_open_kd_request_hle_probe() {
    return KnownNativeCpuCall<0x801938F8u>::kAvailable &&
           KnownNativeCpuCall<0x80194290u>::kAvailable &&
           KnownNativeCpuCall<0x80193AD8u>::kAvailable;
}

#endif
