# Hardware result — GXSetChanCtrl frontier (2026-09-20)

Tracking: #117, #162

## Result

The first real-Switch run after merged #203 confirms that the corrected
`GXSetChanMatColor` bridge is hardware-crossed.

Durable post-main state:

```text
dispatch count         : 2415
post-main dispatch     : 1809
last handled target    : 0x80170474
TaskThread::run hits   : 1
GXSetNumChans hits     : 1
GXSetChanMatColor hits : 1
guest fiber current    : 0x80347498
OS current/running     : 0x80347498 / 0x80347498
FST address            : 0x97dc0000
FST size               : 0x0000fae0
FST structurally valid : YES
```

The renderer remains initialized with an active frame and the same nine
RMCP01 FIFO writes. There are still no display-list calls, no FIFO-produced
drawable work, no `GXCopyDisp`, and no successful or failed present.

## New first blocker

The next durable unsupported dispatch is:

```text
kind   : DIRECT
target : 0x80170570
r1     : 0x80399008
r3     : 0x00000004
stage  : RMCP01_GX_SET_CHAN_MAT_COLOR
action : abort after durable blocker record
```

At pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`, `0x80170570` is
`GXSetChanCtrl`.

Pinned semantics consume:

```text
r3 = channel
r4 = enable
r5 = ambient color source
r6 = material color source
r7 = light mask
r8 = diffuse function
r9 = attenuation function
```

and forward them as:

```cpp
GXSetChanCtrl(
    (GXChannelID)ch,
    en != 0,
    (GXColorSrc)as,
    (GXColorSrc)ms,
    lm,
    (GXDiffuseFn)df,
    (GXAttnFn)af);
```

Only `r3 = 4` is present in the durable blocker report. The concrete hardware
values of `r4..r9` are therefore intentionally not guessed.

## Next hardware acceptance

After the narrow `GXSetChanCtrl` bridge is merged:

1. `GXSetChanCtrl hits` must become non-zero;
2. `0x80170570` must no longer be the DIRECT blocker;
3. preserve the full existing GX hit chain including `GXSetChanMatColor`;
4. preserve TaskThread and default/main scheduler recovery;
5. capture the next exact GX/resource/DVD boundary;
6. separately watch for first display list, drawable FIFO work, `GXCopyDisp`,
   or successful present.

Do not pre-port the neighboring texture/light/draw boundaries before hardware
reaches them.
