# Native work disk cleanup — 2026-10-18

Worktree: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-bl`.

Lossless archival of obsolete executable snapshots recovers space without deleting
accepted/rejected scientific evidence. Only inspected native historical build
folders and the earlier AD repair archive are included. The current integrated
build, all 3D runtime binary snapshots, grids, states, original sensor donors,
geometry references, failure diagnostics, source capsules and foreign sessions
remain untouched. Non-hardlinked files only; no tracked symlink targets selected.

[Receipt, integrity checks and restoration instructions](../integration_evidence/disk_cleanup_20261018/README.md).
The manifest preserves original SHA-256, bytes, mode and timestamp. Each archive
is fully decompressed and hashed before removing its redundant original. Historical
replays using those executables require restoration first; old provenance refers
to the uncompressed bytes. Archives themselves are not committed.

Root `/tmp` remains nearly full. Handbook-attributed scratch directories inspected
there total only a few megabytes; unrelated large work is not safe to remove.
Temporary compiler output for this work remains on the 4 TB disk.

No CFD reruns, geometry changes or numerical changes are part of cleanup.
The [full symmetry roadmap](GOAL3_GENERAL_SYMMETRY.md) and all Goal3 gates remain.

Final result: **33 verified archives, 15,466,331,463 bytes (14.40 GiB) recovered**.
Original snapshots total 22,016,358,504 bytes; exact gzip archives remain locally.
See the [machine-readable final receipt](../integration_evidence/disk_cleanup_20261018/summary.json).
