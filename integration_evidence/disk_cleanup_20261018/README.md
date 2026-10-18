# Lossless historical executable archival — 2026-10-18

Only obsolete, non-hardlinked ELF snapshots in this session's native `*_build_v*`
folders and `ad_repair_archive_v14` are eligible. Current `build-integrated-v2`,
3D Goal3 runtime snapshots, meshes, solution states, original fields, accepted and
rejected scientific evidence, tracked files and foreign work are excluded.
Open executable/file descriptors are checked before compression and removal.

`archives.json` records the original path, full uncompressed SHA-256, byte count,
mode and timestamp. Each gzip stream is completely decompressed and checked before
removing its original. This preserves exact historical bytes, but historical
runners need their original executable restored before use. Existing provenance
hashes refer to those original bytes, not the compressed stream.

Restore one selected binary (ensure space for its `original_bytes` first):

```bash
gzip -dk integration_evidence/SELECTED_BUILD/SELECTED_BINARY.gz
chmod 755 integration_evidence/SELECTED_BUILD/SELECTED_BINARY
sha256sum integration_evidence/SELECTED_BUILD/SELECTED_BINARY
```

Compare the result to `archives.json` before replay. Retain the gzip archive until
the original is verified; `gzip -dk` refuses an existing output. The manifest also
retains original permission bits and nanosecond modification time if needed.

The cleanup is a single low-priority streaming worker. No CFD or tests are run.
Root `/tmp` is excluded because no large safely attributable disposable directory
has been established. Free-space observations can vary with unrelated sessions.

`archive_snapshots.py` is the executed cleanup receipt, not a general scheduled
maintenance tool. Its directory selection was reviewed against the 2026-10-18
worktree. Recheck ownership, active consumers and tracked symlink targets before
any later use; new build folders may have different lifecycle requirements.
