# Save dialog and RGB565 texture frontier — 2026-10-11

The user photographs Mario Kart's corrupted-save dialog after the Wiimote safety
page. The game asks whether to delete the reported data and create replacement
data. This establishes a later visible UI state; it does not establish that the
physical save is corrupt, successful save recovery, menu progression or
playability. See the separate [blank-save correction and evidence limits](NAND_BLANK_SAVE_2026-10-11.md).
NAND status files are not reset at startup and can be stale; they do not establish
fresh GetLength success for this photographed run.

## Fresh terminal evidence

The LAN server remains available at the paused frontier. A second collection
verifies all 42 text reports, 598,547 bytes, against recorded size, SHA-256 and
CRC-32. Live status reports dispatch 827,547, frontier
`GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR@0x80170f2c`, and
`DEBUG_FRONTIER_PAUSED`. The terminal report records 87.854 seconds of translated
execution; the later status snapshot has server uptime 130.905 seconds. These
measurements describe different checkpoints and clocks.

The configured SD NRO has 73,711,672 bytes and SHA-256 identity
`1dbb618e70b356eb…`, associated with candidate `c33c10c`. The responding older
server does not expose an immutable launch-file hash or runtime session identity.
The installed file is verified, but exact attribution of the running process to
that file remains unestablished. Automatic relaunch is not accepted from this
observation.

The earlier report collection recorded an IA8 4×4 load-pass. The terminal
collection instead records a readable RGB565 object, matching fresh init and
LOD status. This changed terminal result is distinct from the earlier snapshot.
The capped post-video trace is unchanged between collections and supplies no
late-call trace. Existing SD PNG/frame dumps may be stale; the supplied photo is
the visual evidence for the dialog, with its original kept private.

## Descriptor and bounded correction

The refused texture is RGB565 (format 4), 32×32, slot 0, repeat on both axes,
with no mipmaps. The initializer reports 64 tiles of type 2; the LOD stage then
sets linear min/mag filtering, zero min/max/bias, no anisotropy or bias clamp,
and disabled edge LOD. The object is readable. The refusal happens at descriptor
admission, before the bridge checks the full texture backing.

Pinned WiiCompiled `a135beb201042b20f390c6695ca6b26768820fb4` packs RGB565 as
4×4 tiles of 32 bytes, so this descriptor requires exactly 2,048 bytes. The
existing audited linear family excludes format 4; its other fields match this
new observed form. The correction admits RGB565 only at **32×32**, with either
clamp/clamp or repeat/repeat. Object/source addresses remain identities and all
eight legal binding slots are forwarded. There is no address whitelist.

The bridge still requires the complete 32-byte object, matching format fields,
zero LOD/bias/user-data/TLUT fields, exact tile count/type/flags, no mipmaps,
and one complete readable 2,048-byte backing range before native submission.
Mixed wrap modes, mirror/reserved wraps, enabled edge LOD, neighboring dimensions,
bad fields and truncated backing retain diagnosed refusals. The original
832×456 legacy RGB565 path remains governed by its existing exact guard; this
correction does not expand that path or other RGB565 shapes.

## Candidate validation

The dedicated contract compares all eight packed words with the pinned guest
initializer and LOD writer, and compares size with the actual pinned native
`GXGetTexBufferSize`. Physical, cached and uncached source aliases, both admitted
samplers, all eight slots, stable host-object refresh and relocation are covered.
Refusal checks exercise neighboring fields/dimensions, wraps, map IDs, object
alignment and full backing bounds. The production bridge runs with actual
Memory checks and verifies unchanged guest CPU, payload and unrelated GX state
in both headless and rendered modes.

The full local texture/depth-LOD contract run passes under ASan, fatal UBSan and
LeakSanitizer. New RGB565 cases pass 66 admissions and 264 diagnosed refusals in
each mode. The complete texture suite passes 856 admissions / 5,583 diagnosed
refusals headless and 856 / 5,619 rendered; all four depth-LOD variants and the
pinned native depth-LOD fixtures also pass. An exact-source private
rendered build, complete successful exact-HEAD CI rollup, verified SD replacement
and attributable corrected RGB565 return remain separate pending gates. No
new native GPU pixel oracle or corrected hardware return is claimed by these
packing/argument contracts.

The descriptor helper is included directly by the texture bridge and its test;
this change does not modify the global PPC ABI/trait headers. The affected bridge
object and NRO must still be rebuilt, and any validation result must match the
new exact source rather than the preceding installed candidate.
