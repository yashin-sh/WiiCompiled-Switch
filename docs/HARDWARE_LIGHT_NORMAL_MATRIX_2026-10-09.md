# Light returned; normal-matrix diagnostic stop — 2026-10-09

The trial of light candidate `c2d862a` shows the Wiimote warning page, then a
crash according to the operator. The fresh terminal report identifies a
controlled diagnostic abort at `GXLoadNrmMtxImm`, PAL `0x80173188`, after 53.496
seconds. This is an unsupported direct call rather than an unexplained GPU
exception. The function receives a stack matrix pointer and matrix ID 0.

Fresh discovery evidence records `GXLoadLightObjImm` entry, followed by later
calls with the light stage, then the normal-matrix stop. This accepts the
observed light return. It does not establish light pixel correctness or support
for every light configuration on hardware. Both capture controllers report
`DISABLED` with fresh run identifiers; no old images or FIFO files are attributed
to this trial. Of 39 retrieved reports, 16 differ from the preceding run and
23 remain unchanged. The exception report is absent.

Presentation windows range from 0.747 to 8.742 Hz and average 3.370 Hz over
29.375 seconds. They include loading and stalls. There is no controlled speed
comparison, steady gameplay measurement or new pixel dump. The sampled snapshot
records 102 successful presents, but those counters do not prove visible content.

## Normal-matrix bridge

The missing-direct-call registry now routes `0x80173188` to a source-owned bridge,
without changing the shared ABI header or generated translations. The pinned
WiiCompiled HLE reads a full big-endian 3x4 matrix; Aurora writes only its
upper-left 3x3, omitting the translation column. The bridge checks the complete
48-byte mapped, nonwrapping range, converts all twelve words before native
output, and preserves CPU registers and guest memory.

Aurora accepts IDs 0 through 27 inclusively. The bridge covers that full range,
including intermediate row IDs; it does not whitelist an observed pointer or
ID. Invalid IDs, null/unmapped/short/wrapping ranges and uninitialized memory
stop before GPU output. Headless execution diagnoses the missing renderer.
Indexed normal matrices remain outside this change because they require vertex
array state and are a distinct input contract.

The public GX conversion suite executes the actual pinned `GXLoadNrmMtxImm`
body and compares its complete 41-byte XF packet against independent guest
bytes. It checks all accepted IDs, aligned and unaligned addresses, an object
ending exactly at the memory boundary, rotated IEEE special values, omission
of translation words, CPU/memory canaries and refusals before output. Both
registry handlers link into the light and normal contracts with their actual
native bodies. The public probe exposes neither handler.

The sanitizer contracts pass: 1,008 complete normal-matrix packets, the existing
864 light packets, headless/refusal cases and registry priority/probe checks.
Merging also requires the private rendered build, SDK syntax, scoped native
provider/retention checks and every published check on the exact PR HEAD to
succeed. No normal-matrix hardware return, later menu or playability is claimed.

Raw reports, generated game code, NROs and captures remain local.
