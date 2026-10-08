# Switch first/latest GPU image checkpoint — 2026-10-08

The PR #336 correction resolves the SD image replacement failure on the tested
Switch path. A fresh run of code `1f04589` saves the first and last completed
GPU surfaces. The status is `COMPLETE`, with both `frame` and `latest_png_frame`
equal to 102. Independent PNG chunk CRC, zlib, row and full-pixel decoding verifies
the files and their agreement with the reported latest-image statistics.

| Completed frame | Dimensions | Uniform RGBA | Interpretation |
| --- | --- | --- | --- |
| 1 | 1280×720 | `(0,0,0,255)` | Opaque black first surface |
| 102 | 1280×720 | `(0,0,0,0)` | Black RGB with zero alpha |

The first file is retained while the latest file changes. The final latest PNG
is distinct from the first, and the fresh run ID differs from the preceding
failed trial. Reaching a complete frame-102 save with capture still enabled
supports the periodic replacement and diagnosed-stop checkpoint path. This
acceptance is scoped to the executed console path, not power-loss durability.

These are actual NVK surface pixels after Aurora rendering, including presentation
scaling. They establish black RGB in the captured completed surfaces. Zero alpha
does not by itself identify the cause of black output. The later partially recorded
Mii frame is neither submitted nor captured. Physical scanout, recognizable game
rendering, input, audio and playability remain unproven.

Execution reaches the same guarded `GXLoadTexObj` as the preceding image trial:
object `0x80397F00`, slot 0, I4 36×32, source `0x109C1780`, caller LR `0x800C45D0`.
The texture admission policy was unchanged. The fresh last-dispatch report retains
coherent main/FST/fiber state, 102 successful presents and no present failures.
The initial FIFO capture is byte-identical to the previously replayed black frame.

The code passes all six public workflows / seven jobs and the exact-head private
rendered build, SDK syntax and link/provider checks. The successful launch, product
identity, 39-report retrieval manifest and complete image analysis remain privately
bound to this run. Raw reports, game-derived PNGs and products stay excluded from
Git and public CI.
