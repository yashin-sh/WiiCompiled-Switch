#pragma once

#include <cstdint>

namespace mkw::runtime_bootstrap {

enum class StopPoint {
    Failed,
    WaitingForTranslatedProduct,
    TranslatedProductLinked,
    WaitingForDataInitializer,
    DataSectionsInitialized,
    DataSectionInitializationFailed,
};

struct Result {
    bool lifecycle_ready = false;
    bool filesystem_ready = false;
    bool timing_ready = false;
    bool input_ready = false;
    bool memory_ready = false;
    bool host_context_scheduler_ready = false;
    bool host_context_first_handoff = false;
    bool host_context_continuation = false;
    bool audio_stubbed = true;
    bool graphics_stubbed = true;

    bool translated_product_linked = false;
    bool translated_product_abi_compatible = false;
    std::uint32_t translated_product_reported_abi = 0;
    const char* translated_product_id = "<none>";
    const char* translated_product_build = "Nintendo-data-free stub";

    bool data_init_handoff_enabled = false;
    bool data_init_handoff_linked = false;
    bool data_init_handoff_abi_compatible = false;
    bool data_initializer_available = false;
    std::uint32_t data_init_handoff_reported_abi = 0;
    bool data_sections_init_attempted = false;
    bool data_sections_initialized = false;

    StopPoint stop_point = StopPoint::Failed;
};

Result start();
void stop() noexcept;
void print(const Result& result);
bool write_report(const Result& result);

} // namespace mkw::runtime_bootstrap
