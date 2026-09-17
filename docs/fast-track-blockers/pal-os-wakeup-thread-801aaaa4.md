# PAL OSWakeupThread — 0x801AAAA4

Tracking: #117

Real Switch hardware reached this direct boundary after the merged `SCGetProductArea` bridge.

Pinned WiiCompiled semantics at `a135beb201042b20f390c6695ca6b26768820fb4`:

- input wait-queue pointer is guest `r3`;
- disable interrupts;
- drain the queue head-to-tail, bounded by a 256-entry safety limit;
- terminated/moribund threads are detached and skipped;
- live threads become READY;
- non-suspended threads are appended to the run queue for their clamped priority `0..31`;
- matching guest fibers are resumed;
- the scheduler pending bit and reschedule counter are set;
- when at least one runnable thread was woken, the normal path may immediately call `SelectThread(0)`;
- restore the previous interrupt state after the scheduling handoff returns.

Switch implementation policy:

- reuse the existing `switch_guest_fiber` HostContext continuation layer;
- reuse the hardware-validated PAL `SelectThread` bridge for immediate rescheduling;
- do not add timer, alarm, VI or other wakeup sources speculatively;
- do not alter adjacent scheduler APIs until hardware reaches them.
