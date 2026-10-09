# Texture descriptors, XFB presentation and frame sequences

This candidate addresses three sources of repeated console iteration. The
[console trial](HARDWARE_CAPTURE_CONTROL_2026-10-09.md) crosses the former Mii
guard and records paired black images with opaque final presentation. The
operator reports visible boot images/low FPS; those images are absent from the
retained PNGs. The revised capture/control still requires a console trial.

## Load descriptors rather than address identities

The Mii load bridge formerly accepted lists of specific object/source addresses
and dimensions. The replacement accepts a bounded I4/RGB5A3 descriptor family,
then proves the entire tiled source is mapped before host allocation or native
calls. Object and source relocation, legal dimensions and binding slots are
validated through the same path, including the previously refused Mii descriptor.

The object must be nonzero, four-byte aligned and completely readable. The eight
words must encode I4 or RGB5A3 consistently, linear min/mag filtering, clamp,
disabled edge LOD, zero LOD/bias/user data/TLUT, and no mipmaps. Slots 0–7 are
allowed. Dimensions decode to 1–1024. I4 uses 8×8 tiles and RGB5A3 4×4 tiles;
each tile occupies 32 bytes. The declared tile count/type/flags must match the
calculated extent. Counts beyond the SDK's 15-bit field are refused, rather than
accepting its wrapped count. The complete source range is checked independently.

This preserves the pinned runtime's descriptor layout and Aurora's native
Init/LOD/UserData/Load calls, with a stable host object for each guest identity
and refreshed current data on every load. CPU state, guest descriptors and source
bytes stay unchanged; the existing checked GX shadow publication remains.
The prior RGB565/IA8 admission and the init/LOD/wrap bridges retain their scopes.
Other sampler states, formats, palettes and mipmap behavior remain refusals.

Attribution: pinned `runtime/src/hle/gx/gx_texture.cpp` writes the SDK fields and
validates source extents; `aurora-main/lib/dolphin/gx/GXTexture.cpp` consumes the
native descriptor and emits eight-slot texture state. Synthetic tests exercise
the actual bridge in both modes, mapped/unmapped extents, partial tiles, invalid
fields/counts/slots/alignment, source updates and native/context preservation.

## Present the display copy, retain the EFB

The previous Switch renderer assigned its swapchain texture directly to the EFB
and presented after `GXCopyDisp`. A copy with clear could therefore display the
post-copy cleared EFB. The missing step is the final selected-XFB copy pass used
by pinned Aurora; a present counter alone did not expose that distinction.

The renderer now keeps an independent persistent EFB and samples
`current_present_source()` into the acquired surface after GX rendering. The
shared fullscreen pass follows pinned Aurora's copy shader: RGB is sampled with
linear/clamp filtering and display alpha is opaque. It does not blend away RGB
when the copied source has zero alpha. This establishes source selection and
copy behavior synthetically, not recognizable Switch game pixels.

The opt-in [image diagnostic](SWITCH_FRAME_DUMP.md) saves the display copy,
final surface and EFB after copy/optional clear, with separate file-frame labels.
A post-clear EFB image is explicitly not a pre-clear screenshot. The selected
display copy preserves the copied image. GPU waits remain bounded and nested
error scopes finish in reverse encode order. Diagnosed stops save completed CPU
snapshots without submitting a partial guest frame.

Real Vulkan tests check four RGBA/BGRA/sRGB formats, orientation, scaling, RGB
with zero source alpha, opaque output, self-sampling refusal and nested readbacks.
Aurora scene tests preserve the copied colored image while its EFB is cleared.
Direct I4 and RGB5A3 partial-tile textures additionally produce exact intensity
and red/blue pixels in independent producer/replay processes.

## Replay a completed prefix

The [sequence recorder](DESKTOP_GX_REPLAY.md) captures from exactly one `GXInit`
through successive completed presents, retaining GX/decoder/resource state.
Version 3 adds an intermediate frame boundary; version 2 stays readable.
The first frame is retained, while a latest prefix is refreshed periodically,
at a diagnosed stop and when capture fails. Unchanged memory ranges reuse their
last serialized snapshot; changed bytes remain ordered memory events. The current partially recorded frame is excluded.

The whole sequence remains bounded to 8 MiB and 64 resources. Unsupported
commands, resource growth/aliases, repeated initialization or budget exhaustion
disable capture with an explicit reason; an earlier labelled complete prefix
can remain. This is not an arbitrary mid-game state snapshot or Dolphin `.dff`.

An independent-process two-frame oracle retains slot/VAT/projection/sampler state,
updates texture data at the same address and checks changed pixels in frame two.
A partial third frame leaves the saved prefix unchanged. Portable format and real
SD-controller tests cover incomplete/failing frames and replacement rollback under
sanitizers. Raw game-derived captures, images and private products remain excluded.
