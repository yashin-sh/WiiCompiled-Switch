# Upstream pin

The initial Nintendo Switch portability audit targets WiiCompiled commit:

`a135beb201042b20f390c6695ca6b26768820fb4`

Upstream repository: `patchzyy/Wiicompiled`

Reason for pinning: this was the latest upstream `main` commit when the M1 audit started on 2026-09-07. Porting work should remain reproducible against this SHA until we intentionally rebase onto a newer upstream revision.

Do not commit game-generated output from a local WiiCompiled build into this repository.
