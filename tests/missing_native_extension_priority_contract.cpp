#include "abi_bridge.h"
#include <cassert>
#include <cstdio>
namespace {
unsigned lookup = 0, native = 0, translated = 0, extension = 0, notes = 0, polls = 0;
void Translated(CpuContext*) {
    ++translated;
}
void Extension(CpuContext*) noexcept {
    ++extension;
}
} // namespace
template <>
struct KnownNativeCpuCall<0x12340001> {
    static constexpr bool kAvailable = true;
    static void Invoke(CpuContext*) noexcept {
        ++native;
    }
};
template <>
struct KnownTranslatedCpuCall<0x12340001> {
    static constexpr bool kAvailable = true;
    static constexpr std::uint32_t kNonvolatileFprWriteMask = 0;
    static constexpr auto Entry = &Translated;
};
template <>
struct KnownTranslatedCpuCall<0x12340002> {
    static constexpr bool kAvailable = true;
    static constexpr std::uint32_t kNonvolatileFprWriteMask = 0;
    static constexpr auto Entry = &Translated;
};
extern "C" MkwSwitchNativeCpuExtension mkw_switch_find_missing_native_cpu_extension(std::uint32_t) noexcept {
    ++lookup;
    return Extension;
}
extern "C" void mkw_switch_note_translated_dispatch(std::uint32_t, CpuContext*) noexcept {
    ++notes;
}
extern "C" void mkw_switch_hle_vi_poll_retrace(CpuContext*) noexcept {
    ++polls;
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char*, std::uint32_t, CpuContext*) noexcept {
    assert(false);
}
int main() {
    CpuContext cpu{};
    InvokeDirectCpu<0x12340001>(&cpu);
    assert(native == 1 && translated == 0 && lookup == 0 && notes == 1 && polls == 1);
    InvokeDirectCpu<0x12340002>(&cpu);
    assert(native == 1 && translated == 1 && lookup == 0 && notes == 2 && polls == 2);
    InvokeDirectCpu<0x12340003>(&cpu);
    assert(lookup == 1 && extension == 1 && notes == 3 && polls == 3);
    std::puts("PASS: static native and translated priority, no fast-path lookup, extension runtime options exactly once");
}
