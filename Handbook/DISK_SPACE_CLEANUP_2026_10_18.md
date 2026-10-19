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

## Follow-up archival

A second reviewed batch includes23 obsolete Goal1 runtime and pre-rebase executable
snapshots. **9,662,616,430 additional bytes (9.00 GiB) recovered**; total across
both batches is **25,128,947,893 bytes (23.40 GiB)** from56 snapshots.
All archives were fully decompressed and hash-verified before original removal.
Current integrated build and Goal3 runtime snapshots remain directly usable.
Historical archived executables require restoration before replay.

[Follow-up scope, manifest and restore instructions](../integration_evidence/disk_cleanup_20261018_followup/README.md).
[Final machine-readable receipt](../integration_evidence/disk_cleanup_20261018_followup/summary.json).
The4TB filesystem now reports about49GiB free; root/tmp remains about509MiB free.
No foreign files or scientific evidence were removed, and no numerical tests were
rerun for this archival/documentation-only change.

## Additional batch — 2026-10-19

Eight reviewed obsolete2D executable snapshots were archived with the same full
decompression/hash verification. **3.79GiB additionally recovered**, for
**27.19GiB cumulative** across64 snapshots. Current integrated build, all3D
runtime snapshots, meshes, states, donor fields and diagnostics remain directly
usable. Historical replays need the corresponding executable restored first.
The observed4TB free space is now about46.4GiB; root/tmp remains nearly full.

[Selection, manifest, integrity checks and restore instructions](../integration_evidence/disk_cleanup_20261019/README.md).
[Final machine-readable receipt](../integration_evidence/disk_cleanup_20261019/summary.json).
No numerical changes, CFD reruns or unrelated-file cleanup were performed.

## Hard-linked 2D executables — 2026-10-19

The previous batches skipped shared executable inodes. A reviewed follow-up now
archives **18 further obsolete 2D executable groups across 55 original paths**.
One compressed inode is retained per group, with a `.gz` hard link beside every
original pathname. All archives passed full decompression/SHA-256 verification
before original removal. Final group/link/size checks pass and no partials remain.

Additional savings: **8.43GiB (9,053,781,576 bytes)**, counting shared storage once;
cumulative archival savings are **35.62GiB**. Byte differences exclude block
rounding; unrelated writes can affect observed free space (now about55GiB4TB,
490MiBroot/tmp). All current builds,3D runtime binaries, scientific evidence and
foreign work remain untouched. One nice19 worker used about50MiB sampled RSS over
520seconds; no CFD/build workload was started alongside it.

[Exact selection, receipt and hard-link restoration instructions](../integration_evidence/disk_cleanup_20261019_hardlinks/README.md).
Historical replays need executable restoration; restoring each pathname separately
can consume extra disk, so restore one per group and relink its other names.
No numerical changes or new mesh-validation claims accompany this cleanup.
