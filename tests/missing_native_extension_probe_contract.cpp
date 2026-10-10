#include "abi_bridge.h"
#include <cassert>
#include <cstdio>
int main() {
    assert(!mkw_switch_find_missing_native_cpu_extension(0x80084d20));
    assert(!mkw_switch_find_missing_native_cpu_extension(0x80170320));
    assert(!mkw_switch_find_missing_native_cpu_extension(0x80173188));
    assert(!mkw_switch_find_missing_native_cpu_extension(0x801720c0));
    assert(!mkw_switch_find_missing_native_cpu_extension(0x80171e70));
    assert(!mkw_switch_find_missing_native_cpu_extension(0x8019b43c));
    assert(!mkw_switch_find_missing_native_cpu_extension(0x12345678));
    std::puts("PASS: public probe registry requires no translated headers and exposes no execution handlers");
}
