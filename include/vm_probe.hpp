#pragma once

#include <cstdint>

namespace mkw::vm_probe {

struct ProbeResult {
    bool aslr_info_ok = false;
    std::uint64_t aslr_base = 0;
    std::uint64_t aslr_size = 0;

    bool alias_info_ok = false;
    std::uint64_t alias_base = 0;
    std::uint64_t alias_size = 0;

    bool heap_info_ok = false;
    std::uint64_t heap_base = 0;
    std::uint64_t heap_size = 0;

    bool stack_info_ok = false;
    std::uint64_t stack_base = 0;
    std::uint64_t stack_size = 0;

    bool fixed_query_ok = false;
    bool fixed_4g_range_unmapped = false;
    bool fixed_base_in_alias = false;
    bool fixed_base_in_heap = false;
    bool fixed_base_in_stack = false;
    std::uint64_t fixed_region_base = 0;
    std::uint64_t fixed_region_size = 0;

    // Candidate for a runtime-selected 4 GiB guest window. The reservation is
    // kept alive while Path B maps a small SharedMemory object inside it.
    bool dynamic_4g_candidate_found = false;
    bool dynamic_4g_reserved = false;
    std::uint64_t dynamic_4g_base = 0;

    // Path A: validate the Horizon/libnx heap -> stack remap primitive.
    bool stack_alias_source_allocated = false;
    bool stack_alias_destination_found = false;
    std::uint32_t stack_alias_map_result = 0;
    bool stack_alias_initial_data_visible = false;
    std::uint32_t stack_alias_unmap_result = 0;
    bool stack_alias_writeback_verified = false;

    // Path B: map one SharedMemory object simultaneously into the runtime-
    // selected guest window and into an independent host VA.
    std::uint32_t shmem_create_result = 0;
    std::uint32_t shmem_guest_map_result = 0;
    std::uint32_t shmem_host_map_result = 0;
    std::uint64_t shmem_guest_base = 0;
    std::uint64_t shmem_host_base = 0;
    bool shmem_guest_to_host_visible = false;
    bool shmem_host_to_guest_visible = false;
    bool shmem_guest_query_ok = false;
    std::uint32_t shmem_guest_type = 0;
    std::uint32_t shmem_guest_perm = 0;
    bool shmem_host_query_ok = false;
    std::uint32_t shmem_host_type = 0;
    std::uint32_t shmem_host_perm = 0;

    // SharedMemory is expected not to support svcSetMemoryPermission. Recording
    // the result confirms the Switch backend must use checked accesses for
    // MMIO/deferred/executable guards rather than guest-view reprotection.
    std::uint32_t shmem_permission_read_result = 0;

    std::uint32_t shmem_guest_unmap_result = 0;
    std::uint32_t shmem_host_unmap_result = 0;
    std::uint32_t shmem_close_result = 0;
};

ProbeResult run();
void print(const ProbeResult& result);
bool write_report(const ProbeResult& result);

} // namespace mkw::vm_probe
