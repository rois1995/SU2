# Native 3D rotated-anisotropic driver envelope — 2026-10-11

Repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-core`, predecessor `d41c49de169444c6053674860da0285b6347fe18`.
Implementation/evidence checkpoint `dd673805245e4946bc995ba193a986f418299155`
is pushed and remote-verified. Full Goal 1 remains active and incomplete. This checkpoint adds actual-driver
fixtures, independent audits and inspectable grids; it changes no production code.
All production archives and thin-archive members match the pushed reader checkpoint.

## Frozen target and exercised behavior

`ENVELOPE_MATRIX.json` freezes the initial size/aspect/rank matrix before its
campaign. Existing tetrahedral guards stay unchanged: mean ratio >= 0.20,
scaled Jacobian >= 0.05, maximum original-field metric edge <= 1.8 (existing
roundoff tolerance only). The target is a 45-degree xy rotation of
`diag(a^2,1,1)`, with a=1.5 here, times `n^2*(1+variation*x)`. Variation is
0 or 0.125. Tensors are affine in space, so the analytic fixture and original
sensor-only tetrahedral P1 field agree. Principal directions are constant in
these cases; spatially changing directions still need their own campaign.

Each actual CNativeRemesher3D factory/driver case starts on a box with seven
physical markers, then uses target scales 1, 1/4, 1 for three real refinement,
coarsening and refinement events. ReplaceMesh performs actual CFD geometry,
solver and current/BDF2-history transfer. Boundary triangles change. Metric
preparation is manufactured and supplied to the backend directly; no CFD
iteration or Hessian construction runs here. The fixture initializes an affine,
synthetic admissible flow and its two histories. It is not a physical simulation.

Every accepted frame independently checks exact positive volume, incidence,
embedding/no overlap, oriented finite-facet coverage, domain volume, markers,
quality/sliver and frozen metric lengths. Original-P1 transported tensors and
current/BDF histories are checked separately. Three actual driver SU2 outputs
and immutable sidecars agree with accepted topology. Final SU2 and CGNS writes,
independent parsing and house reader reloads agree; CGNS dependencies are closed.

## Results

Seven cases qualify, including a preserved audit-only failure with a separately
hash-bound recovery. All start with 48 tetrahedra. Different N/M trajectories
are independently validated; equal connectivity across partitions is not required.

| Case | N/M | Variation | Accepted cells (refine/coarsen/refine) | Minimum q | Minimum J | Maximum metric edge |
|---|---|---|---|---|---|---|
| envelope_pilot_v1 | 1/1 | 0 | 88 / 26 / 68 | 0.365190 | 0.102514 | 1.732051 |
| envelope_varying_v1 | 1/1 | 0.125 | 88 / 36 / 94 | 0.258663 | 0.062176 | 1.770098 |
| envelope_mpi2_m1_v1 | 2/1 | 0.125 | 88 / 36 / 95 | 0.258663 | 0.062176 | 1.757176 |
| envelope_mpi2_m2_v1 | 2/2 | 0.125 | 88 / 36 / 95 | 0.258663 | 0.062176 | 1.757176 |
| envelope_mpi4_m1_v1 | 4/1 | 0.125 | 88 / 8 / 70 | 0.473361 | 0.243658 | 1.782833 |
| envelope_mpi4_m2_v1 | 4/2 | 0.125 | 88 / 9 / 69 | 0.596084 | 0.279145 | 1.785100 |
| envelope_mpi4_m4_v1 | 4/4 | 0.125 | 77 / 12 / 70 | 0.464190 | 0.172292 | 1.758839 |

56 input/accepted/readback frames and 21 metric/current/BDF-state frames pass.
Maximum relative state error is 3.8441482e-16; maximum transported tensor error
is zero. The supplementary analyzer checks 234 exclusive rank-stage rows and
378 nested rows (612 CSV rows), including existing closure and cache arithmetic
checks and an intentionally corrupted-closure adversary.

The N2/M1 original receipt remains FAIL: its strict nonnegative CPU-remainder
check rejected -4.3368086899420177e-19. Corrected auditing reuses the established
lifecycle timing tolerance, without changing any mesh/state/metric guard or
rerunning remeshing. `AUDIT_RECOVERY.json` binds the original receipt/failure,
corrected audit and exports. The original logs/source snapshots remain intact.

`envelope_legacy_control_v1` passes actual compiled default output-helper calls,
existing native2D transfer and MMG roundtrip tests at MPI1, then independent
runtime mesh/tensor/current/BDF audits. The unchanged default-sensor Python
path additionally re-audits all 84 published reader-runtime state frames.
This is a relevant helper regression and production-archive identity check,
not a new full 2D performance campaign.

## First demonstrated rejection and cost

The next frozen case, `envelope_size3_v1` (n=3, 162 initial tetrahedra, N1/M1,
a=1.5, variation0.125), rejects during its first refinement. Default 512 rounds
contain 59 private accepted edits and 453 rejected attempts. No accepted reader
or reference is published; Catch mesh/solution/reference preservation checks pass
before the deliberate completion assertion fails. Initial grid, exact source,
logs and original FAIL receipt remain. Private rejected progress is not yet
exported. Size4 and a=2 expansion are deferred pending diagnosis.

Remesher wall time is 7.917464s, refinement7.822850s. Its exclusive metric phase
uses6.234011s and reconstruction0.806289s. Accumulated nested edge tracing uses
6.495742s across multiple phases; do not add it to exclusive metric time. The
last local rejection reports feature2:58 unable to improve frozen size excess
or quality/sliver bounds. This message does not prove repeated-feature cycling:
a finite queue of distinct failed repairs can also exhaust the fixed budget.
Next: exact private replay, full feature/location and residual diagnostics,
then distinguish budget exhaustion, unavailable reconstruction and redundant
work. Increasing sweeps alone is not an established fix. Keep the full target
and atomic rejection contract intact.

For passing cases, `ENVELOPE_CHECKPOINT_VALIDATION.json` retains per-cycle
parent maxima, accepted edits/s, max/mean and mesh/error/bound/RSS values.
`envelope_rank_phases.csv` retains exclusive and nested rows separately. The
parent includes test-only metric observation and nested reporting, and excludes
requested mesh output; output has separate stage records. Never sum rank maxima.
These are single pilots under foreign four-rank FSI contention. They establish
no scaling, fixed-work speedup or instrumentation-overhead claim. Production
roundoffs and trajectory differences remain visible. Modeled native storage and
process-lifetime RSS are different quantities; no allocator-inclusive certificate.

## Inspect and reproduce

Open `envelope_mpi4_m4_v1/envelope_s0.vtu`, `envelope_s1.vtu` and
`envelope_s2.vtu` in ParaView: they contain accepted tetrahedra, actual transferred
synthetic current/time_n/time_n1 states and six transported sensor components.
The corresponding `envelope_mesh_00001.su2` through `00003.su2` and final
`envelope_s2_written_CGNS.cgns` are the actual house-written meshes. Each case
also retains its starting SU2 grid/config, immutable reference, original donor
and accepted mesh/state JSON. VTU export checks counts and pins its sources.

From the repository root, with the existing configured MPI build:

```bash
python3 integration_evidence/native_3d_core_v1/run_envelope.py \
  integration_evidence/native_3d_core_v1/my_new_case \
  --ranks 4 --workers 2 --size 2 --anisotropy 1 --varying 1
python3 integration_evidence/native_3d_core_v1/analyze_envelope.py
```

Use a fresh folder; the runner refuses overwrites. Actual complete configured
archives, test objects, source snapshots and binary/build-plan identities are
pinned. Optional `--binary-from <qualified case>` verifies archive/member/object/
plan/binary identities before reuse, with a verified-copy fallback when links
fail. ELF binaries, objects and Python caches are excluded from Git. Two identical superseded test executable copies were removed after qualification,
reclaiming1,217,722,368bytes. The qualified shared executable remains in
`envelope_mpi2_m2_v1`; use it for local binary reuse. The disk-compaction receipt
preserves identities; all numerical evidence remains. Matrices
must be extended explicitly after diagnosing the retained failure.

Still required for full Goal1: robust larger/stronger/direction-changing/thin/
wedge/restart cases, measured size/rank limits, full MPI wait/house/vendor traffic
accounting, allocator/cache/ParMETIS/CFD memory calibration, repeated detail OFF/ON
controls and closed SGE correctness/timing/scaling preparation and execution.
No cluster launch is requested overnight; no new3D SGE package is complete yet.
No curved geometry, robust3D BL, full unsteady3D or CFD affordability claim.
