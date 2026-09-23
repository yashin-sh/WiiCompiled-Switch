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

## 2026-09-23 TaskThread receive-slot attribution

A later priority-24 TaskThread run reaches this boundary with an empty sender
wait queue. The receive output slot is correct immediately before
`InvokeDirectCpu<0x801AAAA4>` and stack-shaped immediately after it.

Because `InvokeDirectCpu` runs the Switch time-driven VI poll before the
native OSWakeupThread body, and `OSReceiveMessage` still has guest interrupts
disabled at this point, the current correction is to suppress VI retrace
polling while interrupts are masked. No OSWakeupThread queue/scheduler
semantics are changed by that fix.
