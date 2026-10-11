# Blank system-save reads

The user supplied a console photo showing Mario Kart's "This data is corrupted"
dialogue after the Wiimote warning. A subsequently observed debug session
advanced to dispatch 827,547 and paused on a new RGB565 texture descriptor. The
photo proves that the dialogue is visible; it does not prove that an existing
save contains corrupt data.

The retrieved `NANDGetLength` report records a successful read handle with length
zero, including successful seek and position restoration. The report does not
record the opened path. Static attribution connects the relevant save-reader
caller to `rksys.dat` and an expected length of `0x2BC000`; attributing that exact
live handle to the file remains pending. NAND reports survive process startup,
so a changed report alone does not establish which execution produced it.

## Correction

Pinned WiiCompiled `a135beb201042b20f390c6695ca6b26768820fb4` already supplies
`RuntimeNandSave::CheckRead`. The Switch `OpenSync` implementation previously
bypassed this policy and opened an empty system save as an existing file.

`OpenSync` now applies the pinned policy before opening a stream:

- Reading an empty or entirely zero-filled `rksys.dat` returns missing (`-12`).
- An inspection error, or a nonzero recovery shadow without a usable original,
  returns unknown (`-64`), preserving the recovery data.
- Nonzero files remain readable, allowing the guest to validate their length
  and contents. An available original takes precedence over its shadow.
- Other filenames and write modes retain their existing behavior.

The policy reads files without deleting, truncating, promoting or repairing
them. Safe-read entry points inherit it through `OpenSync`.

An ordinary `rksys.dat` read also writes a bounded, best-effort diagnostic report
with the normalized host path, read policy result, NAND result and owned handle.
This reports the path of the next observed read without changing its result.
Its attempt counter resets with the process and must be paired with the debug
server's runtime session identity when attributing a console run.

## Validation and remaining evidence

The new contract first reproduces the old empty-file result, then passes 162
cases in each of the headless and rendered modes with ASan, fatal UBSan and
LSan. It exercises the actual Switch runtime and Memory slice against the
pinned policy, including empty/zero/nonzero files, late nonzero bytes, recovery
shadows, inspection errors, aliases, ordinary write-mode opens and safe reads.
Recursive snapshots verify file bytes and nanosecond modification timestamps
remain unchanged. Report checks cover escaped paths, refusal and success
handles, process-local attempt counts and nonfatal report-write failures.

These tests do not certify the Mario Kart save format or CRC, and do not verify
safe-write commit behavior on Horizon. The corrected runtime still needs an
attributable fresh console execution. No save data was modified during this
investigation.
