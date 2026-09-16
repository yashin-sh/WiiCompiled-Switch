# Hardware result: SelectThread crossed to OSLoadContext

Date: 2026-09-16
Tracking: #117

## Hardware evidence

A real Switch fast-track run after the SelectThread bridge was merged crossed PAL `SelectThread` (`0x801A9C08`) and stopped at the next unsupported direct native boundary:

```text
kind    : DIRECT
target  : 0x801A1F58
pc      : 0x800060A4
r1      : 0x803990F8
r2      : 0x8038EFA0
r3      : 0x8042A680
r13     : 0x8038CC00
stage   : GUEST_POST_MAIN_ACTIVE
```

This proves the guest scheduler selected context/thread `0x8042A680`, published the switch far enough to leave `SelectThread`, and entered PAL `OSLoadContext` as the first unsupported boundary.

## Pinned WiiCompiled mapping

At pinned WiiCompiled commit `a135beb201042b20f390c6695ca6b26768820fb4`, `0x801A1F58` is the native `OSLoadContext` replacement used because the original SDK routine depends on privileged `mtspr`/`rfi` behavior.

The pinned HLE:

- reads the guest OSContext pointer from `r3` and aborts on null;
- validates saved `SRR0` at `context + 0x198` before mutating CPU state;
- restores GPR0..GPR31 from `context + 0x000..0x07C`;
- restores CR/LR/CTR/XER from `+0x080/+0x084/+0x088/+0x08C`;
- forces GQR0 to zero and restores GQR1..GQR7 from `+0x1A8..+0x1C0`;
- clears mode bit `0x0002` at `+0x1A2` when set;
- restores `SRR0`, `SRR1`, and `pc`;
- performs a dynamic indirect jump to restored `SRR0` as the host equivalent of `rfi`;
- treats a normal return from that restored target as an invalid context-switch path.

No desktop `GuestFiberManager` behavior is required for this non-fiber path.

## Switch fast-track contract

The Switch bridge mirrors that pinned non-fiber context restore and keeps the restored target inside the existing generated indirect-dispatch table. If the saved `SRR0` is not represented in the active translated product, the next durable blocker remains an `INDIRECT_JUMP_MISS` at the exact restored target instead of inventing execution.

Null context, null SRR0, guest-memory faults, and an unexpected restored-target return are also recorded durably before aborting.

## Acceptance

Rebuild the local fast-track NRO from the merged `main` and run it on real Switch hardware. `0x801A1F58` must no longer be the durable unsupported `DIRECT` dispatch. The next result should identify the restored `SRR0` path or the next attributable scheduler/runtime boundary.
