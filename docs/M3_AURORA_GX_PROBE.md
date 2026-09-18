# M3 Aurora GX triangle probe

Tracking: #4, #162

This Nintendo-data-free probe advances one layer above the hardware-validated Dawn/WGSL triangle.

## Path under test

```text
GXInit / GX state
       ↓
GXBegin(GX_TRIANGLES)
GXPosition3f32
GXColor4u8
       ↓
Aurora GX FIFO / command processor
       ↓
Aurora GX shader + graphics pipeline
       ↓
Dawn/WebGPU
       ↓
Vulkan / loaderless mesa-switch NVK
       ↓
VK_NN_vi_surface / NWindow
       ↓
physical Switch display
```

The Dawn/Vulkan/NVK/presentation portion below Aurora is already hardware-proven.

The probe compiles the Aurora GX/GFX implementation directly from the pinned WiiCompiled tree at
`a135beb201042b20f390c6695ca6b26768820fb4`. It does not use the desktop Aurora SDL application layer.

For this isolated first-Aurora-frame gate, the desktop asynchronous/persistent pipeline cache is replaced by a synchronous in-memory cache. This keeps actual Aurora GX shader/pipeline generation while avoiding SDL I/O and SQLite cache policy that are unrelated to proving the renderer on Horizon.

## Build

Initialize the pinned WiiCompiled submodule, then build:

```sh
git submodule update --init --recursive
MKW_JOBS=4 bash scripts/build-m3-aurora-gx-probe.sh
```

Output:

```text
m3-aurora-gx-probe/WiiCompiled-Switch-m3-aurora-gx-probe.nro
```

Copy it to:

```text
/switch/WiiCompiled-Switch-m3-aurora-gx-probe/
  WiiCompiled-Switch-m3-aurora-gx-probe.nro
```

Launch through hbmenu in application/title-override mode.

## Expected hardware result

A large RGB triangle should appear over a changing dark background.

Press `+` to exit.

Durable report:

```text
/switch/WiiCompiled-Switch/m3-aurora-gx-probe.txt
```

Key PASS boundaries:

```text
STAGE CREATE_INSTANCE PASS
STAGE CREATE_SURFACE PASS
STAGE REQUEST_ADAPTER PASS
STAGE REQUEST_DEVICE PASS
STAGE CONFIGURE_SURFACE PASS
STAGE AURORA_GFX_INIT PASS
STAGE GX_INIT PASS
PASS FIRST_AURORA_GX_TRIANGLE_PRESENT
...
STAGE TEARDOWN PASS
RESULT=PASS
```

A visible triangle plus `RESULT=PASS` proves the **Aurora GX frame** milestone. It does not yet prove the WiiCompiled `HleFifoWrite` decoder. The following gate is a fabricated Nintendo-data-free GX/FIFO byte stream fed through the pinned WiiCompiled decoder into this renderer.
