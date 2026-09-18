# M3 pinned HleFifoWrite → Aurora GX probe

Tracking: #4, #162

This Nintendo-data-free probe advances one layer above the hardware-validated Aurora GX triangle.

## Path under test

```text
fabricated GX command bytes
        ↓
pinned WiiCompiled HleFifoWrite
        ↓
CP VCD/VAT decode
        ↓
raw GX draw decode
        ↓
Aurora GX FIFO / command processor
        ↓
Aurora GX shader + graphics pipeline
        ↓
Dawn / WebGPU
        ↓
Vulkan / loaderless NVK
        ↓
Switch display
```

The exact WiiCompiled source pin is:

```text
a135beb201042b20f390c6695ca6b26768820fb4
```

The lower Aurora/Dawn/NVK path already passed on hardware in
`HARDWARE_RESULTS_2026-09-18_M3_AURORA_GX.md`.

## Why this probe is strict

The successful Aurora GX probe configured the vertex descriptor and VAT through
ordinary GX API calls.

This probe deliberately does **not** call `GXSetVtxDesc` or
`GXSetVtxAttrFmt` for the triangle.

Instead, every frame it constructs a 69-byte FIFO sequence containing:

1. CP register `0x50` / VCD_LO:
   - `GX_VA_POS = GX_DIRECT`
   - `GX_VA_CLR0 = GX_DIRECT`
2. CP register `0x60` / VCD_HI = zero;
3. CP register `0x70` / VAT A for VTXFMT0:
   - position = XYZ/F32;
   - color0 = RGBA/RGBA8;
4. `GX_DRAW_TRIANGLES`, vertex count 3;
5. three big-endian position/color vertices.

Every byte is passed individually to the pinned `HleFifoWrite(value, 1)`.
This intentionally avoids the burst helper's direct CP fast path. The CP VCD/VAT
packets are decoded incrementally. Once the complete all-direct triangle packet
is buffered, the pinned implementation selects its own internal raw-direct draw
fast path, which submits the decoded packet to Aurora GX.

The probe validates after each packet that the pinned HLE state contains the
expected VCD/VAT state, the raw-direct draw is closed, no FIFO bytes remain
buffered, Aurora work was marked, and `vertsRemaining` matches the exact pinned
raw-direct fast-path post-state (`3` for this three-vertex packet).

## Build

```sh
git submodule update --init --recursive
MKW_JOBS=4 bash scripts/build-m3-hle-fifo-aurora-probe.sh
```

Output:

```text
m3-hle-fifo-probe/WiiCompiled-Switch-m3-hle-fifo-aurora-probe.nro
```

Copy it to:

```text
/switch/WiiCompiled-Switch-m3-hle-fifo-aurora-probe/
  WiiCompiled-Switch-m3-hle-fifo-aurora-probe.nro
```

Launch through hbmenu in application/title-override mode.

## Expected result

A large RGB triangle should appear over the changing dark background.

The durable report is:

```text
/switch/WiiCompiled-Switch/m3-hle-fifo-aurora-probe.txt
```

Key PASS boundaries:

```text
STAGE AURORA_GFX_INIT PASS
STAGE GX_FIXED_STATE PASS
STAGE HLE_FIFO_DECODER_RESET PASS
STAGE HLE_FIFO_STREAM begin bytes=69 mode=bytewise
STAGE HLE_FIFO_STREAM PASS ...
PASS FIRST_HLE_FIFO_AURORA_TRIANGLE_PRESENT
ACTIVE frames=...
RESULT=PASS
```

A visible triangle plus `STAGE HLE_FIFO_STREAM PASS` and `RESULT=PASS`
proves the **pinned WiiCompiled FIFO → Aurora GX frame** milestone.

It still does not connect any Nintendo-derived RMCP01 command stream. That
remains local-only and is the following graphics gate.
