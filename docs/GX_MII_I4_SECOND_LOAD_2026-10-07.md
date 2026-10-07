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
The rendered SDK compilation passes. All 22 local suites, the private rendered
build and exact-head CI are in progress.
The second-object return, GPU completion and recognizable pixels still need
fresh hardware evidence. Private generated products, NROs and raw archives
remain local-only.
