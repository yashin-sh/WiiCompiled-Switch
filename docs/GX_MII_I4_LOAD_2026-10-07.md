# Observed Mii I4 texture load — 2026-10-07

The [new console frontier](HARDWARE_RESULTS_2026-10-07_VIEWPORT_MII_I4_LOAD_FRONTIER.md)
returns through viewport/depth setup, then refuses GXLoadTexObj `0x80170F2C`
for object `0x80397D80`, slot 0. Extend the existing load bridge for only its
captured eight-word descriptor:
`00000190 00000000 0000FC1F 0084E0D2 00000000 00000000 00000000 00200102`.

It decodes to I4, 32×64, clamp/clamp, no mipmap and physical MEM2 data
`0x109C1A40`. Pinned Aurora's I4 buffer calculation uses 8×8 tiles, 32 bytes
per tile, giving exactly **1,024 bytes**. Check the complete descriptor and
full mapped data range before resolving a host pointer or calling native code.
Existing address canonicalization already supports physical MEM2.

The admitted tuple reuses native GXInitTexObj, linear/linear zero-range LOD,
edge LOD disabled by the captured bit, null user data and GXLoadTexObj slot 0.
Guest CPU bytes, descriptor and payload stay unchanged. The existing guest
GXData dirty flag and BP-sent halfword are published after the native sequence.
A rejected descriptor, missing/short range, null pointer or host exception
stops durably before that bookkeeping. Existing RGB565 and IA8 contracts and
headless behavior remain intact. No new dispatch API or native provider is added.

## Validation

Independent field encoders construct all three descriptors from shape, format,
tile counts and sampler bits; synthetic payloads contain no game texture data.
Tests check **14 valid loads in each mode**, **804 diagnosed headless refusals**
and **810 rendered refusals**, with complete CPU and guest-region snapshots,
correct native arguments/ordering, distinct reused host objects and full range
resolution. Every bit in every descriptor word is varied, along with slot IDs,
matching descriptors at wrong object addresses, missing/short objects and data,
null host pointers and native exceptions. ASan, fatal UBSan and LeakSanitizer
remain active outside sandbox tracing. Shared depth-LOD regressions also pass.

All six compiled mutations are rejected: widened slot, missing word 7 guard,
short range, wrong edge LOD, early bookkeeping and CPU clobber. The complete
synthetic build passes. All 22 local suites, SDK compilation, the private
rendered NRO and exact-head CI are being completed in
[PR #320](https://github.com/yashin-sh/WiiCompiled-Switch/pull/320).
The new load remains hardware-unaccepted until a fresh attributable run returns
through it. Upload, GPU completion and recognizable texture pixels remain open.
Private NROs, generated products and raw reports are excluded from GitHub.
