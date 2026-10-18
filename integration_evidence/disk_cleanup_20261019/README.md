# Historical 2D executable archival — 2026-10-19

Worktree: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-bl`.

The eight reviewed folders are recorded in `selection.json`. Only untracked,
non-hardlinked ELF executable snapshots larger than400MiB qualify. The reused
archival implementation excludes tracked symlink targets and open executables/file
handles, and checks original inode, size and timestamp before removal.

Each archive is completely decompressed and SHA-256 verified before its redundant
original is removed. `archives.json` records original/archive paths, full original
hash, sizes, permissions and timestamp. Exact gzip archives remain locally outside
Git; `summary.json` reports recovered bytes and filesystem observations. The copied
execution receipt records this selection, not a new general cleanup service; review
ownership, consumers and lifecycle requirements before any reuse.

The current integrated build, all3D runtime snapshots, meshes, solutions, original
sensor fields, geometry references, source capsules, accepted/rejected diagnostics,
foreign ClusterResults and unrelated `/tmp` data remain untouched. No numerical
change or CFD/test run is involved. Full Goal3 and the general symmetry scope remain.

Historical replays using one of these executables require restoration first:

```bash
gzip -dk integration_evidence/SELECTED_FOLDER/test_driver.gz
chmod 755 integration_evidence/SELECTED_FOLDER/test_driver
sha256sum integration_evidence/SELECTED_FOLDER/test_driver
```

Ensure room for `original_bytes` and compare against `archives.json`. Existing
provenance refers to those uncompressed bytes. `gzip -dk` refuses an existing output;
keep the archive until restoration has been checked. The manifest retains original
permission bits and nanosecond timestamps when exact metadata restoration is needed.

The worker runs at nice19 with streaming buffers. Root/tmp is excluded because its
large unrelated consumers cannot be safely attributed to this work. Compiler scratch
for the native work remains on the4TB filesystem.

Final result: **8 verified archives, 4,065,739,031 bytes (3.79 GiB) recovered**.
All8 originals passed full decompression/hash verification before removal; final
paths, archive sizes and absence of partial files were checked. Observed4TB free
space after cleanup was about46.4GiB; unrelated activity can change it.
