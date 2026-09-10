#pragma once

// WiiCompiled's generated data_sections_init.cpp includes "memory.h" and uses
// only Memory::Contains/GetPointer for its embedded-section memcpy path. Put
// the Horizon slice first in the include search path so generated code binds to
// the hardware-validated heap-backed Switch Memory implementation instead of
// pulling in the larger desktop runtime declaration.
#include "memory_switch_slice.hpp"
