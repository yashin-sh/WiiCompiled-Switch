# Hardware result: active post-retrace loop confirmed

Date: 2026-09-18
Tracking: #117, #4, #162

## Observation

A new real-Switch run remained black but continued far beyond the previous sustained-liveness sample. The final translated heartbeat recorded:

```text
dispatch count        : 126563
post-main dispatch    : 125958
last target           : 0x8020fcd4
guest pc              : 0x8024373c
r1                    : 0x90112608
r2                    : 0x8038efa0
r3                    : 0x0000365e
r13                   : 0x8038cc00
fast-track stage      : HOST_CONTEXT_SWITCH_ENTER
PAL main              : 0x8000b6b0
main reached          : YES
```

This is more than three times the dispatch count of the 2026-09-17 sustained run (37,148 total / 36,543 post-main).

## Exact RMCP01 attribution

RMCP01 symbols map:

- `0x8020FCD4` = `PostRetraceCallback`;
- `0x8024373C` = `EGG::Thread::start(void*)`.

The heartbeat records a dispatch target together with the caller/current guest PC, so this pair is consistent with translated execution running through a guest thread while the VI bridge invokes the registered post-retrace callback.

## Retrace proof

The Switch `VIWaitForRetrace` / `AdvanceRetrace` bridge publishes the new retrace count, then sets guest `r3 = retraceValue` immediately before invoking the registered pre/post-retrace callbacks.

Therefore the observed:

```text
r3 = 0x0000365e
```

means the callback was being invoked at retrace value **13,918**.

At approximately 60 Hz this corresponds to about **232 seconds (~3 min 52 s)** of accumulated retraces. The timing conversion is only a human-readable estimate; the important hardware fact is the monotonically advanced guest retrace value itself.

## Conclusion

The prolonged black-screen state is **ACTIVE translated/VI execution**, not a durable translated-thread stall at the captured sample.

The screen remains black for an expected reason: the Switch fast-track still replaces WiiCompiled's real GX FIFO/Aurora path with the temporary `GX_HLE_FIFO_Write*` sink.

This result closes the active-vs-stall diagnostic frontier and unblocks the isolated M3 graphics spike (#162):

```text
WiiCompiled HleFifoWrite
        ↓
Aurora GX
        ↓
Dawn/WebGPU
        ↓
Vulkan / Mesa / NVK
        ↓
Horizon/libnx presentation
```

The normal #117 fast-track should keep its sink until the isolated graphics spike proves a replacement path. No new HLE or scheduler behavior is justified by this heartbeat.

## Next engineering frontier

1. preserve this validated CPU/VI runtime path;
2. proceed with #162 as an isolated first-frame probe;
3. first prove clear-frame/triangle presentation on Switch;
4. then feed fabricated Nintendo-data-free FIFO traffic through pinned `HleFifoWrite`;
5. only after that connect the local RMCP01 translated GX stream;
6. keep #154 DVD/FST implementation gated on actual resource-loading evidence.
