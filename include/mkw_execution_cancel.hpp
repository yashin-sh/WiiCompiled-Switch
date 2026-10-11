#pragma once

namespace mkw::execution {

// A cooperative owner-thread stop, distinct from a guest execution failure.
// It must reach the top of each guest stack so C++ guards release their state
// before that context is destroyed or the process returns to hbloader.
struct ExecutionCancelled final {};

// Implemented by the execution owner. Never called by the network worker.
void check_execution_cancel();

} // namespace mkw::execution
