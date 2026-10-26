# Frozen M6 diagnostic decision — 2026-10-26

## Decision and scope

Choose follow-up option **(v): one targeted construction experiment**, specified
below. Do not replace ParMETIS, change weights in production, relax acceptance,
or launch another full timeout campaign on the present evidence.

1. **[MEASURED / CODE VERIFIED] The existing partitioner balances its supplied
   workload proxy successfully.** On validation54, max/mean predicted work improves
   from 4.6991 to 1.08449; cell count ends at 1.08468. This does not validate that
   proxy as a predictor of useful compute. Replacing a partitioning API is not
   supported by the evidence.
2. **[REPRODUCED] Real early-M6 candidates can have positive volumes and still
   fail shape admission.** Sixteen captured failures reproduce independently on
   the exact saved original P1 field. Eleven other snapshots record cavity/skin
   obstruction; one records insufficient metric progress. Neither a resolved local
   replacement nor a proof of infeasibility has been obtained.
3. **[MEASURED] MPI calls consume a material part of a bounded replay.** Per-rank
   measured MPI-call durations are 18.74–26.00 s in a 62.72 s remesher event. The
   dominant direct caller is `CPassiveComm::AllreduceMax`, shared by failure elections
   and other maximum reductions. These durations include waiting/polling; they are
   not network time, useful-compute imbalance, or a full-run speedup estimate.

This is a completed bounded evidence collection, not completion of the broader
causal diagnosis, prediction calibration, full-M6 adaptation or DC-17/DC-18.
The [independent review and reconciliation](M6_REVIEW_RECONCILIATION.md) confirms
this next experiment and sharpens its acceptance criteria. It does not establish
that these early failures cause the late full-M6 timeout.

## Baseline fact sheet

Production implementation: `e9263060cf`; assessment/evidence baseline:
`5f469f7800`. Diagnostic55 changes only the hidden test driver and external analysis/
PMPI helpers. [Manifest](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/diagnostic55/run_manifest.json)
and per-run pins preserve actual source, executable, inputs and runtime hashes.

| Quantity | Verified value | Evidence / limitation |
|---|---|---|
| Original mesh / sensor | 303,359 tetrahedra; 61,330 nodes | Exact restart-coordinate/global-ID match and metric round trip; `diagnostic55/input_manifest.json` |
| Full M6 event | Native maximum 1,805.12 s; whole launch 1,980.41 s; rejected | Validation54 `run.log`, `status.json`; requested native deadline 1,800 s, checked at safe points |
| Final accepted mesh / transfer | None | Original CFD state retained; driver exit zero is not acceptance |
| Private edits / proposal attempts | 24,972 / 31,133 | Raw per-rank `NATIVE3D_DETAIL stage=refine` records |
| Shape refusals | 76,739; 84.8% on ranks 4–5 | Candidate evaluations, possibly several per proposal; not a failure probability |
| Cached failing cells | 235,717 entries at exit | Cache is keyed by cell ID and rejects duplicate local insertion; not cumulative refusals. No independent global identity audit is claimed for this rejected state. |
| Work used / allowed | 199,331 / 19,414,976 | Charged proposal/repair/growth work; automatic allowance is max(4096,64*303359), not a cell count |
| Partition graph / migration | Tetrahedron face-dual graph, two constraints; 102,883 cells moved | `CNativePartition3D.cpp`; raw partition records. Migration count is global and repeated in rank logs, not additive. |
| Graph + partition + cell migration | 3.135–3.200 s/rank | Includes nested ParMETIS 0.6745 s and migration 0.2627–0.2644 s; do not add nested times again |
| Original donor migration | 0.512–0.580 s/rank | Separate outer remesher phase |
| Return, reconstruction, transfer | No completed accepted return path | Zero skipped-stage timers do not measure the cost of a successful workflow |
| Weight clipping | Offline replica: min 2, median 4, p95 153, max 591; none at 1024 | Aggregate 5,664,358 matches production exactly; individual production weights were not exported |
| Held-out prediction / alternate partition | Unavailable / not executed | No region-level useful-compute labels; no fabricated prediction score or runtime speedup |

Raw numbers and line references are retained in
[baseline_records.json](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/diagnostic55/baseline_records.json)
and [m6_rank_phase_timings.csv](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/diagnostic55/m6_rank_phase_timings.csv).

## Verified architecture

`Common/src/adaptation/CNativeRemesher3D.cpp:321–388` starts with private imported
cells, computes `WorkingWeights`, repartitions, and separately migrates original
donors. Line 322 enables this for the configured switch or a worker subset;
validation54 used `ADAP_NATIVE_REPARTITION=YES`, six workers. Lines 397 onward
select the worker communicator. Lines 452–477 select Absolute shape admission and
call Refine/Coarsen. This is incremental mesh modification, not an independent
from-scratch generator. Cross-owner cavity dependencies are imported; interfaces
are not universally frozen.

`CNativePartition3D.cpp:15–45` defines the current proxy. Lines 185–265 build
face-dual adjacency and call (Par)METIS with `ncon=2`, weights `(predicted work,1)`,
equal target shares and tolerances `(1.15,1.15)`. The separate CFD node graph is
in `Common/src/geometry/CPhysicalGeometry.cpp:7420–7463`.

The independent frozen replay uses the same `MakeRemesher()` backend and authority
sidecar, with no CFD advance. Only work and time stop limits change to 2,048 and
180 s. Geometry, metric data, q/J/edge gates, HAUSD and six-rank partitioning remain.
This bounded prefix does not reproduce the exact late state of validation54.

## Bounded timing and construction evidence

| Rank | Event elapsed s | Wrapped MPI s | Outside wrapped MPI s | Useful compute s | Edits / attempts | Shape refusals |
|---|---:|---:|---:|---|---:|---:|
| 0 | 62.723 | 25.909 | 36.814 | unknown | 90 / 90 | 0 |
| 1 | 62.664 | 25.996 | 36.668 | unknown | 89 / 89 | 0 |
| 2 | 62.664 | 18.739 | 43.925 | unknown | 45 / 90 | 592 |
| 3 | 62.664 | 24.251 | 38.413 | unknown | 76 / 90 | 186 |
| 4 | 62.664 | 22.822 | 39.842 | unknown | 86 / 90 | 94 |
| 5 | 62.664 | 24.044 | 38.620 | unknown | 90 / 90 | 7 |

Both bounded runs stop at exactly 2,048 work units, with 476 private edits and
879 shape refusals. Ten selected operation/state counters match on every rank.
No final mesh is accepted. Their rejected states were not compared byte for byte.
Matched counters also do not establish identical candidate sequences or trajectories.
The low-refusal ranks 0–1 produce no rejection witnesses in this prefix; no samples
were invented. The late run's concentration on ranks 4–5 is not reproduced by this
early prefix, so temporal representativeness remains limited.

Refinement is 30.044–30.046 s in the observed run, including 16.001–16.006 s seed
work. Reconstruction wall scopes are 4.417–4.447 s and donor scopes 4.228–4.235 s;
these scopes are not pure useful compute. The original full run's reconstruction
CPU scopes are 330–344 s/rank; neither those nor whole-process CPU establish a
complete compute/wait decomposition. Per-process peak RSS for the full run is
663–720 MiB, not a measured simultaneous whole-job peak.

The matched plain run took 89.999 s and the observed run 62.723 s: an elapsed delta
of -30.3%. This single shared-host pair cannot identify instrumentation overhead.
It does establish matching work counters, complete trace storage (no overflow),
and measured call durations. Detailed scope and limitations:
[MPI report](M6_MPI_CRITICAL_PATH.md), [witness report](M6_CONSTRUCTION_WITNESSES.md).

## Predictor and benefit assessment

[Weight report](M6_WEIGHT_PREDICTORS.md) compares uniform weights, the current
proxy transcription, target complexity and a natural-metric intersection model.
Controlled equal-density rotated anisotropy changes the mismatch terms while
leaving density unchanged. This confirms the diagnostic distinction, not runtime
predictiveness. Calibration/holdout accuracy remains unavailable.

**[HYPOTHETICAL ACCOUNTING]** Subtracting every measured wrapped-MPI duration while
holding each rank's remaining elapsed time fixed gives a maximum residual of
43.925 s and quotient 1.428. This is not an engineering speedup ceiling, a full-M6
bound or a forecast. Coordination changes can change the work and scheduling,
invalidating the assumption. Migration/rebuild/transfer costs must be included in
any future measured benefit.

## One discriminating experiment

Replay the exact transaction around edge **23754–24333** with complete original
metric/geometry and external-contact/dependency context. Its initial 2-to-4-cell
candidate has positive volumes but q=0.183972 and J=0.042790; the subsequent captured
growth attempt reports that the point cannot see its retained skin.

First capture the exact pre-transaction state and every growth/repair attempt;
the current rejected-trial callback lacks ownership/version/external-contact records.
Do not interpret an incomplete snapshot as a replay. Strict replay must reproduce
the original candidate sequence and failure classification before an alternative
is evaluated. Use existing local reconstruction/optimizer facilities under unchanged
limits, with at most two isolated validity-only provisional operations and rollback.

Restore Absolute admission for the terminal local transaction and apply production
`ValidateMetricChange` plus complete geometry, source, interface/contact and applicable
height checks. Integrate every replacement/exposed edge. Separately report the stronger
`ValidateMetricPatch` result: a production-admissible transaction can retain oversized
edges under its progress rules; this is not final mesh acceptance. List all residual
edges above 1.8. No production policy switch is authorized by this experiment.

Start with edge 23754–24333 only. Apply the verified replay to independent edge
81–17687 if needed afterward: its old patch already fails target-metric J, and its
candidate fails both q and J (see the witness table). No full-M6 campaign.
Use the review's bounded protocol: at most one new MPI6 prefix, work 2,048/native
180 s/launcher 240 s; each replay branch at most 256 trials, two provisional operations,
60 s CPU or wall, 1 GiB RSS, and existing 12-growth/64-old/128-new construction limits.
Capture limits are 128 MiB per rank and 25 MiB output per witness. Truncation means
missing evidence. Verify these limits against existing invariant/resource controls
before execution; the protocol is not yet implemented.
Budget one test-driver rebuild (the present build took about six minutes) and
roughly two minutes per bounded replay on this host, not a guaranteed timing.

- If a fully admissible replacement exists locally but coordination dominates its
  delivery, investigate batching/synchronization without weakening failure agreement.
- If only a temporary low-quality state can be constructed and no fully accepted
  replacement follows, construction remains the first blocker; no feasibility proof
  follows from exhausting this bounded search.
- If an artificial dependency/skin restriction blocks an otherwise admissible
  replacement, fix that shared construction/coordination contract before retuning weights.

This is the smallest next evidence gap. Do not require another generic audit first.
No production fix, repaired accepted witness, end-to-end speedup, full-M6 success,
or DC-17/DC-18 closure is claimed.
