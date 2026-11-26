# FP2-P3 test-code corrections

The initial B2a saved481 launcher omitted the N6 inherited binding that selects the qualified curved492 producer for both saved modes. The legacy launcher therefore used curved441 and scalar inputs 0.005499698867956817 and 0.3777507474613889 for saved481. Catch stopped at its first target checkpoint import with one failed assertion and the exception “Mapped target checkpoint predates directional curvature sizing”. It never reached the witness query. The initial receipt highlighted a missing off marker because the launch adapter assumed query entry. The raw log and issued command preserve the earlier checkpoint import failure.

The signed N6 reference command uses `curved492_ramp5_metric_stage_20261113T161313_3653005/target_rank0.bin`, the paired frozen sensor, and scalars 0.0047263575870155755 and 0.28717458874925855. Its check uses the same Catch selector and byte-identical frozen config. Verified the actual merit481 and saved-current launch contracts against their own references. The four target-input differences were isolated to saved481. This is launcher wiring under R3, with no production change or consumed result-gate retry.

The corrected saved481 child completed in its fresh root. It matched the archived N6 command and frozen config, all three historical payload digests, and the ordered planning rows. The previous B2a seal and its failed-input evidence are retained separately. B2a was reselected from the corrected saved481 receipt and the six unchanged independent passes.

## Full workflow switch receipts

The first B2b seal selected the `-n 19` from `nice -n 19` when it read a command containing `mpirun -n 6`. It failed during provenance sealing after accepted events, six matching disk payloads, continued CFD and a verified isolated restart. The actual launch and restart commands contain the required switches, MPI exports and six ranks. The raw main rank logs show six OFF markers and the no-query restart shows zero. The exact FX2 row comparison has zero differences.

The receipt check now looks for the rank flag after the MPI launcher token. The original launcher remains byte-pinned. The correction reuses the terminal controls and rechecks host identities, source and helper pins, payloads on disk, the restart's artifact hashes, and switch counts. The original failed receipt, summary and top seal are archived beside their respective corrected selections.

## C2 first-collapse source pin binding

The initial controller stopped before native launch at the inherited assertion requiring disjoint source pin maps. The signed order and its checksum were present with identical digests in both the primary build manifest and the P3 supplement. The setup log and terminal wait identify the exact assertion. The following missing case status was a consequence of that setup stop.

In the corrected fresh attempt, the two files remain in the primary manifest. The supplemental builder verifies the two digests before omitting those duplicates. The inherited source pin validator, switch settings and first-collapse result checks are used as written.

## C5 trial two restart live observation

The repeat's native restart exited 0, its exact pins verified, and it passed the resource gate at 1817374720 bytes. Its wrapper exited 1 because the required live-runtime receipt was absent (false versus required true). The first failed runtime sample recorded EIO. The inherited permanent error latch then prevented all later runtime samples. The original log has no inode or digest mismatch.

Reused the FP2-C3 qualified correction for an incomplete EIO sample. It allows the next observation attempt, still requires a complete six-rank executable and ELF sample, and preserves every other runtime observation failure. The corrected attempt repeats only isolated restart at the unchanged 600 s wrapper from the same sealed event hashes. Its archive, corrected helper, and new runtime evidence are bound in the recovered receipt.
