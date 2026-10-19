# Historical Goal1 3D core executable archival — 2026-10-19

Six reviewed, superseded Goal1 executable inode groups only, fixed in selection.json. Current integrated build and Goal3 runtime snapshots remain directly usable. No meshes, solution states, original donors, geometry authorities, failure evidence, source capsules, foreign files or dependencies are removed.

One nice19 streaming worker. Every gzip archive is fully decompressed and SHA-256 checked before original hard links are removed. The worker rejects tracked files, symlinks/referenced targets, incomplete link groups, in-use executables and modified files. archives.json records all original names, SHA-256, size, mode and timestamp. summary.json counts physical savings once per inode. No numerical tests accompany archival.

Historical replay now requires restoration: gzip -dk SELECTED_PATH.gz, restore the manifest mode, verify its uncompressed SHA-256, then use ln for the other original names in its group. Restore one per group; decompressing every pathname separately wastes space. Retain archives until restoration is verified.

Verified final result: six groups / 30 original paths, 2535738749 bytes (2.36 GiB) recovered. All archive link/size and original-path checks pass; no partial archives remain. Elapsed 128.70 s.
