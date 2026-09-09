#pragma once

namespace mkw::guest_flat_api_probe {

struct Result {
    bool initialized = false;
    bool runtime_base_available = false;
    bool checked_access_policy = false;
    bool host_pointer_available = false;
    bool mem1_alias_coherent = false;
    bool mem2_alias_coherent = false;
    bool owned_visible = false;
    bool teardown_ok = false;

    bool passed() const {
        return initialized && runtime_base_available && checked_access_policy &&
               host_pointer_available && mem1_alias_coherent &&
               mem2_alias_coherent && owned_visible && teardown_ok;
    }
};

Result run();
void print(const Result& result);
bool append_report(const Result& result);

} // namespace mkw::guest_flat_api_probe
