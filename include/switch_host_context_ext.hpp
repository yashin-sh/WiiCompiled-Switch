#pragma once

#include <host_context.h>

namespace HostContext {

// Switch backend extension used by the guest OSThread bridge. The runtime
// bootstrap already owns the scheduler HostContext while translated execution
// runs; guest scheduling adopts that active context instead of initializing a
// second scheduler on the same host thread.
Handle Current() noexcept;

} // namespace HostContext
