# Hardware result: OSContext resume requires a preserved host continuation

Date: 2026-09-16
Tracking: #117

## Hardware evidence

A real Switch fast-track run from merged `main` after PR #145 stopped at:

```text
WiiCompiled-Switch unsupported translated dispatch
=================================================
kind                  : INDIRECT_JUMP_MISS
target                : 0x80238a78
guest pc              : 0x80238a78
r1                    : 0x803990f8
r2                    : 0x8038efa0
r3                    : 0x00000001
r13                   : 0x8038cc00
fast-track stage      : GUEST_POST_MAIN_ACTIVE
action                : abort after durable blocker record
```

This proves the `OSSleepThread -> SelectThread` handoff crossed on hardware. The selected guest `OSContext` then tried to resume at saved SRR0 `0x80238A78` through the non-fiber `OSLoadContext` fallback.

## Why this is not a missing translated function

At pinned WiiCompiled commit `a135beb201042b20f390c6695ca6b26768820fb4`, the PAL RMCP01 map places:

- `EGG::ProcessMeter::__ct` at `0x8023883C`;
- `EGG::ProcessMeter::measureBeginFrame` at `0x80238A94`.

Therefore `0x80238A78` lies inside `EGG::ProcessMeter::__ct`; it is not a function entry. Adding a synthetic `func_80238A78` translation root would misrepresent the PPC program and lose the already-live translated C++ call stack.

The blocker is instead a continuation-model boundary: an `OSContext` saved by the scheduler may resume at an instruction inside a translated function.

## Pinned WiiCompiled model

Pinned WiiCompiled maps guest `OSThread`s to cooperative host contexts through `GuestFiberManager` when that layer is active. `OSCreateThread` creates the target host context, `SelectThread` adopts the already-running main/scheduler context when necessary, and `SwitchToThread` swaps host stacks. The `OSLoadContext`/SRR0 redispatch path remains only a fallback for guest contexts without a host fiber.

That preserved native stack is what allows translated execution to continue after the original scheduler call rather than attempting to enter generated C++ at an arbitrary PPC interior address.

## Switch implementation boundary

The Switch port already has a hardware/bootstrap-validated AArch64 `HostContext` backend. This change adds only the guest-thread bookkeeping required to use it for the hardware-proven scheduling path:

- adopt the runtime bootstrap's active scheduler HostContext;
- create a 1 MiB host stack for a guest thread created through `OSCreateThread`;
- register the already-running main guest thread on the scheduler HostContext at the first real switch;
- preserve/restore the shared `CpuContext` around host-stack switches;
- mark the host continuation waiting/ready with the corresponding proven sleep/resume paths;
- retain the existing `OSLoadContext` fallback for contexts without a host continuation.

No interior translation is fabricated. No `OSWakeupThread`, timer, alarm, renderer, or unrelated thread-exit behavior is pre-implemented.

## Acceptance

Rebuild from merged `main` and run on real Switch hardware. The durable result must no longer be an `INDIRECT_JUMP_MISS` at `0x80238A78`. A successful crossing means the original translated host stack resumes directly and exposes the next real post-main boundary.
