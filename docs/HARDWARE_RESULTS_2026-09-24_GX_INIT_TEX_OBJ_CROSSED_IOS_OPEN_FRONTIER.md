# Hardware result — GXInitTexObj crossed / IOS_Open path diagnostic frontier (2026-09-24)

Tracking: #117, #154, #162

## Hardware result

The rendered RMCP01 run after the exact GXInitTexObj bridge progresses through
the previously established resource and graphics path:

- local FST remains published and structurally valid;
- `/Boot/Strap/eu/English.szs` reads successfully (299,969 bytes);
- pinned Yaz0/SZS decode succeeds (2,627,200 bytes output);
- the AsyncDisplay VI-idle recovery remains healthy;
- real RMCP01 FIFO work is produced;
- `GXCopyDisp` runs once and one GPU present succeeds with zero failures.

The new durable blocker is:

```text
kind   = DIRECT
target = 0x801938F8
r3     = 0x802A2160
r4     = 0
r5     = 0x803990A0
r6     = 0
stage  = HOST_CONTEXT_SWITCH_RETURNED
```

Pinned WiiCompiled maps `0x801938F8` exactly to
`NAND_IOS_Open_HLE(pathPtr, mode)`.

## Why this PR is diagnostic-only

The live path string at `r3=0x802A2160` is not present in the current durable
blocker record. Pinned `NAND_IOS_Open_HLE` has materially different behavior
for device paths such as `/dev/fs`, `/dev/es`, `/dev/sha`, network
devices and ordinary NAND file paths.

Porting a generic NAND/ISFS subsystem before hardware identifies this exact
path would violate the first-blocker policy.

## Candidate

Extend only the existing blocker diagnostic for target `0x801938F8` to record:

```text
ios open path : <guest C string from r3>
ios open mode : <r4>
```

No IOS, NAND, ISFS, network, filesystem, GX, scheduler or resource behavior is
changed.

## Next hardware acceptance

The next run should return the exact IOS path and mode. That evidence will
select the smallest matching pinned `NAND_IOS_Open_HLE` behavior and nothing
else.
