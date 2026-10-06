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
retention and script/workflow lint pass. Exact-code GitHub workflows and the
private rendered NRO are being validated. Dependency pins and the original upstream patch are
preserved; private game data, NROs and raw archives stay excluded.
