# Historical 2D executable hard-link archival — 2026-10-19

Worktree: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-bl`.

The explicitly reviewed selection contains 18 historical 2D executable inodes
across 55 paths. Previous cleanup skipped hard-linked executables. Archiving a
single pathname would not recover their shared disk allocation; this batch
preserves one verified compressed inode, with a `.gz` hard link at every original
pathname, then removes the original executable links.

`selection.json` fixes the scope. `archives.json` records every original/archive
path, exact uncompressed SHA-256, original size, physical archive size, mode and
mtime. Count reclaimed physical bytes once per group, not once per pathname.
Each archive is fully decompressed and hashed before any original link is removed.
The worker rejects incomplete hard-link groups, tracked files, repository symlink
targets, occupied archive paths and executables/open files in use. Inode, link
count, size and mtime are checked before replacement. Manifest progress is saved
before original removal. A partial/interrupted group requires explicit review;
the worker deliberately refuses an automatic rerun over an existing manifest.

Current integrated build, all 3D runtime snapshots, meshes, solution/restart files,
original metric donors, geometry references, source capsules and accepted/rejected
scientific evidence are untouched. Foreign ClusterResults, dependencies and /tmp
are excluded. No numerical validation or CFD rerun is part of this cleanup.
The single streaming worker runs at nice19, using about 42 MiB RSS initially.

Historical replay requires restoring the executable first. For one selected path:

```bash
gzip -dk integration_evidence/SELECTED_FOLDER/test_driver.gz
chmod 775 integration_evidence/SELECTED_FOLDER/test_driver
sha256sum integration_evidence/SELECTED_FOLDER/test_driver
```

Use the manifest's recorded mode if different, compare its exact SHA-256, and
retain the archive until verified. `gzip -dk` refuses an existing output. Restoring
all paths independently uses more disk than the original hard-link layout; restore
one executable and use `ln` for the other original names in that manifest group to
preserve shared storage. Original nanosecond mtime is recorded for exact metadata
restoration. Make room for the uncompressed `original_bytes` before restoring.

`summary.json` records completion and physical bytes recovered. Free space may also
change due to unrelated machine activity. Full Goal3 and the general symmetry
requirements remain active; this cleanup changes neither geometry nor mesh gates.

Final result: **18 verified groups / 55 original paths, 9,053,781,576 bytes (8.43 GiB) recovered**. All final archive/link checks pass; no partial files remain. The byte total counts each inode once and excludes filesystem block rounding. Observed free space is about55GiB on4TB and490MiB on root/tmp. Archival elapsed520.21seconds, one nice19 worker, about50MiB sampled RSS.
