# PAL GXDrawDone — 0x8016EAB0

Tracking: #117, #162

## Hardware blocker

After #190 crosses PAL `OSSendMessage`, the next direct unsupported boundary is:

```text
kind   : DIRECT
target : 0x8016EAB0
r3     : 0x8042944C
stage  : HOST_CONTEXT_SWITCH_RETURNED
```

## Pinned attribution

At WiiCompiled pin
`a135beb201042b20f390c6695ca6b26768820fb4`, this address is
`GX__DrawDone_8016eab0`.

The pinned runtime clears the guest draw-done flag, drains Aurora GX via
`GXDrawDone()`, invokes the finish bookkeeping, sets bit `0x0008` in the
guest GX data status word, and publishes draw-done flag 1.

Aurora's `GXDrawDone()` drains `aurora::gx::fifo` and invokes its registered
host-side draw-done callback if present. It does not present the NWindow
surface; `GXCopyDisp` remains the explicit rendered fast-track present
boundary.

## Switch implementation

The common bridge mirrors the guest-visible draw-done bookkeeping. In the
rendered fast-track it additionally calls Aurora's real `GXDrawDone()`.
Headless and synthetic builds do not gain a renderer dependency.

Acceptance is hardware crossing this address and stopping at the following
exact boundary if another unsupported call appears.
