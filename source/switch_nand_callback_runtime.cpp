#include "switch_nand_runtime.hpp"

#include "devkita64_gcc_compat.hpp"
#include "abi_bridge.h"

#include <deque>
#include <mutex>

namespace mkw::switch_nand_runtime {
namespace {

struct PendingCallback {
    std::uint32_t callbackPtr = 0u;
    std::int32_t result = 0;
    std::uint32_t commandBlockPtr = 0u;
};

std::mutex g_callbackMutex;
std::deque<PendingCallback> g_pendingCallbacks;
thread_local int g_callbackDrainDepth = 0;

} // namespace

void QueueCallback(std::uint32_t callbackPtr,
                   std::int32_t result,
                   std::uint32_t commandBlockPtr) noexcept {
    if (callbackPtr == 0u) {
        return;
    }
    try {
        std::lock_guard<std::mutex> lock(g_callbackMutex);
        g_pendingCallbacks.push_back({callbackPtr, result, commandBlockPtr});
    } catch (...) {
    }
}

void PumpCallbacks(CpuContext* cpu) noexcept {
    if (!cpu || g_callbackDrainDepth != 0) {
        return;
    }

    ++g_callbackDrainDepth;
    for (int processed = 0; processed < 64; ++processed) {
        PendingCallback callback{};
        {
            std::lock_guard<std::mutex> lock(g_callbackMutex);
            if (g_pendingCallbacks.empty()) {
                break;
            }
            callback = g_pendingCallbacks.front();
            g_pendingCallbacks.pop_front();
        }

        CpuContext callbackCpu = *cpu;
        callbackCpu.gpr[3] = static_cast<std::uint32_t>(callback.result);
        callbackCpu.gpr[4] = callback.commandBlockPtr;
        CpuContextScope callbackScope(&callbackCpu);
        InvokeIndirectCpu(callback.callbackPtr, &callbackCpu);
    }
    --g_callbackDrainDepth;
}

} // namespace mkw::switch_nand_runtime

extern "C" void mkw_switch_pump_deferred_guest_callbacks(CpuContext* cpu) noexcept {
    mkw::switch_nand_runtime::PumpCallbacks(cpu);
}
