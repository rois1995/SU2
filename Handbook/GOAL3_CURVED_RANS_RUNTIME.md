# Goal3 — actual curved RANS/native BL lifecycle

Branch `codex/native-3d-bl`; full Goal3 ACTIVE/incomplete.
Prior qualified source-refresh baseline9bd5fb76e4 remains separately scoped.

## Current outcome

`region_construction_v1/native_curved_rans_n1_m1_v5` reaches COMPLETE twice through
the full native backend:24 coarse tetrahedra ->1368 BL cells ->68 sensor-only cells.
Actual short dual-time RANS/SA steps, pressure-derived metric, conservative replacement,
SU2/CGNS output, actual CGNS restart and subsequent CFD steps execute. Sensor return
residual0. No synthetic sensor substituted; this case clips to the coarse sensor
bound and does not establish response to stronger flow features.

The **workflow test remains FAIL** on four unchanged all-field conservation assertions:
SA current/n-1 after each replacement. Flow totals, positivity, reference, reload and
restart checks pass. Saved SA integral differences are~0.0368/0.0366% at construction
and~0.3694/0.3693% at removal, not roundoff. Production transfer explicitly permits
solver turbulence clipping to change its integral and reports200 then56 clipped values,
with no admissibility recovery patches. These are a strong explanation, not yet a
full independent proof of the bounds accounting. Leave the existing gate unchanged;
no full RANS/MPI/accuracy/performance qualification follows from this failed test.

Independent exact saved-mesh audit is complete; all owned jobs are terminal. The
helper `region_construction_v1/audit_runtime_mesh.py` verifies saved evidence hashes,
positive exact volumes, oriented connected manifold skin/every vertex link, no interior
overlap, first-height/normal feet and analytic curved authority. It independently
integrates actual physical P1 flow/SA states, checks the flow/open ledger and restart,
and explicitly retains SA nonconservation and the native workflow failure.
The report is `independent_mesh.json`: both meshes PASS; physical flow integral
residual<=2.53e-15, flow/SA restart residual0. Maximum first-height error5.14e-15,
geometry-node error2.23e-16 and wall-chord sag.019215. Its workflow status stays FAIL.
The report writer initially failed on a relative helper path after all assertions;
that output error was corrected and the focused saved-data audit completed in40.84s.
No CFD or acceptance changes were needed.

## Connected implementation

Positive cylindrical-annulus complexity integration passes v7:3 selected families,
877746 assertions, all8 sectors/wall-first permutations, independent prism integral,
sphere and rotated/sheared polynomial moments. Prism native6.32953372249 vs independent
6.32953579782,13635 retained points over3 tetrahedra. Actual RANS complexity also
completes; the irreducible BL floor48.7130268608 exceeds requested complexity1.
This is reported rather than changing the field or wall-height constraints.

The preceding actual v4 run successfully constructed1368 cells, but unconditional
legacy coarsening then exhausted a dependency-star bound. Complete region construction
already resamples surface and volume bidirectionally. EngineResult now explicitly
marks a valid changed complete domain rebuild. The backend only schedules incremental
coarsening for other results, with the existing shared work/deadline/return/transfer
gates. No fallback after failed coarsening or bypass of final mesh validation.
Ordinary incremental path/work-limit/time-limit/atomicity control passes18 assertions
in `native_complete_domain_controls_v1`. v5 exercises the changed complete path.

Quadrature source, executed arithmetic/geometry objects and volume-test object remain
byte-identical across this backend change. Scoped v7 reuse is verified in
`annular_controls_reuse_after_domain_api.json`; it does not claim that v7 reran with
the new whole binary. No repeat volume campaign is needed for this unrelated flag.

## Unchanged acceptance and limits

- Geometry authority, first height.05/growth1.2/thickness.2/fade, Hausdorff.04,
  wall surface ceiling.025, q>=.20/J>=.05/full composed edge<=1.8.
-4096 charged construction work/300s per event; v5 construction3028/removal168 work.
- Sensor-only original P1 donors; geometric BL at every actual query, finer requests
  preserved, no hard-normal reset and Hessian noise0.
- No accepted mesh until complete reconstruction and all native gates pass; no local
  fallback chain or budget ladder in place of a construction strategy.
- Serial complete workflow first; MPI2/4 N=M/M<N only after its open transfer question.
- One visible radial wall/planar caps region constructed on one worker. General
  exterior/multiwall/high-ratio multiple fronts and scalable region generation remain open.

The advance clock includes Postprocess automatic metric calls under
COMPUTE_METRIC=YES/ADAP_LOOP=NO. Its solve_seconds is not CFD-only; no affordability
ratio or performance conclusion is justified. No converged viscous force/wall-shear
or M6 accuracy claim. Existing broader Goal3 gates remain in the requirement audit.

## Preserved failures and provenance

The collector retains source snapshots/build/binary/thin-object/config/input/helper
pins and actual grids/solutions/restarts. Do not rewrite prior failed receipts.
- Initial invalid HMIN.1/height.05 request; corrected to HMIN.025.
- RANS v1 generic complexity point-budget exhaustion; rejected axis-fan experiment.
- Volume v2 duplicate/unrepresentable angular partition; v3 incorrect unit-axial oracle.
- One-prism v4/v5 controls pass, but actual rotated-cell RANS v2/v3 fail.
- Exact house Polygon::Clip self-clipping changes5/8 rotated projected triangles;
  source/probe/output preserved in RANS v3/projection_diagnosis.json.
- Expanded v6 sector test exposes subtractive chord cancellation; v7 positive
  projective rays/barycentrics/vertex heights solve it without merging/clamping.
- RANS v4 redundant legacy coarsening failure; v5 accepted meshes, failed SA assertions.

Positive projective charts use exact shared-base triangle intersections, strict
compensated products, exposed geometric radial bands, positive retained weights and
original-source queries. Bounds remain65536 points/16384 work,1e-4 density error and
1e-10/2e-10 volume error/closure. General interacting/outside-slab integration has its
previous explicit limits. Full evidence index: `region_construction_v1/annular_integration_progress.json`.
