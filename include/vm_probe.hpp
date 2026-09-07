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

    bool alias_source_allocated = false;
    bool alias_destination_found = false;
    std::uint32_t alias_map_result = 0;
    bool alias_initial_data_visible = false;

    std::uint32_t permission_read_result = 0;
    std::uint32_t permission_rw_result = 0;

    std::uint32_t alias_unmap_result = 0;
    bool alias_writeback_verified = false;
};

ProbeResult run();
void print(const ProbeResult& result);
bool write_report(const ProbeResult& result);

} // namespace mkw::vm_probe
