# Bounded RGB5A3 EFB copy — 2026-10-06

The [verified configuration run](HARDWARE_RESULTS_2026-10-06_TEXTURE_COPY_CONFIG_COPY_TEX_FRONTIER.md)
reaches GXCopyTex `0x8016FD74`, destination `0x9210A720`, clear 1, after
Clamp(3), Src(0,0,128,128) and Dst(128,128,5,0) return. This candidate
implements that observed copy and its GPU-destination lifetime dependency.
Its console return and copied pixels remain unaccepted.

## Copy contract

Admit the complete observed source/destination tuple, clear word exactly 1
and a cached MEM2 destination. Validate all **32,768 bytes** before frame,
drain or GPU work. Other formats, dimensions, mipmaps, clear words and resource
aliases refuse durably. Null CPU contexts have no effects; headless execution
refuses before renderer or guest-memory work.

Follow pinned `gx_copy.cpp`: ensure an Aurora frame, drain prior draws with
native GXDrawDone, reapply the saved guest-space source rectangle, call real
Aurora GXCopyTex, remember the destination extent and restore the source
rectangle. The bridge additionally checks frame activation and revalidates
configuration after the draw-done callback. Native/resource exceptions refuse
instead of publishing a successful report. All caller CPU bytes are preserved.

Pinned Aurora GXCopyTex resolves the EFB into its GPU copy cache and invokes
the existing EFB RAM scheduling path. This 128×128 color copy exceeds its
automatic small-probe download threshold, so it remains GPU-only. The bridge
does not fill guest RAM with invented texture bytes or force a new CPU download.
Native command recording/return does not establish GPU completion or pixels.

`fast-track-gx-copy-tex.txt` records successful native return, destination,
checked extent and admitted shape. Transition caching avoids repeated SD opens
for the same destination. It does not individually capture GPU output.

## Destination lifetime

Track physical guest overlap, but retain the **exact host pointer** supplied to
Aurora. Switch aliases have different host virtual addresses; reconstructing a
physical-alias pointer would destroy the wrong cache key. An overlapping write
retires the remembered pointer with FIFO-ordered native GXDestroyCopyTex.
Adjacent and zero-length ranges do not retire a copy. Repeated copies at one
destination replace the entry; one notification can retire several destinations.
Release the bookkeeping mutex before native destruction so reentrant write
notifications cannot deadlock. Forget entries during renderer teardown.

The five already-supported DC invalidate/flush/store entry points now follow
pinned `os_cache.cpp`: 32-byte alignment in u64, overflow rejection, complete
mapped-range validation, then copy retirement. They preserve CPU and guest
bytes; invalid spans skip best-effort. Headless startup keeps its previous
no-op behavior. The existing rendered DVD/DCZeroRange DMA-write hook also
retires overlapping copies.

This supplies the EFB-copy component of the pinned GX RAM tracker. The broader
desktop texture/TLUT metadata and display-list cache machinery remain outside
this closure; this change does not claim their complete invalidation semantics.

## Validation

The contract executes production traits/bridge and the real Switch Memory
slice. Only allocation, native calls, renderer readiness and report IO are
seams. Guarded buffers test every complete unaligned 32 KiB window, all five
cache calls through cached/physical/uncached guest aliases, exact endpoints,
cache-line rounding, invalid/overflowing spans, multiple-copy retirement,
reentrance, CPU/canary preservation and SD-report deduplication. Refusals require
a verified report/abort proof over a pipe and real SIGABRT; an unrelated child
assertion cannot satisfy them.

A separate fixture executes the pinned GXGetTexBufferSize body verbatim, with
only its CHECK sink replaced, against independent RGB5A3 4×4-tile byte
expectations. It confirms the observed 32,768-byte extent. These fixtures
contain no game products or captured guest bytes.

The targeted contract passes **65,746 valid calls and 25 diagnosed refusals**
in rendered mode, plus two headless refusals. The pinned size fixture passes
**65,025 cases**. ASan, fatal UBSan and LeakSanitizer are active.

Nine independently compiled defects are rejected: missing draw drain, short
preflight, missing registration, wrong host alias, adjacent-copy eviction,
missing cache-line rounding, unmapped-span acceptance, CPU clobber and repeated
SD opens. All twenty local suites, both AArch64 modes, full synthetic NRO/ELF
retention and script/workflow lint pass. All five exact-code workflows / six jobs and the private Rendered Discovery
build pass at `c18f2577c7b8ab09eec702b275a6f56df641afee`.
The [actual build-switch run](https://github.com/yashin-sh/WiiCompiled-Switch/actions/runs/37506492103) confirms the rendered syntax and
65,746/25/2 contract counts plus 65,025 size fixtures. The
[fast-track run](https://github.com/yashin-sh/WiiCompiled-Switch/actions/runs/37506492220) confirms the real synthetic bridge/probe symbols.
The final private ELF retains **63 required strong functions** and canonical
`g_texCopyState`; **37 scoped symbols** have one expected provider each across
233 host inputs, 19 Rust archives and seven named libraries. Checked FIFO and
vertex mirrors match their preparation contract. Broader symbols and implicit
compiler-injected libraries remain outside this provider audit. Dependency pins and the original upstream patch are
preserved; private game data, NROs and raw archives stay excluded.


## Verified SD deployment

The NRO is **73,543,736 bytes**, SHA-256
`60b1b6649731195722fed9fd610ac7b34d489c93cc8387e9d55f36dccbbcb8c6`.
USB/MTP copies it to
`sdmc:/switch/WiiCompiled-Switch-gx-copy-tex-rendered-discovery.nro`
at **2026-10-06 18:18:20 UTC**. Complete readback matches every byte and hash.
The original nine-file upstream patch bytes and nanosecond modification times
are preserved. Deployment establishes file identity, not a console launch.
A later direct nxlink launch starts at **18:27:14 UTC** and exits **0 at
18:27:37 UTC**, sending **26,769,071 compressed bytes / 2,249 blocks (36.40%)**.
Launch revision `0e96ca8b13f18a03ae69b83e941609ff27a5149a` differs from the
validated code only in Markdown; candidate source hashes, dependency pins and
upstream patch bytes are rechecked before transfer. The exact NRO identity
above is unchanged. Fresh durable reports and this launch's visual observation
remain pending; transfer success does not establish native return, GPU
completion, copied pixels or later execution.
