#include "abi_bridge.h"
#include "mkw_execution_cancel.hpp"
#include "memory.h"

#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <type_traits>

extern "C" void mkw_switch_hle_gx_set_tev_indirect(CpuContext*);

namespace {
constexpr std::uint32_t translated_target = 0x12345560u;
std::mutex guarded;
bool request_stop = false;
unsigned reports = 0, extension_lookups = 0, native_notes = 0;
unsigned vi_polls = 0;
std::uint32_t owner_fp_control = 0;
const char* last_stage = nullptr;
std::uint32_t reported_target = 0;
std::array<std::uint64_t, 18> initial_fprs{};

void TranslatedWithGuards(CpuContext* cpu) {
    std::lock_guard lock(guarded);
    cpu->fpr[14].raw = 0xdeadbeef00112233ull;
    cpu->fpr[31].raw = 0xaabbccddeeff0000ull;
    MkwApplyHostNiMode(4u);
    mkw_switch_report_unsupported_translated_dispatch("LOCKED_TRANSLATED", translated_target, cpu);
    assert(false); // A cancellation must not continue guest instructions.
}

void VerifyFprs(const CpuContext& cpu) {
    for (unsigned i = 14; i <= 31; ++i)
        assert(cpu.fpr[i].raw == initial_fprs[i - 14]);
}

void InvokeAndExpectCancellation(CpuContext& cpu, void (*invoke)(CpuContext*)) {
    assert(TryGetCpuContext() == nullptr);
    try {
        CpuContextScope scope(&cpu);
        invoke(&cpu);
        assert(false);
    } catch (const mkw::execution::ExecutionCancelled&) {
        assert(TryGetCpuContext() == nullptr);
        assert(MkwGetHostFpControl() == owner_fp_control);
        VerifyFprs(cpu);
        assert(guarded.try_lock());
        guarded.unlock();
    } catch (...) {
        assert(false); // Cancellation is distinct from a semantic guest fault.
    }
}
} // namespace

template <>
struct KnownTranslatedCpuCall<translated_target> {
    static constexpr bool kAvailable = true;
    static constexpr std::uint32_t kNonvolatileFprWriteMask = (1u << 14) | (1u << 31);
    static constexpr auto Entry = &TranslatedWithGuards;
};

namespace mkw::execution {
void check_execution_cancel() {
    if (request_stop)
        throw ExecutionCancelled{};
}
} // namespace mkw::execution

extern "C" void mkw_switch_note_translated_dispatch(std::uint32_t, CpuContext*) {
    mkw::execution::check_execution_cancel();
    ++native_notes;
}
extern "C" void mkw_switch_hle_vi_poll_retrace(CpuContext*) {
    ++vi_polls;
}
extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept {
    last_stage = stage;
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char*, std::uint32_t target, CpuContext*) {
    ++reports;
    reported_target = target;
    throw mkw::execution::ExecutionCancelled{};
}
extern "C" MkwSwitchNativeCpuExtension mkw_switch_find_missing_native_cpu_extension(std::uint32_t target) noexcept {
    ++extension_lookups;
    return target == 0x801717acu ? &mkw_switch_hle_gx_set_tev_indirect : nullptr;
}

bool Memory::IsInitialized() noexcept {
    return false;
}
bool Memory::Contains(std::uint32_t, std::size_t) {
    assert(false);
    return false;
}
std::uint8_t* Memory::GetPointer(std::uint32_t, std::size_t) {
    assert(false);
    return nullptr;
}

int main() {
    static_assert(!noexcept(KnownNativeCpuCall<0x80171f80u>::Invoke(nullptr)));
    static_assert(!std::is_nothrow_invocable_v<MkwSwitchNativeCpuExtension, CpuContext*>);
    static_assert(!noexcept(mkw_switch_report_unsupported_translated_dispatch("", 0, nullptr)));
    CpuContext cpu{};
    owner_fp_control = MkwGetHostFpControl();
    for (unsigned i = 14; i <= 31; ++i)
        cpu.fpr[i].raw = initial_fprs[i - 14] = 0x8877665544330000ull + i;
    InvokeAndExpectCancellation(cpu, &InvokeDirectCpu<translated_target>);
    assert(reports == 1 && reported_target == translated_target);
    assert(extension_lookups == 0);

    // These are the actual production bridge refusal paths, including the
    // extension callback ABI and its nested noreturn helper. No SDK setter or
    // guest memory is invoked by these invalid argument tuples.
    cpu.gpr[3] = 16;
    cpu.gpr[4] = 0;
    InvokeAndExpectCancellation(cpu, &InvokeDirectCpu<0x80171f80u>);
    assert(reports == 2 && reported_target == 0x80171f80u);
    assert(last_stage && std::strcmp(last_stage, "RMCP01_GX_SET_TEV_K_ALPHA_SEL") == 0);
    assert(extension_lookups == 0);
    cpu.gpr[3] = 0;
    cpu.gpr[5] = 0;
    cpu.gpr[6] = 0;
    cpu.gpr[7] = 4; // Reserved matrix selector refuses before stack reads.
    InvokeAndExpectCancellation(cpu, &InvokeDirectCpu<0x801717acu>);
    assert(reports == 3 && reported_target == 0x801717acu);
    assert(extension_lookups == 1);
    assert(vi_polls == native_notes && vi_polls == 3);

    const auto reports_before = reports, notes_before = native_notes, polls_before = vi_polls;
    request_stop = true;
    InvokeAndExpectCancellation(cpu, &InvokeDirectCpu<translated_target>);
    assert(reports == reports_before && native_notes == notes_before);
    assert(vi_polls == polls_before);
    std::puts("PASS: typed cancellation crosses actual native/extension refusal paths and releases mutex/FPR/CPU guards; pending stop prevents guest dispatch");
}
