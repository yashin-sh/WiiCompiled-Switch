# Texture-copy configuration returned; GXCopyTex frontier — 2026-10-06

The [validated configuration bridges](GX_TEXTURE_COPY_CONFIG_2026-10-06.md)
return for all three observed tuples. The next DIRECT stop is **GXCopyTex
`0x8016FD74`**, destination **`0x9210A720`**, clear **1**, dispatch **633774 /
126.018 seconds**, LR `0x800C3394`, stack `0x80397B18`. This later run's
visual observation is pending; recognizable game pixels remain unproven.

## Candidate and evidence binding

The private Rendered Discovery NRO is **73,498,680 bytes**, SHA-256
`45d7b6eaec53a3d8df1cdf96832fd76a0d2a6c237fca8e92291e2f790f775e29`.
Validated code is `fec006ad1480865092c12868a1765a6a98df0ac5`; launch revision
`7ef008f8f3747babecf14f96236ff6e765742242` differs only in Markdown. The
candidate source hashes, pins and original upstream patch are checked before
transfer. Direct nxlink starts at **16:47:41 UTC** and exits **0 at 16:47:54
UTC**, sending 26,745,240 compressed bytes / 2,247 blocks (36.39%).

USB/MTP retrieval at **16:54:08 UTC** copies **36 reports / 630,085 bytes**.
Every size/hash, baseline comparison, raw ZIP member and ZIP CRC is verified
independently. Fourteen reports differ from the
[Fog baseline](HARDWARE_RESULTS_2026-10-06_MII_FOG_COPY_CLAMP_FRONTIER.md);
twenty-two are identical and may be retained files. Source timestamps, runtime
build ID, raw guest mirrors and GPU pixels are unavailable. Binding uses the
verified launch and fresh ordered caller/next-blocker progression. Private
game products, NROs, raw reports and archives remain excluded.

The earlier failed transfer and reported manual black-then-error launch are
separate attempts. Their visual result is not assigned to this later run.

## Executed configuration returns

Discovery records the coherent Mii texture caller `RFLiSetupCopyTex`
`0x800C2550` at dispatch **633750**, source format 5, dimensions 128×128,
destination `0x9210A720`, fiber `0x80347498`.

| Target | Dispatch | Captured arguments | Stage on entry |
| --- | --- | --- | --- |
| GXSetCopyClamp `0x8016F618` | 633758 | 3 | GX_SET_COPY_FILTER |
| GXSetTexCopySrc `0x8016F478` | 633765 | 0, 0, 128, 128 | GX_SET_COPY_CLAMP |
| GXSetTexCopyDst `0x8016F4DC` | 633766 | 128, 128, 5, 0 | GX_SET_TEX_COPY_SRC |
| GXCopyTex `0x8016FD74` | 633774 | destination `0x9210A720`, clear 1 | GX_SET_TEX_COPY_DST |

The ordered distinct calls, restored caller/fiber/stack and later durable
blocker establish each preceding bridge returned. Scope is these executed
tuples; alternate setters, raw guest GXData mirrors and native GPU state are
not individually captured. GXCopyTex has arrived but has not returned.

## Invariants and limits

The heartbeat at dispatch **633639**, before the configuration entries,
records **5,988 FIFO writes** (2450/0/372/3166), **102 successful presents / 0
failures**, zero replay calls, six TaskThread hits, valid FST and coherent
fiber/OS current/running identities `0x80347498`. The display-list report
passes End with 32 bytes in a 64-byte buffer at `0x921032C0`, saved context
enabled. These snapshots precede the new copy stop; they do not establish
configuration FIFO effects, copied texture pixels or recognizable images.

The watchdog has **120 samples: 118 ACTIVE and two recovered STALE**, maximum
interval **2,342 ms**, final ACTIVE. Elapsed time is not a performance comparison.

## Next implementation audit

Pinned `runtime/src/hle/gx/gx_copy.cpp` first ensures an active Aurora frame
and calls native GXDrawDone to drain prior draws. It reapplies the saved source
rectangle, calls native GXCopyTex with a guest-backed destination, records the
destination extent and restores the source rectangle. Aurora resolves the EFB
into a cached GPU texture and schedules its existing readback path; it does
not immediately populate large destination buffers with CPU pixels.

The Switch GuestToHostPtr helper requires an explicit nonzero length; forwarding
the pinned default-length call would yield null. The observed RGB5A3 128×128
destination needs complete 32,768-byte range validation before native effects.
Destination lifetime/cache invalidation also needs auditing: existing Switch
DC range maintenance still omits the pinned GX RAM tracker. A GXCopyTex bridge
must address that dependency rather than silently leave recycled GPU copies.
Actual texture copying, readback, later APIs and game pixels remain open.
