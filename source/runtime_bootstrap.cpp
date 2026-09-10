#include "runtime_bootstrap.hpp"

#include "horizon_runtime_services.hpp"
#include "memory_switch_slice.hpp"
#include "translated_product.hpp"
#include "translated_product_handoff.hpp"

#include <host_context.h>
#include <switch.h>

#include <cstdio>

namespace mkw::runtime_bootstrap {
namespace {

constexpr const char* kUpstreamCommit = "a135beb201042b20f390c6695ca6b26768820fb4";
constexpr std::size_t kWorkerStackSize = 256 * 1024;

#if defined(MKW_ENABLE_DATA_INIT_HANDOFF) && MKW_ENABLE_DATA_INIT_HANDOFF
constexpr bool kDataInitHandoffEnabled = true;
#else
constexpr bool kDataInitHandoffEnabled = false;
#endif

HostContext::Handle g_scheduler = nullptr;
HostContext::Handle g_worker = nullptr;
volatile unsigned g_worker_phase = 0;

const char* ready(bool value) {
    return value ? "READY" : "NOT READY";
}

const char* stubbed(bool value) {
    return value ? "STUBBED" : "UNEXPECTEDLY ACTIVE";
}

const char* enabled(bool value) {
    return value ? "ENABLED" : "DISABLED";
}

const char* product_state(const Result& result) {
    if (!result.translated_product_linked) {
        return "NOT LINKED";
    }
    return result.translated_product_abi_compatible ? "LINKED" : "ABI MISMATCH";
}

const char* handoff_state(const Result& result) {
    if (!result.data_init_handoff_linked) {
        return "NOT LINKED";
    }
    return result.data_init_handoff_abi_compatible ? "LINKED" : "ABI MISMATCH";
}

const char* initializer_state(const Result& result) {
    return result.data_initializer_available ? "AVAILABLE" : "NOT AVAILABLE";
}

const char* data_init_state(const Result& result) {
    if (!result.data_sections_init_attempted) {
        return "NOT ATTEMPTED";
    }
    return result.data_sections_initialized ? "PASS" : "FAIL";
}

const char* stop_point_name(StopPoint stop_point) {
    switch (stop_point) {
    case StopPoint::WaitingForTranslatedProduct:
        return "WAITING_FOR_TRANSLATED_PRODUCT";
    case StopPoint::TranslatedProductLinked:
        return "TRANSLATED_PRODUCT_LINKED";
    case StopPoint::WaitingForDataInitializer:
        return "WAITING_FOR_DATA_INITIALIZER";
    case StopPoint::DataSectionsInitialized:
        return "DATA_SECTIONS_INITIALIZED";
    case StopPoint::DataSectionInitializationFailed:
        return "DATA_SECTION_INITIALIZATION_FAILED";
    case StopPoint::Failed:
    default:
        return "FAILED_BEFORE_TRANSLATED_PRODUCT_BOUNDARY";
    }
}

void worker_entry(void*) {
    g_worker_phase = 1;
    HostContext::Switch(g_scheduler);

    g_worker_phase = 2;
    HostContext::Switch(g_scheduler);

    // The bootstrap owns this worker only to validate the real WiiCompiled
    // HostContext contract. It must never return through the raw trampoline.
    for (;;) {
        HostContext::Switch(g_scheduler);
    }
}

bool core_ready(const Result& result) {
    return result.lifecycle_ready &&
           result.filesystem_ready &&
           result.timing_ready &&
           result.input_ready &&
           result.memory_ready &&
           result.host_context_scheduler_ready &&
           result.host_context_first_handoff &&
           result.host_context_continuation;
}

void emit(FILE* out, const Result& result) {
    std::fprintf(out, "\nWiiCompiled Horizon runtime bootstrap\n");
    std::fprintf(out, "====================================\n");
    std::fprintf(out, "upstream pin           : %s\n", kUpstreamCommit);
    std::fprintf(out, "critical SDL path      : NONE\n");
    std::fprintf(out, "lifecycle              : %s\n", ready(result.lifecycle_ready));
    std::fprintf(out, "filesystem             : %s\n", ready(result.filesystem_ready));
    std::fprintf(out, "timing                 : %s\n", ready(result.timing_ready));
    std::fprintf(out, "libnx HID              : %s\n", ready(result.input_ready));
    std::fprintf(out, "Memory::Init           : %s\n", ready(result.memory_ready));
    std::fprintf(out, "HostContext scheduler  : %s\n", ready(result.host_context_scheduler_ready));
    std::fprintf(out, "HostContext handoff #1 : %s\n", ready(result.host_context_first_handoff));
    std::fprintf(out, "HostContext continuation: %s\n", ready(result.host_context_continuation));
    std::fprintf(out, "audio backend          : %s\n", stubbed(result.audio_stubbed));
    std::fprintf(out, "graphics backend       : %s\n", stubbed(result.graphics_stubbed));
    std::fprintf(out, "translated product     : %s\n", product_state(result));
    std::fprintf(out, "translated product ABI : expected=%u reported=%u\n",
                 translated_product::kAbiVersion,
                 result.translated_product_reported_abi);
    std::fprintf(out, "translated product id  : %s\n", result.translated_product_id);
    std::fprintf(out, "translated build       : %s\n", result.translated_product_build);
    std::fprintf(out, "data-init handoff      : %s\n", enabled(result.data_init_handoff_enabled));
    std::fprintf(out, "data-init provider     : %s\n", handoff_state(result));
    std::fprintf(out, "data-init handoff ABI  : expected=%u reported=%u\n",
                 translated_product_handoff::kAbiVersion,
                 result.data_init_handoff_reported_abi);
    std::fprintf(out, "data initializer       : %s\n", initializer_state(result));
    std::fprintf(out, "data sections init     : %s\n", data_init_state(result));
    std::fprintf(out, "runtime data root      : %s\n",
                 horizon_runtime_services::application_root().string().c_str());
    std::fprintf(out, "runtime logs root      : %s\n",
                 horizon_runtime_services::logs_root().string().c_str());
    std::fprintf(out, "runtime cache root     : %s\n",
                 horizon_runtime_services::cache_root().string().c_str());
    std::fprintf(out, "runtime config root    : %s\n",
                 horizon_runtime_services::config_root().string().c_str());
    std::fprintf(out, "runtime NAND root      : %s\n",
                 horizon_runtime_services::nand_root().string().c_str());
    std::fprintf(out, "stop point             : %s\n", stop_point_name(result.stop_point));
    std::fprintf(out, "hardware validation    : REQUIRED (attach runtime-bootstrap.txt)\n");
}

} // namespace

Result start() {
    Result result{};
    result.data_init_handoff_enabled = kDataInitHandoffEnabled;

    const auto services = horizon_runtime_services::initialize();
    result.lifecycle_ready = services.lifecycle_ready;
    result.filesystem_ready = services.filesystem_ready;
    result.timing_ready = services.timing_ready;
    result.input_ready = services.input_ready;
    result.audio_stubbed = services.audio == horizon_runtime_services::BackendState::Stubbed;
    result.graphics_stubbed = services.graphics == horizon_runtime_services::BackendState::Stubbed;

    if (!result.lifecycle_ready || !result.filesystem_ready ||
        !result.timing_ready || !result.input_ready) {
        return result;
    }

    Memory::Init(Memory::Config::WiiDefaults());
    result.memory_ready = Memory::IsInitialized();
    if (!result.memory_ready) {
        return result;
    }

    g_worker_phase = 0;
    result.host_context_scheduler_ready = HostContext::InitializeScheduler(&g_scheduler);
    if (!result.host_context_scheduler_ready) {
        Memory::Reset();
        return result;
    }

    g_worker = HostContext::Create(kWorkerStackSize, worker_entry, nullptr);
    if (!g_worker) {
        HostContext::ShutdownScheduler(g_scheduler);
        g_scheduler = nullptr;
        Memory::Reset();
        result.host_context_scheduler_ready = false;
        return result;
    }

    HostContext::Switch(g_worker);
    result.host_context_first_handoff = g_worker_phase == 1;
    if (result.host_context_first_handoff) {
        HostContext::Switch(g_worker);
        result.host_context_continuation = g_worker_phase == 2;
    }

    // The worker is suspended at a scheduler handoff and is no longer needed.
    HostContext::Destroy(g_worker);
    g_worker = nullptr;

    const auto product = translated_product::inspect();
    result.translated_product_linked = product.linked;
    result.translated_product_abi_compatible = product.abi_compatible;
    result.translated_product_reported_abi = product.reported_abi;
    result.translated_product_id = product.product_id;
    result.translated_product_build = product.build_description;

    const auto data_handoff = translated_product_handoff::inspect();
    result.data_init_handoff_linked = data_handoff.linked;
    result.data_init_handoff_abi_compatible = data_handoff.abi_compatible;
    result.data_initializer_available = data_handoff.data_initializer_available;
    result.data_init_handoff_reported_abi = data_handoff.reported_abi;

    if (core_ready(result)) {
        if (!result.translated_product_linked) {
            // Public builds deliberately stop here: translated game output is
            // produced and linked only by the user's local WiiCompiled build.
            result.stop_point = StopPoint::WaitingForTranslatedProduct;
        } else if (result.translated_product_abi_compatible) {
            result.stop_point = StopPoint::TranslatedProductLinked;

            if (result.data_init_handoff_enabled) {
                if (!result.data_init_handoff_linked ||
                    !result.data_init_handoff_abi_compatible ||
                    !result.data_initializer_available) {
                    result.stop_point = StopPoint::WaitingForDataInitializer;
                } else {
                    result.data_sections_init_attempted = true;
                    result.data_sections_initialized =
                        translated_product_handoff::run_data_initializer();
                    result.stop_point = result.data_sections_initialized
                        ? StopPoint::DataSectionsInitialized
                        : StopPoint::DataSectionInitializationFailed;
                }
            }
        }
    }

    return result;
}

void stop() noexcept {
    if (g_worker) {
        HostContext::Destroy(g_worker);
        g_worker = nullptr;
    }
    if (g_scheduler) {
        HostContext::ShutdownScheduler(g_scheduler);
        g_scheduler = nullptr;
    }
    if (Memory::IsInitialized()) {
        Memory::Reset();
    }
    g_worker_phase = 0;
}

void print(const Result& result) {
    emit(stdout, result);
    std::printf("\n");
    consoleUpdate(nullptr);
}

bool write_report(const Result& result) {
    const auto path = horizon_runtime_services::application_root() / "runtime-bootstrap.txt";
    std::FILE* out = std::fopen(path.string().c_str(), "w");
    if (!out) {
        return false;
    }
    emit(out, result);
    std::fclose(out);
    return true;
}

} // namespace mkw::runtime_bootstrap
