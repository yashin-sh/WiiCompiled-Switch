# Hardware result — OSSetPeriodicAlarm crossed / SoundPlayer::SetVolume frontier (2026-09-27)

Tracking: #117

## Real-Switch evidence

The supplied rendered fast-track run built after the merged
`OSSetPeriodicAlarm (0x801A08E0)` bridge moves durably beyond that OS alarm
boundary.

The new exact unsupported direct boundary is:

```text
kind             : DIRECT
target           : 0x800A35E0
guest pc         : 0x800060A4
r1               : 0x80399028
r2               : 0x8038EFA0
r3               : 0x90359920
r4               : 0x9088AF04
r5               : 0x0000000D
r6               : 0x9088AF0C
r7               : 0x00000000
r8               : 0x00000000
r13              : 0x8038CC00
fast-track stage : RMCP01_GX_FLUSH
```

Runtime invariants remain healthy through the blocker:

- 60,595 translated / 59,989 post-main dispatches;
- 2,100 StaticR dispatches;
- 1,261 RMCP01 FIFO writes;
- 85 GXCopyDisp calls;
- 85 successful GPU presents / 0 failures;
- FST structurally valid;
- renderer initialized and frame-active;
- English.szs and StaticR.rel reads preserved;
- SZS decode PASS.

The same run also advances into the real sound archive:

```text
/sound/revo_kart.brsar
  read 64 bytes at offset 0
  read 645856 bytes at offset 453408
  read 453344 bytes at offset 64
```

This proves `OSSetPeriodicAlarm (0x801A08E0)` is hardware-crossed.

## Pinned attribution

Pinned WiiCompiled
`patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`
maps PAL `0x800A35E0` to
`nw4r::snd::SoundPlayer::SetVolume`.

Its native override calls `MusicAttenuation::SetSoundPlayerVolume`. The
guest-visible base semantics are:

1. take the SoundPlayer pointer from PPC r3;
2. take the scalar float volume from PPC f1;
3. clamp it exactly as the original NW4R function:
   - values below 0 become 0;
   - values from 0 through 1 are preserved;
   - values above 1 become 1;
   - NaN follows the pinned unordered-compare path and becomes 1;
4. write the resulting float to `soundPlayer + 0x2C`.

Pinned WiiCompiled also layers optional desktop external-media attenuation over
that guest volume. That is host policy, not required guest state, and is not
ported here.

## Minimal Switch implementation

Add only `KnownNativeCpuCall<0x800A35E0>` with:

- PPC EABI float extraction from f1;
- the exact pinned clamp behavior;
- one guest-memory write at `SoundPlayer + 0x2C`.

Do not pre-port neighboring NW4R sound APIs, DSP mail, AX mix callbacks, audio
device output, or desktop media attenuation.

## Hardware acceptance

A later rendered real-Switch run must move durably beyond
`SoundPlayer::SetVolume (0x800A35E0)` while preserving scheduler, resource,
FIFO, GXCopyDisp and GPU-present invariants.
