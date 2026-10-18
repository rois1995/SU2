# Follow-up historical executable archival — 2026-10-18

Worktree: SU2_NativeIntegrated, branch codex/native-3d-bl.

Extends the earlier cleanup to obsolete Goal1 `native_3d_core_v1` runtime
snapshots and historical `native_post_rebase_metric_v1` executables. Selection
requires an untracked, non-hardlinked, executable regular file larger than400MiB.
Symlink targets and open executables/file descriptors are excluded. Current
`build-integrated-v2` and Goal3 runtime snapshots are outside the selection.
No meshes, solution states, frozen donor fields, source capsules, geometry,
accepted/rejected diagnostics, ClusterResults or foreign work are removed.

`archive_snapshots.py` records the exact reviewed selection and checks.
Each gzip archive is fully decompressed and SHA-256 verified before its redundant
original is removed. `archives.json` preserves original path, bytes, SHA-256,
permissions and timestamp; `cleanup.log` preserves per-file progress.
Archives remain locally, outside Git. This script is an execution receipt:
review ownership, consumers, symlinks and lifecycle requirements before reuse.

Historical runners using an archived executable require restoration first:

```bash
gzip -dk integration_evidence/SELECTED_FOLDER/SELECTED_EXECUTABLE.gz
chmod 755 integration_evidence/SELECTED_FOLDER/SELECTED_EXECUTABLE
sha256sum integration_evidence/SELECTED_FOLDER/SELECTED_EXECUTABLE
```

Check against `archives.json`; ensure sufficient restoration space beforehand.
`gzip -dk` refuses an existing output. The manifest retains exact original modes
and nanosecond timestamps if these are needed. Historical provenance hashes refer
to uncompressed bytes. Current tests were not rerun for archival-only changes.

See `summary.json` for final recovered bytes and filesystem observations.
