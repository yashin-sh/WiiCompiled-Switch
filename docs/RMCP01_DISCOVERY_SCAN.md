# RMCP01 global Discovery Scan

The Discovery Scan reduces the number of real-Switch round trips without
turning unknown Wii behavior into guessed behavior.

It has two complementary layers.

## 1. Static local-product coverage scan

The local translated RMCP01 product already contains every statically emitted
`InvokeDirectCpu<...>` dependency. The scanner compares all of those direct
targets with:

- Switch `KnownNativeCpuCall` coverage;
- generated/local `KnownTranslatedCpuCall` traits;
- optional first-hit evidence from a Discovery NRO trace.

Run it directly:

```bash
python3 scripts/scan-local-rmcp01-dispatch-coverage.py
```

JSON:

```bash
python3 scripts/scan-local-rmcp01-dispatch-coverage.py --json
```

After a Discovery NRO run:

```bash
python3 scripts/scan-local-rmcp01-dispatch-coverage.py \
  --trace /path/to/fast-track-discovery-targets.txt
```

This means we can inspect all statically missing direct boundaries before
hardware reaches them one by one.

## 2. Runtime Discovery NRO

Build:

```bash
MKW_JOBS=4 bash scripts/build-local-rendered-discovery-scan.sh
```

Output:

```text
WiiCompiled-Switch-local-rendered-discovery-scan.nro
```

The Discovery build records the first runtime hit of up to 8192 distinct guest
targets to:

```text
/switch/WiiCompiled-Switch/fast-track-discovery-targets.txt
```

Each record contains the target, dispatch index, guest PC/LR, r1-r8, r13,
current guest fiber and fast-track stage.

The 8192-slot table in `source/fast_track_crash_diagnostics.cpp` is fixed-size and allocation-free. A target is written only on its
first hit, so a 600k-dispatch run does not generate a 600k-line file.
Records are emitted before the callee executes. A first-hit entry proves
arrival, not return. The native/translated path increments the dispatch
counter; an unknown DIRECT frontier records the current counter without a
new increment. VI polling can add callback dispatches before that record.

A later distinct caller/frontier, coherent captured state and verified loop
control flow can establish repeated returns without logging every iteration.
This method accepted ten texture-matrix returns in the
[dated matrix baseline](HARDWARE_RESULTS_2026-10-02_DISCOVERY_GX_TEX_COORD_SCALE_FRONTIER.md)
and the eight disabled coordinate triples in the
[coordinate report](HARDWARE_RESULTS_2026-10-03_DISCOVERY_GX_TEV_DIRECT_FRONTIER.md).
The latter has +35 Scale-to-later-caller dispatches rather than the callback-free
forecast +23, then +13 to the TEV frontier rather than +1. VI polling is
compatible with these deltas; first-hit records do not count every callback.
Scope and limits are defined in the
[validation policy](FAST_TRACK_VALIDATION_POLICY.md).

## Safety rule

Discovery mode does **not** blindly return from unknown calls.

Unknown or stateful boundaries still produce the normal durable blocker and
hard-stop. This is intentional: skipping a scheduler, NAND write, GX state
change, callback or other stateful call would corrupt control flow and make all
later "discoveries" unreliable.

Acceleration comes from:

1. statically exposing all missing direct dependencies at once;
2. recording which covered targets are actually reached and in what order;
3. implementing bounded audited GX setter families with wrapper/Aurora,
   argument and any required guest-mirror contracts;
4. preserving the first trustworthy stateful hard frontier.

## Bundle one artifact for analysis

After copying diagnostics from one run to a new local directory, generate
coverage from that same first-hit trace, then include it in the ZIP:

```bash
python3 scripts/scan-local-rmcp01-dispatch-coverage.py --json \
  --trace /path/to/copied/WiiCompiled-Switch/fast-track-discovery-targets.txt \
  > local-product/rmcp01-dispatch-coverage.json
python3 scripts/package-fast-track-run.py \
  /path/to/copied/WiiCompiled-Switch \
  --full --raw \
  --coverage local-product/rmcp01-dispatch-coverage.json
```

The archive then contains both:

- the runtime first-hit trace and normal hardware diagnostics;
- `rmcp01-dispatch-coverage.json`, which lists every statically emitted direct
  target and highlights coverage gaps.

The Discovery NRO is therefore a scanner, not an emulator fallback.

Record candidate revision, exact NRO size/SHA-256 and transfer outcome alongside
the archive. Retained SD files are not automatically fresh; compare hashes
against the previous run and use the attributable blocker/trace cohort.
See [log bundle guidance](FAST_TRACK_LOG_BUNDLES.md). Static missing-target
counts are coverage gaps, not a count of future hardware blockers.

The [coordinate candidate](GX_TEX_COORD_BATCH_2026-10-03.md), code `91a4a01` /
NRO `64ba8377...`, now has 28 retrieved reports totaling 526,932 bytes, with
12 changed from the preceding run. They establish all eight
`Gen2(c,1,4,60,0,125)`, `Scale(c,0,0,0)` and `Bias(c,0,0)` triples for c=0..7.
The restored caller `0x80241380` at dispatch 605620 and the later durable
GXSetTevDirect `0x80171B58` frontier at 605633 support that bounded acceptance.
Arrival at Direct stage 0 is observed; its return and other TEV neighbors remain unproven.
Enabled Scale/Bias branches remain host-tested only. The user saw black;
the 1556 FIFO writes and 99 successful presents / 0 failures at snapshot
605367 precede the loop and do not prove later native emissions or game pixels.

The subsequent [audit run](HARDWARE_RESULTS_2026-10-03_AUDIT_GX_TEV_DIRECT_FRONTIER.md),
code `b3484117` / `7ecbc8a9...`, has 28 verified retrieved reports, 528,058
bytes and 12 changed files. It preserves the accepted normal path to Direct
stage 0 at dispatch 608381, elapsed 107,925 ms; the user again saw black.
This accepts normal-path non-regression, while the negative failure branches
remain host/static evidence. The [six-setter TEV candidate](GX_TEV_SCALAR_BATCH_2026-10-03.md)
passed local gates, all five GitHub workflows on code `e76e8f38`, its private
build and [bounded hardware progression](HARDWARE_RESULTS_2026-10-03_TEV_SCALAR_KCOLOR_FRONTIER.md).
The 28 reports, 528,821 bytes, include eleven changed files and verified
hashes/ZIP CRC. Six first-hit setters plus the checked executed loop and
later KColor boundary establish all sixteen default iterations returned.
Direct-to-KColor is +129 rather than callback-free +111; that delta alone
does not establish exact callback counts or 96 individual returns. KColor
ID 0 / pointer `0x80398FCC` blocks at 605056, 98,265 ms, stage SwapMode.
The user saw black and an error at exit. Snapshot 604804 precedes the loop;
its 1556 FIFO writes / 99 successful presents / 0 failures do not prove
later native emissions or pixels. KColor bytes and later setters remain
unproven, and alternate scalar arguments retain host evidence only.

The [TEV color/table batch](GX_TEV_COLOR_BATCH_2026-10-03.md) passed all five
GitHub workflows and its exact private build (code `1333b0e2`, NRO `a56be881...`).
Its [fresh console result](HARDWARE_RESULTS_2026-10-03_TEV_COLOR_ALPHA_COMPARE_FRONTIER.md)
now establishes all twelve calls returned, with AlphaCompare `0x80172088`
as the latest arrival boundary. The separate
[AlphaCompare candidate](GX_ALPHA_COMPARE_2026-10-03.md) preserves the pinned
native forwarding and existing host validity flag; its console return is pending.


## Latest console result — TEV colors crossed (2026-10-03)

The [fresh color/table hardware result](HARDWARE_RESULTS_2026-10-03_TEV_COLOR_ALPHA_COMPARE_FRONTIER.md)
supersedes the earlier pending color/table status. All twelve executed calls
returned through the coherent later caller; AlphaCompare `0x80172088`,
(7,0,0,7,0), is the new DIRECT hard stop. Black output and a crash persist.
Actual RGBA bytes and recognizable game pixels remain unproven. The elapsed
time includes an unexplained watchdog sampling gap, so it is not a performance
measurement. Prior dated results above retain their original scope.
