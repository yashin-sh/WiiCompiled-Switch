// Narrow Switch bridge for integer PPC helpers that can remain reachable from
// fast-track HLE code without pulling WiiCompiled's desktop ppc_helpers.cpp.
//
// Keep this implementation pinned to WiiCompiled a135beb: cntlzw returns 32
// for zero and otherwise the number of leading zero bits.
#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK)

#include "isa/ppc_isa_int.h"

#include <cstdint>

extern "C" std::uint32_t PPC_Cntlzw(std::uint32_t value) {
    return PPC_CntlzwInline(value);
}

#endif
