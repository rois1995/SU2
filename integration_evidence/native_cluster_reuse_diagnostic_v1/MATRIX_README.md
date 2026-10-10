# Next frozen check: strict OFF vs BOTH with one tested executable

Job582344 shows identical accepted mesh/tensors for OFF/SCORES/METRIC/BOTH/AUDIT,
zero sampled cache mismatches, and numerical audit PASS in every case. Historical
low-bit differences have no demonstrated practical consequence here. Keep those
findings; do not widen numerical tolerances or spend another campaign reproducing
them. This matrix finishes the workload directions and partition modes not reached
by the old cross-executable comparison.

From the complete cluster checkout root, using codex/native-unsteady-performance:

```bash
git checkout codex/native-unsteady-performance
git pull --ff-only origin codex/native-unsteady-performance
python3 integration_evidence/native_cluster_reuse_diagnostic_v1/matrix_prepare.py
qsub integration_evidence/native_cluster_reuse_diagnostic_v1/RunReuseMatrixSGE.sh
```

**Do not rebuild for this job.** It uses the existing job582344 test_driver,
SHA2565827ffbe8b7ac9dd0336d000a33a8d649975579f1cd57f623a440a00fb87019e.
Preparation verifies that binary, its original C++ source/test/build pins and
unchanged build options from build-native/reuse_diagnostic_checkpoint.json.
Only Python orchestration/docs changed after that build. It then creates a new
build-native/reuse_matrix_checkpoint.json with the current closed dependencies.
Neither checkpoint nor any executable/result is overwritten. If the new matrix
checkpoint already exists, retain/rename it before preparation. Do not edit or
rebuild the checkout while queued/running. Compute nodes need no Git.

One ordinary shared-node four-slot allocation runs focused units on MPI1/2/4,
then16 sequential frozen remeshes: Euler-to-BL and BL-to-Euler, each at N4/M4 NO,
N4/M4 YES, N4/M3 YES and N4/M2 YES, with paired OFF/BOTH roles. Role control means
reuse OFF and role profile means BOTH, **using the same executable**; the old
archived binaries remain preserved but are not numerical references in this job.
All modes disable the sampled audit, so no fresh-check overhead enters timings.
Every actual working/retained testcase folder is printed.

The gate stays strict: all independent topology/reference/height/quality/length/
transported-tensor and timing-accounting audits must pass; accepted meshes and all
four tensor CSVs must be byte-identical within each pair; operation/selection
counts must match. Any failure stops further pairs and retains evidence. PASS
certifies these frozen comparisons only, not an unsteady lifecycle, converged
CFD or general scaling result. No CFD timesteps/Hessians are computed here.

Download only:

```text
ClusterResults/reuse_matrix_JOB_ID/
ClusterResults/jobs/JOB_ID_reuse_matrix_launcher/
```

Grids/tensors are under cases/frozen_*_n4_m*_p*_r1_control/ and *_profile/.
Configs, logs, independent reports, full remesh/repartition timings, bounded
rank/operation profiles and absolute UTC times are retained. Shared frozen inputs
are deduplicated where hardlinks work, with verified-copy fallback. No ClusterRaw
or flow-output series is produced. Temporary work is removed only after verified
compact export. Old evidence is untouched. Submit one owned job at a time; a
single default repetition is enough for the first correctness/coverage check.
