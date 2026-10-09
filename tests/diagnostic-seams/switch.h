#pragma once
#include <cstdint>
using u8 = std::uint8_t;
using u64 = std::uint64_t;
struct DiagnosticRegister {
    std::uint64_t x = 0;
};
struct ThreadExceptionDump {
    std::uint32_t error_desc = 0, esr = 0;
    DiagnosticRegister pc, lr, sp, far, cpu_gprs[29];
};
std::uint64_t armGetSystemTick();
std::uint64_t armGetSystemTickFreq();
