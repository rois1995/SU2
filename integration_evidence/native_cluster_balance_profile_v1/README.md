# Native reconstruction balance pilot (SGE)

Compile branch **`codex/native-unsteady-performance`** in the existing
`SU2_NativeIntegrated` cluster checkout. This new package follows the reviewed
32-case cluster audit (581995). It measures which operations and regions cause
imbalance before changing partition weights. It does not implement live repartition.
No local solver/build runs or cluster submissions are performed by Codex.

## Prepare on the cluster login/build host

First preserve the **existing validated** `build-native/UnitTests/test_driver`
**before rebuilding**. Its expected SHA256 is
`b8bb7bc0bf3ba701232513d0c4d0e812e3483cac605da6ecd13e1a4845b7971e`.
It is the executable used by the completed previous correctness/performance
campaign. A differing binary is rejected rather than silently relabeled as the
control. `control_source_pins.json` preserves its reviewed source identities.

From the checkout root:

```bash
git checkout codex/native-unsteady-performance
git pull --ff-only origin codex/native-unsteady-performance
python3 integration_evidence/native_cluster_balance_profile_v1/prepare.py --preserve-control
./ninja -C build-native -j2 UnitTests/test_driver
python3 integration_evidence/native_cluster_balance_profile_v1/prepare.py
qsub integration_evidence/native_cluster_balance_profile_v1/RunBalanceProfileSGE.sh
```

Reuse the existing MPI/GCC/Meson build settings. Only the test driver needs
rebuilding for this frozen pilot; no new CFD executable is used. The preserved
control lives at `build-native/balance-control/test_driver` inside the checkout.
If it was already replaced, recover the original validated executable from your
previous candidate build/archive; simply recompiling the old revision may not
produce its exact hash. Preserve an existing checkpoint before preparing a new
build. Do not rebuild or alter pinned sources/inputs/tools while queued/running.
The checkpoint records identities and Meson settings; it is **not** proof that a
binary was built from those sources. The cluster gate verifies the actual behavior.

This uses your normal shared-node `aero-ags.q` / `mpi` allocation: four slots,
one job, all cases sequentially on the same allocation. Compute nodes need no
Git. `machinefile.$JOB_ID` must be inside this checkout; `MACHINEFILE_PATH` can
select another closed path. No exclusive-node reservation is requested.

## Rerun after the job 582197 hardlink error

The cluster scratch filesystem refused `os.link` with EPERM while staging the
first fixture, before any MPI/SU2 test. The exporter now falls back to an
exclusively created, hash-verified copy when links are unsupported or refused.
Both fixture staging and case exports use it. Existing files are never
silently overwritten, and copy/hash failures retain unverified working files.
No C++ source or solver binary changed in this compatibility fix.

After pulling, preserve the failed job's checkpoint and prepare again because
package hashes changed. **No rebuild or control replacement is needed**:

```bash
git pull --ff-only origin codex/native-unsteady-performance
mv -n build-native/balance_profile_checkpoint.json build-native/balance_profile_checkpoint_582197.json
python3 integration_evidence/native_cluster_balance_profile_v1/prepare.py
qsub integration_evidence/native_cluster_balance_profile_v1/RunBalanceProfileSGE.sh
```

Previous failed output/launcher folders remain untouched; the new SGE ID creates
a fresh results directory. Successful copy fallbacks retain the same selected
files and audits, but cannot provide hardlink disk savings. Per-file manifests
record `storage` and `validation.json` records `export_storage_counts` plus
logical/unique-file byte totals.

## What runs

The candidate first runs focused native/profile/passive-communication unit tests
at MPI1/2/4. Then one repetition runs **16 frozen remeshes**: Euler-to-BL and
BL-to-Euler, each with control/profile pairs at N4/M4/no working repartition,
N4/M4/weighted partition, N4/M3/weighted and N4/M2/weighted. There is no CFD time
marching and no series of timestep solutions. Inputs are the same closed frozen
RAE fixtures used in the reviewed campaign, including the original raw sensor.

Every case must pass its independent mesh/geometry/first-height/transported
metric audit and the existing exclusive timing-accounting check. Each pair must
produce **byte-identical adapted grids and all four transported-tensor files**;
otherwise the campaign stops before further pairs. Failed exports retain their
unverified working directory. The unit gate includes tests that profiling leaves
MPI coupled reconstruction unchanged and retains failed-attempt costs.

The new environment flag `SU2_NATIVE_BALANCE_PROFILE=YES` is opt-in; unset/NO
creates no profile files. The profile candidate records eight operation totals
per worker and at most **32 expensive private attempts per worker**, including
rejected work, cavity size/bounding box, boundary-cell count, seeds/round,
query/evaluation/eviction counts, wall and process CPU time. CPU time is scoped to
private reconstruction, excluding import/collective waits; unavailable clocks
have -1 in hotspots and do not count toward CPU coverage. Boundary cells mean
any marked boundary, not exclusively viscous walls. Hotspots are the bounded
largest wall-time attempts, not an unbiased spatial sample or exact remaining
work forecast. No per-query clocks are added to the millions of metric queries.

The existing logs retain working repartition **estimation, graph, ParMETIS and
migration**, engine initialization, candidate selection, imports, private
reconstruction, validation/commit and return-to-CFD/idle costs. Exclusive means
close against engine time; rank maxima cannot be added. Frozen cases do not run
CFD geometry replacement/solution transfer: the previous actual campaign remains
the source for those lifecycle costs. Profile CSV output is separately timed and
included in full remesh timing, outside engine-adapt timing. Even with profiling
off the candidate performs the flag-consistency elections; the paired total
therefore measures all instrumentation overhead, not just record insertion.

Use one repetition first. After reviewing it, a separately submitted repeat
campaign can reverse pair/workload order and rotate partition order:

```bash
qsub -v REPEATS=3 integration_evidence/native_cluster_balance_profile_v1/RunBalanceProfileSGE.sh
```

Do not launch both jobs concurrently. Shared-node contention/affinity/RSS/CPU
pressure samples are saved compactly; timing remains approximate. This pilot is
not a 192-core scaling certificate or a composed-BL-gradation certificate.

## Results and disk use

Download these two folders, preserving their layout:

- `ClusterResults/balance_profile_JOB_ID/`
- `ClusterResults/jobs/JOB_ID_balance_profile_launcher/`

Inspect `validation.json` for PASS, paired identity/overhead, rank/operation costs,
32-hotspot records, phase-accounting and node observations. Grids and tensors
are under `cases/frozen_*_n4_m*_p*_r*_control/` and the corresponding `_profile/`.
The runner prints every actual working folder and retained case folder.

Only configs, immutable input mesh/raw sensor/source-flow provenance, adapted
or rejected meshes/reference sidecars, transported tensors, timing/profile CSV,
logs, receipts and audit reports are retained. Tools are saved **once per job**.
Identical inputs and numerical outputs are **hardlinked within that job when
supported**, otherwise copied, with SHA256 verification in either case. All paths are closed; no export symlinks refer to another
checkout. Use `rsync -aH` when downloading if possible to preserve hardlink savings;
ordinary downloads may expand existing hardlinks. If the cluster filesystem
requires copy fallback, there are no corresponding hardlinks to preserve. Each case has the physical files needed for
re-audit using the shared `tools/integration_evidence/audit_native_frozen_case.py`.

Task-owned temporary outputs are removed only after a verified compact export.
Successful unit stages retain logs and receipts, not incidental geometry files;
failed unit stages retain diagnostics. Previous campaigns are untouched and
`ClusterRaw` is not required. One small source-flow provenance VTU per fixture
remains because the existing independent auditor expects it; no new solution
VTU/restart histories are generated. Logical and unique-file byte totals are
recorded (before the final validation receipt update).

Local preparation checks use fake files only:

```bash
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_cluster_balance_profile_v1/check_package.py
```

New C++ compilation, MPI behavior and real numerical audits remain **pending the
user-run cluster job**. Older frozen performance source pins are preserved and
intentionally do not match this implementation; use this package for the pilot,
not the historical `SubmitMetricComparison.sh` pins.
