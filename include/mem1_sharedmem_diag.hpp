#pragma once

namespace mkw::mem1_sharedmem_diag {

// Reproduce the full-size 24 MiB MEM1 SharedMemory path one primitive at a time.
// Every step is persisted to vm-probe.txt before the potentially failing call.
void run();

} // namespace mkw::mem1_sharedmem_diag
