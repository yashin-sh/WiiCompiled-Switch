#pragma once

#include <cstddef>

namespace mkw::memory_init_probe {

struct Result {
    bool initialized = false;
    bool region_count = false;
    bool runtime_guest_base = false;
    bool checked_access_policy = false;
    bool contains_policy = false;
    bool mem1_aliases = false;
    bool mem1_big_endian = false;
    bool mem2_aliases = false;
    bool mem2_big_endian = false;
    bool locked_cache = false;
    bool reset = false;
    std::size_t described_regions = 0;

    bool passed() const {
        return initialized && region_count && runtime_guest_base &&
               checked_access_policy && contains_policy &&
               mem1_aliases && mem1_big_endian &&
               mem2_aliases && mem2_big_endian &&
               locked_cache && reset;
    }
};

Result run();
void print(const Result& result);
bool append_report(const Result& result);

} // namespace mkw::memory_init_probe
