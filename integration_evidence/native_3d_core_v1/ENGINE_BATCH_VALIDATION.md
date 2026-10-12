# Integrated native 3D batching qualification

2026-10-12. Branch `codex/native-3d-transport-accounting`, starting from
`fa17b3ecee48754bb2104155800a6c60bf93d153`. Working repository:
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`. **Milestone PASS; Goal1 ACTIVE.**

The real engine now batches up to eight independent refinement or admissible
coarsening proposals per worker. Complete-star metadata lookup, deduplicated cell
fetch and candidate-specific vertex reservations share communication. Each proposal
still undergoes private geometry and full original-P1 metric validation and its own
atomic publication. Recovery/invalid-field repairs retain the single-candidate path.
Shared stale payloads reject their dependents; common storage/protocol failure refuses
the batch. Sequential publications are protected by complete, vertex-disjoint stars.
The current sensor-only donor authority, thresholds and completion policy are intact.
Epochs and actual proposed/attempted/accepted/rejected edits are counted separately.
Batch1/batch8 is an internal test-fixture control, not a new CFD configuration option.

The larger size8 three-event driver now completes within the existing512epoch budget:
accepted tetrahedra **2415 → 467 → 2542**. The earlier coarsening-only implementation
completed the removal event but exhausted512single-candidate refinement rounds at
its next event; that failure is preserved in `engine_batch_milestone_v1`.
A real MPI4 return/rebuild failure exposed an empty CFD rank. The root fix in
`CPoint::SetPoints` builds empty/isolated-row edge connectivity without dereferencing
empty CSR entry pointers, preserving existing nonempty behavior. Targeted bridge
MPI1/2/4 and the complete integrated regression pass with this shared fix.

## Qualification and independent evidence

- Actual configured **68-object** MPI/MMG/CGNS driver and production archives.
  Source, test objects, archives, thin members and binary hashes are verified.
- **104 selected C++ cases per rank on MPI1/2/4**, including2D bridge, default/budget,
  distributed stale/conflict/atomic refusal, repair and actual return/rebuild controls.
  Each rank count also passes bridge geometry/current-BDF/SU2-CGNS checks and eight
  independent distributed auditors, including the new eight-component batch audit.
- Size8N1M1: three complete real driver events, eight geometry/input/readback frames
  and three donor/accepted current+BDF state files. The current rerun matches the
  earlier fully independently audited numerical frames exactly; fresh actual outputs
  and runtime/work accounting are checked. This is explicit certificate reuse,
  not a claim that a second full geometry audit was run.
- Rotated, spatially varying, aspect2/H20-thin wedge on **N4/M2 with CGNS restart**:
  fresh full independent audit of the saved current three-event output, including
  reference sidecars, geometry/markers, original-P1 tensors, state transfer and reload.
- Six actual metric/Hessian/CFD/BDF lifecycle frames and their closed phase/counter
  records; serial MMG2D/MMG3D round trips,48assertions. Lifecycle means actual driver
  rebuilding/transfer and metric construction here, not a physical CFD convergence run.
- **51 lightweight cluster-tool checks**, actual103-case core registration and a real
  closed same-binary batch1/batch8 package preparation/verification smoke. No qsub or
  MPI numerical comparison was launched. Copied large smoke binaries were removed
  after SHA capture; rebuild/freeze a fresh runnable package using the README.

| Current driver case | Accepted tetrahedra | Minimum q across events | Minimum scaled Jacobian | Maximum metric edge | Maximum state relative error |
|---|---|---|---|---|---|
| size8 N1/M1 | 2415 → 467 → 2542 | 0.2091173775 | 0.05517324867 | 1.799855316119 | 3.9328e-16 |
| aspect2 H20 N4/M2 CGNS restart | 417 → 64 → 413 | 0.2084904833 | 0.06672752033 | 1.799025803373 | 1.1298e-14 |

Transported tensor error is zero in both declared cases (including physical and
reference-box checks for H20). Final strict criteria remain q>=.20, scaledJ>=.05,
full original-P1 metric edge length<=1.8, exact conformity/domain/marker preservation.
H20 is an affine manufactured thin-domain check, not production3D BL generation.

Inspectable size8 grids/VTUs and corresponding original/accepted states:
`engine_batch_milestone_v4/n8_n1_m1_default512/`.
Inspectable current MPI4/M2 CGNS grids/restart/states and fresh certificate:
`engine_batch_cgns_audit_recovery_v1/n4_n4_m2_thin_strong_cgns/`.
Complete mixed regressions/audits/lifecycle:
`engine_batch_milestone_v5/`; final verified reuse and MMG:
`engine_batch_qualification_v1/`. [Structured report](ENGINE_BATCH_VALIDATION.json)
and its pinned receipts give exact artifact identities and case scope.

## Preserved failures and collection corrections

Historical failed receipts stay FAIL: v1 contains the real next-refinement budget
failure; v2's unrelated batch auditor initially inferred component from node IDs;
v3 exposed the real empty-rank rebuild assertion. Both implementation/auditor fixes
and retained raw/source snapshots are available. v4's equivalence collector assumed
SU2 filenames for a CGNS checkpoint; its KeyError did not establish mesh drift and
was resolved by a fresh full audit of saved current CGNS data. v5's collector expected
`independent_audit.json` after the successful lifecycle auditor, which actually writes
`lifecycle_audit.json`. Final qualification reuses every completed successful stage
only after artifact/source/build verification; no numerical case was rerun for that
basename correction. The tooling-smoke wrapper similarly corrected Catch's
`103 matching test cases` label after a successful exit103 listing; its failed local
collector receipt is retained. These are explicitly scoped recovery certificates,
not promotion of failed aggregate receipts.

## Performance limits and next integrated step

Completion within512epochs demonstrates increased edit throughput per epoch,
**not measured speedup**. Changed accepted meshes and later resampled P1 sources
make this a matched-target operational-cost comparison, not a fixed-work ratio.
Use `native_cluster_3d_reservation_comparison_v1/README.md` for same-binary batch1/8
SGE correctness, three paired repetitions, detailOFF/ON and N=M/M<N comparisons.
Both timing controls retain the explicit1024diagnostic allowance so the baseline
can finish. No cluster submissions/results or local timing campaign are claimed.

The512limit remains an arbitrary per-engine synchronized-round allowance, applied
independently to refinement and coarsening. At the final check an already complete
result may pass; remaining invalid cells, missing measurements or pending removal
candidates reject the entire event. A rejected local proposal is not proof that the
target is unmeshable. The production driver stops collectively before replacement;
accepted state retention is not automatic CFD continuation or a new recovery file.

The user's next authorized large step combines typed blockage diagnosis and
progress-aware execution with explicit bounded work/time policy, preserving strict
final gates, MPI collective agreement, history/ID/memory admission and atomic state
publication. Measure residual progress rather than only edit counts; distinguish
operator exhaustion, resource/communication limits and proven infeasibility. Do not
label a kernel-only ceiling as complete metric-to-CFD-ready adaptation accounting.
Repeated cluster costs/scaling/overhead, weight calibration, actual MPI wait/wire and
practical memory/rank envelopes remain open Goal1 requirements. No curved geometry,
robust3DBL, physical unsteady accuracy or general affordability is established.
