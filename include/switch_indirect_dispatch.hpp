#pragma once

#include <cstddef>
#include <cstdint>

struct CpuContext;

// Keep this generated-data contract byte-for-byte compatible with the pinned
// WiiCompiled emitter. base_dispatch/indirect_dispatch_*.cpp includes
// abi_bridge.h and initializes these records directly.
struct RawDispatchRecord {
    std::uint32_t address = 0;
    void (*entry)(CpuContext*) = nullptr;
    std::uint32_t nonvolatileFprWriteMask = 0xFFFFC000u;
    bool preserveNonvolatileGprs = false;
};

struct StaticIndirectDispatchPage {
    std::uint32_t firstEntry = 0;
    std::uint16_t entryCount = 0;
    std::uint16_t reserved = 0;
};

struct StaticIndirectDispatchSegment {
    const StaticIndirectDispatchPage* pages = nullptr;
    std::uint16_t firstPage = 0;
    std::uint16_t pageCount = 0;
};

struct StaticIndirectDispatchTable {
    const char* profileName = nullptr;
    const StaticIndirectDispatchSegment* segments = nullptr;
    const RawDispatchRecord* entries = nullptr;
    std::size_t entryCount = 0;
};

// Generated base_dispatch sources publish exactly one immutable table during
// static initialization. Registration is deliberately lightweight on Switch:
// no desktop registry, allocation, mutex, or Nintendo data is needed here.
void RegisterStaticIndirectDispatchTable(const StaticIndirectDispatchTable* table) noexcept;

class StaticIndirectDispatchTableRegistrar {
public:
    explicit StaticIndirectDispatchTableRegistrar(const StaticIndirectDispatchTable* table) noexcept {
        RegisterStaticIndirectDispatchTable(table);
    }
};

// Resolve and execute one translated target through the generated immutable
// dispatch table. Returns false when no published table contains the address so
// the caller can emit the normal durable fast-track blocker.
bool mkw_switch_try_dispatch_indirect(std::uint32_t target, CpuContext* cpu);
