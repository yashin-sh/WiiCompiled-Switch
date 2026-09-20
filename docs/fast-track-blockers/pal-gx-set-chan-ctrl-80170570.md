# PAL GXSetChanCtrl — 0x80170570

Tracking: #117, #162

## Hardware blocker

The first hardware run after merged #203 crosses `GXSetChanMatColor` and
stops at:

```text
kind   : DIRECT
target : 0x80170570
r1     : 0x80399008
r3     : 0x00000004
stage  : RMCP01_GX_SET_CHAN_MAT_COLOR
```

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` maps this exact PAL entry point
to `GX__SetChanCtrl_80170570`.

## Pinned semantics

The native override consumes PPC `r3..r9` as:

```text
channel         = r3
enable          = r4
ambientSource   = r5
materialSource  = r6
lightMask       = r7
diffuseFn       = r8
attenuationFn   = r9
```

and forwards directly to Aurora:

```cpp
GXSetChanCtrl(
    (GXChannelID)channel,
    enable != 0,
    (GXColorSrc)ambientSource,
    (GXColorSrc)materialSource,
    lightMask,
    (GXDiffuseFn)diffuseFn,
    (GXAttnFn)attenuationFn);
```

Unlike the immediately preceding material-color boundary, pinned
`GXSetChanCtrl` does not perform a guest-memory read and does not call
`EnsureAuroraFrameActive()`.

The hardware blocker records `r3 = 4`. It does not record `r4..r9`, so
their concrete game values are not guessed.

## Switch implementation

The Switch bridge:

- reads `r3..r9` from `CpuContext`;
- records stage `RMCP01_GX_SET_CHAN_CTRL`;
- in the rendered fast-track, performs the same enum casts and
  `enable != 0` normalization as pinned WiiCompiled;
- calls Aurora `GXSetChanCtrl`;
- keeps the synthetic/headless path Nintendo-data-free and side-effect-free;
- exposes exact address `0x80170570` through `KnownNativeCpuCall`;
- adds a durable `GXSetChanCtrl hits` counter.

No neighboring texture, light-object, ambient-color, or draw boundary is
pre-ported.
