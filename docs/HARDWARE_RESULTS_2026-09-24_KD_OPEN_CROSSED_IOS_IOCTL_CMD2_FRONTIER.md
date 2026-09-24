# Hardware result — KD open crossed / IOS_Ioctl cmd 2 frontier (2026-09-24)

Tracking: #117, #154, #162

## Hardware evidence

The rendered real-Switch run built after merged #232 validates the exact
`/dev/net/kd/request` open bridge:

```text
status=open-pass
path=/dev/net/kd/request
mode=0
fd=2000
```

The later durable blocker is:

```text
kind   = DIRECT
target = 0x80194290
r3     = 0x000007D0
r4     = 0x00000002
r5     = 0x80356F20
r6     = 0x00000020
stage  = HOST_CONTEXT_SWITCH_RETURNED
```

`0x7D0` is fd 2000, proving the opened KD handle is consumed by the next
guest IOS call.

The same run preserves the established invariants:

- FST remains structurally valid;
- `/Boot/Strap/eu/English.szs` remains read-pass;
- SZS decode remains pass at 2,627,200 output bytes;
- `GXInitTexObj` remains `init-pass`;
- real RMCP01 FIFO work is produced;
- `GXCopyDisp=1`;
- one surface present succeeds with zero failures;
- the liveness watchdog remains ACTIVE.

## Pinned mapping

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`
maps `0x80194290` exactly to `NAND_IOS_Ioctl_Entry_HLE`.

The live registers identify:

```text
fd     = 2000
cmd    = 2
inBuf  = 0x80356F20
inLen  = 0x20
outBuf = r7  (not captured yet)
outLen = r8  (not captured yet)
```

For `DeviceKind::KdRequest`, pinned command 2 is the NWC24
"try suspend scheduler" request. During the boot phase the pinned handler
writes `-42` to the output result word and returns IOS result 0.

## Why the next candidate is diagnostic-only

The current durable blocker does not record `r7/r8`, but those registers are
the output-buffer pointer and length consumed by the pinned command-2
semantics. Porting the command before hardware captures those live values would
fabricate part of the guest ABI.

The candidate therefore extends only the durable blocker record with
`r7/r8` and explicit IOS_Ioctl fd/cmd/in/out fields. It changes no IOS or
network behavior.

## Next hardware acceptance

The next run must capture the exact `outBuf/outLen` pair at
`IOS_Ioctl (0x80194290)`. Only then should the exact KD command-2 behavior be
ported.
