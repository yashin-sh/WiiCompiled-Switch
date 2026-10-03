# Depth-texture LOD validation fix (2026-10-03)

The [accepted Fog/ZCompLoc run](HARDWARE_RESULTS_2026-10-03_FOG_Z_COMP_DEPTH_LOD_FRONTIER.md)
created a 4×4 depth texture successfully, then intentionally aborted in
`GXInitTexObjLOD` `0x80170A4C`. Its full format **22 / `GX_TF_Z24X8`** was
missing from `GetTexObjBlockLayout`, even though guest init and the pinned
native enum recognize it. The descriptor is readable and its remaining
structural fields are consistent; the diagnostic label does not prove corrupt
texture data.

## Change and scope

The structural layout table now recognizes **full format 22** alongside
RGBA8: 4×4 tiles, block type 3. This matches the pinned guest metadata's
low-nibble layout and native Z24X8 format/buffer-size definitions. No generic
low-nibble masking is introduced: unknown full formats remain refused.
Dimensions, block counts/types, wrap fields, flags, object readability and LOD
argument guards retain their existing checks. No change to texture loading,
decoding, memory ownership or the native renderer is included.

For the captured object `0x80384170`, data `0x802A2B60`, wrap 1/1, no mipmaps,
nearest filters and zero LOD/bias/bool/aniso arguments, existing forwarding
uses the cached native object created by GXInitTexObj. The existing guest
writeback changes only word0 **`0x00000095 → 0x00000105`**; word1 stays zero
and words2..7 stay unchanged. CPU state and texture bytes are unchanged.
Native LOD runs before guest writeback. It updates mode words and does not
resolve or decode the texture payload.

The actual private Rendered Discovery target leaves the optional
`MKW_STRICT_GX_TEXTURE_OBSERVED_TUPLES` undefined, hence false. This fix does
not extend the legacy strict allowlist; enabling that option still diagnoses
`GX_INIT_TEX_OBJ_LOD_UNPROVEN_TUPLE` for the new object. Both configurations
are explicitly tested.

## Executable validation

`bash scripts/test-gx-texture-load.sh` retains the preceding RGB565/IA8 load
contracts and invokes `scripts/test-gx-depth-lod.sh`. Existing CI therefore
executes the new contracts through its current texture gate.

The new contract compiles the real texture bridge and Switch Memory
implementation with allocation-only host seams, ASan, fatal UBSan and active
LeakSanitizer. It runs rendered modes 0/1 with strict modes 0/1:

- 66 positive calls in each non-strict rendered mode: the exact captured
  descriptor, repeated cached-object use and a synthetic structural layout
  matrix at tile/dimension boundaries from 1 to 1024. The matrix exercises
  validation, not native image creation at every size.
- Diagnosed real SIGABRT refusals: 19 in headless/non-strict, 21 in
  rendered/non-strict, and 20 in each strict mode. These cover unreadable
  objects, corrupt wrap/format/address/block/flag fields, invalid LOD arguments,
  optional strict rejection and rendered missing-host/native-exception paths.
- Complete CPU and region snapshots verify positive writeback and no guest
  writes on refusal. Rendered sinks verify exact arguments, cached-host
  identity, native-before-guest ordering and no texture data read by LOD.
- A separate fixture executes verbatim pinned Aurora `GXInitTexObjLOD`, its
  real register macro and filter table with mode-word storage replaced. It
  checks the exact `0x95 → 0x105` result and 1024 unrelated-bit preservation
  cases. This is native host evidence, not console graphics acceptance.
- Four compiled temporary mutations are rejected: removing format 22,
  assigning the wrong block type, masking away full-format distinctions and
  inverting the edge-LOD bit.

Expected-abort children disable Linux dump collection with PR_SET_DUMPABLE=0,
retaining real SIGABRT checks and parent LeakSanitizer. The preceding texture
load contract now uses the same mechanism; Linux core-pipe handlers can ignore
its existing RLIMIT_CORE=0. No sanitizer is disabled.

Local texture/LOD contracts, native fixture, four mutation rejections, rendered
AArch64 syntax, C++ formatting and script lint pass. The private Rendered
Discovery build of code `b5f0a2b0d50266e7538f76cd3438306c3a6293e9` passed at
**2026-10-03 19:49:29 UTC**. Only the corrected bridge, final ELF link and NRO
packaging ran (three build tasks, about 30 seconds including configuration).
The 73,396,280-byte NRO has SHA-256
`596ba38a52d588b61eb1edff241d3b6969c8a2c459e70d849a3236f3ed02551a`.
All **35 required strong text symbols** are retained. A fresh scan of 225
explicit host inputs, 19 Rust archives and seven named image libraries found
one Aurora GXTexture.o provider each for GXInitTexObj and GXInitTexObjLOD;
the bridge defines its own entry points and references those native functions.
This scoped check does not prove whole-link duplicate ownership. Dependency
pins, the user's integration patch bytes/nine mtimes and the preceding Fog NRO
are preserved. All **five workflows / six jobs** pass on exact code
`b5f0a2b0d50266e7538f76cd3438306c3a6293e9`. Actual CI logs confirm the old
texture-load modes, all four depth-LOD configurations/counts and the pinned
native fixture. The first nxlink attempt at 20:05:21 UTC could not connect (exit 1).
The later retry transferred this exact NRO with exit 0 at **20:49:11 UTC**.
Fresh verified reports now accept the observed LOD return: `lod-pass`,
word0 `0x105`, word1 zero and coherent later GXBeginDisplayList arrival.
See [the accepted hardware result](HARDWARE_RESULTS_2026-10-03_DEPTH_LOD_DISPLAY_LIST_FRONTIER.md)
for scope, attribution and limits. The user confirms black output followed by
an error; no recognizable image is established.

## Checked continuation and limits

The checked local caller continues from LOD through inline matrix setup to a
constructor that prepares GX display lists. Static inspection identifies
`GXBeginDisplayList` `0x80172E00`, followed later by `GXEndDisplayList`
`0x80172EB4`; these lack local KnownNative bridges. The later test now
establishes Begin arrival as the new DIRECT boundary;
End remains a static forecast and neither recording call has returned. Their
stateful recording effects require their own audit: pinned Aurora redirects
FIFO writes to a bounded caller buffer, optionally saves/restores GX shadow
state and returns the 32-byte-rounded written length. A no-op would lose
those effects. No generated game code is published, and no display-list
behavior is bypassed here.

Recognizable pixels, depth texture upload/decoding, sustained scenes,
input/audio correctness and representative performance remain unproven.
