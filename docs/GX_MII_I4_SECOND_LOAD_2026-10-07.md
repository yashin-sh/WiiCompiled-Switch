# Second captured Mii I4 object — 2026-10-07

The [fresh run](HARDWARE_RESULTS_2026-10-07_MII_I4_SECOND_LOAD_FRONTIER.md)
accepts the first I4 load and reaches the second at object `0x80397DC0`, slot 0.
Its complete descriptor, dimensions, sampler and physical payload match the
already admitted object `0x80397D80`. Extend the exact object-identity guard to
those **two observed addresses only**. All eight captured words, slot 0,
I4 32×64, no mipmap and full **1,024-byte** data range remain required.

The two objects share physical MEM2 data `0x109C1A40`, but their host GXTexObj
instances stay distinct and are reused independently. Existing native init,
LOD, null user-data and binding run before guest bookkeeping. CPU, descriptor
and payload bytes remain unchanged. Unknown identities, descriptors, slots,
short ranges, null pointers and native exceptions retain diagnosed stops.
Existing RGB565, IA8 and headless behavior remain intact. No new API, dispatch
trait or native provider is added.

The independent fixtures encode four objects over three formats, with one
shared I4 data region. ASan/fatal UBSan/LSan contracts pass **16 valid loads per
mode**, **1,076 headless / 1,084 rendered diagnosed refusals**, including every
bit of every descriptor, wrong mapped object identities, full/short/missing
ranges, complete guest/CPU preservation and native-before-bookkeeping order.
Tests require all four host objects to be distinct, including the I4 pair.
No proprietary payload is copied into the fixtures.

All eight compiled mutations are rejected, including a widened object range
and collapse of the pair into one host object. The full synthetic build passes.
All 22 local suites, rendered SDK compilation, full synthetic retention and
lint pass on code `3256e81`. The immutable-image, network-disabled private
Rendered Discovery build passes; 71 required strong functions and 48 scoped
unique providers are verified across 235 host inputs, 19 Rust archives and
seven named libraries. The NRO contains **73,621,560 bytes**, SHA-256
`fe2a28d984164f24f0a45d8d70dd5ee399dec040c33a89ff04bed31c46ef79cd`.
Dependency pins and existing upstream patch bytes/ns mtimes are preserved.
Exact final-head CI, merge and deployment evidence are tracked in
[PR #321](https://github.com/yashin-sh/WiiCompiled-Switch/pull/321). Deployment
requires all five workflows / six jobs at the final head; subsequent Markdown
updates must leave every non-Markdown built input unchanged.
The [09:42 Netloader run](HARDWARE_RESULTS_2026-10-07_MII_RGB5A3_LOAD_FRONTIER.md)
now accepts second-object and intervening helper return before a distinct
RGB5A3 load. GPU completion and recognizable pixels still need fresh evidence. Private generated products, NROs and raw archives
remain local-only.
