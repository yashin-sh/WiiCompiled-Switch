#pragma once

// Nintendo-data-free syntax-only CI shim.
//
// Pinned gx_internal.h includes aurora_events.h, whose desktop implementation
// pulls SDL3 and runtime configuration code that are not part of the devkitA64
// public CI image. Rendered HLE bridge sources do not consume any of those
// event helpers directly; they only need gx_internal.h's GX declarations.
//
// Keep this shim intentionally empty so the compile gate still consumes the
// real pinned gx_internal.h and Aurora GX headers while avoiding unrelated
// desktop SDL dependencies. The private rendered build continues to use the
// real aurora_events.h.
