# Captured second Mii I4 object in the next pass — 2026-10-08

The [first-frame capture run](HARDWARE_FIRST_FRAME_REPLAY_2026-10-08.md)
later stops at the guarded `GXLoadTexObj` boundary for object `0x80397FC0`,
slot 0, I4 32×64. Its complete descriptor matches the earlier source tuple:
physical data `0x109C1A40`, word 3 `0x0084E0D2`, linear filtering,
clamp/clamp, no mipmaps and disabled edge LOD.

The correction admits only that additional identity with the existing exact
descriptor and full 1,024-byte tiled range checks. The relocated source tuple
remains restricted to its previously captured first-pass pair. There is no
general object-address range or fallback texture acceptance.

## Attribution and behavior

Pinned WiiCompiled `a135beb201042b20f390c6695ca6b26768820fb4` maps the target
to `GX__LoadTexObj_80170f2c`, with object/map arguments from r3/r4. Its
native load validates the tiled range, initializes host texture/sampler state,
binds it, then publishes guest GX dirty/bp-sent state. The bounded Switch
bridge preserves its existing Init → LOD → UserData → Load sequence and
publishes guest bookkeeping only after successful native work.

The checked local caller loads `base + 352`, calls the draw helper, then
unconditionally loads `base + 416` at the captured LR. The current object
is the second identity at the next descriptor base. This supports an explicit
caller inference that the preceding first-object load and helper returned in
this run. It does not establish the payload address used by every earlier
occurrence, or prove an individually captured relocated-source return.

The native object cache remains keyed by guest object identity, so sharing the
same data does not merge the first and second objects. CPU registers, guest
descriptors and texture bytes remain unchanged. Null pointers and native
exceptions still stop before guest bookkeeping.

## Validation scope

The synthetic contract adds the new identity to repeated loads, independent
native-object checks, all 256 single-bit descriptor mutations, wrong map IDs,
neighboring-object rejection and native error cases. Missing/31-byte
descriptors and missing/512/992/1023-byte payload mappings must stop. Both
next-pass objects reject the unobserved relocated-source descriptor transplant.
All payload fixtures use fabricated patterns, without game data.

The targeted ASan/fatal UBSan/LSan contract passes in both modes: 42 loads,
3,804 headless refusals and 3,832 rendered refusals. The existing depth-LOD
contracts also pass. Public CI and the private rendered build must pass on this
candidate before deployment. The new second-object return and recognizable
game pixels remain hardware-pending. First-frame replay still covers only the
initial untextured quad; later-frame checkpoint support is unchanged.

Raw reports, captures, images, private products and product metadata stay local.
