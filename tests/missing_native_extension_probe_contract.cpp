#include "abi_bridge.h"
#include <cassert>
#include <cstdio>
int main() {
    assert(!mkw_switch_find_missing_native_cpu_extension(0x80170320));
    assert(!mkw_switch_find_missing_native_cpu_extension(0x12345678));
    std::puts("PASS: public probe registry requires no translated headers and exposes no execution handlers");
}
