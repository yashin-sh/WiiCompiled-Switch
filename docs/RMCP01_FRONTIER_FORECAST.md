# RMCP01 frontier forecast

`scripts/forecast-rmcp01-frontier.py` adds a static forecast layer on top of
the hardware-first porting workflow.

It does **not** authorize a patch. Its job is to tell us what public
`doldecomp/mkw` callsites suggest may come next, and whether those candidate
symbols already have a Switch native mapping.

## Basic use

After the one-time public decomp checkout:

```bash
python3 scripts/forecast-rmcp01-frontier.py --fetch 0x80170A4C
```

Later:

```bash
python3 scripts/forecast-rmcp01-frontier.py 0x80170A4C
```

Or point it directly at a copied blocker file:

```bash
python3 scripts/forecast-rmcp01-frontier.py \
  --blocker /path/to/fast-track-dispatch-blocker.txt
```

Useful tuning:

```bash
python3 scripts/forecast-rmcp01-frontier.py \
  --window 24 \
  --depth 20 \
  0x80170A4C
```

JSON is available for future automation:

```bash
python3 scripts/forecast-rmcp01-frontier.py --json 0x80170A4C
```

## What it does

For the current PAL symbol, the helper:

1. attributes the address through the existing RMCP01 attribution helper;
2. finds public `doldecomp/mkw` callsites of that symbol;
3. scans the following source window for direct calls;
4. resolves candidate PAL symbol addresses when public metadata exists;
5. compares candidate addresses against the repo's `KnownNativeCpuCall`
   mappings;
6. highlights native SDK/library candidates that are currently unmapped;
7. marks mapped bridges that appear hardware-constrained as higher-risk repeat
   blockers;
8. ranks the result as a **static hint**, not an execution prediction.

## Coverage labels

- `mapped-native`: a `KnownNativeCpuCall` mapping exists;
- `mapped-native-constrained`: a mapping exists but local code appears to
  retain hardware-exact guards;
- `native-unmapped-candidate`: public RVL/NW4R symbol with no native mapping;
- `translated-source`: public game-side source, normally handled by the
  locally translated product;
- `unclassified` / `unknown-symbol`: attribution is insufficient.

A mapped function can still become the next blocker if the bridge is
tuple-restricted. Recent repeated `GXInitTexObjLOD` objects are the clearest
example.

## Why this is useful

The hardware log still answers:

> What actually stopped this run?

The forecast answers a different question:

> What should we inspect now so the next hardware result is faster to
> understand?

That lets us pre-read decomp/WiiCompiled semantics and identify likely missing
coverage without pre-porting anything.

## Limits

Static callsite adjacency cannot reliably predict:

- thread scheduling order;
- dynamically computed callbacks;
- virtual calls;
- indirect calls;
- runtime object addresses;
- exact GXTexObj descriptor contents;
- which thread reaches a candidate first.

DTK can provide deeper binary-level control-flow information when the user's
local RMCP01 files are available, but the same policy applies: prediction is
analysis only.

## Decision rule

```text
forecast
   ↓
prepare attribution / semantics
   ↓
real Switch run
   ↓
durable exact blocker
   ↓
minimal patch
```

Never reverse the final two steps.
