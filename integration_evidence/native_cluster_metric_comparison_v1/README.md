# Post-rebase metric old/new cluster comparison

This is an SGE comparison of complete baseline/candidate lifecycles. It includes
metric recovery/construction, remeshing working repartition/migration, CFD mesh
repartition, transfer and adapted output. Transfer is nested in replacement.
No local performance benchmark or cluster submission is performed by Codex.

Keep the existing baseline checkout and build unchanged at **e6995fbff565955a5677ffcbcc66af9acf96a8f1**.
Use a separate complete checkout of `rois1995/SU2:codex/native-unsteady-performance`
for the candidate. Run [the prediction correctness gate](CORRECTNESS_README.md)
first; its new runtime checks were moved to SGE at the user's request. Git 1.8 works with `git checkout`; compute nodes need
no Git. Both repositories contain their own complete inputs, initialized
dependencies and binaries, with no runtime links into the other checkout.

The candidate at d4416246224aebee506bb1b5601c553cb8af0dc0 has completed the
20-stage correctness gate in cluster job 581856, with saved-output audits reviewed.
The prepared source/input pins still match. The N=4 pilot below is ready for the
user to submit; no repeat correctness run is needed for this evidence/docs update.
RANS composed nodal gradation remains a separate recorded limitation (max ratio
2.73259), even though mesh quality, height and both-history transfer pass. Neither
this correctness PASS nor the small-case timings establish a performance gain.

If a baseline checkout is needed, clone the fork on the login host and run
`git checkout e6995fbff565955a5677ffcbcc66af9acf96a8f1` before initializing its
submodules and building. A detached baseline checkout is intentional. Preserve
the previous downloaded `ClusterResults` unchanged.

In the candidate checkout on the login/build host:

```bash
git fetch origin codex/native-unsteady-performance
git checkout -b codex/native-unsteady-performance origin/codex/native-unsteady-performance
git submodule update --init --recursive
CC=mpicc CXX=mpicxx python3 meson.py setup build-native --buildtype=debugoptimized \
  -Dwith-mpi=enabled -Dwith-omp=false -Denable-tests=true -Denable-cgns=true \
  -Denable-autodiff=false -Denable-directdiff=false \
  -Denable-mixedprec=false -Denable-singleprec=false -Denable-mmg=false
./ninja -C build-native -j2 SU2_CFD/src/SU2_CFD UnitTests/test_driver
```

If that local branch already exists, use `git checkout codex/native-unsteady-performance`
and `git pull --ff-only origin codex/native-unsteady-performance`. Match the old
build's compiler/MPI and Meson options, including CPU architecture. Record build
logs and options; a source hash check alone does not establish binary provenance.
Do not rebuild or replace either binary while its jobs are queued/running.

The existing wrapper uses `aero-ags.q`, `mpi`, the supplied GCC paths and
`machinefile.$JOB_ID`. Cluster site settings remain as in the first campaign.
Use the usual shared-node allocation, as requested; do not add an exclusive-node
resource. The helper's dependencies serialize our jobs while other users may
share the node. Treat timings as approximate shared-node measurements.

The submission helper also supports older cluster Bash: its qsub argument array
is always nonempty under `set -u`. If an earlier version failed with
`hold[@]: unbound variable`, pull the current branch and repeat the submission
command. That first-job expansion error occurs before qsub; no jobs were
submitted by that failed attempt. No binary rebuild or correctness rerun is
needed for this submission-script-only fix.

First run one comparison repetition with the same two adaptation events as the
downloaded baseline. From the candidate checkout root:

```bash
ADAP_WORKERS=4:3:2 bash integration_evidence/native_cluster_metric_comparison_v1/SubmitMetricComparison.sh /absolute/path/to/baseline-checkout 4 1 2
```

This queues eight jobs, each containing four sequential partition/worker modes.
Each job waits for the preceding job (`qsub -hold_jid`); no tests in this campaign
run concurrently. The helper validates both sets of source pins, unchanged inputs
and closed binary paths before submitting anything. An existing external job can
be added as the first dependency with `HOLD_JID=JOB_ID`. A submission failure does
not cancel preceding jobs; IDs and order remain in the TSV receipt.

The first candidate pilot (581887/581889/581891/581893) completed the first
no-repartition case in each job, then stopped exporting because the old shared
tools bundle differed. The collector now keeps tools inside each exported case,
so future campaigns can coexist with earlier evidence. Pull the current candidate
and repeat the pilot; no binary rebuild is needed. Keep failed jobs and their raw
cases unchanged. The remaining M4/M3/M2 repartition modes were not run in those
candidate jobs, so that partial pilot cannot establish the complete comparison.

After independently auditing the pilot, use the same command with `4 3 2` for
three repetitions (24 jobs). Baseline/candidate order reverses in even repetitions.
The resulting repeat identifier is in the submission TSV: each independent job
uses `r1` case names, and its unique SGE job ID prevents collisions. Keep the TSV.
After these controls pass, a longer campaign can use `4 3 10` (ten adaptation
events, 2200 RANS / 1100 Euler steps). Increase ranks to 8/16 only after successful
numerical gates; choose `ADAP_WORKERS=8:4:2` / `16:8:4` for those trials. The small
RAE mesh cannot establish scaling of a large production problem.

Download only `ClusterResults` from each checkout, into separate directories
`baseline/ClusterResults` and `candidate/ClusterResults`. Keep the audit tools
with the exported data: new candidate cases have their own `cases/CASE/tools/`
folder and recorded tool hashes; the historical baseline uses its root `tools/`
bundle. Use each export's own auditors, as in the original campaign README.
Do not replace older tool bundles. No raw output is
deleted; `ClusterRaw` remains on the cluster. Adapted grids, original-reference
sidecars, original donor states, both transported BDF histories and logs are in
the collected cases, so the grids remain directly inspectable.

Assess the following before claiming improvement:

- All independent mesh quality/length/reference/first-height, tensor transport
  (frozen cases), positivity and both-history transfer gates must pass.
- Compare repeated full adaptation totals and per-window costs, including the
  terminal window's metric without remeshing; also report applied-window totals.
- Compare metric work counters, sensor gradation residuals and the separate
  composed-field diagnostic. Nodal convergence is not a P1/composed certificate.
- Frozen cases retain the same raw target; verify output/work equality before
  calling them fixed-work speedups. Actual recovery changes can produce different
  metrics, meshes and CFD trajectories; report that alongside their timings.
- Check identical initial conservative flow before adaptation. New Hessians and
  sensor metrics are expected to change; do not require old/new metric equality.
- Check rank imbalance and hardware/process/memory observations. Do not add rank
  maxima, sum per-rank HWM as a simultaneous peak, or infer speedup from ratios
  whose CFD denominator changed. Noise remains zero.

The original sixteen pilots prove N=4 numerical feasibility but have one repeat
and overlapping jobs. They are preserved descriptive evidence, not an isolated
performance control for the new implementation. Neither this campaign nor the
merge certifies native 3D remeshing or converged aerodynamic accuracy.

The completed follow-up pilot 581943–581950 and all phase/hash evidence are in [pilot_581943_581950/README.md](pilot_581943_581950/README.md). All 32 execution/collection cases pass; independent numerical checks cover eight exactly repeated no-repartition cases, with the other 24 awaiting the reusable one-worker saved-output SGE audit described there. No CFD rerun or rebuild is needed for that postprocessing. Balance measurements and improved initial weights should precede a live-repartition prototype.
