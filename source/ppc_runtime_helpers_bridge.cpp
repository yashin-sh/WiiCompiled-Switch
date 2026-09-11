// Pull in WiiCompiled's generic PPC ISA helper implementation only for
// translated execution modes that can emit calls to PPC_* helpers. Keeping the
// source in the pinned submodule avoids carrying a divergent Switch copy.
#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK)
#include "../third_party/WiiCompiled/runtime/src/ppc_helpers.cpp"
#endif
