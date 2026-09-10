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
