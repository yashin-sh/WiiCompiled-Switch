#include "horizon_guest_flat.hpp"

#include <switch.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace mkw::horizon_guest_flat {
namespace {

constexpr std::size_t kMaxRegions = 32;
constexpr std::size_t kMaxSections = 32;
constexpr std::size_t kGuardSize = 0x1000;

constexpr std::uint32_t kErrorInvalidInput = 0xFFFF0001u;
constexpr std::uint32_t kErrorTooManyRegions = 0xFFFF0002u;
constexpr std::uint32_t kErrorTooManySections = 0xFFFF0003u;
constexpr std::uint32_t kErrorNoGuestWindow = 0xFFFF0004u;
constexpr std::uint32_t kErrorGuestReservation = 0xFFFF0005u;
constexpr std::uint32_t kErrorHostAllocation = 0xFFFF0006u;
constexpr std::uint32_t kErrorAlreadyActive = 0xFFFF0008u;

struct Section {
    bool used = false;
    Backing backing = Backing::Owned;
    std::uint32_t owned_base = 0;
    std::uint64_t size = 0;
    std::uint8_t* host_view = nullptr;
};

struct RegionMap {
    std::uint32_t base = 0;
    std::uint64_t size = 0;
    std::uint64_t section_offset = 0;
    std::size_t section_index = 0;
};

std::array<Section, kMaxSections> g_sections{};
std::array<RegionMap, kMaxRegions> g_regions{};
std::size_t g_section_count = 0;
std::size_t g_region_count = 0;
std::uint8_t* g_guest_base = nullptr;
VirtmemReservation* g_guest_reservation = nullptr;
bool g_active = false;

std::uint64_t round_up_page(std::uint64_t value) {
    return (value + (kGuestPageSize - 1)) &
           ~(static_cast<std::uint64_t>(kGuestPageSize) - 1);
}

bool backing_offset(const RegionRequest& region, std::uint64_t& out_offset) {
    switch (region.backing) {
    case Backing::Owned:
        out_offset = 0;
        return true;
    case Backing::Mem1:
        out_offset = region.base & 0x1FFFFFFFu;
        return true;
    case Backing::Mem2: {
        const std::uint64_t folded = region.base & 0x1FFFFFFFu;
        if (folded < 0x10000000u) {
            return false;
        }
        out_offset = folded - 0x10000000u;
        return true;
    }
    }
    return false;
}

std::uint32_t section_owned_base(const RegionRequest& region) {
    return region.backing == Backing::Owned ? region.base : 0;
}

bool same_section_key(const Section& section, const RegionRequest& region) {
    return section.used &&
           section.backing == region.backing &&
           section.owned_base == section_owned_base(region);
}

std::size_t find_or_create_section(const RegionRequest& region, bool& ok) {
    for (std::size_t i = 0; i < g_section_count; ++i) {
        if (same_section_key(g_sections[i], region)) {
            ok = true;
            return i;
        }
    }

    if (g_section_count >= kMaxSections) {
        ok = false;
        return 0;
    }

    const std::size_t index = g_section_count++;
    auto& section = g_sections[index];
    section.used = true;
    section.backing = region.backing;
    section.owned_base = section_owned_base(region);
    ok = true;
    return index;
}

bool shutdown_internal() {
    for (std::size_t i = g_section_count; i > 0; --i) {
        auto& section = g_sections[i - 1];
        std::free(section.host_view);
        section.host_view = nullptr;
    }

    if (g_guest_reservation != nullptr) {
        virtmemLock();
        virtmemRemoveReservation(g_guest_reservation);
        virtmemUnlock();
    }

    g_sections = {};
    g_regions = {};
    g_section_count = 0;
    g_region_count = 0;
    g_guest_base = nullptr;
    g_guest_reservation = nullptr;
    g_active = false;
    return true;
}

void set_report(InitReport* report,
                bool window_found,
                bool reserved,
                std::uint32_t result,
                std::uint32_t failed_address = 0) {
    if (report == nullptr) {
        return;
    }
    report->guest_window_found = window_found;
    report->guest_window_reserved = reserved;
    report->guest_base = reinterpret_cast<std::uintptr_t>(g_guest_base);
    report->section_count = g_section_count;
    report->region_count = g_region_count;
    report->guest_mapping_count = 0;
    report->result = result;
    report->failed_guest_address = failed_address;
}

bool fail_init(InitReport* report, std::uint32_t result, std::uint32_t address = 0) {
    const bool found = g_guest_base != nullptr;
    const bool reserved = g_guest_reservation != nullptr;
    set_report(report, found, reserved, result, address);
    shutdown_internal();
    return false;
}

void emit_smoke(FILE* out, const SmokeResult& r) {
    std::fprintf(out, "\nHorizon GuestFlat checked heap-backed smoke test\n");
    std::fprintf(out, "===============================================\n");
    std::fprintf(out, "guest window found   : %s\n", r.init.guest_window_found ? "YES" : "NO");
    std::fprintf(out, "guest reservation    : %s\n", r.init.guest_window_reserved ? "OK" : "FAILED");
    std::fprintf(out, "guest base token     : 0x%016llx\n",
                 static_cast<unsigned long long>(r.init.guest_base));
    std::fprintf(out, "sections             : %zu\n", r.init.section_count);
    std::fprintf(out, "requested regions    : %zu\n", r.init.region_count);
    std::fprintf(out, "direct guest mappings: %zu (expected 0)\n", r.init.guest_mapping_count);
    std::fprintf(out, "init result          : 0x%08x\n", r.init.result);
    if (r.init.failed_guest_address != 0) {
        std::fprintf(out, "failed guest address : 0x%08x\n", r.init.failed_guest_address);
    }
    std::fprintf(out, "MEM1 phys -> cached  : %s\n", r.mem1_host_to_cached_guest ? "OK" : "FAILED/NOT RUN");
    std::fprintf(out, "MEM1 cached -> phys  : %s\n", r.mem1_uncached_guest_to_host ? "OK" : "FAILED/NOT RUN");
    std::fprintf(out, "MEM1 alias identity  : %s\n", r.mem1_host_alias_coherent ? "OK" : "FAILED/NOT RUN");
    std::fprintf(out, "MEM2 phys -> cached  : %s\n", r.mem2_host_to_cached_guest ? "OK" : "FAILED/NOT RUN");
    std::fprintf(out, "MEM2 cached -> phys  : %s\n", r.mem2_uncached_guest_to_host ? "OK" : "FAILED/NOT RUN");
    std::fprintf(out, "MEM2 alias identity  : %s\n", r.mem2_host_alias_coherent ? "OK" : "FAILED/NOT RUN");
    std::fprintf(out, "Owned checked view   : %s\n", r.owned_host_to_guest ? "OK" : "FAILED/NOT RUN");
    std::fprintf(out, "teardown             : %s\n", r.teardown_ok ? "OK" : "FAILED");
    std::fprintf(out, "GuestFlat smoke      : %s\n", r.passed() ? "PASS" : "FAIL");
}

} // namespace

bool initialize(const RegionRequest* regions, std::size_t count, InitReport* report) {
    if (report != nullptr) {
        *report = {};
    }
    if (g_active || g_guest_base != nullptr || g_guest_reservation != nullptr) {
        if (report != nullptr) {
            report->result = kErrorAlreadyActive;
        }
        return false;
    }
    if ((regions == nullptr && count != 0) || count > kMaxRegions) {
        if (report != nullptr) {
            report->result = count > kMaxRegions ? kErrorTooManyRegions : kErrorInvalidInput;
        }
        return false;
    }

    for (std::size_t i = 0; i < count; ++i) {
        const auto& request = regions[i];
        if (request.size == 0) {
            continue;
        }
        if (static_cast<std::uint64_t>(request.base) + request.size > kGuestSpaceSize) {
            return fail_init(report, kErrorInvalidInput, request.base);
        }

        std::uint64_t offset = 0;
        if (!backing_offset(request, offset) || offset + request.size > kGuestSpaceSize) {
            return fail_init(report, kErrorInvalidInput, request.base);
        }

        bool section_ok = false;
        const std::size_t section_index = find_or_create_section(request, section_ok);
        if (!section_ok) {
            return fail_init(report, kErrorTooManySections, request.base);
        }

        const std::uint64_t needed_size = round_up_page(offset + request.size);
        if (needed_size == 0 || needed_size > kGuestSpaceSize) {
            return fail_init(report, kErrorInvalidInput, request.base);
        }
        if (needed_size > g_sections[section_index].size) {
            g_sections[section_index].size = needed_size;
        }

        if (g_region_count >= kMaxRegions) {
            return fail_init(report, kErrorTooManyRegions, request.base);
        }
        g_regions[g_region_count++] = RegionMap{
            request.base, request.size, offset, section_index};
    }

    // Keep the same runtime guest-base contract as WiiCompiled, but reserve VA
    // only. The Switch checked path must not dereference this token directly.
    virtmemLock();
    void* candidate = virtmemFindAslr(static_cast<std::size_t>(kGuestSpaceSize), kGuardSize);
    if (candidate != nullptr) {
        g_guest_base = static_cast<std::uint8_t*>(candidate);
        g_guest_reservation = virtmemAddReservation(
            candidate, static_cast<std::size_t>(kGuestSpaceSize));
    }
    virtmemUnlock();

    if (g_guest_base == nullptr) {
        return fail_init(report, kErrorNoGuestWindow);
    }
    if (g_guest_reservation == nullptr) {
        return fail_init(report, kErrorGuestReservation);
    }

    // hbloader provides a mandatory heap override. Allocate Wii backing from
    // that existing heap instead of asking Horizon for additional large
    // SharedMemory objects, which hit Kernel LimitReached on hardware.
    for (std::size_t i = 0; i < g_section_count; ++i) {
        auto& section = g_sections[i];
        section.host_view = static_cast<std::uint8_t*>(
            std::calloc(1, static_cast<std::size_t>(section.size)));
        if (section.host_view == nullptr) {
            return fail_init(report, kErrorHostAllocation);
        }
    }

    g_active = true;
    set_report(report, true, true, 0);
    return true;
}

bool is_active() {
    return g_active;
}

std::uint8_t* guest_base() {
    return g_guest_base;
}

std::uint8_t* host_pointer(std::uint32_t guest_address) {
    if (!g_active) {
        return nullptr;
    }
    for (std::size_t i = 0; i < g_region_count; ++i) {
        const auto& region = g_regions[i];
        if (guest_address < region.base) {
            continue;
        }
        const std::uint64_t relative =
            static_cast<std::uint64_t>(guest_address) - region.base;
        if (relative >= region.size) {
            continue;
        }
        const auto& section = g_sections[region.section_index];
        if (section.host_view == nullptr) {
            return nullptr;
        }
        return section.host_view + region.section_offset + relative;
    }
    return nullptr;
}

void shutdown() {
    shutdown_internal();
}

SmokeResult run_smoke_test() {
    SmokeResult result{};

    constexpr RegionRequest regions[] = {
        {0x00000000u, 0x4000u, Backing::Mem1},
        {0x80000000u, 0x4000u, Backing::Mem1},
        {0xC0000000u, 0x4000u, Backing::Mem1},
        {0x10000000u, 0x4000u, Backing::Mem2},
        {0x90000000u, 0x4000u, Backing::Mem2},
        {0xD0000000u, 0x4000u, Backing::Mem2},
        {0x7E000000u, 0x4000u, Backing::Owned},
    };

    result.initialized = initialize(regions, std::size(regions), &result.init);
    if (!result.initialized) {
        result.teardown_ok = shutdown_internal();
        return result;
    }

    auto* mem1_phys = host_pointer(0x00000000u);
    auto* mem1_cached = host_pointer(0x80000000u);
    auto* mem1_uncached = host_pointer(0xC0000000u);
    if (mem1_phys && mem1_cached && mem1_uncached) {
        mem1_phys[0] = 0x11;
        result.mem1_host_to_cached_guest = mem1_cached[0] == 0x11;
        mem1_cached[1] = 0x22;
        result.mem1_uncached_guest_to_host = mem1_phys[1] == 0x22;
        mem1_uncached[2] = 0x33;
        result.mem1_host_alias_coherent =
            mem1_phys == mem1_cached &&
            mem1_phys == mem1_uncached &&
            mem1_phys[2] == 0x33;
    }

    auto* mem2_phys = host_pointer(0x10000000u);
    auto* mem2_cached = host_pointer(0x90000000u);
    auto* mem2_uncached = host_pointer(0xD0000000u);
    if (mem2_phys && mem2_cached && mem2_uncached) {
        mem2_phys[0] = 0x44;
        result.mem2_host_to_cached_guest = mem2_cached[0] == 0x44;
        mem2_cached[1] = 0x55;
        result.mem2_uncached_guest_to_host = mem2_phys[1] == 0x55;
        mem2_uncached[2] = 0x66;
        result.mem2_host_alias_coherent =
            mem2_phys == mem2_cached &&
            mem2_phys == mem2_uncached &&
            mem2_phys[2] == 0x66;
    }

    auto* owned = host_pointer(0x7E000000u);
    if (owned != nullptr) {
        owned[0] = 0x77;
        auto* checked = host_pointer(0x7E000000u);
        result.owned_host_to_guest = checked != nullptr && checked[0] == 0x77;
    }

    result.teardown_ok = shutdown_internal();
    return result;
}

void print_smoke_result(const SmokeResult& result) {
    std::printf("\n");
    emit_smoke(stdout, result);
    std::printf("\n");
    consoleUpdate(nullptr);
}

bool append_smoke_report(const SmokeResult& result) {
    FILE* out = std::fopen("sdmc:/switch/WiiCompiled-Switch/vm-probe.txt", "a");
    if (out == nullptr) {
        return false;
    }
    emit_smoke(out, result);
    std::fclose(out);
    return true;
}

} // namespace mkw::horizon_guest_flat
