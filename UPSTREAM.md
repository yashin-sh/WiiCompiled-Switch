# Upstream pin

The Nintendo Switch port consumes WiiCompiled from the repository submodule at `third_party/WiiCompiled`.

Pinned WiiCompiled commit:

`a135beb201042b20f390c6695ca6b26768820fb4`

Upstream repository: `patchzyy/Wiicompiled`

This was the latest upstream `main` commit when the M1 audit started on 2026-09-07. Porting work remains reproducible against this SHA until an intentional upstream rebase is reviewed and regression-tested.

Initialize the exact pinned source with:

```sh
./scripts/bootstrap-upstream.sh
```

or:

```sh
git submodule update --init --recursive
```

CI verifies that the checked-out submodule HEAD equals the pinned SHA before building the Switch NRO.

Do not commit locally generated game-derived output into this repository.

## GX audit status

A follow-up audit on 2026-09-13 compared the pinned source with upstream commit `209405dfb72c73d6bc26f5214bf4dfe0f8baae2f` for GX/Aurora behavior relevant to the future Switch graphics path.

The primary affected file, `aurora-main/lib/gx/command_processor.cpp`, has the same blob SHA at the pinned commit and the audited newer upstream commit:

`5f69705e96fce50d69235c106dd7bf4816281724`

That confirms the currently pinned port already inherits the audited GX risks; they are not merely future-rebase concerns.

Tracked blockers before trustworthy M3 graphics validation:

- #109 — release-safe FIFO bounds checks
- #110 — prevent draw-call merges across different `GXVtxFmt` values
- #111 — guard degenerate `GX_LINESTRIP` vertex counts
- #112 — expose unsupported indexed XF loads in Release builds

Full notes: `docs/UPSTREAM_GX_AUDIT_2026-09-13.md`.

Any future upstream rebase should explicitly re-check these four issues and record whether the upstream revision fixes, changes, or still contains each behavior before the submodule pin is advanced.
