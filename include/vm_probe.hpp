#pragma once

#include <cstdint>

namespace mkw::vm_probe {

struct ProbeResult {
    bool aslr_info_ok = false;
    std::uint64_t aslr_base = 0;
    std::uint64_t aslr_size = 0;

    bool fixed_query_ok = false;
    bool fixed_4g_range_unmapped = false;
    std::uint64_t fixed_region_base = 0;
    std::uint64_t fixed_region_size = 0;

    bool random_4g_candidate_found = false;
    std::uint64_t random_4g_base = 0;

    // Path A: mirror heap-backed memory into the Horizon stack region.
    bool stack_alias_source_allocated = false;
    bool stack_alias_destination_found = false;
    std::uint32_t stack_alias_map_result = 0;
    bool stack_alias_initial_data_visible = false;
    std::uint32_t stack_alias_unmap_result = 0;
    bool stack_alias_writeback_verified = false;

    // Path B: create kernel SharedMemory and map it twice, including one map
    // at WiiCompiled's desired fixed AArch64 guest base.
    std::uint32_t shmem_create_result = 0;
    std::uint32_t shmem_guest_map_result = 0;
    std::uint32_t shmem_host_map_result = 0;
    std::uint64_t shmem_host_base = 0;
    bool shmem_initial_data_visible = false;
    std::uint32_t shmem_permission_read_result = 0;
    std::uint32_t shmem_permission_rw_result = 0;
    bool shmem_writeback_verified = false;
    std::uint32_t shmem_guest_unmap_result = 0;
    std::uint32_t shmem_host_unmap_result = 0;
    std::uint32_t shmem_close_result = 0;
};

ProbeResult run();
void print(const ProbeResult& result);
bool write_report(const ProbeResult& result);

} // namespace mkw::vm_probe
