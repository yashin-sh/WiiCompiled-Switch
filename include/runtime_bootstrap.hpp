#pragma once

namespace mkw::runtime_bootstrap {

enum class StopPoint {
    Failed,
    WaitingForUserData,
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
    bool user_game_data_present = false;
    StopPoint stop_point = StopPoint::Failed;
};

Result start();
void stop() noexcept;
void print(const Result& result);
bool write_report(const Result& result);

} // namespace mkw::runtime_bootstrap
