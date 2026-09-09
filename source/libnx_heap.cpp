#include <cstddef>

// libnx normally sizes the homebrew heap to consume almost all memory that is
// available to the process. WiiCompiled needs a separate pool of Horizon
// SharedMemory objects for MEM1/MEM2 aliases, so leave substantial headroom
// instead of letting the default heap absorb it first.
//
// This target is intentionally full-memory/title-override only. 512 MiB gives
// newlib/malloc ample room while leaving enough process memory for Wii RAM
// backings, renderer allocations, and later runtime services.
extern "C" {
std::size_t __nx_heap_size = 0x20000000ULL; // 512 MiB, 2 MiB aligned.
}
