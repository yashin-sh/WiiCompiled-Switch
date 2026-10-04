# Hardware result — Begin/End returned, sphere frontier (2026-10-04)

The [SU-state correction](GX_SU_STATE_2026-10-04.md), runtime code
`4a7e48b5214f6abd0e15b5b75d62572ecec6bc8d`, built the 73,470,008-byte private
Rendered Discovery NRO, SHA-256
`9bdc32fcd7a6e85a5b4fdbff0037afc807a344a486114652d542bfcdf8362088`.
All thirteen local contracts, six compiled mutation checks, five exact-code
GitHub workflows / six jobs, rendered syntax, the private build and thirteen
scoped provider checks passed. Documentation commit `e37657f` changes only
Markdown. Candidate source/artifact hashes, pins and the original integration
patch were reverified before launch.

The earlier connection failure was not a console run. Following the user's
netloader readiness, UDP discovery received `bootnx` from
`192.168.1.194:28280` at **10:01:05 UTC**. No TCP preflight consumed the
listener. Nxlink transferred the exact NRO with **exit 0 at 2026-10-04
10:01:32.332932 UTC** (12:01:32 Europe/Paris): 26,726,477 compressed bytes,
2247 blocks. The displayed 36.38% is a compression ratio.

USB/MTP retrieval at **10:05:14 UTC** preserved **30 reports / 535,712 bytes**.
Eight changed versus the preceding pending-SU-state run; twenty-two are
identical and cannot independently be dated. Source timestamps are unavailable.
Every manifest size/hash, baseline delta, full ZIP CRC and archived raw byte
was independently verified. The new SU report, changed display-list status,
new frontier and coherent discovery/heartbeat cohort attribute this run to
the transferred candidate. Raw files remain private under
`.deps/network-tests/gx-su-state/runs/20261004T100514Z/`.

The visual observation and any on-screen error code are pending the user's
reply. The durable blocker records an intentional unsupported-DIRECT abort;
it does not establish displayed error text or recognizable game pixels.

## Accepted recording progression

The fresh SU report records `su-flush-pass`, guest GXData `0x803437C0`, dirty
word **1 → 0**, `phase=begin` and **updated mask 0**. The actual native SU
routine returned with no eligible coordinate emissions on this state. Thus
hardware accepts this empty-update branch; nonempty SU size/bias emissions
and their selective guest mirrors retain host proof, not console acceptance.

The display-list status is now `begin-pass`, buffer `0x80394F00`, capacity
16384 bytes and save-context flag 1. Its zero byte field is written at Begin;
it is not the completed size of earlier lists. Each later Begin overwrites
the prior End status file.

| Fresh evidence | Dispatch | Context |
| --- | --- | --- |
| First Begin | 607366 | LR `0x8021A568`, stack `0x80394E40`, 16 KiB buffer |
| First End arrival | 607409 | same caller, stage GXBegin, incoming r3 `0xA8` |
| Later allocation after End | 607482 | target `0x80229814`, LR `0x8021BEC8`, r3 `0x40` |
| Sphere arrival | 607503 | LR `0x8021BEC8`, same stack, parameters `(4,8)` |

The checked constructor reads End's returned r3, stores the length, passes
that length into allocation, copies the recorded bytes, flushes the allocation
and starts the next recording iteration. The later allocation snapshot has
stage End and a **64-byte allocation argument** on that path. The following
successful Begin would refuse nested recording unless the preceding End had
cleared it. Together these establish **Begin and at least one End returned,
with a later observed 64-byte length passed to allocation**. First-hit
discovery is not a trace of every invocation. The first End's incoming r3
`0xA8` is not its return count. Copied list bytes and all per-list lengths
were not separately captured.

## New direct boundary

The new blocker is **GXDrawSphere `0x80172A30`**, kind DIRECT, dispatch
**607503**, elapsed **104178 ms**, r3 **4**, r4 **8**, LR **`0x8021BEC8`**,
stack **`0x80394E40`**, stage `RMCP01_GX_BEGIN_DISPLAY_LIST`. The pinned native
library provides this routine. Its called branch agrees with the static
constructor forecast. Sphere arrived but has not returned; the current list
is at its Begin stage. A later `(8,16)` branch remains a static forecast.

The next bridge must preserve native vertex-format save/restore, checked FIFO
ownership, geometry and CPU/guest/HLE effects. Forwarding without executing
and testing its attribute writer/state dependencies would not establish
correct recording or pixels. No sphere or replay hardware acceptance is made.

## Health and remaining limits

The watchdog records 102 samples: 101 ACTIVE and one recovered STALE sample,
maximum interval 1356 ms. The later heartbeat/post-main snapshot at 607482
records 1576 guest FIFO writes, 149 GXBegin hits (previously 141), last word
`3F800000`, 3826 StaticR dispatches, six TaskThread runs and valid 64,224-byte
FST. It retains 99 successful presents / zero failures and zero display-list
replay calls. The graphics report is unchanged. These counters support
recording progression but do not establish new visible content or replay.

Nonempty SU emissions, all recording layouts/counts, sphere return, list replay,
sustained execution, recognizable game images, input/audio correctness and
representative-scene performance remain open. Earlier dated results retain
their original scope.
