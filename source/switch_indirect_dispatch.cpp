#include "abi_bridge.h"
#include "switch_indirect_dispatch.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>

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

    return lower < first + count && table->entries[lower].address == address
        ? &table->entries[lower]
        : nullptr;
}

class NonvolatileFprGuard {
public:
    NonvolatileFprGuard(CpuContext* cpu, std::uint32_t mask) noexcept
        : cpu_(cpu), mask_(mask & kPpcAllNonvolatileFprMask) {
        if (!cpu_) {
            return;
        }
        for (std::uint32_t reg = 14u; reg <= 31u; ++reg) {
            if ((mask_ & (1u << reg)) != 0u) {
                saved_[reg - 14u] = cpu_->fpr[reg];
            }
        }
    }

    ~NonvolatileFprGuard() noexcept {
        if (!cpu_) {
            return;
        }
        for (std::uint32_t reg = 14u; reg <= 31u; ++reg) {
            if ((mask_ & (1u << reg)) != 0u) {
                cpu_->fpr[reg] = saved_[reg - 14u];
            }
        }
    }

    NonvolatileFprGuard(const NonvolatileFprGuard&) = delete;
    NonvolatileFprGuard& operator=(const NonvolatileFprGuard&) = delete;

private:
    CpuContext* cpu_ = nullptr;
    std::uint32_t mask_ = 0u;
    PPC_FPR saved_[18]{};
};

class NonvolatileGprGuard {
public:
    NonvolatileGprGuard(CpuContext* cpu, bool enabled) noexcept
        : cpu_(enabled ? cpu : nullptr) {
        if (!cpu_) {
            return;
        }
        for (std::uint32_t reg = 14u; reg <= 31u; ++reg) {
            saved_[reg - 14u] = cpu_->gpr[reg];
        }
    }

    ~NonvolatileGprGuard() noexcept {
        if (!cpu_) {
            return;
        }
        for (std::uint32_t reg = 14u; reg <= 31u; ++reg) {
            cpu_->gpr[reg] = saved_[reg - 14u];
        }
    }

    NonvolatileGprGuard(const NonvolatileGprGuard&) = delete;
    NonvolatileGprGuard& operator=(const NonvolatileGprGuard&) = delete;

private:
    CpuContext* cpu_ = nullptr;
    std::uint32_t saved_[18]{};
};
} // namespace

void RegisterStaticIndirectDispatchTable(const StaticIndirectDispatchTable* table) noexcept {
    g_staticIndirectDispatchTable.store(table, std::memory_order_release);
}

bool mkw_switch_try_dispatch_indirect(std::uint32_t target, CpuContext* cpu) {
    if (!cpu) {
        return false;
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
