# Hardware result — KD open crossed / IOS_Ioctl cmd 2 frontier (2026-09-24)

Tracking: #117, #154, #162

## Hardware evidence

The rendered real-Switch run validates the exact merged KD open bridge:

```text
status=open-pass
path=/dev/net/kd/request
mode=0
fd=2000
```

The durable blocker is now captured with all six IOS_Ioctl arguments:

```text
kind   = DIRECT
target = 0x80194290
r3     = 0x000007D0
r4     = 0x00000002
r5     = 0x80356F20
r6     = 0x00000020
r7     = 0x80356F40
r8     = 0x00000020
stage  = HOST_CONTEXT_SWITCH_RETURNED
```

Therefore the live call is:

```text
fd     = 2000
cmd    = 2
inBuf  = 0x80356F20
inLen  = 0x20
outBuf = 0x80356F40
outLen = 0x20
```

The opened fd 2000 is consumed by the later IOS call, so the
`/dev/net/kd/request` open remains hardware-crossed.

## Preserved invariants

The same run preserves the established resource, scheduler and renderer path:

- FST published at `0x97DC0000`, 64,224 bytes / 2,096 entries;
- `/Boot/Strap/eu/English.szs` read-pass at 299,969 bytes;
- SZS decode-pass: 299,969 compressed bytes consumed / 2,627,200 produced;
- `GXInitTexObj` remains init-pass at 832x456, format 4;
- `TaskThread::run hits = 1`;
- `VIWaitForRetrace hits = 6`, `OSReceiveMessage hits = 4`,
  `OSSleepThread hits = 4`, `SelectThread hits = 13`;
- 29 RMCP01 FIFO writes with produced work;
- `GXCopyDisp calls = 1`;
- one successful surface present and zero failures;
- independent watchdog samples remain ACTIVE before the terminal blocker.

This proves the run reached the same established path before the new IOS
frontier. It does not prove visual Mario Kart pixel correctness.

## Pinned mapping and behavior

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`
maps `0x80194290` exactly to `NAND_IOS_Ioctl_Entry_HLE`.

For fd 2000, `DeviceKind::KdRequest` dispatches command 2 to
`HandleKdIoctl` as NWC24 "try suspend scheduler". In the initial
`KdTrySuspendPhase::Boot` phase the pinned behavior is exactly:

```cpp
g_kdBootProbeSeen = true;
WriteReturn(outBuf, outLen, -42);
return 0;
```

Pinned `WriteReturn` writes a 32-bit guest result only when `outBuf != 0`
and `outLen >= 4`. Hardware now proves `outBuf=0x80356F40` and
`outLen=0x20`, so the required write is fully attributable.

## Minimal candidate

The Switch bridge therefore implements only the first observed KD command-2
Boot probe:

1. require fd 2000 and command 2;
2. require the observed 0x20-byte input/output shape and mapped guest buffers;
3. reject a repeated command-2 invocation as a fresh unsupported frontier;
4. write WC24 result `-42` to the live output result word;
5. return IOS result `0` in r3.

It does not implement command 1, command 3, the post-resume command-2 phase,
ioctlv, close, sockets, DNS, NCD, IP, SSL, or any neighboring network service.

## Next hardware acceptance

After the candidate is merged with all five public CI gates green, the next
real-Switch run must:

1. preserve the KD `open-pass` and all established FST/DVD/SZS/scheduler/GX
   invariants;
2. execute the first command-2 bridge and durably progress beyond
   `0x80194290`;
3. expose the next exact blocker, native exception, or genuine liveness stall.

The command-2 bridge is not hardware-crossed merely because its status file is
written. Durable execution beyond the target or a later milestone is required.
