# Optimizer quality-prefix disk recovery

The first GQ wrapper refused to launch when workspace free space was 1.82 billion bytes. The existing guard requires more than 2 billion bytes. The first wrapper logs and terminal receipt are in `first_GQ/`. The build and G1, G2 and G3 were already qualified and remained pinned.

The phase C and D backup executables have verified `.zst` archives beside their original paths. The listed executable paths resolve to SHA256-matched recovery copies on the root filesystem. The original manifests and compressed files stay in the evidence tree. The raw paths and executable modes are recorded in the archive receipt.

If root scratch is cleared, decompress an archive to a new regular file on a volume with space. Verify the restored SHA256 against `raw_sha256`, set the recorded mode, then point the original local-copy symlink to that file. The archives use standard Zstandard framing.

[Archive receipt](space_recovery.json)
