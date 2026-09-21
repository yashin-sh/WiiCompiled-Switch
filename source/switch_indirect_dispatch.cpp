#include "switch_indirect_dispatch.hpp"

#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <atomic>
#include <cstddef>
#include <cstdint>

extern "C" void mkw_switch_hle_task_thread_run(CpuContext* cpu);
extern "C" void mkw_switch_hle_egg_async_display_end_render(CpuContext* cpu) noexcept;

namespace {
std::atomic<const StaticIndirectDispatchTable*> g_staticIndirectDispatchTable{nullptr};

const RawDispatchRecord* FindStaticIndirectDispatchEntry(
    const StaticIndirectDispatchTable* table,
    std::uint32_t address) noexcept {
    if (!table || !table->segments || !table->entries || table->entryCount == 0u) {
        return nullptr;
    }

    const auto& segment = table->segments[address >> 24];
    const std::uint32_t pageNumber = (address >> 12) & 0x0FFFu;
    if (!segment.pages || pageNumber < segment.firstPage) {
        return nullptr;
    }

    const std::uint32_t relativePage = pageNumber - segment.firstPage;
    if (relativePage >= segment.pageCount) {
        return nullptr;
    }

    const auto& page = segment.pages[relativePage];
    const std::size_t first = page.firstEntry;
    const std::size_t count = page.entryCount;
    if (first > table->entryCount || count > table->entryCount - first) {
        return nullptr;
    }

    std::size_t lower = first;
    std::size_t upper = first + count;
    while (lower < upper) {
        const std::size_t middle = lower + (upper - lower) / 2u;
        const std::uint32_t candidate = table->entries[middle].address;
        if (candidate < address) {
            lower = middle + 1u;
        } else {
            upper = middle;
        }
    }

    if (lower < first + count && table->entries[lower].address == address) {
        return &table->entries[lower];
    }
    return nullptr;
}

struct NonvolatileFprGuard {
    NonvolatileFprGuard(CpuContext* cpu, std::uint32_t mask) noexcept
        : cpu(cpu), mask(mask & kPpcAllNonvolatileFprMask) {
        if (!cpu) {
            return;
        }
        for (std::uint32_t reg = 14u; reg <= 31u; ++reg) {
            if ((this->mask & (1u << reg)) != 0u) {
                saved[reg - 14u] = cpu->fpr[reg];
            }
        }
    }

    ~NonvolatileFprGuard() noexcept {
        if (!cpu) {
            return;
        }
        for (std::uint32_t reg = 14u; reg <= 31u; ++reg) {
            if ((mask & (1u << reg)) != 0u) {
                cpu->fpr[reg] = saved[reg - 14u];
            }
        }
    }

    CpuContext* cpu = nullptr;
    std::uint32_t mask = 0u;
    PPC_FPR saved[18]{};
};

struct NonvolatileGprGuard {
    NonvolatileGprGuard(CpuContext* cpu, bool enabled) noexcept
        : cpu(enabled ? cpu : nullptr) {
        if (!this->cpu) {
            return;
        }
        for (std::uint32_t reg = 14u; reg <= 31u; ++reg) {
            saved[reg - 14u] = this->cpu->gpr[reg];
        }
    }

    ~NonvolatileGprGuard() noexcept {
        if (!cpu) {
            return;
        }
        for (std::uint32_t reg = 14u; reg <= 31u; ++reg) {
            cpu->gpr[reg] = saved[reg - 14u];
        }
    }

    CpuContext* cpu = nullptr;
    std::uint32_t saved[18]{};
};
} // namespace

void RegisterStaticIndirectDispatchTable(const StaticIndirectDispatchTable* table) noexcept {
    g_staticIndirectDispatchTable.store(table, std::memory_order_release);
}

bool mkw_switch_try_dispatch_indirect(std::uint32_t target, CpuContext* cpu) {
    if (!cpu) {
        return false;
    }

    // PAL EGG::TaskThread::run is a pinned WiiCompiled native override reached
    // through Thread::start's virtual run() slot, so it never appears in the
    // generated translated indirect table. Resolve that hardware-proven native
    // boundary before consulting translated entries.
    if (target == 0x80242D7Cu) {
        mkw_switch_hle_task_thread_run(cpu);
        return true;
    }

    // PAL EGG::AsyncDisplay::endRender is another pinned WiiCompiled native
    // override that is reached through an indirect call and therefore is not
    // present in the translated table. Hardware first reached it only after
    // real RMCP01 FIFO render work was produced.
    if (target == 0x8020FF9Cu) {
        mkw_switch_hle_egg_async_display_end_render(cpu);
        return true;
    }

    const auto* table = g_staticIndirectDispatchTable.load(std::memory_order_acquire);
    const RawDispatchRecord* record = FindStaticIndirectDispatchEntry(table, target);
    if (!record || !record->entry) {
        return false;
    }

    NonvolatileGprGuard gprGuard(cpu, record->preserveNonvolatileGprs);
    NonvolatileFprGuard fprGuard(cpu, record->nonvolatileFprWriteMask);
    record->entry(cpu);
    return true;
}

#else

void RegisterStaticIndirectDispatchTable(const StaticIndirectDispatchTable*) noexcept {}

bool mkw_switch_try_dispatch_indirect(std::uint32_t, CpuContext*) {
    return false;
}

#endif
